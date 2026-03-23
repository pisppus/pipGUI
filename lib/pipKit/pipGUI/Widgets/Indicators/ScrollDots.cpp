#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        constexpr uint8_t kScrollDotsMaxVisibleForTaper = 7;
        constexpr uint32_t kScrollDotsCountAnimMs = 220;
        constexpr uint8_t kScrollDotsAnimSlotCount = 4;

        struct ScrollDotsLayout
        {
            int16_t left = 0;
            int16_t top = 0;
            int16_t totalW = 0;
            int16_t h = 0;
            int16_t baseX0 = 0;
            int16_t baseY = 0;
        };

        struct ScrollDotsAnimState
        {
            bool valid = false;
            int16_t reqX = 0;
            int16_t reqY = 0;
            uint8_t radius = 0;
            uint8_t spacing = 0;
            uint8_t prevCount = 0;
            uint8_t count = 0;
            uint32_t changeStartMs = 0;
            uint32_t touchMs = 0;
        };

        ScrollDotsAnimState g_scrollDotsAnim[kScrollDotsAnimSlotCount];

        static inline uint8_t scrollDotsActiveWidth(uint8_t radius, uint8_t spacing) noexcept
        {
            const int16_t diameter = (int16_t)radius * 2 + 1;
            int16_t width = (int16_t)spacing + radius + 1;
            if (width < diameter)
                width = diameter;
            return (uint8_t)width;
        }

        static inline int16_t dotsTotalWidth(uint8_t count, uint8_t spacing, uint8_t activeWidth) noexcept
        {
            if (count <= 1)
                return (int16_t)activeWidth;
            if (count > kScrollDotsMaxVisibleForTaper)
                return (int16_t)(kScrollDotsMaxVisibleForTaper * (int16_t)spacing);
            return (int16_t)((count - 1) * (int16_t)spacing + (int16_t)activeWidth);
        }

        static inline int16_t dotsLeft(const GUI &gui, int16_t x, int16_t totalW) noexcept
        {
            return (x == center) ? (int16_t)(((int16_t)gui.screenWidth() - totalW) / 2) : x;
        }

        static inline void normalizeDotsMetrics(uint8_t &radius, uint8_t &spacing) noexcept
        {
            if (radius < 1)
                radius = 1;
            const uint8_t minSpacing = (uint8_t)(radius * 2 + 2);
            if (spacing < minSpacing)
                spacing = minSpacing;
        }

        static inline uint16_t blendOverBackground(uint16_t bg565, uint16_t fg565, uint8_t alpha) noexcept
        {
            return (alpha >= 255) ? fg565 : (uint16_t)detail::blend565(bg565, fg565, alpha);
        }

        static inline float scrollDotsWindowBase(uint8_t count, uint8_t index) noexcept
        {
            if (count <= kScrollDotsMaxVisibleForTaper)
                return 0.0f;

            const int maxBase = (int)count - (int)kScrollDotsMaxVisibleForTaper;
            int base = (int)index - (int)(kScrollDotsMaxVisibleForTaper / 2);
            if (base < 0)
                base = 0;
            if (base > maxBase)
                base = maxBase;
            return (float)base;
        }

        static inline float taperSlotScale(float distanceToActive) noexcept
        {
            if (distanceToActive <= 0.5f)
                return 1.0f;
            if (distanceToActive <= 1.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 0.5f) / 1.0f);
                return 0.84f + (1.0f - 0.84f) * (1.0f - t);
            }
            if (distanceToActive <= 2.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 1.5f) / 1.0f);
                return 0.66f + (0.84f - 0.66f) * (1.0f - t);
            }
            if (distanceToActive <= 3.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 2.5f) / 1.0f);
                return 0.50f + (0.66f - 0.50f) * (1.0f - t);
            }
            return 0.38f;
        }

        static inline float taperSlotOpacity(float distanceToActive) noexcept
        {
            if (distanceToActive <= 0.5f)
                return 1.0f;
            if (distanceToActive <= 1.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 0.5f) / 1.0f);
                return 0.80f + (1.0f - 0.80f) * (1.0f - t);
            }
            if (distanceToActive <= 2.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 1.5f) / 1.0f);
                return 0.62f + (0.80f - 0.62f) * (1.0f - t);
            }
            if (distanceToActive <= 3.5f)
            {
                const float t = detail::motion::clamp01((distanceToActive - 2.5f) / 1.0f);
                return 0.44f + (0.62f - 0.44f) * (1.0f - t);
            }
            return 0.30f;
        }

        static inline bool isTaperWrapJump(uint8_t count,
                                           uint8_t activeIndex,
                                           uint8_t prevIndex,
                                           bool animate,
                                           int8_t animDirection) noexcept
        {
            if (!animate || count <= kScrollDotsMaxVisibleForTaper || count < 2)
                return false;

            return ((prevIndex == 0 && activeIndex == (uint8_t)(count - 1) && animDirection < 0) ||
                    (prevIndex == (uint8_t)(count - 1) && activeIndex == 0 && animDirection > 0));
        }

        static inline ScrollDotsLayout makeScrollDotsLayout(const GUI &gui,
                                                            int16_t x,
                                                            int16_t y,
                                                            uint8_t count,
                                                            uint8_t radius,
                                                            uint8_t spacing) noexcept
        {
            ScrollDotsLayout layout;
            const uint8_t activeWidth = scrollDotsActiveWidth(radius, spacing);
            layout.h = (int16_t)(radius * 2 + 1);
            layout.totalW = dotsTotalWidth(count, spacing, activeWidth);
            layout.left = dotsLeft(gui, x, layout.totalW);
            layout.top = (y == center) ? (int16_t)(((int16_t)gui.screenHeight() - layout.h) / 2) : y;
            layout.baseY = (int16_t)(layout.top + layout.h / 2);

            if (count > kScrollDotsMaxVisibleForTaper)
                layout.baseX0 = (int16_t)(layout.left + (kScrollDotsMaxVisibleForTaper / 2) * (int16_t)spacing);
            else
                layout.baseX0 = (int16_t)(layout.left + (int16_t)activeWidth / 2);

            return layout;
        }

        struct ClipRectGuard
        {
            pipcore::Sprite *target = nullptr;
            int32_t prevX = 0;
            int32_t prevY = 0;
            int32_t prevW = 0;
            int32_t prevH = 0;

            ClipRectGuard(pipcore::Sprite *spr, int16_t x, int16_t y, int16_t w, int16_t h)
                : target(spr)
            {
                if (!target || w <= 0 || h <= 0)
                {
                    target = nullptr;
                    return;
                }

                target->getClipRect(&prevX, &prevY, &prevW, &prevH);
                target->setClipRect(x, y, w, h);
            }

            ~ClipRectGuard()
            {
                if (target)
                    target->setClipRect(prevX, prevY, prevW, prevH);
            }
        };

        static ScrollDotsAnimState &scrollDotsState(GUI &gui,
                                                    int16_t x,
                                                    int16_t y,
                                                    uint8_t radius,
                                                    uint8_t spacing,
                                                    uint8_t count,
                                                    float &countProgress,
                                                    uint8_t &fromCount,
                                                    uint8_t &toCount)
        {
            const uint32_t now = gui.platform() ? gui.platform()->nowMs() : 0;
            ScrollDotsAnimState *slot = nullptr;
            ScrollDotsAnimState *oldest = &g_scrollDotsAnim[0];

            for (uint8_t i = 0; i < kScrollDotsAnimSlotCount; ++i)
            {
                ScrollDotsAnimState &candidate = g_scrollDotsAnim[i];
                if (!candidate.valid)
                {
                    slot = &candidate;
                    break;
                }
                if (candidate.reqX == x &&
                    candidate.reqY == y &&
                    candidate.radius == radius &&
                    candidate.spacing == spacing)
                {
                    slot = &candidate;
                    break;
                }
                if (candidate.touchMs < oldest->touchMs)
                    oldest = &candidate;
            }

            if (!slot)
                slot = oldest;

            if (!slot->valid)
            {
                slot->valid = true;
                slot->reqX = x;
                slot->reqY = y;
                slot->radius = radius;
                slot->spacing = spacing;
                slot->prevCount = count;
                slot->count = count;
                slot->changeStartMs = 0;
            }
            else
            {
                slot->reqX = x;
                slot->reqY = y;
                slot->radius = radius;
                slot->spacing = spacing;
                if (slot->count != count)
                {
                    slot->prevCount = slot->count;
                    slot->count = count;
                    slot->changeStartMs = now;
                }
            }

            slot->touchMs = now;
            fromCount = slot->prevCount;
            toCount = slot->count;
            countProgress = 1.0f;

            if (slot->changeStartMs != 0 && slot->prevCount != slot->count)
            {
                const uint32_t elapsed = now - slot->changeStartMs;
                if (elapsed >= kScrollDotsCountAnimMs)
                {
                    slot->prevCount = slot->count;
                    slot->changeStartMs = 0;
                    fromCount = slot->count;
                    toCount = slot->count;
                }
                else
                {
                    countProgress = detail::motion::easeInOutCubic((float)elapsed / (float)kScrollDotsCountAnimMs);
                }
            }

            return *slot;
        }
    }

    void GUI::updateScrollDotsImpl(int16_t x, int16_t y,
                                   uint8_t count,
                                   uint8_t activeIndex,
                                   uint8_t prevIndex,
                                   float animProgress,
                                   int8_t animDirection,
                                   uint16_t activeColor,
                                   uint16_t inactiveColor,
                                   uint8_t radius,
                                   uint8_t spacing)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animDirection,
                               activeColor, inactiveColor, radius, spacing);
            return;
        }

        normalizeDotsMetrics(radius, spacing);

        float countProgress = 1.0f;
        uint8_t fromCount = count;
        uint8_t toCount = count;
        scrollDotsState(*this, x, y, radius, spacing, count, countProgress, fromCount, toCount);

        const ScrollDotsLayout fromLayout = makeScrollDotsLayout(*this, x, y, fromCount, radius, spacing);
        const ScrollDotsLayout toLayout = makeScrollDotsLayout(*this, x, y, toCount, radius, spacing);
        const ScrollDotsLayout drawLayout = makeScrollDotsLayout(*this, x, y, count, radius, spacing);

        const int16_t pad = (int16_t)(radius + 3);
        const int16_t rx = (fromLayout.left < toLayout.left) ? fromLayout.left : toLayout.left;
        const int16_t ry = drawLayout.top;
        const int16_t unionRight = ((fromLayout.left + fromLayout.totalW) > (toLayout.left + toLayout.totalW))
                                       ? (fromLayout.left + fromLayout.totalW)
                                       : (toLayout.left + toLayout.totalW);
        int16_t rw = (int16_t)((unionRight - rx) + pad * 2);
        int16_t rh = (int16_t)(drawLayout.h + pad * 2);

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        const uint16_t bg565 = detail::color888To565(_render.bgColor);

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size(rw, rh)
            .color(bg565)
            .draw();
        drawScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animDirection,
                           activeColor, inactiveColor, radius, spacing);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), rw, rh);
            flushDirty();
        }
    }

    void GUI::drawScrollDotsImpl(int16_t x, int16_t y,
                                 uint8_t count,
                                 uint8_t activeIndex,
                                 uint8_t prevIndex,
                                 float animProgress,
                                 int8_t animDirection,
                                 uint16_t activeColor,
                                 uint16_t inactiveColor,
                                 uint8_t radius,
                                 uint8_t spacing)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animDirection,
                                 activeColor, inactiveColor, radius, spacing);
            return;
        }

        const uint8_t requestedActiveIndex = activeIndex;

        if (count == 0)
        {
            activeIndex = 0;
            prevIndex = 0;
        }
        else
        {
            if (activeIndex >= count)
                activeIndex = (uint8_t)(count - 1);
            if (prevIndex >= count)
                prevIndex = activeIndex;
        }

        animProgress = detail::motion::clamp01(animProgress);
        const bool animate = (activeIndex != prevIndex) && (animProgress < 1.0f);
        const float navProgress = animate ? detail::motion::cubicBezierEase(animProgress, 0.22f, 0.0f, 0.18f, 1.0f) : 1.0f;

        normalizeDotsMetrics(radius, spacing);

        if (!getDrawTarget())
            return;

        float countProgress = 1.0f;
        uint8_t fromCount = count;
        uint8_t toCount = count;
        scrollDotsState(*this, x, y, radius, spacing, count, countProgress, fromCount, toCount);
        const bool countAnimating = (fromCount != toCount && countProgress < 1.0f);

        if (!countAnimating && count == 0)
            return;

        const bool taper = (count > kScrollDotsMaxVisibleForTaper);
        const bool oldTaper = (fromCount > kScrollDotsMaxVisibleForTaper);
        const bool canAnimateCount = countAnimating && !taper && !oldTaper;
        const bool taperWrapJump = isTaperWrapJump(count, activeIndex, prevIndex, animate, animDirection);

        const ScrollDotsLayout layout = makeScrollDotsLayout(*this, x, y, count, radius, spacing);
        const ScrollDotsLayout fromLayout = makeScrollDotsLayout(*this, x, y, fromCount, radius, spacing);
        const ScrollDotsLayout toLayout = makeScrollDotsLayout(*this, x, y, toCount, radius, spacing);

        const uint16_t bg565 = detail::color888To565(_render.bgColor);

        fillRect()
            .pos(layout.left, layout.top)
            .size(layout.totalW, layout.h)
            .color(bg565)
            .draw();

        const int16_t clipLeft = countAnimating && fromCount != toCount
                                     ? ((fromLayout.left < toLayout.left) ? fromLayout.left : toLayout.left)
                                     : layout.left;
        const int16_t clipRight = countAnimating && fromCount != toCount
                                      ? (((fromLayout.left + fromLayout.totalW) > (toLayout.left + toLayout.totalW))
                                             ? (fromLayout.left + fromLayout.totalW)
                                             : (toLayout.left + toLayout.totalW))
                                      : (layout.left + layout.totalW);
        ClipRectGuard clipGuard(getDrawTarget(), clipLeft, layout.top, (int16_t)(clipRight - clipLeft), layout.h);

        if (canAnimateCount && count == 0)
        {
            const int16_t oldBaseX0 = fromLayout.baseX0;
            const int16_t singleBaseX0 = toLayout.baseX0;
            const uint8_t survivorIndex = (fromCount > 0 && requestedActiveIndex < fromCount) ? requestedActiveIndex : 0;

            if (fromCount > 1)
            {
                const float stageSplit = 0.58f;
                const float phaseToSingle = (countProgress < stageSplit)
                                                ? detail::motion::easeInOutCubic(countProgress / stageSplit)
                                                : 1.0f;
                const float phaseFadeOut = (countProgress > stageSplit)
                                               ? detail::motion::easeInOutCubic((countProgress - stageSplit) / (1.0f - stageSplit))
                                               : 0.0f;

                for (uint8_t i = 0; i < fromCount; ++i)
                {
                    const float oldCx = (float)(oldBaseX0 + (int16_t)i * (int16_t)spacing);
                    if (i == survivorIndex)
                    {
                        const float drawCx = oldCx + (float)(singleBaseX0 - oldCx) * phaseToSingle;
                        const uint8_t alpha = (uint8_t)((1.0f - phaseFadeOut) * 255.0f + 0.5f);
                        const uint16_t color = blendOverBackground(bg565, activeColor, alpha);
                        fillCircle((int16_t)roundf(drawCx), layout.baseY, (int16_t)radius, color);
                    }
                    else
                    {
                        const float driftCx = oldCx + (float)(singleBaseX0 - oldCx) * (phaseToSingle * 0.20f);
                        const uint8_t alpha = (uint8_t)((1.0f - phaseToSingle) * 255.0f + 0.5f);
                        const uint16_t color = blendOverBackground(bg565, inactiveColor, alpha);
                        if (alpha > 0)
                            fillCircle((int16_t)roundf(driftCx), layout.baseY, (int16_t)radius, color);
                    }
                }
            }
            else
            {
                const float drawCx = (float)oldBaseX0 + (float)(singleBaseX0 - oldBaseX0) * countProgress;
                const uint8_t alpha = (uint8_t)((1.0f - countProgress) * 255.0f + 0.5f);
                const uint16_t color = blendOverBackground(bg565, activeColor, alpha);
                if (alpha > 0)
                    fillCircle((int16_t)roundf(drawCx), layout.baseY, (int16_t)radius, color);
            }
            return;
        }
        else if (canAnimateCount)
        {
            const uint8_t maxCount = (fromCount > count) ? fromCount : count;
            for (uint8_t i = 0; i < maxCount; ++i)
            {
                const bool oldVisible = i < fromCount;
                const bool newVisible = i < count;
                if (!oldVisible && !newVisible)
                    continue;

                const float oldCx = (float)(fromLayout.baseX0 + (int16_t)i * (int16_t)spacing);
                const float newCx = (float)(layout.baseX0 + (int16_t)i * (int16_t)spacing);
                float drawCx = newCx;
                uint8_t alpha = 255;

                if (oldVisible && newVisible)
                {
                    drawCx = oldCx + (newCx - oldCx) * countProgress;
                }
                else if (oldVisible)
                {
                    drawCx = oldCx + (float)(layout.left - fromLayout.left) * countProgress;
                    alpha = (uint8_t)((1.0f - countProgress) * 255.0f + 0.5f);
                }
                else
                {
                    drawCx = newCx - (float)(layout.left - fromLayout.left) * (1.0f - countProgress);
                    alpha = (uint8_t)(countProgress * 255.0f + 0.5f);
                }

                const uint16_t color = blendOverBackground(bg565, inactiveColor, alpha);
                fillCircle((int16_t)roundf(drawCx), layout.baseY, (int16_t)radius, color);
            }
        }
        else if (taper)
        {
            const auto renderTaperLayer = [&](float windowBase, float focalIndex, int16_t hiddenActive, float alphaScale, bool hideActive) noexcept
            {
                const int startIndex = (int)floorf(windowBase) - 1;
                const int endIndex = (int)ceilf(windowBase) + (int)kScrollDotsMaxVisibleForTaper + 1;

                for (int i = startIndex; i <= endIndex; ++i)
                {
                    if (i < 0 || i >= (int)count)
                        continue;

                    const float slot = (float)i - windowBase;
                    if (slot < -0.75f || slot > (float)kScrollDotsMaxVisibleForTaper - 0.25f)
                        continue;

                    const float activeSlot = focalIndex - windowBase;
                    const float distanceToActive = fabsf(slot - activeSlot);
                    const float radiusScale = taperSlotScale(distanceToActive);
                    float opacity = taperSlotOpacity(distanceToActive) * alphaScale;
                    if (opacity > 1.0f)
                        opacity = 1.0f;

                    const float cxf = (float)layout.left + slot * (float)spacing;
                    const int16_t cx = (int16_t)roundf(cxf);
                    int16_t r = (int16_t)((float)radius * radiusScale + 0.5f);
                    if (r < 1)
                        r = 1;

                    if (hideActive && i == (int)hiddenActive)
                        continue;

                    const uint8_t a = (uint8_t)(opacity * 255.0f + 0.5f);
                    const uint16_t color = blendOverBackground(bg565, inactiveColor, a);
                    fillCircle(cx, layout.baseY, r, color);
                }
            };

            if (taperWrapJump)
            {
                const float wrapProgress = detail::motion::smoothstep(navProgress);
                renderTaperLayer(scrollDotsWindowBase(count, prevIndex), (float)prevIndex, (int16_t)prevIndex, 1.0f - wrapProgress, true);
                renderTaperLayer(scrollDotsWindowBase(count, activeIndex), (float)activeIndex, (int16_t)activeIndex, wrapProgress, true);
            }
            else
            {
                const float prevBase = scrollDotsWindowBase(count, prevIndex);
                const float activeBase = scrollDotsWindowBase(count, activeIndex);
                const float windowBase = animate
                                             ? (prevBase + (activeBase - prevBase) * navProgress)
                                             : activeBase;
                const float focalIndex = animate
                                             ? ((float)prevIndex + ((float)activeIndex - (float)prevIndex) * navProgress)
                                             : (float)activeIndex;
                renderTaperLayer(windowBase, focalIndex, (int16_t)activeIndex, 1.0f, !animate);
            }
        }
        else
        {
            for (uint8_t i = 0; i < count; i++)
            {
                const int16_t cx = (int16_t)(layout.baseX0 + (int16_t)i * (int16_t)spacing);
                fillCircle(cx, layout.baseY, (int16_t)radius, inactiveColor);
            }
        }

        if (count == 0)
            return;

        if (count == 1)
        {
            fillCircle(layout.baseX0, layout.baseY, (int16_t)radius, activeColor);
            return;
        }

        if (animate)
        {
            const float p = navProgress;
            float xPrev = (float)(layout.baseX0 + (int16_t)((int)prevIndex - (int)activeIndex) * (int16_t)spacing);
            float xCurr = (float)layout.baseX0;
            if (taper && !taperWrapJump)
            {
                const float prevBase = scrollDotsWindowBase(count, prevIndex);
                const float activeBase = scrollDotsWindowBase(count, activeIndex);
                const float windowBase = prevBase + (activeBase - prevBase) * navProgress;
                xPrev = (float)layout.left + ((float)prevIndex - windowBase) * (float)spacing;
                xCurr = (float)layout.left + ((float)activeIndex - windowBase) * (float)spacing;
            }
            else if (!taper)
            {
                xPrev = (float)(layout.baseX0 + (int16_t)prevIndex * (int16_t)spacing);
                xCurr = (float)(layout.baseX0 + (int16_t)activeIndex * (int16_t)spacing);
            }
            else
            {
                const float prevBase = scrollDotsWindowBase(count, prevIndex);
                const float activeBase = scrollDotsWindowBase(count, activeIndex);
                xPrev = (float)layout.left + ((float)prevIndex - prevBase) * (float)spacing;
                xCurr = (float)layout.left + ((float)activeIndex - activeBase) * (float)spacing;
            }

            if (taperWrapJump)
            {
                const uint8_t prevAlpha = (uint8_t)((1.0f - navProgress) * 255.0f + 0.5f);
                const uint8_t currAlpha = (uint8_t)(navProgress * 255.0f + 0.5f);
                if (prevAlpha > 0)
                    fillCircle((int16_t)roundf(xPrev), layout.baseY, (int16_t)radius, blendOverBackground(bg565, activeColor, prevAlpha));
                if (currAlpha > 0)
                    fillCircle((int16_t)roundf(xCurr), layout.baseY, (int16_t)radius, blendOverBackground(bg565, activeColor, currAlpha));
                return;
            }
            float drawnLeftX = 0.0f;
            float drawnRightX = 0.0f;

            if (p < 0.5f)
            {
                float normalized_p = p * 2.0f;
                if (animDirection > 0)
                {
                    drawnLeftX = xPrev;
                    drawnRightX = xPrev + (xCurr - xPrev) * normalized_p;
                }
                else
                {
                    drawnRightX = xPrev;
                    drawnLeftX = xPrev + (xCurr - xPrev) * normalized_p;
                }
            }
            else
            {
                float normalized_p = (p - 0.5f) * 2.0f;
                if (animDirection > 0)
                {
                    drawnRightX = xCurr;
                    drawnLeftX = xPrev + (xCurr - xPrev) * normalized_p;
                }
                else
                {
                    drawnLeftX = xCurr;
                    drawnRightX = xPrev + (xCurr - xPrev) * normalized_p;
                }
            }

            if (drawnLeftX > drawnRightX)
            {
                float temp = drawnLeftX;
                drawnLeftX = drawnRightX;
                drawnRightX = temp;
            }

            int16_t lx = (int16_t)roundf(drawnLeftX);
            int16_t rx = (int16_t)roundf(drawnRightX);
            int16_t gap = (int16_t)(rx - lx);

            if (gap <= (int16_t)(radius * 2))
            {
                int16_t cx = (int16_t)((lx + rx) / 2);
                fillCircle(cx, layout.baseY, (int16_t)radius, activeColor);
            }
            else
            {
                int16_t pillX = (int16_t)(lx - (int16_t)radius);
                int16_t pillW = (int16_t)(gap + (int16_t)radius * 2 + 1);
                int16_t pillY = layout.top;
                uint8_t rad = (uint8_t)(radius < 255 ? radius : 255);
                fillRoundRect(pillX, pillY, pillW, layout.h, rad, activeColor);
            }
        }
        else
        {
            int16_t cx = (int16_t)(layout.baseX0 + (int16_t)activeIndex * (int16_t)spacing);
            if (taper)
            {
                const float windowBase = scrollDotsWindowBase(count, activeIndex);
                cx = (int16_t)roundf((float)layout.left + ((float)activeIndex - windowBase) * (float)spacing);
            }
            fillCircle(cx, layout.baseY, (int16_t)radius, activeColor);
        }
    }
}

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

        struct ScrollDotsAnimState
        {
            bool valid = false;
            int16_t reqX = 0;
            int16_t reqY = 0;
            uint8_t dotRadius = 0;
            uint8_t spacing = 0;
            uint8_t activeWidth = 0;
            uint8_t prevCount = 0;
            uint8_t count = 0;
            uint32_t changeStartMs = 0;
            uint32_t touchMs = 0;
        };

        ScrollDotsAnimState g_scrollDotsAnim[kScrollDotsAnimSlotCount];

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

        static ScrollDotsAnimState &scrollDotsState(GUI &gui,
                                                    int16_t x,
                                                    int16_t y,
                                                    uint8_t dotRadius,
                                                    uint8_t spacing,
                                                    uint8_t activeWidth,
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
                    candidate.dotRadius == dotRadius &&
                    candidate.spacing == spacing &&
                    candidate.activeWidth == activeWidth)
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
                slot->dotRadius = dotRadius;
                slot->spacing = spacing;
                slot->activeWidth = activeWidth;
                slot->prevCount = count;
                slot->count = count;
                slot->changeStartMs = 0;
            }
            else
            {
                slot->reqX = x;
                slot->reqY = y;
                slot->dotRadius = dotRadius;
                slot->spacing = spacing;
                slot->activeWidth = activeWidth;
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
                               bool animate,
                               int8_t animDirection,
                               uint16_t activeColor,
                               uint16_t inactiveColor,
                               uint8_t dotRadius,
                               uint8_t spacing,
                               uint8_t activeWidth)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                           activeColor, inactiveColor, dotRadius, spacing, activeWidth);
            return;
        }

        if (dotRadius < 1)
            dotRadius = 1;
        if (spacing < (uint8_t)(dotRadius * 2 + 2))
            spacing = (uint8_t)(dotRadius * 2 + 2);
        if (activeWidth < (uint8_t)(dotRadius * 2 + 1))
            activeWidth = (uint8_t)(dotRadius * 2 + 1);

        float countProgress = 1.0f;
        uint8_t fromCount = count;
        uint8_t toCount = count;
        scrollDotsState(*this, x, y, dotRadius, spacing, activeWidth, count, countProgress, fromCount, toCount);

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        const int16_t fromW = dotsTotalWidth(fromCount, spacing, activeWidth);
        const int16_t toW = dotsTotalWidth(toCount, spacing, activeWidth);
        const int16_t totalW = (fromCount == toCount)
                                   ? toW
                                   : (int16_t)(fromW + (int16_t)((float)(toW - fromW) * countProgress + (toW >= fromW ? 0.5f : -0.5f)));

        int16_t top = y;
        if (top == center)
            top = AutoY(h);

        int16_t pad = 2;
        const int16_t oldLeft = dotsLeft(*this, x, fromW);
        const int16_t newLeft = dotsLeft(*this, x, toW);
        const int16_t drawLeft = dotsLeft(*this, x, totalW);
        const int16_t rx = (oldLeft < newLeft) ? oldLeft : newLeft;
        int16_t ry = top;
        const int16_t unionRight = ((oldLeft + fromW) > (newLeft + toW)) ? (oldLeft + fromW) : (newLeft + toW);
        int16_t rw = (int16_t)((unionRight - rx) + pad * 2);
        int16_t rh = (int16_t)(h + pad * 2);

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size(rw, rh)
            .color(detail::color888To565(_render.bgColor))
            .draw();
        drawScrollDotsImpl(drawLeft, top, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                       activeColor, inactiveColor, dotRadius, spacing, activeWidth);

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
                            bool animate,
                            int8_t animDirection,
                            uint16_t activeColor,
                            uint16_t inactiveColor,
                            uint8_t dotRadius,
                            uint8_t spacing,
                            uint8_t activeWidth)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                             activeColor, inactiveColor, dotRadius, spacing, activeWidth);
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
        const float navProgress = animate ? detail::motion::easeInOutCubic(animProgress) : 1.0f;

        if (dotRadius < 1)
            dotRadius = 1;
        if (spacing < (uint8_t)(dotRadius * 2 + 2))
            spacing = (uint8_t)(dotRadius * 2 + 2);
        if (activeWidth < (uint8_t)(dotRadius * 2 + 1))
            activeWidth = (uint8_t)(dotRadius * 2 + 1);

        auto t = getDrawTarget();
        if (!t)
            return;

        float countProgress = 1.0f;
        uint8_t fromCount = count;
        uint8_t toCount = count;
        scrollDotsState(*this, x, y, dotRadius, spacing, activeWidth, count, countProgress, fromCount, toCount);
        const bool countAnimating = (fromCount != toCount && countProgress < 1.0f);

        if (!countAnimating && count == 0)
            return;

        const int halfWindow = (int)(kScrollDotsMaxVisibleForTaper / 2);
        const bool taper = (count > kScrollDotsMaxVisibleForTaper);
        const bool oldTaper = (fromCount > kScrollDotsMaxVisibleForTaper);
        const bool canAnimateCount = countAnimating && !taper && !oldTaper;

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW;
        int16_t left;
        int16_t baseX0;
        if (taper)
        {
            totalW = (int16_t)(kScrollDotsMaxVisibleForTaper * (int16_t)spacing);
            left = x;
            if (left == center)
                left = AutoX(totalW);
            baseX0 = left + (int16_t)(halfWindow * (int16_t)spacing);
        }
        else
        {
            totalW = (count <= 1) ? (int16_t)activeWidth : (int16_t)((count - 1) * (int16_t)spacing + (int16_t)activeWidth);
            left = x;
            if (left == center)
                left = AutoX(totalW);
            baseX0 = left + (int16_t)activeWidth / 2;
        }

        int16_t top = y;
        if (top == center)
            top = AutoY(h);

        int16_t baseY = top + h / 2;

        fillRect()
            .pos(left, top)
            .size(totalW, h)
            .color(detail::color888To565(_render.bgColor))
            .draw();

        if (canAnimateCount && count == 0)
        {
            const int16_t oldTotalW = dotsTotalWidth(fromCount, spacing, activeWidth);
            const int16_t oldLeft = dotsLeft(*this, x, oldTotalW);
            const int16_t oldBaseX0 = oldLeft + (int16_t)activeWidth / 2;
            const int16_t singleLeft = dotsLeft(*this, x, activeWidth);
            const int16_t singleBaseX0 = singleLeft + (int16_t)activeWidth / 2;
            const uint16_t bg565 = detail::color888To565(_render.bgColor);
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
                        const uint16_t color = (alpha >= 255) ? activeColor : (uint16_t)detail::blend565(bg565, activeColor, alpha);
                        fillCircle((int16_t)roundf(drawCx), baseY, (int16_t)dotRadius, color);
                    }
                    else
                    {
                        const float driftCx = oldCx + (float)(singleBaseX0 - oldCx) * (phaseToSingle * 0.20f);
                        const uint8_t alpha = (uint8_t)((1.0f - phaseToSingle) * 255.0f + 0.5f);
                        const uint16_t color = (alpha >= 255) ? inactiveColor : (uint16_t)detail::blend565(bg565, inactiveColor, alpha);
                        if (alpha > 0)
                            fillCircle((int16_t)roundf(driftCx), baseY, (int16_t)dotRadius, color);
                    }
                }
            }
            else
            {
                const float drawCx = (float)oldBaseX0 + (float)(singleBaseX0 - oldBaseX0) * countProgress;
                const uint8_t alpha = (uint8_t)((1.0f - countProgress) * 255.0f + 0.5f);
                const uint16_t color = (alpha >= 255) ? activeColor : (uint16_t)detail::blend565(bg565, activeColor, alpha);
                if (alpha > 0)
                    fillCircle((int16_t)roundf(drawCx), baseY, (int16_t)dotRadius, color);
            }
            return;
        }
        else if (canAnimateCount)
        {
            const int16_t oldTotalW = dotsTotalWidth(fromCount, spacing, activeWidth);
            const int16_t oldLeft = dotsLeft(*this, x, oldTotalW);
            const int16_t oldBaseX0 = oldLeft + (int16_t)activeWidth / 2;
            const uint8_t maxCount = (fromCount > count) ? fromCount : count;
            const uint16_t bg565 = detail::color888To565(_render.bgColor);

            for (uint8_t i = 0; i < maxCount; ++i)
            {
                const bool oldVisible = i < fromCount;
                const bool newVisible = i < count;
                if (!oldVisible && !newVisible)
                    continue;

                float oldCx = (float)(oldBaseX0 + (int16_t)i * (int16_t)spacing);
                float newCx = (float)(baseX0 + (int16_t)i * (int16_t)spacing);
                float drawCx = newCx;
                uint8_t alpha = 255;

                if (oldVisible && newVisible)
                {
                    drawCx = oldCx + (newCx - oldCx) * countProgress;
                }
                else if (oldVisible)
                {
                    drawCx = oldCx + (float)(left - oldLeft) * countProgress;
                    alpha = (uint8_t)((1.0f - countProgress) * 255.0f + 0.5f);
                }
                else
                {
                    drawCx = newCx - (float)(left - oldLeft) * (1.0f - countProgress);
                    alpha = (uint8_t)(countProgress * 255.0f + 0.5f);
                }

                const uint16_t color = (alpha >= 255) ? inactiveColor : (uint16_t)detail::blend565(bg565, inactiveColor, alpha);
                fillCircle((int16_t)roundf(drawCx), baseY, (int16_t)dotRadius, color);
            }
        }
        else if (taper)
        {
            for (int slot = 0; slot < kScrollDotsMaxVisibleForTaper; slot++)
            {
                int i = (int)activeIndex - halfWindow + slot;
                if (i < 0 || i >= (int)count)
                    continue;
                int dist = (i == (int)activeIndex) ? 0 : abs(i - (int)activeIndex);
                float t = (float)dist / (float)(halfWindow + 1);
                if (t > 1.0f) t = 1.0f;
                float radiusScale = 1.0f - t * 0.75f;
                if (radiusScale < 0.28f)
                    radiusScale = 0.28f;
                float opacity = 1.0f - t * 0.8f;
                if (opacity < 0.25f)
                    opacity = 0.25f;
                int16_t cx = (int16_t)(left + slot * (int16_t)spacing);
                int16_t r = (int16_t)((float)dotRadius * radiusScale + 0.5f);
                if (r < 1)
                    r = 1;
                uint16_t color = activeColor;
                if (i != (int)activeIndex)
                {
                    uint8_t a = (uint8_t)(opacity * 255.0f + 0.5f);
                    uint16_t bg565 = detail::color888To565(_render.bgColor);
                    color = (uint16_t)detail::blend565(bg565, inactiveColor, a);
                }
                fillCircle(cx, baseY, r, color);
            }
        }
        else
        {
            for (uint8_t i = 0; i < count; i++)
            {
                int16_t cx = (int16_t)(baseX0 + (int16_t)i * (int16_t)spacing);
                fillCircle(cx, baseY, (int16_t)dotRadius, inactiveColor);
            }
        }

        if (count == 0)
            return;

        if (count == 1)
        {
            fillCircle(baseX0, baseY, (int16_t)dotRadius, activeColor);
            return;
        }

        if (animate)
        {
            const float p = navProgress;
            float xPrev = (float)(baseX0 + (int16_t)((int)prevIndex - (int)activeIndex) * (int16_t)spacing);
            float xCurr = (float)baseX0;
            if (!taper)
            {
                xPrev = (float)(baseX0 + (int16_t)prevIndex * (int16_t)spacing);
                xCurr = (float)(baseX0 + (int16_t)activeIndex * (int16_t)spacing);
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

            if (gap <= (int16_t)(dotRadius * 2))
            {
                int16_t cx = (int16_t)((lx + rx) / 2);
                fillCircle(cx, baseY, (int16_t)dotRadius, activeColor);
            }
            else
            {
                int16_t pillX = (int16_t)(lx - (int16_t)dotRadius);
                int16_t pillW = (int16_t)(gap + (int16_t)dotRadius * 2);
                int16_t pillY = (int16_t)(baseY - (int16_t)dotRadius);
                uint8_t rad = (uint8_t)(dotRadius < 255 ? dotRadius : 255);
                fillRoundRect(pillX, pillY, pillW, (int16_t)((int16_t)dotRadius * 2), rad, activeColor);
            }
        }
        else if (!taper)
        {
            int16_t cx = baseX0 + (int16_t)activeIndex * (int16_t)spacing;
            fillCircle(cx, baseY, (int16_t)dotRadius, activeColor);
        }
    }
}

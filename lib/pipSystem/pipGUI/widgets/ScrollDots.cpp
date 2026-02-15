#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>

namespace pipgui
{
    static inline void calcDotsWindow(uint8_t count, uint8_t activeIndex,
                                      uint8_t &startIndex, uint8_t &visibleCount)
    {
        // iOS-like dots: show a limited window around the active index.
        // Keep it odd so active tends to be centered.
        const uint8_t maxVisible = 9;
        if (count <= maxVisible)
        {
            startIndex = 0;
            visibleCount = count;
            return;
        }

        visibleCount = maxVisible;

        int16_t half = (int16_t)maxVisible / 2;
        int16_t start = (int16_t)activeIndex - half;
        if (start < 0)
            start = 0;
        int16_t maxStart = (int16_t)count - (int16_t)maxVisible;
        if (start > maxStart)
            start = maxStart;
        startIndex = (uint8_t)start;
    }

    void GUI::updateScrollDots(int16_t x, int16_t y,
                               uint8_t count,
                               uint8_t activeIndex,
                               uint8_t prevIndex,
                               float animProgress,
                               bool animate,
                               int8_t animDirection,
                               uint32_t activeColor,
                               uint32_t inactiveColor,
                               uint8_t dotRadius,
                               uint8_t spacing,
                               uint8_t activeWidth)
    {
        if (count == 0)
            return;

        if (!_flags.spriteEnabled || !_display)
        {
            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawScrollDots(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                           activeColor, inactiveColor, dotRadius, spacing, activeWidth);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        if (dotRadius < 1)
            dotRadius = 1;
        if (spacing < (uint8_t)(dotRadius * 2 + 2))
            spacing = (uint8_t)(dotRadius * 2 + 2);
        if (activeWidth < (uint8_t)(dotRadius * 2 + 1))
            activeWidth = (uint8_t)(dotRadius * 2 + 1);

        uint8_t startIndex = 0;
        uint8_t visibleCount = 0;
        if (activeIndex >= count)
            activeIndex = (uint8_t)(count - 1);
        calcDotsWindow(count, activeIndex, startIndex, visibleCount);

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW = (visibleCount <= 1) ? (int16_t)activeWidth : (int16_t)((visibleCount - 1) * (int16_t)spacing + (int16_t)activeWidth);

        int16_t left = x;
        if (left == center)
            left = AutoX(totalW);

        int16_t top = y;
        if (top == center)
            top = AutoY(h);

        int16_t pad = 2;
        int16_t rx = left;
        int16_t ry = top;
        int16_t rw = (int16_t)(totalW + pad * 2);
        int16_t rh = (int16_t)(h + pad * 2);

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), rw, rh, _bgColor);
        drawScrollDots(left, top, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                       activeColor, inactiveColor, dotRadius, spacing, activeWidth);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), rw, rh);
        flushDirty();
    }

    void GUI::drawScrollDots(int16_t x, int16_t y,
                            uint8_t count,
                            uint8_t activeIndex,
                            uint8_t prevIndex,
                            float animProgress,
                            bool animate,
                            int8_t animDirection,
                            uint32_t activeColor,
                            uint32_t inactiveColor,
                            uint8_t dotRadius,
                            uint8_t spacing,
                            uint8_t activeWidth)
    {
        if (count == 0)
            return;

        if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
        {
            updateScrollDots(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                             activeColor, inactiveColor, dotRadius, spacing, activeWidth);
            return;
        }

        if (spacing < (uint8_t)(dotRadius * 2 + 2))
            spacing = (uint8_t)(dotRadius * 2 + 2);
        if (activeWidth < (uint8_t)(dotRadius * 2 + 1))
            activeWidth = (uint8_t)(dotRadius * 2 + 1);

        uint8_t startIndex = 0;
        uint8_t visibleCount = 0;
        calcDotsWindow(count, activeIndex, startIndex, visibleCount);

        uint8_t localActive = (activeIndex >= startIndex) ? (uint8_t)(activeIndex - startIndex) : 0;
        uint8_t localPrev = localActive;
        if (prevIndex >= startIndex)
        {
            uint8_t lp = (uint8_t)(prevIndex - startIndex);
            if (lp < visibleCount)
                localPrev = lp;
        }

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW = (visibleCount <= 1) ? (int16_t)activeWidth : (int16_t)((visibleCount - 1) * (int16_t)spacing + (int16_t)activeWidth);

        int16_t left = x;
        if (left == center)
            left = AutoX(totalW);

        int16_t top = y;
        if (top == center)
            top = AutoY(h);

        int16_t baseY = top + h / 2;
        int16_t baseX0 = left + (int16_t)activeWidth / 2;

        fillRect(left, top, totalW, h, _bgColor);

        bool hasLeftMore = (startIndex > 0);
        bool hasRightMore = (startIndex + visibleCount < count);

        auto radiusForLocal = [&](uint8_t local) -> uint8_t
        {
            if (dotRadius <= 1)
                return dotRadius;
            // Shrink edge dots when there are more pages outside the window.
            // 2-step falloff: outermost smaller.
            if (hasLeftMore)
            {
                if (local == 0)
                    return (uint8_t)std::max(1, (int)dotRadius / 2);
                if (local == 1)
                    return (uint8_t)std::max(1, (int)(dotRadius * 3) / 4);
            }
            if (hasRightMore)
            {
                if (local + 1 == visibleCount)
                    return (uint8_t)std::max(1, (int)dotRadius / 2);
                if (local + 2 == visibleCount)
                    return (uint8_t)std::max(1, (int)(dotRadius * 3) / 4);
            }
            return dotRadius;
        };

        for (uint8_t i = 0; i < visibleCount; i++)
        {
            int16_t cx = (int16_t)(baseX0 + (int16_t)i * (int16_t)spacing);
            uint8_t rr = radiusForLocal(i);
            fillCircleFrc(cx, baseY, (int16_t)rr, inactiveColor);
        }

        if (count == 1)
        {
            fillCircleFrc(baseX0, baseY, (int16_t)dotRadius, activeColor);
            return;
        }

        if (animate)
        {
            float xPrev = (float)(baseX0 + (int16_t)localPrev * (int16_t)spacing);
            float xCurr = (float)(baseX0 + (int16_t)localActive * (int16_t)spacing);
            float drawnLeftX = 0.0f;
            float drawnRightX = 0.0f;

            if (animProgress < 0.5f)
            {
                float normalized_p = animProgress * 2.0f;
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
                float normalized_p = (animProgress - 0.5f) * 2.0f;
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

            // Active indicator keeps full radius.
            fillCircleFrc(lx, baseY, (int16_t)dotRadius, activeColor);
            fillCircleFrc(rx, baseY, (int16_t)dotRadius, activeColor);

            if (fabsf(drawnRightX - drawnLeftX) > (float)dotRadius * 0.5f)
            {
                fillRect(lx, (int16_t)(baseY - (int16_t)dotRadius), (int16_t)(rx - lx), h, activeColor);
            }
        }
        else
        {
            int16_t cx = baseX0 + (int16_t)localActive * (int16_t)spacing;
            fillCircleFrc(cx, baseY, (int16_t)dotRadius, activeColor);
        }
    }
}
#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>

namespace pipgui
{
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

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW = (count <= 1) ? (int16_t)activeWidth : (int16_t)((count - 1) * (int16_t)spacing + (int16_t)activeWidth);

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

        if (activeIndex >= count)
            activeIndex = (uint8_t)(count - 1);
        if (prevIndex >= count)
            prevIndex = activeIndex;

        if (animProgress < 0.0f)
            animProgress = 0.0f;
        if (animProgress > 1.0f)
            animProgress = 1.0f;

        if (dotRadius < 1)
            dotRadius = 1;
        if (spacing < (uint8_t)(dotRadius * 2 + 2))
            spacing = (uint8_t)(dotRadius * 2 + 2);
        if (activeWidth < (uint8_t)(dotRadius * 2 + 1))
            activeWidth = (uint8_t)(dotRadius * 2 + 1);

        auto t = getDrawTarget();
        if (!t)
            return;

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW = (count <= 1) ? (int16_t)activeWidth : (int16_t)((count - 1) * (int16_t)spacing + (int16_t)activeWidth);

        int16_t left = x;
        if (left == center)
            left = AutoX(totalW);

        int16_t top = y;
        if (top == center)
            top = AutoY(h);

        int16_t baseY = top + h / 2;
        int16_t baseX0 = left + (int16_t)activeWidth / 2;

        fillRect(left, top, totalW, h, _bgColor);

        for (uint8_t i = 0; i < count; i++)
        {
            int16_t cx = (int16_t)(baseX0 + (int16_t)i * (int16_t)spacing);
            fillCircleFrc(cx, baseY, (int16_t)dotRadius, inactiveColor);
        }

        if (count == 1)
        {
            fillCircleFrc(baseX0, baseY, (int16_t)dotRadius, activeColor);
            return;
        }

        if (animate)
        {
            float xPrev = (float)(baseX0 + (int16_t)prevIndex * (int16_t)spacing);
            float xCurr = (float)(baseX0 + (int16_t)activeIndex * (int16_t)spacing);
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

            fillCircleFrc(lx, baseY, (int16_t)dotRadius, activeColor);
            fillCircleFrc(rx, baseY, (int16_t)dotRadius, activeColor);

            if (fabsf(drawnRightX - drawnLeftX) > (float)dotRadius * 0.5f)
            {
                fillRect(lx, (int16_t)(baseY - (int16_t)dotRadius), (int16_t)(rx - lx), h, activeColor);
            }
        }
        else
        {
            int16_t cx = baseX0 + (int16_t)activeIndex * (int16_t)spacing;
            fillCircleFrc(cx, baseY, (int16_t)dotRadius, activeColor);
        }
    }
}
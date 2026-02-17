#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        float easeInOutCubic(float t)
        {
            if (t <= 0.0f) return 0.0f;
            if (t >= 1.0f) return 1.0f;
            if (t < 0.5f)
                return 4.0f * t * t * t;
            float f = 2.0f * t - 2.0f;
            return 0.5f * f * f * f + 1.0f;
        }
    }
    void GUI::updateScrollDotsImpl(int16_t x, int16_t y,
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
            drawScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
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

        const uint8_t maxVisibleForTaper = 7;
        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW = (count <= 1) ? (int16_t)activeWidth
            : (count > maxVisibleForTaper) ? (int16_t)(maxVisibleForTaper * (int16_t)spacing)
            : (int16_t)((count - 1) * (int16_t)spacing + (int16_t)activeWidth);

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

        fillRect()
            .at((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size(rw, rh)
            .color(_bgColor)
            .draw();
        drawScrollDotsImpl(left, top, count, activeIndex, prevIndex, animProgress, animate, animDirection,
                       activeColor, inactiveColor, dotRadius, spacing, activeWidth);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), rw, rh);
        flushDirty();
    }

    void GUI::drawScrollDotsImpl(int16_t x, int16_t y,
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
            updateScrollDotsImpl(x, y, count, activeIndex, prevIndex, animProgress, animate, animDirection,
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

        const uint8_t maxVisibleForTaper = 7;
        const int halfWindow = (int)(maxVisibleForTaper / 2);
        bool taper = (count > maxVisibleForTaper);

        int16_t h = (int16_t)(dotRadius * 2 + 1);
        int16_t totalW;
        int16_t left;
        int16_t baseX0;
        if (taper)
        {
            totalW = (int16_t)(maxVisibleForTaper * (int16_t)spacing);
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
            .at(left, top)
            .size(totalW, h)
            .color(_bgColor)
            .draw();

        if (taper)
        {
            for (int slot = 0; slot < maxVisibleForTaper; slot++)
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
                uint32_t color = (i == (int)activeIndex)
                    ? activeColor
                    : detail::blend888(_bgColor, inactiveColor, (uint8_t)(opacity * 255.0f + 0.5f));
                fillCircleFrc(cx, baseY, r, color);
            }
        }
        else
        {
            for (uint8_t i = 0; i < count; i++)
            {
                int16_t cx = (int16_t)(baseX0 + (int16_t)i * (int16_t)spacing);
                fillCircleFrc(cx, baseY, (int16_t)dotRadius, inactiveColor);
            }
        }

        if (count == 1)
        {
            fillCircleFrc(baseX0, baseY, (int16_t)dotRadius, activeColor);
            return;
        }

        if (animate)
        {
            float p = easeInOutCubic(animProgress);
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
                fillCircleFrc(cx, baseY, (int16_t)dotRadius, activeColor);
            }
            else
            {
                int16_t pillX = (int16_t)(lx - (int16_t)dotRadius);
                int16_t pillW = (int16_t)(gap + (int16_t)dotRadius * 2);
                int16_t pillY = (int16_t)(baseY - (int16_t)dotRadius);
                uint8_t rad = (uint8_t)(dotRadius < 255 ? dotRadius : 255);
                fillRoundRectFrc(pillX, pillY, pillW, (int16_t)((int16_t)dotRadius * 2), rad, activeColor);
            }
        }
        else if (!taper)
        {
            int16_t cx = baseX0 + (int16_t)activeIndex * (int16_t)spacing;
            fillCircleFrc(cx, baseY, (int16_t)dotRadius, activeColor);
        }
    }
}
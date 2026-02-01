#include <pipGUI/core/api/pipGUI.h>
#include <pipGUI/core/utils/Colors.h>
#include <math.h>

namespace pipgui
{
    static uint16_t blendWhite(uint16_t color, uint8_t intensity)
    {
        if (intensity == 0)
            return color;
        if (intensity >= 255)
            return 0xFFFF;

        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;

        r += ((31 - r) * intensity) >> 8;
        g += ((63 - g) * intensity) >> 8;
        b += ((31 - b) * intensity) >> 8;

        return (uint16_t)((r << 11) | (g << 5) | b);
    }

    template <typename ColorFunc>
    static void drawRingArcStroke(LovyanGFX *t,
                                  int16_t cx, int16_t cy,
                                  int16_t r,
                                  uint8_t thickness,
                                  float startDeg,
                                  float endDeg,
                                  ColorFunc colorAtAngle)
    {
        if (!t)
            return;
        if (r <= 0)
            return;
        if (thickness < 1)
            thickness = 1;
        if (startDeg > endDeg)
            return;

        int16_t dotR = (thickness >= 2) ? (int16_t)(thickness / 2) : 1;
        float rMid = (float)r - (float)dotR;
        if (rMid < 1.0f)
            rMid = 1.0f;

        float maxStepRad = ((float)dotR * 0.85f) / rMid;
        float stepDeg = maxStepRad * 57.2957795f;
        if (stepDeg < 1.0f)
            stepDeg = 1.0f;
        if (stepDeg > 6.0f)
            stepDeg = 6.0f;

        t->startWrite();
        for (float a = startDeg; a <= endDeg + 0.001f; a += stepDeg)
        {
            float rad = (a - 90.0f) * 0.01745329252f;
            int16_t px = cx + (int16_t)roundf(cosf(rad) * rMid);
            int16_t py = cy + (int16_t)roundf(sinf(rad) * rMid);
            uint16_t col = colorAtAngle(a);
            t->fillSmoothCircle(px, py, dotR, col);
        }
        t->endWrite();
    }

    template <typename ColorFunc>
    static void drawRingArcStrokeWrapped(LovyanGFX *t,
                                         int16_t cx, int16_t cy,
                                         int16_t r,
                                         uint8_t thickness,
                                         float startDeg,
                                         float endDeg,
                                         ColorFunc colorAtAngle)
    {
        if (startDeg > endDeg)
            return;

        while (startDeg < 0.0f)
            startDeg += 360.0f;
        while (startDeg >= 360.0f)
            startDeg -= 360.0f;

        while (endDeg < 0.0f)
            endDeg += 360.0f;
        while (endDeg >= 360.0f)
            endDeg -= 360.0f;

        if (startDeg <= endDeg)
        {
            drawRingArcStroke(t, cx, cy, r, thickness, startDeg, endDeg, colorAtAngle);
        }
        else
        {
            drawRingArcStroke(t, cx, cy, r, thickness, startDeg, 360.0f, colorAtAngle);
            drawRingArcStroke(t, cx, cy, r, thickness, 0.0f, endDeg, colorAtAngle);
        }
    }

    void GUI::drawProgressBar(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t value,
                              uint16_t baseColor,
                              uint16_t fillColor,
                              uint8_t radius,
                              ProgressAnim anim)
    {
        if (w <= 0 || h <= 0)
            return;
        if (value > 100)
            value = 100;

        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            updateProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        auto t = getDrawTarget();

        if (x == center)
        {
            int16_t left = 0;
            int16_t availW = _screenWidth;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Left)
                {
                    left += _statusBarHeight;
                    availW -= _statusBarHeight;
                }
                else if (_statusBarPos == Right)
                {
                    availW -= _statusBarHeight;
                }
            }
            if (availW < w)
                availW = w;
            x = left + (availW - w) / 2;
        }
        if (y == center)
        {
            int16_t top = 0;
            int16_t availH = _screenHeight;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Top)
                {
                    top += _statusBarHeight;
                    availH -= _statusBarHeight;
                }
                else if (_statusBarPos == Bottom)
                {
                    availH -= _statusBarHeight;
                }
            }
            if (availH < h)
                availH = h;
            y = top + (availH - h) / 2;
        }

        uint8_t r = radius;
        if (r > h / 2)
            r = h / 2;
        if (r < 1)
            r = 0;

        t->fillSmoothRoundRect(x, y, w, h, r, baseColor);

        int16_t innerX = x;
        int16_t innerY = y;
        int16_t innerW = w;
        int16_t innerH = h;

        int16_t fillR = r;

        if (anim == Indeterminate)
        {
            uint32_t now = nowMs();

            int16_t segmentW = innerW / 4;
            if (segmentW < 20)
                segmentW = 20;
            if (segmentW > innerW)
                segmentW = innerW;

            uint32_t animPeriod = 1600;
            int32_t totalDist = (int32_t)innerW + segmentW;

            float p = (float)(now % animPeriod) / (float)animPeriod;
            float eased = p * p * (3.0f - 2.0f * p);
            int32_t offset = (int32_t)(eased * (float)totalDist) - segmentW;

            int16_t segX = innerX + (int16_t)offset;
            int16_t segLeft = segX;
            int16_t segRight = segX + segmentW;
            if (segLeft < innerX)
                segLeft = innerX;
            if (segRight > innerX + innerW)
                segRight = innerX + innerW;
            int16_t segW = segRight - segLeft;

            if (segW > 0)
                t->fillSmoothRoundRect(segLeft, innerY, segW, innerH, fillR, fillColor);

            return;
        }

        if (value == 0)
            return;

        int16_t fillW = (int16_t)((innerW * (uint16_t)value) / 100U);
        if (fillW <= 0)
            return;
        if (fillW > innerW)
            fillW = innerW;

        t->fillSmoothRoundRect(innerX, innerY, fillW, innerH, fillR, fillColor);

        if (anim == ProgressAnimNone)
            return;

        uint32_t now = nowMs();

        auto getCornerOffset = [&](int16_t px) -> int16_t
        {
            int16_t offsetLeft = 0;
            int16_t offsetRight = 0;

            if (px < innerX + fillR)
            {
                int16_t dx = (innerX + fillR) - px - 1;
                if (dx < 0)
                    dx = 0;
                float dy = sqrtf((float)(fillR * fillR) - (float)(dx * dx));
                offsetLeft = (int16_t)(fillR - dy);
            }

            int16_t rightCircleCenter = innerX + fillW - fillR;
            if (px >= rightCircleCenter)
            {
                int16_t dx = px - rightCircleCenter;
                if (dx >= fillR)
                    dx = fillR - 1;
                if (dx < 0)
                    dx = 0;

                float dy = sqrtf((float)(fillR * fillR) - (float)(dx * dx));
                offsetRight = (int16_t)(fillR - dy);
            }

            return (offsetLeft > offsetRight) ? offsetLeft : offsetRight;
        };

        if (anim == Stripes)
        {
            uint16_t stripeColor = pipgui::detail::lighten565(fillColor, 20);
            int16_t stripeW = 8;
            int16_t gapW = 8;
            int16_t period = stripeW + gapW;
            int16_t phase = (int16_t)((now / 35U) % (uint32_t)period);

            t->startWrite();
            for (int16_t px = innerX; px < innerX + fillW; ++px)
            {
                int16_t local = (px - innerX + period - phase) % period;
                if (local < stripeW)
                {
                    int16_t offset = getCornerOffset(px);
                    int16_t lineH = innerH - (offset * 2);
                    if (lineH > 0)
                        t->writeFastVLine(px, innerY + offset, lineH, stripeColor);
                }
            }
            t->endWrite();
        }
        else if (anim == Shimmer)
        {
            int16_t bandW = (innerW * 2) / 3;
            if (bandW < 40)
                bandW = 40;
            int16_t totalDist = innerW + bandW;
            uint32_t animPeriod = 2000;
            int16_t progressShift = (int16_t)((now % animPeriod) * totalDist / animPeriod);

            int16_t shimmerLeft = innerX - bandW + progressShift;
            int16_t shimmerRight = shimmerLeft + bandW;
            int16_t shimmerCenter = shimmerLeft + bandW / 2;
            int16_t halfBand = bandW / 2;

            int16_t barLeft = innerX;
            int16_t barRight = innerX + fillW;

            int16_t drawLeft = (shimmerLeft > barLeft) ? shimmerLeft : barLeft;
            int16_t drawRight = (shimmerRight < barRight) ? shimmerRight : barRight;

            if (drawRight > drawLeft)
            {
                t->startWrite();
                for (int16_t px = drawLeft; px < drawRight; ++px)
                {
                    int16_t dist = abs(px - shimmerCenter);
                    float norm = (float)dist / (float)halfBand;
                    if (norm > 1.0f)
                        norm = 1.0f;
                    float curve = 1.0f - (norm * norm);
                    uint8_t intensity = (uint8_t)(110.0f * curve);

                    if (intensity < 2)
                        continue;
                    uint16_t col = blendWhite(fillColor, intensity);

                    int16_t offset = getCornerOffset(px);
                    int16_t lineH = innerH - (offset * 2);
                    if (lineH > 0)
                        t->writeFastVLine(px, innerY + offset, lineH, col);
                }
                t->endWrite();
            }
        }
    }

    void GUI::drawProgressBar(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t value,
                              uint16_t color,
                              uint8_t radius,
                              ProgressAnim anim)
    {
        uint16_t base = pipgui::detail::lighten565(color, 10);
        drawProgressBar(x, y, w, h, value, base, color, radius, anim);
    }

    void GUI::drawCircularProgressBar(int16_t x, int16_t y,
                                      int16_t r,
                                      uint8_t thickness,
                                      uint8_t value,
                                      uint16_t baseColor,
                                      uint16_t fillColor,
                                      ProgressAnim anim)
    {
        if (r <= 0)
            return;
        if (value > 100)
            value = 100;
        if (thickness < 1)
            thickness = 1;

        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            updateCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
            return;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        int16_t cx = x;
        int16_t cy = y;

        if (cx == center)
        {
            int16_t left = 0;
            int16_t availW = _screenWidth;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Left)
                {
                    left += _statusBarHeight;
                    availW -= _statusBarHeight;
                }
                else if (_statusBarPos == Right)
                {
                    availW -= _statusBarHeight;
                }
            }
            cx = left + availW / 2;
        }

        if (cy == center)
        {
            int16_t top = 0;
            int16_t availH = _screenHeight;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Top)
                {
                    top += _statusBarHeight;
                    availH -= _statusBarHeight;
                }
                else if (_statusBarPos == Bottom)
                {
                    availH -= _statusBarHeight;
                }
            }
            cy = top + availH / 2;
        }
        drawRingArcStroke(t, cx, cy, r, thickness, 0.0f, 360.0f, [&](float) -> uint16_t
                          { return baseColor; });

        if (anim == Indeterminate)
        {
            uint32_t now = nowMs();
            uint32_t animPeriod = 1600;
            float p = (float)(now % animPeriod) / (float)animPeriod;
            float eased = p * p * (3.0f - 2.0f * p);

            float segDeg = 70.0f;
            float pos = eased * 360.0f;
            float startDeg = pos - segDeg * 0.5f;
            float endDeg = pos + segDeg * 0.5f;

            drawRingArcStrokeWrapped(t, cx, cy, r, thickness, startDeg, endDeg, [&](float) -> uint16_t
                                     { return fillColor; });
            return;
        }

        if (value == 0)
            return;

        float fillSpan = (float)value * 360.0f / 100.0f;
        if (fillSpan <= 0.0f)
            return;
        if (fillSpan > 360.0f)
            fillSpan = 360.0f;

        if (anim == ProgressAnimNone)
        {
            drawRingArcStroke(t, cx, cy, r, thickness, 0.0f, fillSpan, [&](float) -> uint16_t
                              { return fillColor; });
            return;
        }

        uint32_t now = nowMs();

        if (anim == Stripes)
        {
            uint16_t stripeColor = pipgui::detail::lighten565(fillColor, 20);
            int16_t stripeW = 12;
            int16_t gapW = 12;
            int16_t period = stripeW + gapW;
            int16_t phase = (int16_t)((now / 35U) % (uint32_t)period);

            drawRingArcStroke(t, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint16_t
                              {
                                  int32_t ai = (int32_t)(a + 0.5f);
                                  int16_t local = (int16_t)((ai + period - phase) % period);
                                  return (local < stripeW) ? stripeColor : fillColor;
                              });
        }
        else if (anim == Shimmer)
        {
            float bandW = (fillSpan * 2.0f) / 3.0f;
            if (bandW < 50.0f)
                bandW = 50.0f;
            float totalDist = fillSpan + bandW;
            uint32_t animPeriod = 2000;
            float shift = (float)(now % animPeriod) * totalDist / (float)animPeriod;

            float shimmerLeft = -bandW + shift;
            float shimmerCenter = shimmerLeft + bandW / 2.0f;
            float halfBand = bandW / 2.0f;

            drawRingArcStroke(t, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint16_t
                              {
                                  float dist = fabsf(a - shimmerCenter);
                                  float norm = dist / halfBand;
                                  if (norm > 1.0f)
                                      norm = 1.0f;
                                  float curve = 1.0f - (norm * norm);
                                  uint8_t intensity = (uint8_t)(110.0f * curve);
                                  if (intensity < 2)
                                      return fillColor;
                                  return blendWhite(fillColor, intensity);
                              });
        }
    }

    void GUI::drawCircularProgressBar(int16_t x, int16_t y,
                                      int16_t r,
                                      uint8_t thickness,
                                      uint8_t value,
                                      uint16_t color,
                                      ProgressAnim anim)
    {
        uint16_t base = pipgui::detail::lighten565(color, 10);
        drawCircularProgressBar(x, y, r, thickness, value, base, color, anim);
    }

    void GUI::updateProgressBar(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t value,
                               uint16_t baseColor,
                               uint16_t fillColor,
                               uint8_t radius,
                               ProgressAnim anim)
    {
        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t pad = 2;

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        _sprite.fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2), _bgColor);
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
        flushDirty();
    }

    void GUI::updateProgressBar(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t value,
                               uint16_t color,
                               uint8_t radius,
                               ProgressAnim anim)
    {
        uint16_t base = pipgui::detail::lighten565(color, 10);
        updateProgressBar(x, y, w, h, value, base, color, radius, anim);
    }

    void GUI::updateProgressBar(ProgressBarState &s,
                               int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t value,
                               uint16_t baseColor,
                               uint16_t fillColor,
                               uint8_t radius,
                               ProgressAnim anim)
    {
        if (w <= 0 || h <= 0)
            return;

        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t pad = 2;

        if (value > 100)
            value = 100;

        bool needFull = !s.inited || s.anim != anim;
        if (anim != ProgressAnimNone && anim != None)
            needFull = true;
        if (needFull)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;

            _sprite.fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2), _bgColor);
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;

            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
            flushDirty();

            s.inited = true;
            s.value = value;
            s.anim = anim;
            s.segL = 0;
            s.segR = 0;
            return;
        }

        int16_t innerW = w;
        int16_t fillWPrev = (int16_t)((innerW * (uint16_t)s.value) / 100U);
        int16_t fillWNew = (int16_t)((innerW * (uint16_t)value) / 100U);
        if (fillWPrev < 0)
            fillWPrev = 0;
        if (fillWNew < 0)
            fillWNew = 0;
        if (fillWPrev > innerW)
            fillWPrev = innerW;
        if (fillWNew > innerW)
            fillWNew = innerW;

        int16_t l = (fillWPrev < fillWNew) ? fillWPrev : fillWNew;
        int16_t r = (fillWPrev > fillWNew) ? fillWPrev : fillWNew;
        if (l == r)
        {
            s.value = value;
            return;
        }

        int16_t cx = (int16_t)(rx + l - pad);
        int16_t cw = (int16_t)((r - l) + pad * 2);
        if (cx < rx - pad)
            cx = rx - pad;
        if (cx + cw > rx + w + pad)
            cw = (int16_t)((rx + w + pad) - cx);

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);
        _sprite.setClipRect(cx, (int16_t)(ry - pad), cw, (int16_t)(h + pad * 2));

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        _sprite.fillRect(cx, (int16_t)(ry - pad), cw, (int16_t)(h + pad * 2), _bgColor);
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        _sprite.setClipRect(clipX, clipY, clipW, clipH);

        invalidateRect(cx, (int16_t)(ry - pad), cw, (int16_t)(h + pad * 2));
        flushDirty();

        s.value = value;
        s.anim = anim;
    }

    void GUI::updateCircularProgressBar(int16_t x, int16_t y,
                                        int16_t r,
                                        uint8_t thickness,
                                        uint8_t value,
                                        uint16_t baseColor,
                                        uint16_t fillColor,
                                        ProgressAnim anim)
    {
        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        int16_t cx = x;
        int16_t cy = y;
        if (cx == center)
            cx = AutoX(0);
        if (cy == center)
            cy = AutoY(0);

        int16_t pad = 2;
        int16_t rr = (int16_t)(r + pad);
        _sprite.fillRect((int16_t)(cx - rr), (int16_t)(cy - rr), (int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1), _bgColor);
        drawCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(cx - rr), (int16_t)(cy - rr), (int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1));
        flushDirty();
    }

    void GUI::updateCircularProgressBar(int16_t x, int16_t y,
                                        int16_t r,
                                        uint8_t thickness,
                                        uint8_t value,
                                        uint16_t color,
                                        ProgressAnim anim)
    {
        uint16_t base = pipgui::detail::lighten565(color, 10);
        updateCircularProgressBar(x, y, r, thickness, value, base, color, anim);
    }
}

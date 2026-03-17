#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <math.h>

namespace pipgui
{
    static uint16_t blendWhite(uint16_t color, uint8_t intensity)
    {
        if (intensity == 0)
            return color;
        return (uint16_t)pipgui::detail::blend565(color, (uint16_t)0xFFFF, intensity);
    }

    template <typename ColorFunc>
    static void drawRingArcStrokeArc(GUI &g,
                                     int16_t cx, int16_t cy,
                                     int16_t r,
                                     uint8_t thickness,
                                     float startDeg,
                                     float endDeg,
                                     ColorFunc colorAtAngle)
    {
        if (r <= 0)
            return;
        if (thickness < 1)
            thickness = 1;
        if (startDeg > endDeg)
            return;

        const int16_t outerR = r;
        int16_t innerR = (int16_t)(r - (int16_t)thickness + 1);
        if (innerR < 1)
            innerR = 1;

        float segStep = 1.5f;
        float radStep = segStep * 0.01745329252f;
        float rr = (float)((outerR + innerR) / 2);
        if (rr < 1.0f)
            rr = 1.0f;

        float maxStepRad = radStep;
        if (thickness > 2)
            maxStepRad = std::min(0.10f, (float)thickness * 0.015f);

        float stepDeg = maxStepRad * 57.2957795f;
        if (stepDeg < 1.0f)
            stepDeg = 1.0f;
        if (stepDeg > 6.0f)
            stepDeg = 6.0f;

        for (float a = startDeg; a < endDeg - 0.0001f; a += stepDeg)
        {
            float a0 = a;
            float a1 = a + stepDeg;
            if (a1 > endDeg)
                a1 = endDeg;

            float mid = (a0 + a1) * 0.5f;
            uint16_t col = colorAtAngle(mid);

            const float g0 = 90.0f - a0;
            const float g1 = 90.0f - a1;

            for (int16_t rrInt = innerR; rrInt <= outerR; ++rrInt)
                g.drawArc()
                    .pos(cx, cy)
                    .radius(rrInt)
                    .startDeg(g0)
                    .endDeg(g1)
                    .color(col)
                    .draw();
        }
    }

    template <typename ColorFunc>
    static void drawRingArcCaps(GUI &g,
                                int16_t cx, int16_t cy,
                                int16_t r,
                                uint8_t thickness,
                                float startDeg,
                                float endDeg,
                                ColorFunc colorAtAngle)
    {
        if (r <= 0)
            return;
        if (thickness < 1)
            thickness = 1;
        if (startDeg > endDeg)
            return;

        const int16_t capR = (thickness >= 3) ? (int16_t)(thickness / 2) : 1;
        float rMid = (float)r - (float)capR;
        if (rMid < 1.0f)
            rMid = 1.0f;

        auto drawCapAt = [&](float deg)
        {
            float rad = (90.0f - deg) * 0.01745329252f;
            int16_t px = (int16_t)lroundf((float)cx + cosf(rad) * rMid);
            int16_t py = (int16_t)lroundf((float)cy + sinf(rad) * rMid);
            uint16_t col = colorAtAngle(deg);
            g.fillCircle().pos(px, py).radius(capR).color(col).draw();
        };

        drawCapAt(startDeg);
        if (endDeg != startDeg)
            drawCapAt(endDeg);
    }

    template <typename ColorFunc>
    static void drawRingArcStrokeArcWrapped(GUI &g,
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
            drawRingArcStrokeArc(g, cx, cy, r, thickness, startDeg, endDeg, colorAtAngle);
        }
        else
        {
            drawRingArcStrokeArc(g, cx, cy, r, thickness, startDeg, 360.0f, colorAtAngle);
            drawRingArcStrokeArc(g, cx, cy, r, thickness, 0.0f, endDeg, colorAtAngle);
        }
    }

    static inline uint16_t lighten565Progress(uint16_t c, uint8_t amount)
    {
        uint16_t a = (uint16_t)amount * 8U;
        if (a > 255U)
            a = 255U;
        return (uint16_t)detail::blend565(c, (uint16_t)0xFFFF, (uint8_t)a);
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

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        auto t = getDrawTarget();

        if (x == center)
        {
            int16_t availW = _render.screenWidth;
            if (availW < w)
                availW = w;
            x = (availW - w) / 2;
        }
        if (y == center)
        {
            int16_t top = 0;
            int16_t availH = _render.screenHeight;
            int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0)
            {
                if (_status.pos == Top)
                {
                    top += sb;
                    availH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    availH -= sb;
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

        fillRoundRect(x, y, w, h, r, baseColor);

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
                fillRoundRect(segLeft, innerY, segW, innerH, (uint8_t)fillR, fillColor);

            return;
        }

        if (value == 0)
            return;

        int16_t fillW = (int16_t)((innerW * (uint16_t)value) / 100U);
        if (fillW <= 0)
            return;
        if (fillW > innerW)
            fillW = innerW;

        fillRoundRect(innerX, innerY, fillW, innerH, (uint8_t)fillR, fillColor);

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

        if (anim == Shimmer)
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
                        fillRect(px, innerY + offset, 1, lineH, col);
                }
            }
        }
    }

    // -------- Progress text helpers (percentage or custom text) --------

    void GUI::drawProgressText(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               const String &text,
                               TextAlign align,
                               uint32_t textColor,
                               uint32_t bgColor,
                               uint16_t fontPx)
    {
        if (w <= 0 || h <= 0 || text.length() == 0)
            return;

        // Route through sprite update path when rendering to display
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;
            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;

            drawProgressText(x, y, w, h, text, align, textColor, bgColor, fontPx);

            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;
            flushDirty();
            return;
        }

        if (x == center)
            x = AutoX(w);
        if (y == center)
            y = AutoY(h);

        if (fontPx == 0)
        {
            uint16_t target = (uint16_t)((h > 6) ? (h - 4) : h);
            if (target < 8)
                target = 8;
            fontPx = target;
        }

        uint16_t prevSize = _typo.psdfSizePx;
        uint16_t prevWeight = _typo.psdfWeight;
        setFontSize(fontPx);
        setFontWeight(Medium);

        int16_t tw = 0, th = 0;
        measureText(text, tw, th);

        int16_t tx = x;
        int16_t ty = (int16_t)(y + (h - th) / 2);

        switch (align)
        {
        case AlignLeft:
            tx = (int16_t)(x + 4);
            break;
        case AlignRight:
            tx = (int16_t)(x + w - tw - 4);
            break;
        case AlignCenter:
        default:
            tx = (int16_t)(x + (w - tw) / 2);
            break;
        }

        uint16_t bg565 = detail::color888To565(bgColor);
        uint16_t fg565 = detail::color888To565(textColor);
        drawTextAligned(text, tx, ty, fg565, bg565, AlignLeft);

        setFontSize(prevSize);
        setFontWeight(prevWeight);
    }

    void GUI::drawProgressPercent(int16_t x, int16_t y,
                                  int16_t w, int16_t h,
                                  uint8_t value,
                                  TextAlign align,
                                  uint32_t textColor,
                                  uint32_t bgColor,
                                  uint16_t fontPx)
    {
        if (value > 100)
            value = 100;
        String s;
        s.reserve(5);
        s += String((int)value);
        s += "%";
        drawProgressText(x, y, w, h, s, align, textColor, bgColor, fontPx);
    }

    void GUI::drawProgressBar(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t value,
                              uint16_t color,
                              uint8_t radius,
                              ProgressAnim anim)
    {
        uint16_t base = lighten565Progress(color, 10);
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

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
            return;
        }

        int16_t cx = x;
        int16_t cy = y;

        if (cx == center)
        {
            int16_t availW = _render.screenWidth;
            cx = availW / 2;
        }

        if (cy == center)
        {
            int16_t top = 0;
            int16_t availH = _render.screenHeight;
            int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0)
            {
                if (_status.pos == Top)
                {
                    top += sb;
                    availH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    availH -= sb;
                }
            }
            cy = top + availH / 2;
        }

        drawRingArcStrokeArc(*this, cx, cy, r, thickness, 0.0f, 360.0f, [&](float) -> uint32_t
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

            drawRingArcStrokeArcWrapped(*this, cx, cy, r, thickness, startDeg, endDeg, [&](float) -> uint32_t
                                        { return fillColor; });
            drawRingArcCaps(*this, cx, cy, r, thickness, startDeg, endDeg, [&](float) -> uint32_t
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
            drawRingArcStrokeArc(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float) -> uint32_t
                                 { return fillColor; });

            drawRingArcCaps(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float) -> uint32_t
                            { return fillColor; });
            return;
        }

        uint32_t now = nowMs();

        if (anim == Shimmer)
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

            drawRingArcStrokeArc(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint32_t
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

            drawRingArcCaps(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint32_t
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
        uint16_t base = lighten565Progress(color, 10);
        drawCircularProgressBar(x, y, r, thickness, value, base, color, anim);
    }

    void GUI::updateProgressBar(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t value,
                               uint16_t baseColor,
                               uint16_t fillColor,
                               uint8_t radius,
                               ProgressAnim anim,
                               bool doFlush)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t pad = 2;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
            .color(detail::color888To565(_render.bgColor))
            .draw();
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
            if (doFlush)
                flushDirty();
        }
    }

    void GUI::updateProgressBar(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t value,
                               uint16_t color,
                               uint8_t radius,
                               ProgressAnim anim,
                               bool doFlush)
    {
        uint16_t base = lighten565Progress(color, 10);
        updateProgressBar(x, y, w, h, value, base, color, radius, anim, doFlush);
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

        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
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
            bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;

            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;

            fillRect()
                .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
                .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
                .color(detail::color888To565(_render.bgColor))
                .draw();
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;

            if (!prevRender)
            {
                invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
                flushDirty();
            }

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
        _render.sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);
        _render.sprite.setClipRect(cx, (int16_t)(ry - pad), cw, (int16_t)(h + pad * 2));

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos(cx, (int16_t)(ry - pad))
            .size(cw, (int16_t)(h + pad * 2))
            .color(detail::color888To565(_render.bgColor))
            .draw();
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        _render.sprite.setClipRect(clipX, clipY, clipW, clipH);

        if (!prevRender)
        {
            invalidateRect(cx, (int16_t)(ry - pad), cw, (int16_t)(h + pad * 2));
            flushDirty();
        }

        s.value = value;
        s.anim = anim;
    }

    void GUI::updateCircularProgressBar(int16_t x, int16_t y,
                                        int16_t r,
                                        uint8_t thickness,
                                        uint8_t value,
                                        uint16_t baseColor,
                                        uint16_t fillColor,
                                        ProgressAnim anim,
                                        bool doFlush)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
            return;
        }

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        int16_t cx = x;
        int16_t cy = y;
        if (cx == center)
            cx = AutoX(0);
        if (cy == center)
            cy = AutoY(0);

        int16_t pad = 2;
        int16_t rr = (int16_t)(r + pad);
        fillRect()
            .pos((int16_t)(cx - rr), (int16_t)(cy - rr))
            .size((int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1))
            .color(detail::color888To565(_render.bgColor))
            .draw();
        drawCircularProgressBar(x, y, r, thickness, value, baseColor, fillColor, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(cx - rr), (int16_t)(cy - rr), (int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1));
            if (doFlush)
                flushDirty();
        }
    }

    void GUI::updateCircularProgressBar(int16_t x, int16_t y,
                                        int16_t r,
                                        uint8_t thickness,
                                        uint8_t value,
                                        uint16_t color,
                                        ProgressAnim anim,
                                        bool doFlush)
    {
        uint16_t base = lighten565Progress(color, 10);
        updateCircularProgressBar(x, y, r, thickness, value, base, color, anim, doFlush);
    }
}

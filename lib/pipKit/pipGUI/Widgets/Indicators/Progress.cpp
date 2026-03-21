#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Draw/Internal.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        constexpr float kDegToRad = 0.01745329252f;
        constexpr float kRadToDeg = 57.2957795f;
        constexpr uint32_t kIndeterminatePeriodMs = 1600U;
        constexpr uint32_t kShimmerPeriodMs = 2000U;
        constexpr int16_t kSpritePad = 2;
        constexpr int kRingSubSamples = 2;
        constexpr int kRingSubSampleCount = kRingSubSamples * kRingSubSamples;

        [[nodiscard]] uint16_t blendWhite(uint16_t color, uint8_t intensity)
        {
            if (intensity == 0)
                return color;
            return static_cast<uint16_t>(pipgui::detail::blend565(color, static_cast<uint16_t>(0xFFFF), intensity));
        }

        [[nodiscard]] uint16_t lighten565Progress(uint16_t c, uint8_t amount)
        {
            uint16_t a = static_cast<uint16_t>(amount) * 8U;
            if (a > 255U)
                a = 255U;
            return static_cast<uint16_t>(detail::blend565(c, static_cast<uint16_t>(0xFFFF), static_cast<uint8_t>(a)));
        }

        [[nodiscard]] float normalizeDeg(float deg) noexcept
        {
            while (deg < 0.0f)
                deg += 360.0f;
            while (deg >= 360.0f)
                deg -= 360.0f;
            return deg;
        }

        [[nodiscard]] bool angleInSweep(float deg, float startDeg, float endDeg) noexcept
        {
            deg = normalizeDeg(deg);
            startDeg = normalizeDeg(startDeg);
            endDeg = normalizeDeg(endDeg);
            if (startDeg <= endDeg)
                return deg >= startDeg && deg <= endDeg;
            return deg >= startDeg || deg <= endDeg;
        }

        template <typename ColorFunc>
        void rasterRingAA(pipcore::Sprite *spr,
                          int16_t cx, int16_t cy,
                          int16_t r,
                          uint8_t thickness,
                          bool fullRing,
                          float startDeg,
                          float endDeg,
                          ColorFunc colorAtAngle)
        {
            if (!spr || r <= 0 || thickness < 1)
                return;

            Surface565 s;
            if (!getSurface565(spr, s))
                return;

            const int16_t capR = (thickness >= 3) ? static_cast<int16_t>(thickness / 2) : 1;
            const float outerR = static_cast<float>(r) + 0.5f;
            const float innerR = static_cast<float>(r - thickness) + 0.5f;
            const float outerR2 = outerR * outerR;
            const float innerR2 = innerR > 0.0f ? innerR * innerR : 0.0f;

            const int16_t minX = static_cast<int16_t>(cx - r - capR - 1);
            const int16_t maxX = static_cast<int16_t>(cx + r + capR + 1);
            const int16_t minY = static_cast<int16_t>(cy - r - capR - 1);
            const int16_t maxY = static_cast<int16_t>(cy + r + capR + 1);

            const float subStep = 1.0f / static_cast<float>(kRingSubSamples);
            const float subBias = 0.5f * subStep;
            const uint8_t *gamma = gammaTable();

            for (int16_t py = minY; py <= maxY; ++py)
            {
                if (py < s.clipY || py > s.clipB)
                    continue;
                for (int16_t px = minX; px <= maxX; ++px)
                {
                    if (px < s.clipX || px > s.clipR)
                        continue;

                    int covered = 0;
                    for (int sy = 0; sy < kRingSubSamples; ++sy)
                    {
                        const float dy = (static_cast<float>(py - cy) - 0.5f) + subBias + static_cast<float>(sy) * subStep;
                        for (int sx = 0; sx < kRingSubSamples; ++sx)
                        {
                            const float dx = (static_cast<float>(px - cx) - 0.5f) + subBias + static_cast<float>(sx) * subStep;
                            const float d2 = dx * dx + dy * dy;
                            if (d2 > outerR2 || d2 < innerR2)
                                continue;

                            if (!fullRing)
                            {
                                const float deg = normalizeDeg(atan2f(dy, dx) * kRadToDeg + 90.0f);
                                if (!angleInSweep(deg, startDeg, endDeg))
                                    continue;
                            }
                            ++covered;
                        }
                    }

                    if (covered == 0)
                        continue;

                    float colorDeg = 0.0f;
                    if (fullRing)
                    {
                        colorDeg = 0.0f;
                    }
                    else
                    {
                        colorDeg = normalizeDeg(atan2f(static_cast<float>(py - cy), static_cast<float>(px - cx)) * kRadToDeg + 90.0f);
                        if (!angleInSweep(colorDeg, startDeg, endDeg))
                            colorDeg = normalizeDeg((startDeg + endDeg) * 0.5f);
                    }

                    const Color565 c = makeColor565(colorAtAngle(colorDeg));
                    uint16_t *dst = s.buf + static_cast<int32_t>(py) * s.stride + px;
                    if (covered >= kRingSubSampleCount)
                        *dst = c.fg;
                    else
                    {
                        const uint8_t alpha = gamma[(covered * 255) / kRingSubSampleCount];
                        blendStore(dst, c, alpha);
                    }
                }
            }
        }

        template <typename ColorFunc>
        void drawRingCaps(GUI &g,
                          int16_t cx, int16_t cy,
                          int16_t r,
                          uint8_t thickness,
                          float startDeg,
                          float endDeg,
                          ColorFunc colorAtAngle)
        {
            if (r <= 0 || thickness < 1)
                return;

            const int16_t capR = (thickness >= 3) ? static_cast<int16_t>(thickness / 2) : 1;
            float rMid = static_cast<float>(r) - static_cast<float>(capR);
            if (rMid < 1.0f)
                rMid = 1.0f;

            auto drawCap = [&](float deg) {
                const float rad = (deg - 90.0f) * kDegToRad;
                const int16_t px = static_cast<int16_t>(lroundf(static_cast<float>(cx) + cosf(rad) * rMid));
                const int16_t py = static_cast<int16_t>(lroundf(static_cast<float>(cy) + sinf(rad) * rMid));
                g.fillCircle().pos(px, py).radius(capR).color(colorAtAngle(normalizeDeg(deg))).draw();
            };

            drawCap(startDeg);
            if (fabsf(endDeg - startDeg) > 0.01f)
                drawCap(endDeg);
        }
    }

    template <typename ColorFunc>
    static void drawRingArcStrokeArc(pipcore::Sprite *spr,
                                     int16_t cx, int16_t cy,
                                     int16_t r,
                                     uint8_t thickness,
                                     float startDeg,
                                     float endDeg,
                                     ColorFunc colorAtAngle)
    {
        rasterRingAA(spr, cx, cy, r, thickness, false, startDeg, endDeg, colorAtAngle);
    }

    static void drawRingStrokeFull(pipcore::Sprite *spr,
                                   int16_t cx, int16_t cy,
                                   int16_t r,
                                   uint8_t thickness,
                                   uint16_t color)
    {
        rasterRingAA(spr, cx, cy, r, thickness, true, 0.0f, 360.0f, [&](float) -> uint16_t { return color; });
    }

    [[nodiscard]] static uint8_t clampRoundRadius(int16_t w, int16_t h, uint8_t radius) noexcept
    {
        int16_t limit = h / 2;
        if (w / 2 < limit)
            limit = w / 2;
        if (limit <= 0)
            return 0;
        return radius > static_cast<uint8_t>(limit) ? static_cast<uint8_t>(limit) : radius;
    }

    template <typename ColorFunc>
    static void drawRingArcStrokeArcWrapped(pipcore::Sprite *spr,
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
            drawRingArcStrokeArc(spr, cx, cy, r, thickness, startDeg, endDeg, colorAtAngle);
        }
        else
        {
            drawRingArcStrokeArc(spr, cx, cy, r, thickness, startDeg, 360.0f, colorAtAngle);
            drawRingArcStrokeArc(spr, cx, cy, r, thickness, 0.0f, endDeg, colorAtAngle);
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

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            return;
        }

        if (!getDrawTarget())
            return;

        if (x == center)
            x = AutoX(w);
        if (y == center)
            y = AutoY(h);

        uint8_t r = clampRoundRadius(w, h, radius);

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

            uint32_t animPeriod = kIndeterminatePeriodMs;
            int32_t totalDist = (int32_t)innerW + segmentW;

            float p = (float)(now % animPeriod) / (float)animPeriod;
            const float eased = detail::motion::smoothstep(p);
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
                fillRoundRect(segLeft, innerY, segW, innerH, clampRoundRadius(segW, innerH, static_cast<uint8_t>(fillR)), fillColor);

            return;
        }

        if (value == 0)
            return;

        int16_t fillW = (int16_t)((innerW * (uint16_t)value) / 100U);
        if (fillW <= 0)
            return;
        if (fillW > innerW)
            fillW = innerW;

        fillRoundRect(innerX, innerY, fillW, innerH, clampRoundRadius(fillW, innerH, static_cast<uint8_t>(fillR)), fillColor);

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
            uint32_t animPeriod = kShimmerPeriodMs;
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
                               uint16_t textColor565,
                               uint16_t bgColor565,
                               TextAlign align,
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

            drawProgressText(x, y, w, h, text, textColor565, bgColor565, align, fontPx);

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
        case TextAlign::Left:
            tx = (int16_t)(x + 4);
            break;
        case TextAlign::Right:
            tx = (int16_t)(x + w - tw - 4);
            break;
        case TextAlign::Center:
        default:
            tx = (int16_t)(x + (w - tw) / 2);
            break;
        }

        drawTextAligned(text, tx, ty, textColor565, bgColor565, TextAlign::Left);

        setFontSize(prevSize);
        setFontWeight(prevWeight);
    }

    void GUI::drawProgressPercent(int16_t x, int16_t y,
                                  int16_t w, int16_t h,
                                  uint8_t value,
                                  uint16_t textColor565,
                                  uint16_t bgColor565,
                                  TextAlign align,
                                  uint16_t fontPx)
    {
        if (value > 100)
            value = 100;
        String s;
        s.reserve(5);
        s += String((int)value);
        s += "%";
        drawProgressText(x, y, w, h, s, textColor565, bgColor565, align, fontPx);
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

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr)
            return;

        drawRingStrokeFull(spr, cx, cy, r, thickness, baseColor);

        if (anim == Indeterminate)
        {
            uint32_t now = nowMs();
            uint32_t animPeriod = kIndeterminatePeriodMs;
            float p = (float)(now % animPeriod) / (float)animPeriod;
            const float eased = detail::motion::smoothstep(p);

            float segDeg = 70.0f;
            float pos = eased * 360.0f;
            float startDeg = pos - segDeg * 0.5f;
            float endDeg = pos + segDeg * 0.5f;

            drawRingArcStrokeArcWrapped(spr, cx, cy, r, thickness, startDeg, endDeg, [&](float) -> uint32_t
                                        { return fillColor; });
            drawRingCaps(*this, cx, cy, r, thickness, startDeg, endDeg, [&](float) -> uint32_t
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

        if (fillSpan >= 359.5f)
        {
            drawRingStrokeFull(spr, cx, cy, r, thickness, fillColor);
            return;
        }

        if (anim == ProgressAnimNone)
        {
            drawRingArcStrokeArc(spr, cx, cy, r, thickness, 0.0f, fillSpan, [&](float) -> uint32_t
                                 { return fillColor; });
            drawRingCaps(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float) -> uint32_t
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
            uint32_t animPeriod = kShimmerPeriodMs;
            float shift = (float)(now % animPeriod) * totalDist / (float)animPeriod;

            float shimmerLeft = -bandW + shift;
            float shimmerCenter = shimmerLeft + bandW / 2.0f;
            float halfBand = bandW / 2.0f;

            drawRingArcStrokeArc(spr, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint32_t
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
            drawRingCaps(*this, cx, cy, r, thickness, 0.0f, fillSpan, [&](float a) -> uint32_t
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
        const int16_t edgePad = clampRoundRadius(w, h, radius) + 1;
        const int16_t updatePad = edgePad > kSpritePad ? edgePad : kSpritePad;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - kSpritePad), (int16_t)(ry - kSpritePad))
            .size((int16_t)(w + kSpritePad * 2), (int16_t)(h + kSpritePad * 2))
            .color(_render.bgColor565)
            .draw();
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - kSpritePad), (int16_t)(ry - kSpritePad), (int16_t)(w + kSpritePad * 2), (int16_t)(h + kSpritePad * 2));
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
        const int16_t edgePad = clampRoundRadius(w, h, radius) + 1;
        const int16_t updatePad = edgePad > kSpritePad ? edgePad : kSpritePad;

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
                .pos((int16_t)(rx - updatePad), (int16_t)(ry - updatePad))
                .size((int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2))
                .color(_render.bgColor565)
                .draw();
            drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;

            if (!prevRender)
            {
                invalidateRect((int16_t)(rx - updatePad), (int16_t)(ry - updatePad), (int16_t)(w + updatePad * 2), (int16_t)(h + updatePad * 2));
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

        int16_t cx = (int16_t)(rx + l - updatePad);
        int16_t cw = (int16_t)((r - l) + updatePad * 2);
        if (cx < rx - updatePad)
            cx = rx - updatePad;
        if (cx + cw > rx + w + updatePad)
            cw = (int16_t)((rx + w + updatePad) - cx);

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _render.sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);
        _render.sprite.setClipRect(cx, (int16_t)(ry - updatePad), cw, (int16_t)(h + updatePad * 2));

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos(cx, (int16_t)(ry - updatePad))
            .size(cw, (int16_t)(h + updatePad * 2))
            .color(_render.bgColor565)
            .draw();
        drawProgressBar(x, y, w, h, value, baseColor, fillColor, radius, anim);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        _render.sprite.setClipRect(clipX, clipY, clipW, clipH);

        if (!prevRender)
        {
            invalidateRect(cx, (int16_t)(ry - updatePad), cw, (int16_t)(h + updatePad * 2));
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

        int16_t rr = (int16_t)(r + kSpritePad);
        fillRect()
            .pos((int16_t)(cx - rr), (int16_t)(cy - rr))
            .size((int16_t)(rr * 2 + 1), (int16_t)(rr * 2 + 1))
            .color(_render.bgColor565)
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

#include <pipGUI/Core/API/pipGUI.hpp>

namespace pipgui
{
    namespace
    {
        static inline uint16_t computeGlowStrength(uint8_t glowStrength, GlowAnim anim,
                                                   uint16_t pulsePeriodMs, uint32_t now)
        {
            if (anim != Pulse || pulsePeriodMs == 0)
                return glowStrength;

            const uint32_t PI_Q16 = 205887;
            const uint32_t TWO_PI_Q16 = 411774;
            uint32_t angle = (uint32_t)(((uint64_t)(now % pulsePeriodMs) * TWO_PI_Q16) / pulsePeriodMs);
            bool neg = (angle > PI_Q16);
            uint32_t x = neg ? (angle - PI_Q16) : angle;
            uint32_t xpm = (uint32_t)((uint64_t)x * (PI_Q16 - x) >> 16);
            uint32_t den = (uint32_t)((5ULL * PI_Q16 * PI_Q16) >> 16) - 4 * xpm;
            uint32_t abs_sin = den ? (uint32_t)(((uint64_t)(16 * xpm) << 16) / den) : 0;
            int32_t sin_val = neg ? -(int32_t)abs_sin : (int32_t)abs_sin;
            uint32_t p = 32768 + (sin_val >> 1);
            uint32_t base = 19661 + (uint32_t)((uint64_t)45875 * p >> 16);
            uint32_t res = (uint32_t)(((uint64_t)glowStrength << 8) * base >> 24);
            return (uint16_t)(res > 255 ? 255 : res);
        }

        static const uint32_t inv16[257] = {
            0,
            65536, 32768, 21845, 16384, 13107, 10923, 9362, 8192, 7282, 6554,
            5958, 5461, 5041, 4681, 4369, 4096, 3855, 3641, 3449, 3277,
            3121, 2979, 2849, 2731, 2621, 2521, 2427, 2341, 2260, 2185,
            2114, 2048, 1986, 1928, 1872, 1820, 1771, 1724, 1680, 1638,
            1598, 1560, 1524, 1489, 1456, 1425, 1394, 1365, 1337, 1311,
            1285, 1260, 1237, 1214, 1192, 1170, 1150, 1130, 1111, 1092,
            1074, 1057, 1040, 1024, 1008, 993, 978, 964, 950, 936,
            923, 910, 897, 885, 873, 862, 851, 840, 829, 819,
            809, 799, 789, 780, 771, 762, 753, 745, 736, 728,
            720, 712, 705, 697, 690, 683, 676, 669, 662, 655,
            649, 643, 636, 630, 624, 618, 612, 607, 601, 596,
            590, 585, 580, 575, 570, 565, 560, 555, 551, 546,
            542, 537, 533, 529, 524, 520, 516, 512, 508, 504,
            500, 496, 493, 489, 485, 482, 478, 475, 471, 468,
            465, 462, 458, 455, 452, 449, 446, 443, 440, 437,
            434, 431, 428, 426, 423, 420, 418, 415, 412, 410,
            407, 405, 402, 400, 397, 395, 392, 390, 388, 385,
            383, 381, 379, 377, 374, 372, 370, 368, 366, 364,
            362, 360, 358, 356, 354, 352, 350, 349, 347, 345,
            343, 341, 340, 338, 336, 335, 333, 331, 330, 328,
            327, 325, 323, 322, 320, 319, 317, 316, 314, 313,
            312, 310, 309, 307, 306, 305, 303, 302, 301, 299,
            298, 297, 295, 294, 293, 292, 290, 289, 288, 287,
            286, 284, 283, 282, 281, 280, 279, 278, 277, 275,
            274, 273, 272, 271, 270, 269, 268, 267, 266, 265,
            264, 263, 262, 261, 260};
    }

    template <typename Fn>
    void GUI::renderToSpriteAndFlush(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall)
    {
        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawCall();
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
        if (!prevRender)
        {
            invalidateRect(x, y, w, h);
            flushDirty();
        }
    }

    void GUI::drawGlowCircle(int16_t x, int16_t y, int16_t r,
                             uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength,
                             GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (r <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        const int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        const uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        const uint16_t glow = glowColor >= 0 ? (uint16_t)glowColor : detail::blend565WithWhite(fillColor, 80);
        const uint16_t strength = computeGlowStrength(glowStrength, anim, pulsePeriodMs, nowMs());

        if (glowSize == 0 || strength < 2)
        {
            fillCircle(x, y, r, fillColor);
            return;
        }

        const uint32_t inv = 65535U / glowSize;
        for (int off = glowSize; off > 0; --off)
        {
            uint32_t n = (uint32_t)(glowSize - off + 1) * inv;
            uint32_t curve = (uint32_t)((uint64_t)(n * n >> 16) * n >> 16);
            uint8_t alpha = (uint8_t)min(255U, (uint32_t)strength * curve >> 16);
            if (alpha < 2)
                continue;
            fillCircle(x, y, (int16_t)(r + off), detail::blend565(bg, glow, alpha));
        }
        fillCircle(x, y, r, fillColor);
    }

    void GUI::updateGlowCircle(int16_t x, int16_t y, int16_t r,
                               uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                               uint8_t glowSize, uint8_t glowStrength,
                               GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        const int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        const uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        constexpr int16_t pad = 2;
        renderToSpriteAndFlush(
            (int16_t)(x - outerR - pad), (int16_t)(y - outerR - pad),
            (int16_t)(diam + pad * 2), (int16_t)(diam + pad * 2),
            [&]
            { drawGlowCircle(x, y, r, fillColor, (int16_t)bg, glowColor, glowSize, glowStrength, anim, pulsePeriodMs); });
    }

    void GUI::drawGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radius, uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                           uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        drawGlowRect(x, y, w, h, radius, radius, radius, radius,
                     fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    }

    void GUI::updateGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        updateGlowRect(x, y, w, h, radius, radius, radius, radius,
                       fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    }

    void GUI::drawGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                           uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                           uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGlowRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL,
                           fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        if (x == center)
            x = (int16_t)(AutoX(w + 2 * glowSize) + glowSize);
        if (y == center)
            y = (int16_t)(AutoY(h + 2 * glowSize) + glowSize);

        const uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        const uint16_t glow = glowColor >= 0 ? (uint16_t)glowColor : detail::blend565WithWhite(fillColor, 80);
        const uint16_t strength = computeGlowStrength(glowStrength, anim, pulsePeriodMs, nowMs());

        if (glowSize == 0 || strength < 2)
        {
            fillRoundRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL, fillColor);
            return;
        }

        const uint32_t inv = 65535U / glowSize;
        for (int off = glowSize; off > 0; --off)
        {
            uint32_t n = (uint32_t)(glowSize - off + 1) * inv;
            uint32_t curve = (uint32_t)((uint64_t)(n * n >> 16) * n >> 16);
            uint8_t alpha = (uint8_t)min(255U, (uint32_t)strength * curve >> 16);
            if (alpha < 2)
                continue;

            const int16_t xx = x - off, yy = y - off;
            const int16_t ww = w + off * 2, hh = h + off * 2;
            const int16_t maxR = (ww < hh ? ww : hh) >> 1;
            auto clamp = [&](int16_t rv) -> uint8_t
            {
                int16_t v = rv + off;
                return (uint8_t)(v > maxR ? maxR : (v < 0 ? 0 : v));
            };
            fillRoundRect(xx, yy, ww, hh,
                          clamp(radiusTL), clamp(radiusTR),
                          clamp(radiusBR), clamp(radiusBL),
                          detail::blend565(bg, glow, alpha));
        }
        fillRoundRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL, fillColor);
    }

    void GUI::updateGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                             uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGlowRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL,
                         fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        if (x == center)
            x = (int16_t)(AutoX(w + 2 * glowSize) + glowSize);
        if (y == center)
            y = (int16_t)(AutoY(h + 2 * glowSize) + glowSize);

        const uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        constexpr int16_t pad = 2;
        renderToSpriteAndFlush(
            (int16_t)(x - glowSize - pad), (int16_t)(y - glowSize - pad),
            (int16_t)(w + glowSize * 2 + pad * 2), (int16_t)(h + glowSize * 2 + pad * 2),
            [&]
            { drawGlowRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL,
                           fillColor, (int16_t)bg, glowColor, glowSize, glowStrength, anim, pulsePeriodMs); });
    }

    bool GUI::ensureBlurWorkBuffers(uint32_t smallLen, int16_t sw, int16_t sh) noexcept
    {
        if (smallLen == 0 || sw <= 0 || sh <= 0)
            return false;

        pipcore::GuiPlatform *plat = platform();

        auto alloc565Pair = [&](uint16_t *&a, uint16_t *&b, size_t n) -> bool
        {
            void *na = detail::guiAlloc(plat, n * sizeof(uint16_t), pipcore::GuiAllocCaps::PreferExternal);
            void *nb = detail::guiAlloc(plat, n * sizeof(uint16_t), pipcore::GuiAllocCaps::PreferExternal);
            if (!na || !nb)
            {
                detail::guiFree(plat, na);
                detail::guiFree(plat, nb);
                return false;
            }
            detail::guiFree(plat, a);
            detail::guiFree(plat, b);
            a = (uint16_t *)na;
            b = (uint16_t *)nb;
            return true;
        };

        auto alloc32Triple = [&](uint32_t *&a, uint32_t *&b, uint32_t *&c, size_t n) -> bool
        {
            void *na = detail::guiAlloc(plat, n * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
            void *nb = detail::guiAlloc(plat, n * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
            void *nc = detail::guiAlloc(plat, n * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
            if (!na || !nb || !nc)
            {
                detail::guiFree(plat, na);
                detail::guiFree(plat, nb);
                detail::guiFree(plat, nc);
                return false;
            }
            detail::guiFree(plat, a);
            detail::guiFree(plat, b);
            detail::guiFree(plat, c);
            a = (uint32_t *)na;
            b = (uint32_t *)nb;
            c = (uint32_t *)nc;
            return true;
        };

        if ((!_blur.smallIn || !_blur.smallTmp || _blur.workLen < smallLen) &&
            !alloc565Pair(_blur.smallIn, _blur.smallTmp, smallLen))
            return false;
        _blur.workLen = smallLen;

        if ((_blur.rowCap < (uint16_t)(sw + 1) || !_blur.rowR || !_blur.rowG || !_blur.rowB) &&
            !alloc32Triple(_blur.rowR, _blur.rowG, _blur.rowB, (size_t)(sw + 1)))
            return false;
        _blur.rowCap = (uint16_t)(sw + 1);

        if ((_blur.colCap < (uint16_t)(sh + 1) || !_blur.colR || !_blur.colG || !_blur.colB) &&
            !alloc32Triple(_blur.colR, _blur.colG, _blur.colB, (size_t)(sh + 1)))
            return false;
        _blur.colCap = (uint16_t)(sh + 1);

        return true;
    }

    void GUI::drawBlurRegion(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, BlurDirection dir, bool gradient,
                             uint8_t materialStrength, uint8_t noiseAmount, int32_t materialColor)
    {
        (void)noiseAmount;
        if (w <= 0 || h <= 0)
            return;
        if (radius < 1)
            radius = 1;

        if (x < 0)
        {
            w += x;
            x = 0;
        }
        if (y < 0)
        {
            h += y;
            y = 0;
        }
        if (x + w > _render.screenWidth)
            w = _render.screenWidth - x;
        if (y + h > _render.screenHeight)
            h = _render.screenHeight - y;
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
            return;
        }

        pipcore::Sprite *spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        uint16_t *screenBuf = (uint16_t *)spr->getBuffer();
        const int16_t stride = spr->width();
        if (!screenBuf || stride <= 0)
            return;

        auto swap16 = [](uint16_t v)
        { return (uint16_t)((v >> 8) | (v << 8)); };
        auto read565 = [&](int32_t i)
        { return swap16(screenBuf[i]); };
        auto write565 = [&](int32_t i, uint16_t c)
        { screenBuf[i] = swap16(c); };

        const int16_t sw = (int16_t)((w + 1) >> 1);
        const int16_t sh = (int16_t)((h + 1) >> 1);
        const uint32_t smallLen = (uint32_t)sw * (uint32_t)sh;
        if (!smallLen || !ensureBlurWorkBuffers(smallLen, sw, sh))
            return;

        uint16_t *smallIn = _blur.smallIn, *smallTmp = _blur.smallTmp;
        uint32_t *rowR = _blur.rowR, *rowG = _blur.rowG, *rowB = _blur.rowB;
        uint32_t *colR = _blur.colR, *colG = _blur.colG, *colB = _blur.colB;

        for (int16_t sy = 0; sy < sh; ++sy)
        {
            const int16_t y0 = y + (sy << 1);
            const int16_t y1 = (y0 + 1 < y + h) ? y0 + 1 : y0;
            const int32_t row0 = (int32_t)y0 * stride;
            const int32_t row1 = (int32_t)y1 * stride;
            for (int16_t sx = 0; sx < sw; ++sx)
            {
                const int16_t x0 = x + (sx << 1);
                const int16_t x1 = (x0 + 1 < x + w) ? x0 + 1 : x0;
                const uint16_t c00 = read565(row0 + x0);
                const uint16_t c10 = read565(row0 + x1);
                const uint16_t c01 = read565(row1 + x0);
                const uint16_t c11 = read565(row1 + x1);
                smallIn[(uint32_t)sy * sw + sx] = (uint16_t)(((((c00 >> 11) + (c10 >> 11) + (c01 >> 11) + (c11 >> 11)) >> 2) << 11) |
                                                             (((((c00 >> 5) & 0x3F) + ((c10 >> 5) & 0x3F) + ((c01 >> 5) & 0x3F) + ((c11 >> 5) & 0x3F)) >> 2) << 5) |
                                                             (((c00 & 0x1F) + (c10 & 0x1F) + (c01 & 0x1F) + (c11 & 0x1F)) >> 2));
            }
        }

        const uint8_t radiusSmall = (uint8_t)((radius + 1) >> 1);

        if (radiusSmall > 0)
        {
            auto blurH = [&]()
            {
                for (int16_t iy = 0; iy < sh; ++iy)
                {
                    const uint32_t off = (uint32_t)iy * sw;
                    rowR[0] = rowG[0] = rowB[0] = 0;
                    for (int16_t ix = 0; ix < sw; ++ix)
                    {
                        uint16_t c = smallIn[off + ix];
                        rowR[ix + 1] = rowR[ix] + ((c >> 11) & 0x1F);
                        rowG[ix + 1] = rowG[ix] + ((c >> 5) & 0x3F);
                        rowB[ix + 1] = rowB[ix] + (c & 0x1F);
                    }
                    for (int16_t ix = 0; ix < sw; ++ix)
                    {
                        const int xa_ = (ix - (int)radiusSmall < 0) ? 0 : ix - (int)radiusSmall;
                        const int xb_ = (ix + (int)radiusSmall >= sw) ? sw - 1 : ix + (int)radiusSmall;
                        const uint32_t inv = inv16[(uint32_t)(xb_ - xa_ + 1)];
                        smallTmp[off + ix] = (uint16_t)(((((rowR[xb_ + 1] - rowR[xa_]) * inv) >> 16) << 11) |
                                                        ((((rowG[xb_ + 1] - rowG[xa_]) * inv) >> 16) << 5) |
                                                        (((rowB[xb_ + 1] - rowB[xa_]) * inv) >> 16));
                    }
                }
            };

            auto blurV = [&]()
            {
                for (int16_t ix = 0; ix < sw; ++ix)
                {
                    colR[0] = colG[0] = colB[0] = 0;
                    for (int16_t iy = 0; iy < sh; ++iy)
                    {
                        uint16_t c = smallTmp[(uint32_t)iy * sw + ix];
                        colR[iy + 1] = colR[iy] + ((c >> 11) & 0x1F);
                        colG[iy + 1] = colG[iy] + ((c >> 5) & 0x3F);
                        colB[iy + 1] = colB[iy] + (c & 0x1F);
                    }
                    for (int16_t iy = 0; iy < sh; ++iy)
                    {
                        const int ya_ = (iy - (int)radiusSmall < 0) ? 0 : iy - (int)radiusSmall;
                        const int yb_ = (iy + (int)radiusSmall >= sh) ? sh - 1 : iy + (int)radiusSmall;
                        const uint32_t inv = inv16[(uint32_t)(yb_ - ya_ + 1)];
                        smallIn[(uint32_t)iy * sw + ix] = (uint16_t)(((((colR[yb_ + 1] - colR[ya_]) * inv) >> 16) << 11) |
                                                                     ((((colG[yb_ + 1] - colG[ya_]) * inv) >> 16) << 5) |
                                                                     (((colB[yb_ + 1] - colB[ya_]) * inv) >> 16));
                    }
                }
            };

            blurH();
            blurV();
            blurH();
            blurV();
        }

        const bool useMaterial = (materialStrength > 0);
        const uint16_t mat565 = useMaterial
                                    ? (materialColor >= 0 ? detail::color888To565((uint32_t)materialColor)
                                                          : detail::color888To565(_render.bgColor))
                                    : 0;

        const bool gradH = gradient && (dir == LeftRight || dir == RightLeft);
        const bool gradV = gradient && (dir == TopDown || dir == BottomUp);

        for (int16_t iy = 0; iy < h; ++iy)
        {
            uint8_t alphaRow = 255;
            if (gradV)
            {
                if (dir == TopDown)
                    alphaRow = h <= 1 ? 255 : (uint8_t)(255 - iy * 255 / (h - 1));
                else if (dir == BottomUp)
                    alphaRow = h <= 1 ? 255 : (uint8_t)(iy * 255 / (h - 1));
            }

            const int32_t screenOff = (int32_t)(y + iy) * stride + x;
            const uint32_t syOff = (uint32_t)min(iy >> 1, (int)sh - 1) * (uint32_t)sw;

            for (int16_t ix = 0; ix < w; ++ix)
            {
                uint8_t alpha = alphaRow;
                if (gradH)
                {
                    if (dir == LeftRight)
                        alpha = w <= 1 ? 255 : (uint8_t)(255 - ix * 255 / (w - 1));
                    else if (dir == RightLeft)
                        alpha = w <= 1 ? 255 : (uint8_t)(ix * 255 / (w - 1));
                }

                const uint32_t sx = (uint32_t)min(ix >> 1, (int)sw - 1);
                uint16_t mixed = detail::blend565(read565(screenOff + ix), smallIn[syOff + sx], alpha);

                if (useMaterial)
                    mixed = detail::blend565(mixed, mat565,
                                             gradH || gradV ? (uint8_t)((uint16_t)materialStrength * alpha / 255U)
                                                            : materialStrength);

                write565(screenOff + ix, mixed);
            }
        }
    }

    void GUI::updateBlurRegion(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t radius, BlurDirection dir, bool gradient,
                               uint8_t materialStrength, uint8_t noiseAmount, int32_t materialColor)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
            return;
        }
        renderToSpriteAndFlush(x, y, w, h,
                               [&]
                               { drawBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor); });
    }
}

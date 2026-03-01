#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{
    namespace
    {
        static inline uint16_t computeGlowStrength(uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs, uint32_t now)
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

        static inline int16_t clampRadiusToRect(int16_t w, int16_t h, int16_t r)
        {
            int16_t maxR = (w < h ? w : h) / 2;
            return r > maxR ? maxR : (r < 0 ? 0 : r);
        }
    }

    void GUI::beginDirectRender()
    {
        _savedRenderState.renderToSprite = _flags.renderToSprite;
        _savedRenderState.activeSprite = _render.activeSprite;
        _flags.renderToSprite = 0;
    }

    void GUI::endDirectRender()
    {
        _flags.renderToSprite = _savedRenderState.renderToSprite;
        _render.activeSprite = _savedRenderState.activeSprite;
    }

    template <typename Fn>
    void GUI::renderToSpriteAndFlush(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall)
    {
        _savedRenderState.renderToSprite = _flags.renderToSprite;
        _savedRenderState.activeSprite = _render.activeSprite;
        _flags.renderToSprite = 1;
        _render.activeSprite = &_render.sprite;
        drawCall();
        _flags.renderToSprite = _savedRenderState.renderToSprite;
        _render.activeSprite = _savedRenderState.activeSprite;
        invalidateRect(x, y, w, h);
        flushDirty();
    }

    void GUI::drawGlowCircle(int16_t x, int16_t y, int16_t r,
                             uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength,
                             GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (r <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.renderToSprite)
        {
            updateGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        uint16_t glow = glowColor >= 0 ? (uint16_t)glowColor : detail::blend565WithWhite(fillColor, 80);
        uint16_t strength = computeGlowStrength(glowStrength, anim, pulsePeriodMs, nowMs());

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
            beginDirectRender();
            drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            endDirectRender();
            return;
        }

        int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        int16_t pad = 2;

        renderToSpriteAndFlush(
            (int16_t)(x - outerR - pad), (int16_t)(y - outerR - pad),
            (int16_t)(diam + pad * 2), (int16_t)(diam + pad * 2),
            [&]()
            { drawGlowCircle(x, y, r, fillColor, (int16_t)bg, glowColor, glowSize, glowStrength, anim, pulsePeriodMs); });
    }

    void GUI::drawGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radius, uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                           uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        drawGlowRect(x, y, w, h, radius, radius, radius, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    }

    void GUI::updateGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        updateGlowRect(x, y, w, h, radius, radius, radius, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    }

    void GUI::drawGlowRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                           uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                           uint8_t glowSize, uint8_t glowStrength, GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.renderToSprite)
        {
            updateGlowRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL,
                           fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        if (x == center)
            x = (int16_t)(AutoX(w + 2 * glowSize) + glowSize);
        if (y == center)
            y = (int16_t)(AutoY(h + 2 * glowSize) + glowSize);

        uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        uint16_t glow = glowColor >= 0 ? (uint16_t)glowColor : detail::blend565WithWhite(fillColor, 80);
        uint16_t strength = computeGlowStrength(glowStrength, anim, pulsePeriodMs, nowMs());

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

            int16_t xx = x - off, yy = y - off, ww = w + off * 2, hh = h + off * 2;
            fillRoundRect(xx, yy, ww, hh,
                          (uint8_t)clampRadiusToRect(ww, hh, (int16_t)radiusTL + off),
                          (uint8_t)clampRadiusToRect(ww, hh, (int16_t)radiusTR + off),
                          (uint8_t)clampRadiusToRect(ww, hh, (int16_t)radiusBR + off),
                          (uint8_t)clampRadiusToRect(ww, hh, (int16_t)radiusBL + off),
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
            beginDirectRender();
            drawGlowRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL,
                         fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            endDirectRender();
            return;
        }

        if (x == center)
            x = (int16_t)(AutoX(w + 2 * glowSize) + glowSize);
        if (y == center)
            y = (int16_t)(AutoY(h + 2 * glowSize) + glowSize);

        uint16_t bg = bgColor >= 0 ? (uint16_t)bgColor : _render.bgColor;
        int16_t pad = 2;

        renderToSpriteAndFlush(
            (int16_t)(x - glowSize - pad), (int16_t)(y - glowSize - pad),
            (int16_t)(w + glowSize * 2 + pad * 2), (int16_t)(h + glowSize * 2 + pad * 2),
            [&]()
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

        if (_flags.spriteEnabled && _disp.display && !_flags.renderToSprite)
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
            int16_t y0 = y + (sy << 1), y1 = y0 + 1;
            bool hasY1 = (y1 < y + h);
            for (int16_t sx = 0; sx < sw; ++sx)
            {
                int16_t x0 = x + (sx << 1);
                bool hasX1 = (x0 + 1 < x + w);
                uint16_t c00 = read565((int32_t)y0 * stride + x0);
                uint16_t c10 = hasX1 ? read565((int32_t)y0 * stride + x0 + 1) : c00;
                uint16_t c01 = hasY1 ? read565((int32_t)y1 * stride + x0) : c00;
                uint16_t c11 = (hasX1 & hasY1) ? read565((int32_t)y1 * stride + x0 + 1) : c00;
                uint32_t cnt = 1u + hasX1 + hasY1 + (hasX1 & hasY1);
                uint32_t sR = ((c00 >> 11) & 0x1F) + ((c10 >> 11) & 0x1F) + ((c01 >> 11) & 0x1F) + ((c11 >> 11) & 0x1F);
                uint32_t sG = ((c00 >> 5) & 0x3F) + ((c10 >> 5) & 0x3F) + ((c01 >> 5) & 0x3F) + ((c11 >> 5) & 0x3F);
                uint32_t sB = (c00 & 0x1F) + (c10 & 0x1F) + (c01 & 0x1F) + (c11 & 0x1F);
                smallIn[(uint32_t)sy * sw + sx] = (uint16_t)(((sR / cnt) << 11) | ((sG / cnt) << 5) | (sB / cnt));
            }
        }

        uint8_t radiusSmall = (uint8_t)max(1, (int)(radius + 1) >> 1);

        auto blurH = [&]()
        {
            for (int16_t iy = 0; iy < sh; ++iy)
            {
                uint32_t off = (uint32_t)iy * sw;
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
                    int xa = max(0, ix - (int)radiusSmall), xb = min(sw - 1, ix + (int)radiusSmall);
                    uint32_t n = (uint32_t)(xb - xa + 1);
                    smallTmp[off + ix] = (uint16_t)((((rowR[xb + 1] - rowR[xa]) / n) << 11) |
                                                    (((rowG[xb + 1] - rowG[xa]) / n) << 5) |
                                                    ((rowB[xb + 1] - rowB[xa]) / n));
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
                    int ya = max(0, iy - (int)radiusSmall), yb = min(sh - 1, iy + (int)radiusSmall);
                    uint32_t n = (uint32_t)(yb - ya + 1);
                    smallIn[(uint32_t)iy * sw + ix] = (uint16_t)((((colR[yb + 1] - colR[ya]) / n) << 11) |
                                                                 (((colG[yb + 1] - colG[ya]) / n) << 5) |
                                                                 ((colB[yb + 1] - colB[ya]) / n));
                }
            }
        };

        blurH();
        blurV();
        blurH();
        blurV();

        const bool useMaterial = (materialStrength > 0);
        const uint16_t mat565 = useMaterial
                                    ? (materialColor >= 0 ? detail::color888To565((uint32_t)materialColor)
                                                          : detail::color888To565(_render.bgColor))
                                    : 0;

        for (int16_t iy = 0; iy < h; ++iy)
        {
            uint8_t alphaBase = 255;
            if (gradient)
            {
                if (dir == TopDown)
                    alphaBase = h <= 1 ? 255 : (uint8_t)(255 - iy * 255 / (h - 1));
                else if (dir == BottomUp)
                    alphaBase = h <= 1 ? 255 : (uint8_t)(iy * 255 / (h - 1));
            }

            int32_t screenOff = (int32_t)(y + iy) * stride + x;
            uint32_t syOff = (uint32_t)min(iy >> 1, (int)sh - 1) * (uint32_t)sw;

            for (int16_t ix = 0; ix < w; ++ix)
            {
                uint8_t alpha = alphaBase;
                if (gradient)
                {
                    if (dir == LeftRight)
                        alpha = w <= 1 ? 255 : (uint8_t)(255 - ix * 255 / (w - 1));
                    else if (dir == RightLeft)
                        alpha = w <= 1 ? 255 : (uint8_t)(ix * 255 / (w - 1));
                }

                uint32_t sx = (uint32_t)min(ix >> 1, (int)sw - 1);
                uint16_t mixed = detail::blend565(read565(screenOff + ix), smallIn[syOff + sx], alpha);

                if (useMaterial)
                    mixed = detail::blend565(mixed, mat565,
                                             gradient ? (uint8_t)((uint16_t)materialStrength * alpha / 255U) : materialStrength);

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
            beginDirectRender();
            drawBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
            endDirectRender();
            return;
        }
        renderToSpriteAndFlush(x, y, w, h, [&]()
                               { drawBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor); });
    }
}
#include "Internal.hpp"

namespace pipgui
{
    namespace
    {
        struct AlphaLut
        {
            uint8_t values[256];
            uint8_t firstNonZero;
        };

        struct NativeColor565
        {
            uint16_t native;
            uint32_t fgRb;
            uint32_t fgG;
        };

        static inline NativeColor565 makeNativeColor565(uint16_t color565)
        {
            return {
                pipcore::Sprite::swap16(color565),
                ((color565 & 0xF800u) << 5) | (color565 & 0x001Fu),
                color565 & 0x07E0u};
        }

        static inline void blendNative565(uint16_t *dst, const NativeColor565 &color, uint8_t alpha)
        {
            if (alpha == 0)
                return;
            if (alpha == 255)
            {
                *dst = color.native;
                return;
            }

            const uint16_t bg = pipcore::Sprite::swap16(*dst);
            const uint32_t inv = 255u - alpha;
            const uint32_t bgRb = ((uint32_t)(bg & 0xF800u) << 5) | (bg & 0x001Fu);
            const uint32_t bgG = bg & 0x07E0u;
            const uint32_t rb = ((color.fgRb * alpha + bgRb * inv) >> 8) & 0x001F001Fu;
            const uint32_t g = ((color.fgG * alpha + bgG * inv) >> 8) & 0x000007E0u;
            *dst = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
        }

        static inline const AlphaLut &alphaLutFor(float kScale, float kOffset,
                                                  float coverageGamma, float edgeContrast)
        {
            static AlphaLut lut{};
            static float cachedScale = 0.0f;
            static float cachedOffset = 0.0f;
            static float cachedGamma = 0.0f;
            static float cachedContrast = 0.0f;
            static bool ready = false;

            if (ready && cachedScale == kScale && cachedOffset == kOffset &&
                cachedGamma == coverageGamma && cachedContrast == edgeContrast)
                return lut;

            for (uint32_t sample = 0; sample < 256; ++sample)
            {
                float alpha = (float)sample * kScale + kOffset;
                if (alpha <= 0.0f)
                {
                    lut.values[sample] = 0;
                    continue;
                }
                if (alpha >= 1.0f)
                {
                    lut.values[sample] = 255;
                    continue;
                }
                if (edgeContrast != 1.0f)
                {
                    alpha = 0.5f + (alpha - 0.5f) * edgeContrast;
                    if (alpha <= 0.0f)
                    {
                        lut.values[sample] = 0;
                        continue;
                    }
                    if (alpha >= 1.0f)
                    {
                        lut.values[sample] = 255;
                        continue;
                    }
                }
                if (coverageGamma != 1.0f)
                    alpha = powf(alpha, coverageGamma);
                lut.values[sample] = (uint8_t)(alpha * 255.0f + 0.5f);
            }
            lut.firstNonZero = 0;
            while (lut.firstNonZero < 255 && lut.values[lut.firstNonZero] == 0)
                ++lut.firstNonZero;
            cachedScale = kScale;
            cachedOffset = kOffset;
            cachedGamma = coverageGamma;
            cachedContrast = edgeContrast;
            ready = true;
            return lut;
        }

        struct GlyphRowSampler
        {
            pipcore::Platform *plat;
            const uint8_t *row0;
            const uint8_t *row1;
            int glyphL;
            int glyphR;
            uint32_t fy;

            inline uint8_t sample(int32_t u16) const
            {
                const int x0Raw = u16 >> 16;
                const int x1Raw = x0Raw + 1;
                const uint32_t fx = (uint32_t)u16 & 0xFFFFu;

                int x0 = x0Raw;
                int x1 = x1Raw;
                if (x0 < glyphL)
                    x0 = glyphL;
                else if (x0 > glyphR)
                    x0 = glyphR;
                if (x1 < glyphL)
                    x1 = glyphL;
                else if (x1 > glyphR)
                    x1 = glyphR;
                const int32_t a00 = (int32_t)plat->readProgmemByte(row0 + x0);
                const int32_t a10 = (int32_t)plat->readProgmemByte(row0 + x1);
                const int32_t a01 = (int32_t)plat->readProgmemByte(row1 + x0);
                const int32_t a11 = (int32_t)plat->readProgmemByte(row1 + x1);

                const int32_t a0 = (a00 << 16) + (a10 - a00) * (int32_t)fx;
                const int32_t a1 = (a01 << 16) + (a11 - a01) * (int32_t)fx;
                int32_t alpha = ((a0 + (int32_t)(((int64_t)(a1 - a0) * fy) >> 16)) + 0x8000) >> 16;
                if (alpha < 0)
                    alpha = 0;
                else if (alpha > 255)
                    alpha = 255;
                return (uint8_t)alpha;
            }
        };

        struct GlyphSampler
        {
            pipcore::Platform *plat;
            const uint8_t *atlasData;
            int32_t atlasStride;
            int glyphL;
            int glyphR;
            int glyphB;
            int glyphT;

            inline GlyphRowSampler row(int32_t v16) const
            {
                const int y0Raw = v16 >> 16;
                const int y1Raw = y0Raw + 1;
                int y0 = y0Raw;
                int y1 = y1Raw;
                if (y0 < glyphB)
                    y0 = glyphB;
                else if (y0 > glyphT)
                    y0 = glyphT;
                if (y1 < glyphB)
                    y1 = glyphB;
                else if (y1 > glyphT)
                    y1 = glyphT;
                return {
                    plat,
                    atlasData + (uint32_t)y0 * atlasStride,
                    atlasData + (uint32_t)y1 * atlasStride,
                    glyphL,
                    glyphR,
                    (uint32_t)v16 & 0xFFFFu};
            }
        };
    }

    void GUI::drawTextImmediate(const String &text, int16_t rx, int16_t ry,
                                int16_t tw, int16_t th,
                                uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        drawTextImmediateMasked(text, rx, ry, tw, th, fg565, bg565, align, 0, 0, 0);
    }

    void GUI::drawTextImmediateMasked(const String &text, int16_t rx, int16_t ry,
                                      int16_t, int16_t,
                                      uint16_t fg565, uint16_t, TextAlign,
                                      int16_t fadeBoxX, int16_t fadeBoxW, uint8_t fadePx)
    {
        const FontData *font = fontDataForId(_typo.currentFontId);
        if (!_typo.psdfSizePx || !font)
            return;

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width(), maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;
        const int32_t clipR = clipX + clipW;
        const int32_t clipB = clipY + clipH;
        const int32_t fadeBoxR = (int32_t)fadeBoxX + fadeBoxW;
        const int32_t fadePxClamped = (fadePx > 0 && fadeBoxW > 2)
                                          ? std::min<int32_t>((int32_t)fadePx, std::max<int32_t>(1, fadeBoxW / 3))
                                          : 0;
        const bool useFade = (fadePxClamped > 0 && fadeBoxW > 0);

        const float sizePx = (float)_typo.psdfSizePx;
        const float distanceScale = font->distanceRange * (sizePx / font->nominalSizePx);
        const float weightBias = weightBiasFor(_typo.psdfWeight, sizePx, font);
        const float readabilityBias = (distanceScale > 0.0001f) ? (readabilityDarkenPxFor(sizePx) / distanceScale) : 0.0f;
        const float coverageGamma = weightCoverageGammaFor(_typo.psdfWeight) * readabilityGammaFor(sizePx);
        const float edgeContrast = readabilityContrastFor(sizePx);
        const float biasOffset = 0.5f + weightBias;
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = (biasOffset + readabilityBias) - distanceScale * 0.5f;
        const AlphaLut &alphaLut = alphaLutFor(kScale, kOffset, coverageGamma, edgeContrast);
        const uint8_t s8Min = alphaLut.firstNonZero;

        const NativeColor565 fg = makeNativeColor565(fg565);

        pipcore::Platform *const plat = platform();
        const float padScale = sizePx * (1.0f / 128.0f);

        forEachGlyph(text.c_str(), (int)text.length(), font, sizePx,
                     [&](const Glyph *g, float penX, float penY, bool nl) -> bool
                     {
                         if (nl || !g)
                             return true;

                         const float absPenX = rx + penX;
                         const float absPenY = ry + penY;
                         const float padLeftPx = (float)g->padLeft * padScale;
                         const float padRightPx = (float)g->padRight * padScale;
                         const float padTopPx = (float)g->padTop * padScale;
                         const float padBottomPx = (float)g->padBottom * padScale;
                         const float gx0 = absPenX + padLeftPx;
                         const float gy0 = absPenY - padTopPx;
                         const float gx1 = absPenX + padRightPx;
                         const float gy1 = absPenY - padBottomPx;

                         int ix0 = floorToInt(gx0);
                         int ix1 = ceilToInt(gx1);
                         int iy0 = floorToInt(gy0);
                         int iy1 = ceilToInt(gy1);

                         if (ix1 <= clipX || iy1 <= clipY || ix0 >= clipR || iy0 >= clipB)
                             return true;
                         if (ix0 < clipX)
                             ix0 = clipX;
                         if (iy0 < clipY)
                             iy0 = clipY;
                         if (ix1 > clipR)
                             ix1 = clipR;
                         if (iy1 > clipB)
                             iy1 = clipB;
                         if (ix0 >= ix1 || iy0 >= iy1)
                             return true;

                         const float gw = gx1 - gx0;
                         const float gh = gy1 - gy0;
                         const float invW = (gw != 0.f) ? 1.f / gw : 0.f;
                         const float invH = (gh != 0.f) ? 1.f / gh : 0.f;

                         const int glyphL = (int)g->atlasLeft;
                         const int glyphB = (int)g->atlasBottom;
                         const int glyphR = std::max(glyphL, (int)g->atlasRight - 1);
                         const int glyphT = std::max(glyphB, (int)g->atlasTop - 1);
                         const float atlasW = (float)(g->atlasRight - g->atlasLeft);
                         const float atlasH = (float)(g->atlasTop - g->atlasBottom);

                         const int32_t atlasDu = (int32_t)(atlasW * invW * 65536.f);
                         const int32_t atlasDv = (int32_t)(atlasH * invH * 65536.f);
                         const int32_t atlasU0 = (int32_t)(((float)g->atlasLeft + atlasW * ((float)ix0 + 0.5f - gx0) * invW) * 65536.f);
                         const int32_t atlasV0 = (int32_t)(((float)g->atlasBottom + atlasH * ((float)iy0 + 0.5f - gy0) * invH) * 65536.f);
                         const GlyphSampler sampler{
                             plat,
                             font->atlasData,
                             (int32_t)font->atlasWidth,
                             glyphL,
                             glyphR,
                             glyphB,
                             glyphT};

                         int32_t atlasV = atlasV0;
                         if (!useFade)
                         {
                             for (int py = iy0; py < iy1; ++py, atlasV += atlasDv)
                             {
                                 const GlyphRowSampler rowSampler = sampler.row(atlasV);
                                 int32_t atlasU = atlasU0;
                                 uint16_t *dst = buf + (int32_t)py * stride + ix0;
                                 for (int px = ix0; px < ix1; ++px, ++dst, atlasU += atlasDu)
                                 {
                                     const uint8_t s8 = rowSampler.sample(atlasU);
                                     if (s8 <= s8Min)
                                         continue;
                                     const uint8_t alpha = alphaLut.values[s8];
                                     if (alpha)
                                         blendNative565(dst, fg, alpha);
                                 }
                             }
                         }
                        else
                        {
                            const int fadeLeftEnd = fadeBoxX + fadePxClamped;
                            const int fadeRightStart = fadeBoxR - fadePxClamped;
                            for (int py = iy0; py < iy1; ++py, atlasV += atlasDv)
                            {
                                 const GlyphRowSampler rowSampler = sampler.row(atlasV);
                                 int32_t atlasU = atlasU0;
                                 uint16_t *dst = buf + (int32_t)py * stride + ix0;
                                 int px = ix0;

                                 for (; px < ix1 && px < fadeBoxX; ++px, ++dst, atlasU += atlasDu)
                                 {
                                 }

                                 const int leftLimit = std::min(ix1, fadeLeftEnd);
                                 for (; px < leftLimit; ++px, ++dst, atlasU += atlasDu)
                                 {
                                     const uint8_t edgeAlpha = detail::fadeEdgeAlpha(px, fadeBoxX, fadeBoxR, fadePxClamped);
                                     if (edgeAlpha == 0)
                                         continue;

                                     const uint8_t s8 = rowSampler.sample(atlasU);
                                     if (s8 <= s8Min)
                                         continue;

                                     uint16_t alpha = alphaLut.values[s8];
                                     if (!alpha)
                                         continue;
                                     if (edgeAlpha < 255)
                                     {
                                         alpha = (uint16_t)((alpha * edgeAlpha + 127U) / 255U);
                                         if (!alpha)
                                             continue;
                                     }
                                     blendNative565(dst, fg, (uint8_t)alpha);
                                 }

                                 const int middleLimit = std::min(ix1, fadeRightStart);
                                 for (; px < middleLimit; ++px, ++dst, atlasU += atlasDu)
                                 {
                                     const uint8_t s8 = rowSampler.sample(atlasU);
                                     if (s8 <= s8Min)
                                         continue;

                                     const uint8_t alpha = alphaLut.values[s8];
                                     if (alpha)
                                         blendNative565(dst, fg, alpha);
                                 }

                                 const int rightLimit = std::min(ix1, fadeBoxR);
                                 for (; px < rightLimit; ++px, ++dst, atlasU += atlasDu)
                                 {
                                     const uint8_t edgeAlpha = detail::fadeEdgeAlpha(px, fadeBoxX, fadeBoxR, fadePxClamped);
                                     if (edgeAlpha == 0)
                                         continue;

                                     const uint8_t s8 = rowSampler.sample(atlasU);
                                     if (s8 <= s8Min)
                                         continue;

                                     uint16_t alpha = alphaLut.values[s8];
                                     if (!alpha)
                                         continue;
                                     if (edgeAlpha < 255)
                                     {
                                         alpha = (uint16_t)((alpha * edgeAlpha + 127U) / 255U);
                                         if (!alpha)
                                             continue;
                                     }
                                     blendNative565(dst, fg, (uint8_t)alpha);
                                 }

                                 for (; px < ix1; ++px, ++dst, atlasU += atlasDu)
                                 {
                                 }
                             }
                         }
                         return true;
                     });
    }

    void GUI::drawTextAligned(const String &text, int16_t x, int16_t y,
                              uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)tw) : x;
        if (align == AlignCenter)
            rx -= (tw + 1) / 2;
        else if (align == AlignRight)
            rx -= tw;

        int16_t ry = (y == -1) ? AutoY((int32_t)th) : y;
        const int16_t weightExpandX = weightExpandXPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);
        const int16_t weightExpandY = weightExpandYPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);
        if (weightExpandX > 0 || weightExpandY > 0)
        {
            rx += weightExpandX;
            ry += weightExpandY;
        }
        drawTextImmediate(text, rx, ry, tw, th, fg565, bg565, align);
    }

    void GUI::drawText(const String &text, int16_t x, int16_t y,
                       uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (!_flags.spriteEnabled || _flags.inSpritePass || !_disp.display)
            return drawTextAligned(text, x, y, fg565, bg565, align);
        updateText(text, x, y, fg565, bg565, align);
    }

    void GUI::updateText(const String &text, int16_t x, int16_t y,
                         uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (!_typo.psdfSizePx || !fontDataForId(_typo.currentFontId))
            return;

        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)tw) : x;
        if (align == AlignCenter)
            rx -= (tw + 1) / 2;
        else if (align == AlignRight)
            rx -= tw;
        int16_t ry = (y == -1) ? AutoY((int32_t)th) : y;
        const int16_t weightExpandX = weightExpandXPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);
        const int16_t weightExpandY = weightExpandYPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);
        if (weightExpandX > 0 || weightExpandY > 0)
        {
            rx += weightExpandX;
            ry += weightExpandY;
        }

        constexpr int16_t pad = 4;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().pos(rx - pad, ry - pad).size(tw + pad * 2, th + pad * 2).color(bg565).draw();
        drawTextImmediate(text, rx, ry, tw, th, fg565, bg565, align);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect(rx - pad, ry - pad, tw + pad * 2, th + pad * 2);
    }

}

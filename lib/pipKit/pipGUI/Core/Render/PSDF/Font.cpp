#include <algorithm>
#include <math.h>
#include <pipGUI/Core/API/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/BuilderAccess.hpp>
#include <pipGUI/Core/Utils/Colors.hpp>
#include <pipGUI/Core/Render/Draw/Blend.hpp>
#include <pipGUI/Fonts/WixMadeForDisplay/WixMadeForDisplay.hpp>
#include <pipGUI/Fonts/WixMadeForDisplay/metrics.hpp>
#include <pipGUI/Fonts/KronaOne/KronaOne.hpp>
#include <pipGUI/Fonts/KronaOne/metrics.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint16_t kMarqueeGapPx = 18;
        constexpr uint8_t kMarqueeEdgeFadePx = 10;

        static inline uint16_t clampFontWeight(uint16_t weight)
        {
            return std::min<uint16_t>(900, std::max<uint16_t>(100, weight));
        }

        static inline float normalizedWeightDelta(uint16_t weight)
        {
            return ((float)clampFontWeight(weight) - 400.0f) / 500.0f;
        }

        static inline float weightStrokePxFor(uint16_t weight, float sizePx)
        {
            const float delta = normalizedWeightDelta(weight);
            const float magnitude = fabsf(delta);
            const float baseStrokePx = sizePx * 0.105f + 0.12f;

            if (delta >= 0.0f)
                return delta * baseStrokePx * (1.0f + 0.50f * magnitude);
            return delta * baseStrokePx * 0.85f * (1.0f + 0.25f * magnitude);
        }

        static inline float weightCoverageGammaFor(uint16_t weight)
        {
            const float delta = normalizedWeightDelta(weight);
            if (delta >= 0.0f)
                return 1.0f - 0.20f * delta;
            return 1.0f + 0.28f * -delta;
        }

        static inline float weightMeasureExpandXPx(uint16_t weight, float sizePx)
        {
            return std::max(0.0f, weightStrokePxFor(weight, sizePx) * 0.85f);
        }

        static inline float weightMeasureExpandYPx(uint16_t weight, float sizePx)
        {
            return std::max(0.0f, weightStrokePxFor(weight, sizePx) * 0.32f);
        }

        static size_t prevUtf8Boundary(const String &text, size_t len)
        {
            if (len == 0)
                return 0;

            const char *buf = text.c_str();
            size_t pos = len - 1;
            while (pos > 0 && (((uint8_t)buf[pos] & 0xC0U) == 0x80U))
                --pos;
            return pos;
        }

    }

    struct Glyph
    {
        uint32_t codepoint;
        uint16_t advance;
        int8_t padLeft;
        int8_t padBottom;
        int8_t padRight;
        int8_t padTop;
        uint16_t atlasLeft;
        uint16_t atlasBottom;
        uint16_t atlasRight;
        uint16_t atlasTop;

        __attribute__((always_inline)) float unpackAdvance() const
        {
            return advance * (1.0f / 256.0f);
        }
        __attribute__((always_inline)) float unpackPadLeft() const
        {
            return padLeft * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadBottom() const
        {
            return padBottom * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadRight() const
        {
            return padRight * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadTop() const
        {
            return padTop * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackAtlasLeft() const
        {
            return (float)atlasLeft;
        }
        __attribute__((always_inline)) float unpackAtlasBottom() const
        {
            return (float)atlasBottom;
        }
        __attribute__((always_inline)) float unpackAtlasRight() const
        {
            return (float)atlasRight;
        }
        __attribute__((always_inline)) float unpackAtlasTop() const
        {
            return (float)atlasTop;
        }
    };
    static_assert(sizeof(Glyph) == 20, "Glyph struct size check");

    struct PSDFFontData
    {
        const uint8_t *atlasData;
        uint16_t atlasWidth;
        uint16_t atlasHeight;
        float distanceRange;
        float nominalSizePx;
        float ascender;
        float descender;
        float lineHeight;
        const void *glyphs;
        uint16_t glyphCount;
    };

    static inline float weightBiasFor(uint16_t weight, float sizePx, const PSDFFontData *font)
    {
        const float distanceScale = font->distanceRange * (sizePx / font->nominalSizePx);
        if (distanceScale <= 0.0001f)
            return 0.0f;
        return weightStrokePxFor(weight, sizePx) / distanceScale;
    }

    static const PSDFFontData fontWixMadeForDisplay = {
        ::WixMadeForDisplay,
        psdf_wixfor::AtlasWidth, psdf_wixfor::AtlasHeight,
        psdf_wixfor::DistanceRange, psdf_wixfor::NominalSizePx,
        psdf_wixfor::Ascender, psdf_wixfor::Descender,
        psdf_wixfor::LineHeight,
        psdf_wixfor::Glyphs, psdf_wixfor::GlyphCount};

    static const PSDFFontData fontKronaOne = {
        ::KronaOne,
        psdf_krona::AtlasWidth, psdf_krona::AtlasHeight,
        psdf_krona::DistanceRange, psdf_krona::NominalSizePx,
        psdf_krona::Ascender, psdf_krona::Descender,
        psdf_krona::LineHeight,
        psdf_krona::Glyphs, psdf_krona::GlyphCount};

    static const PSDFFontData *g_fontRegistry[8] = {
        &fontWixMadeForDisplay, &fontKronaOne,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    static uint8_t g_fontCount = 2;

    static inline const PSDFFontData *fontDataForId(FontId fontId)
    {
        const uint16_t id = (uint16_t)fontId;
        return (id < g_fontCount) ? g_fontRegistry[id] : nullptr;
    }

    namespace detail
    {
        struct TextFontGuard
        {
            GUI *gui;
            FontId prevId;
            uint16_t prevSz;
            uint16_t prevWeight;
            bool hasPrev;

            explicit TextFontGuard(GUI *g)
                : gui(g),
                  prevId(g->fontId()),
                  prevSz(g->fontSize()),
                  prevWeight(g->fontWeight()),
                  hasPrev(fontDataForId(g->fontId()) != nullptr)
            {
            }

            ~TextFontGuard()
            {
                if (hasPrev)
                    static_cast<void>(gui->setFont(prevId));
                gui->setFontSize(prevSz);
                gui->setFontWeight(prevWeight);
            }
        };
    } // namespace detail

    FontId GUI::registerFont(const uint8_t *atlasData,
                             uint16_t atlasWidth, uint16_t atlasHeight,
                             float distanceRange, float nominalSizePx,
                             float ascender, float descender, float lineHeight,
                             const void *glyphs, uint16_t glyphCount)
    {
        if (g_fontCount >= 8)
            return (FontId)255;

        static PSDFFontData registry[8];
        PSDFFontData *f = &registry[g_fontCount - 2];
        *f = {atlasData, atlasWidth, atlasHeight, distanceRange, nominalSizePx,
              ascender, descender, lineHeight, glyphs, glyphCount};

        FontId newId = (FontId)g_fontCount;
        g_fontRegistry[g_fontCount++] = f;
        return newId;
    }

    static inline uint32_t decodeUtf8(const char *s, int &i, int len)
    {
        if (i >= len)
            return (uint32_t)'?';
        uint8_t c0 = (uint8_t)s[i++];
        if ((c0 & 0x80U) == 0)
            return c0;

        auto isCont = [](uint8_t c)
        { return (c & 0xC0U) == 0x80U; };

        if ((c0 & 0xE0U) == 0xC0U)
        {
            if (i >= len)
                return (uint32_t)'?';
            uint8_t c1 = (uint8_t)s[i++];
            if (!isCont(c1) || c0 < 0xC2U)
                return (uint32_t)'?';
            return ((uint32_t)(c0 & 0x1FU) << 6) | (uint32_t)(c1 & 0x3FU);
        }
        if ((c0 & 0xF0U) == 0xE0U)
        {
            if (i + 1 >= len)
                return (uint32_t)'?';
            uint8_t c1 = (uint8_t)s[i++], c2 = (uint8_t)s[i++];
            if (!isCont(c1) || !isCont(c2))
                return (uint32_t)'?';
            if (c0 == 0xE0U && c1 < 0xA0U)
                return (uint32_t)'?';
            if (c0 == 0xEDU && c1 >= 0xA0U)
                return (uint32_t)'?';
            return ((uint32_t)(c0 & 0x0FU) << 12) | ((uint32_t)(c1 & 0x3FU) << 6) | (uint32_t)(c2 & 0x3FU);
        }
        if ((c0 & 0xF8U) == 0xF0U)
        {
            if (i + 2 >= len)
                return (uint32_t)'?';
            uint8_t c1 = (uint8_t)s[i++], c2 = (uint8_t)s[i++], c3 = (uint8_t)s[i++];
            if (!isCont(c1) || !isCont(c2) || !isCont(c3))
                return (uint32_t)'?';
            if (c0 == 0xF0U && c1 < 0x90U)
                return (uint32_t)'?';
            if (c0 > 0xF4U || (c0 == 0xF4U && c1 > 0x8FU))
                return (uint32_t)'?';
            return ((uint32_t)(c0 & 0x07U) << 18) | ((uint32_t)(c1 & 0x3FU) << 12) |
                   ((uint32_t)(c2 & 0x3FU) << 6) | (uint32_t)(c3 & 0x3FU);
        }
        return (uint32_t)'?';
    }

    static inline const Glyph *findGlyph(const PSDFFontData *font, uint32_t cp)
    {
        const Glyph *glyphs = (const Glyph *)font->glyphs;
        int lo = 0, hi = (int)font->glyphCount - 1;
        while (lo <= hi)
        {
            int mid = (lo + hi) >> 1;
            uint32_t v = glyphs[mid].codepoint;
            if (v == cp)
                return &glyphs[mid];
            if (v < cp)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return nullptr;
    }

    static inline float spaceWidth(const PSDFFontData *font)
    {
        static const PSDFFontData *s_font = nullptr;
        static float s_cached = 0.30f;
        if (font == s_font)
            return s_cached;
        s_font = font;
        const Glyph *g = findGlyph(font, (uint32_t)'n');
        return (s_cached = g ? g->unpackAdvance() * 0.5f : 0.30f);
    }

    bool GUI::setFont(FontId fontId)
    {
        if (fontDataForId(fontId))
        {
            _typo.currentFontId = fontId;
            return true;
        }
        return false;
    }
    FontId GUI::fontId() const noexcept { return _typo.currentFontId; }

    void GUI::configureTextStyles(uint16_t h1Px, uint16_t h2Px, uint16_t bodyPx, uint16_t captionPx)
    {
        _typo.h1Px = h1Px ? h1Px : 24;
        _typo.h2Px = h2Px ? h2Px : 18;
        _typo.bodyPx = bodyPx ? bodyPx : 14;
        _typo.captionPx = captionPx ? captionPx : 12;
    }

    void GUI::setTextStyle(TextStyle style)
    {
        uint16_t px = _typo.bodyPx;
        if (style == H1)
            px = _typo.h1Px;
        else if (style == H2)
            px = _typo.h2Px;
        else if (style == Caption)
            px = _typo.captionPx;
        setFontSize(px);
    }

    void GUI::setFontSize(uint16_t px) { _typo.psdfSizePx = px; }
    uint16_t GUI::fontSize() const noexcept { return _typo.psdfSizePx; }

    void GUI::setFontWeight(uint16_t weight)
    {
        _typo.psdfWeight = clampFontWeight(weight);
    }
    uint16_t GUI::fontWeight() const noexcept { return _typo.psdfWeight; }

    template <typename Fn>
    static inline bool forEachGlyph(const char *s, int len,
                                    const PSDFFontData *font, float sizePx,
                                    Fn &&callback)
    {
        static const PSDFFontData *s_cFont = nullptr;
        static uint32_t s_cCp = 0;
        static const Glyph *s_cG = nullptr;

        const float spAdv = spaceWidth(font) * sizePx;
        const float lineAdv = font->lineHeight * sizePx;
        float penX = 0.f, penY = 0.f;

        for (int i = 0; i < len;)
        {
            uint32_t cp = decodeUtf8(s, i, len);

            if (cp == '\r')
                continue;

            if (cp == '\n')
            {
                if (!callback(nullptr, penX, penY + font->ascender * sizePx, true))
                    return false;
                penX = 0.f;
                penY += lineAdv;
                continue;
            }
            if (cp == ' ')
            {
                penX += spAdv;
                continue;
            }
            if (cp == '\t')
            {
                penX += spAdv * 4.f;
                continue;
            }

            const Glyph *g;
            if (font == s_cFont && cp == s_cCp)
            {
                g = s_cG;
            }
            else
            {
                g = findGlyph(font, cp);
                if (!g)
                    g = findGlyph(font, (uint32_t)'?');
                s_cFont = font;
                s_cCp = cp;
                s_cG = g;
            }
            if (!g)
                continue;

            if (!callback(g, penX, penY + font->ascender * sizePx, false))
                return false;
            penX += g->unpackAdvance() * sizePx;
        }
        return true;
    }

    bool GUI::measureText(const String &text, int16_t &outW, int16_t &outH) const
    {
        outW = outH = 0;
        const PSDFFontData *font = fontDataForId(_typo.currentFontId);
        if (!_typo.psdfSizePx || !font)
            return false;
        int len = (int)text.length();
        if (!len)
            return true;

        const float sizePx = (float)_typo.psdfSizePx;
        uint16_t lines = 1;
        float maxX = 0.f, curX = 0.f;

        forEachGlyph(text.c_str(), len, font, sizePx,
                     [&](const Glyph *g, float penX, float, bool nl) -> bool
                     {
                         if (nl)
                         {
                             if (penX > maxX)
                                 maxX = penX;
                             curX = 0.f;
                             ++lines;
                         }
                         else
                         {
                             curX = penX + g->unpackAdvance() * sizePx;
                         }
                         return true;
                     });

        if (curX > maxX)
            maxX = curX;
        outW = (int16_t)ceilf(maxX);
        outH = (int16_t)ceilf((float)lines * font->lineHeight * sizePx);

        const int16_t weightExpandX = (int16_t)ceilf(weightMeasureExpandXPx(_typo.psdfWeight, sizePx));
        const int16_t weightExpandY = (int16_t)ceilf(weightMeasureExpandYPx(_typo.psdfWeight, sizePx));
        if (weightExpandX > 0)
            outW = (int16_t)(outW + weightExpandX * 2);
        if (weightExpandY > 0)
        {
            outH = (int16_t)(outH + weightExpandY * 2);
        }
        return true;
    }

    static inline uint8_t sampleGlyphBilinear(pipcore::Platform *plat,
                                              const PSDFFontData *font,
                                              const Glyph &glyph,
                                              int32_t u16, int32_t v16)
    {
        const int aw = (int)font->atlasWidth;
        const int glyphL = (int)glyph.atlasLeft;
        const int glyphB = (int)glyph.atlasBottom;
        const int glyphR = std::max(glyphL, (int)glyph.atlasRight - 1);
        const int glyphT = std::max(glyphB, (int)glyph.atlasTop - 1);
        auto rd = [&](int x, int y) -> int32_t
        {
            if (x < glyphL)
                x = glyphL;
            else if (x > glyphR)
                x = glyphR;
            if (y < glyphB)
                y = glyphB;
            else if (y > glyphT)
                y = glyphT;
            return (int32_t)plat->readProgmemByte(&font->atlasData[(uint32_t)y * aw + x]);
        };
        return sampleBilinearSDF(rd, u16, v16, font->atlasWidth, font->atlasHeight);
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
        const PSDFFontData *font = fontDataForId(_typo.currentFontId);
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
        const float coverageGamma = weightCoverageGammaFor(_typo.psdfWeight);
        const float biasOffset = 0.5f + weightBias;
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = biasOffset - distanceScale * 0.5f;
        const uint16_t fgNative = pipcore::Sprite::swap16(fg565);

        float dthr = (0.f - kOffset) / kScale;
        uint8_t s8Min = (dthr > 0.f && dthr < 255.f) ? (uint8_t)dthr : 0;

        pipcore::Platform *plat = platform();

        forEachGlyph(text.c_str(), (int)text.length(), font, sizePx,
                     [&](const Glyph *g, float penX, float penY, bool nl) -> bool
                     {
                         if (nl || !g)
                             return true;

                         const float absPenX = rx + penX;
                         const float absPenY = ry + penY;

                         int ix0 = (int)floorf(absPenX + g->unpackPadLeft() * sizePx);
                         int ix1 = (int)ceilf(absPenX + g->unpackPadRight() * sizePx);
                         int iy0 = (int)floorf(absPenY - g->unpackPadTop() * sizePx);
                         int iy1 = (int)ceilf(absPenY - g->unpackPadBottom() * sizePx);

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

                         const float gx0 = absPenX + g->unpackPadLeft() * sizePx;
                         const float gy0 = absPenY - g->unpackPadTop() * sizePx;
                         const float gw = (absPenX + g->unpackPadRight() * sizePx) - gx0;
                         const float gh = (absPenY - g->unpackPadBottom() * sizePx) - gy0;
                         const float invW = (gw != 0.f) ? 1.f / gw : 0.f;
                         const float invH = (gh != 0.f) ? 1.f / gh : 0.f;

                         const float atlasW = (float)(g->atlasRight - g->atlasLeft);
                         const float atlasH = (float)(g->atlasTop - g->atlasBottom);

                         const int32_t atlasDu = (int32_t)(atlasW * invW * 65536.f);
                         const int32_t atlasDv = (int32_t)(atlasH * invH * 65536.f);
                         const int32_t atlasU0 = (int32_t)(((float)g->atlasLeft + atlasW * ((float)ix0 + 0.5f - gx0) * invW) * 65536.f);
                         const int32_t atlasV0 = (int32_t)(((float)g->atlasBottom + atlasH * ((float)iy0 + 0.5f - gy0) * invH) * 65536.f);

                         int32_t atlasV = atlasV0;
                         for (int py = iy0; py < iy1; ++py, atlasV += atlasDv)
                         {
                             const int32_t row = (int32_t)py * stride;
                             int32_t atlasU = atlasU0;

                             for (int px = ix0; px < ix1; ++px, atlasU += atlasDu)
                             {
                                 const uint8_t edgeAlpha = useFade ? detail::fadeEdgeAlpha(px, fadeBoxX, fadeBoxR, fadePxClamped) : 255;
                                 if (edgeAlpha == 0)
                                     continue;

                                 uint8_t s8 = sampleGlyphBilinear(plat, font, *g, atlasU, atlasV);
                                 if (s8 <= s8Min)
                                     continue;

                                 const float a = (float)s8 * kScale + kOffset;
                                 float aa = a;
                                 if (aa > 0.0f && aa < 1.0f && coverageGamma != 1.0f)
                                     aa = powf(aa, coverageGamma);

                                 uint16_t alpha = 0;
                                 if (aa >= 1.f)
                                     alpha = 255;
                                 else if (aa > 0.f)
                                     alpha = (uint16_t)(aa * 255.f + 0.5f);
                                 if (alpha == 0)
                                     continue;

                                 if (edgeAlpha < 255)
                                 {
                                     alpha = (uint16_t)((alpha * edgeAlpha + 127U) / 255U);
                                     if (alpha == 0)
                                         continue;
                                 }

                                 const int32_t idx = row + px;
                                 if (alpha >= 255)
                                 {
                                     buf[idx] = fgNative;
                                 }
                                 else
                                 {
                                     const uint16_t dst = pipcore::Sprite::swap16(buf[idx]);
                                     buf[idx] = pipcore::Sprite::swap16(detail::blend565(dst, fg565, (uint8_t)alpha));
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
        const int16_t weightExpandX = (int16_t)ceilf(weightMeasureExpandXPx(_typo.psdfWeight, (float)_typo.psdfSizePx));
        const int16_t weightExpandY = (int16_t)ceilf(weightMeasureExpandYPx(_typo.psdfWeight, (float)_typo.psdfSizePx));
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

    bool GUI::drawTextMarquee(const String &text, int16_t x, int16_t y,
                              int16_t maxWidth, uint16_t fg565,
                              TextAlign align, const MarqueeTextOptions &opts)
    {
        if (maxWidth <= 0)
            return false;

        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return false;

        int16_t boxX = (x == -1) ? AutoX((int32_t)maxWidth) : x;
        if (align == AlignCenter)
            boxX -= maxWidth / 2;
        else if (align == AlignRight)
            boxX -= maxWidth;

        const int16_t boxY = (y == -1) ? AutoY((int32_t)th) : y;
        if (tw <= maxWidth)
            return false;
        const int16_t weightExpandX = (int16_t)ceilf(weightMeasureExpandXPx(_typo.psdfWeight, (float)_typo.psdfSizePx));
        const int16_t weightExpandY = (int16_t)ceilf(weightMeasureExpandYPx(_typo.psdfWeight, (float)_typo.psdfSizePx));

        pipcore::Sprite *target = getDrawTarget();
        if (!target)
            return false;

        int32_t prevClipX = 0, prevClipY = 0, prevClipW = 0, prevClipH = 0;
        target->getClipRect(&prevClipX, &prevClipY, &prevClipW, &prevClipH);

        int32_t clipX = boxX;
        int32_t clipY = boxY;
        int32_t clipW = maxWidth;
        int32_t clipH = th;

        if (prevClipW > 0 && prevClipH > 0)
        {
            const int32_t prevRight = prevClipX + prevClipW;
            const int32_t prevBottom = prevClipY + prevClipH;
            const int32_t boxRight = clipX + clipW;
            const int32_t boxBottom = clipY + clipH;

            if (clipX < prevClipX)
                clipX = prevClipX;
            if (clipY < prevClipY)
                clipY = prevClipY;

            const int32_t finalRight = (boxRight < prevRight) ? boxRight : prevRight;
            const int32_t finalBottom = (boxBottom < prevBottom) ? boxBottom : prevBottom;
            clipW = finalRight - clipX;
            clipH = finalBottom - clipY;
        }

        if (clipW <= 0 || clipH <= 0)
        {
            target->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);
            return true;
        }

        target->setClipRect(clipX, clipY, clipW, clipH);

        const uint16_t speedPxPerSec = opts.speedPxPerSec ? opts.speedPxPerSec : 28;
        const uint16_t holdStartMs = opts.holdStartMs;
        const int32_t loopPx = tw + kMarqueeGapPx;

        const uint32_t now = nowMs();
        uint32_t elapsedMs = now;
        if (opts.phaseStartMs != 0)
            elapsedMs = (now >= opts.phaseStartMs) ? (now - opts.phaseStartMs) : 0U;

        float offset = 0.0f;
        if (speedPxPerSec > 0 && loopPx > 0 && elapsedMs > holdStartMs)
        {
            const float scrollMs = (float)(elapsedMs - holdStartMs);
            const float distancePx = (scrollMs * (float)speedPxPerSec) / 1000.0f;
            offset = fmodf(distancePx, (float)loopPx);
        }

        drawTextImmediateMasked(text, (int16_t)lroundf((float)boxX - offset) + weightExpandX, boxY + weightExpandY,
                                tw, th, fg565, 0, AlignLeft, boxX, maxWidth, kMarqueeEdgeFadePx);
        if (speedPxPerSec > 0 && loopPx > 0)
        {
            drawTextImmediateMasked(text, (int16_t)lroundf((float)boxX - offset + loopPx) + weightExpandX, boxY + weightExpandY,
                                    tw, th, fg565, 0, AlignLeft, boxX, maxWidth, kMarqueeEdgeFadePx);
        }

        target->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);
        if (speedPxPerSec > 0)
            requestRedraw();
        return true;
    }

    bool GUI::drawTextEllipsized(const String &text, int16_t x, int16_t y,
                                 int16_t maxWidth, uint16_t fg565,
                                 TextAlign align)
    {
        if (maxWidth <= 0)
            return false;

        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return false;
        if (tw <= maxWidth)
            return false;

        int16_t dotsW = 0, dotsH = 0;
        const String dots("...");
        if (!measureText(dots, dotsW, dotsH))
            return false;

        String clipped;
        if (dotsW >= maxWidth)
        {
            clipped = dots;
        }
        else
        {
            String candidate = text;
            int16_t width = 0, height = 0;
            while (candidate.length() > 0)
            {
                const size_t cut = prevUtf8Boundary(candidate, candidate.length());
                candidate.remove(cut);
                String trial = candidate + dots;
                if (measureText(trial, width, height) && width <= maxWidth)
                {
                    clipped = trial;
                    break;
                }
            }
            if (clipped.length() == 0)
                clipped = dots;
        }

        if (clipped.length() == 0)
            return false;

        drawTextAligned(clipped, x, y, fg565, 0, align);
        return true;
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
        const int16_t weightExpandX = (int16_t)ceilf(weightMeasureExpandXPx(_typo.psdfWeight, (float)_typo.psdfSizePx));
        const int16_t weightExpandY = (int16_t)ceilf(weightMeasureExpandYPx(_typo.psdfWeight, (float)_typo.psdfSizePx));
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

    template <bool IsUpdate>
    void TextFluentT<IsUpdate>::draw()
    {
        if (_text.length() == 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::callByMode<IsUpdate>(
            [&]
            { detail::BuilderAccess::updateText(*_gui, _text, _x, _y, _fg565, _bg565, _align); },
            [&]
            { detail::BuilderAccess::drawText(*_gui, _text, _x, _y, _fg565, _bg565, _align); });
    }
    template void TextFluentT<false>::draw();
    template void TextFluentT<true>::draw();

    void DrawTextMarqueeFluent::draw()
    {
        if (_text.length() == 0 || _maxWidth <= 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::BuilderAccess::drawTextMarquee(*_gui, _text, _x, _y, _maxWidth, _fg565, _align, _opts);
    }

    void DrawTextEllipsizedFluent::draw()
    {
        if (_text.length() == 0 || _maxWidth <= 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::BuilderAccess::drawTextEllipsized(*_gui, _text, _x, _y, _maxWidth, _fg565, _align);
    }
}

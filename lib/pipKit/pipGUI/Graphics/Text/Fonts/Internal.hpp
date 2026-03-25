#pragma once

#include <algorithm>
#include <math.h>
#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Graphics/Draw/Blend.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Text/Fonts/KronaOne/metrics.hpp>
#include <pipGUI/Graphics/Text/Fonts/WixMadeForDisplay/metrics.hpp>

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

        static inline float readabilityFactorFor(float sizePx)
        {
            if (sizePx <= 11.0f)
                return 1.0f;
            if (sizePx >= 28.0f)
                return 0.0f;
            return (28.0f - sizePx) * (1.0f / 17.0f);
        }

        static inline float readabilityDarkenPxFor(float sizePx)
        {
            return 0.18f * readabilityFactorFor(sizePx);
        }

        static inline float readabilityContrastFor(float sizePx)
        {
            return 1.0f + 0.34f * readabilityFactorFor(sizePx);
        }

        static inline float readabilityGammaFor(float sizePx)
        {
            return 1.0f - 0.10f * readabilityFactorFor(sizePx);
        }

        static inline float weightMeasureExpandXPx(uint16_t weight, float sizePx)
        {
            return std::max(0.0f, weightStrokePxFor(weight, sizePx) * 0.85f);
        }

        static inline float weightMeasureExpandYPx(uint16_t weight, float sizePx)
        {
            return std::max(0.0f, weightStrokePxFor(weight, sizePx) * 0.32f);
        }

        static inline size_t prevUtf8Boundary(const String &text, size_t len)
        {
            if (len == 0)
                return 0;

            const char *buf = text.c_str();
            size_t pos = len - 1;
            while (pos > 0 && (((uint8_t)buf[pos] & 0xC0U) == 0x80U))
                --pos;
            return pos;
        }

        static inline int floorToInt(float value)
        {
            const int ivalue = (int)value;
            return (value < (float)ivalue) ? (ivalue - 1) : ivalue;
        }

        static inline int ceilToInt(float value)
        {
            const int ivalue = (int)value;
            return (value > (float)ivalue) ? (ivalue + 1) : ivalue;
        }

        static inline int16_t weightExpandXPxInt(uint16_t weight, float sizePx)
        {
            return (int16_t)ceilToInt(weightMeasureExpandXPx(weight, sizePx));
        }

        static inline int16_t weightExpandYPxInt(uint16_t weight, float sizePx)
        {
            return (int16_t)ceilToInt(weightMeasureExpandYPx(weight, sizePx));
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

        __attribute__((always_inline)) float unpackAdvance() const { return advance * (1.0f / 256.0f); }
        __attribute__((always_inline)) float unpackPadLeft() const { return padLeft * (1.0f / 128.0f); }
        __attribute__((always_inline)) float unpackPadBottom() const { return padBottom * (1.0f / 128.0f); }
        __attribute__((always_inline)) float unpackPadRight() const { return padRight * (1.0f / 128.0f); }
        __attribute__((always_inline)) float unpackPadTop() const { return padTop * (1.0f / 128.0f); }
        __attribute__((always_inline)) float unpackAtlasLeft() const { return (float)atlasLeft; }
        __attribute__((always_inline)) float unpackAtlasBottom() const { return (float)atlasBottom; }
        __attribute__((always_inline)) float unpackAtlasRight() const { return (float)atlasRight; }
        __attribute__((always_inline)) float unpackAtlasTop() const { return (float)atlasTop; }
    };
    static_assert(sizeof(Glyph) == 20, "Glyph struct size check");

    struct FontData
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
        const void *kerningPairs;
        uint16_t kerningPairCount;
        int8_t tracking128;
    };

    struct TextLayoutBox
    {
        int16_t width = 0;
        int16_t height = 0;
        int16_t originX = 0;
        int16_t originY = 0;
    };

    struct KerningPair
    {
        uint32_t left;
        uint32_t right;
        int16_t adjust;

        __attribute__((always_inline)) float unpackAdjust() const { return adjust * (1.0f / 256.0f); }
    };

    [[nodiscard]] const FontData *fontDataForId(FontId fontId);

    static inline float fontTrackingPx(const FontData *font, float sizePx)
    {
        if (!font || font->tracking128 == 0)
            return 0.0f;
        return (font->tracking128 * (1.0f / 128.0f)) * sizePx;
    }

    static inline float weightTrackingAdjustPx(uint16_t weight, float sizePx)
    {
        const float delta = normalizedWeightDelta(weight);
        if (delta == 0.0f || sizePx <= 0.0f)
            return 0.0f;

        if (delta > 0.0f)
            return -delta * sizePx * 0.012f;
        return (-delta) * sizePx * 0.0035f;
    }

    static inline float fontKerningAdjustPx(const FontData *font, uint32_t prevCodepoint, uint32_t codepoint, float sizePx)
    {
        if (!font || !font->kerningPairs || font->kerningPairCount == 0 || sizePx <= 0.0f)
            return 0.0f;

        const KerningPair *pairs = (const KerningPair *)font->kerningPairs;
        int lo = 0;
        int hi = (int)font->kerningPairCount - 1;
        while (lo <= hi)
        {
            const int mid = (lo + hi) >> 1;
            const KerningPair &pair = pairs[mid];
            if (pair.left == prevCodepoint && pair.right == codepoint)
                return pair.unpackAdjust() * sizePx;
            if (pair.left < prevCodepoint || (pair.left == prevCodepoint && pair.right < codepoint))
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return 0.0f;
    }

    static inline float weightBiasFor(uint16_t weight, float sizePx, const FontData *font)
    {
        const float distanceScale = font->distanceRange * (sizePx / font->nominalSizePx);
        if (distanceScale <= 0.0001f)
            return 0.0f;
        return weightStrokePxFor(weight, sizePx) / distanceScale;
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

    static inline const Glyph *findGlyph(const FontData *font, uint32_t cp)
    {
        const Glyph *glyphs = (const Glyph *)font->glyphs;
        int lo = 0;
        int hi = (int)font->glyphCount - 1;
        while (lo <= hi)
        {
            const int mid = (lo + hi) >> 1;
            const uint32_t value = glyphs[mid].codepoint;
            if (value == cp)
                return &glyphs[mid];
            if (value < cp)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return nullptr;
    }

    static inline float spaceWidth(const FontData *font)
    {
        static const FontData *s_font = nullptr;
        static float s_cached = 0.30f;
        if (font == s_font)
            return s_cached;
        s_font = font;
        const Glyph *glyph = findGlyph(font, (uint32_t)'n');
        s_cached = glyph ? glyph->unpackAdvance() * 0.5f : 0.30f;
        return s_cached;
    }

    template <typename Fn>
    static inline bool forEachGlyph(const char *s, int len, const FontData *font, float sizePx, uint16_t weight, Fn &&callback)
    {
        static const FontData *s_cachedFont = nullptr;
        static uint32_t s_cachedCodepoint = 0;
        static const Glyph *s_cachedGlyph = nullptr;

        const float spaceAdvance = spaceWidth(font) * sizePx;
        const float lineAdvance = font->lineHeight * sizePx;
        const float trackingAdvance = fontTrackingPx(font, sizePx) + weightTrackingAdjustPx(weight, sizePx);
        float penX = 0.0f;
        float penY = 0.0f;
        uint32_t prevCodepoint = 0;

        for (int i = 0; i < len;)
        {
            const uint32_t codepoint = decodeUtf8(s, i, len);
            if (codepoint == '\r')
                continue;
            if (codepoint == '\n')
            {
                if (!callback(nullptr, penX, penY + font->ascender * sizePx, true))
                    return false;
                penX = 0.0f;
                penY += lineAdvance;
                prevCodepoint = 0;
                continue;
            }
            if (codepoint == ' ')
            {
                penX += spaceAdvance;
                prevCodepoint = 0;
                continue;
            }
            if (codepoint == '\t')
            {
                penX += spaceAdvance * 4.0f;
                prevCodepoint = 0;
                continue;
            }

            const Glyph *glyph = nullptr;
            if (font == s_cachedFont && codepoint == s_cachedCodepoint)
            {
                glyph = s_cachedGlyph;
            }
            else
            {
                glyph = findGlyph(font, codepoint);
                if (!glyph)
                    glyph = findGlyph(font, (uint32_t)'?');
                s_cachedFont = font;
                s_cachedCodepoint = codepoint;
                s_cachedGlyph = glyph;
            }
            if (!glyph)
                continue;

            if (prevCodepoint != 0)
            {
                penX += trackingAdvance;
                penX += fontKerningAdjustPx(font, prevCodepoint, codepoint, sizePx);
            }

            if (!callback(glyph, penX, penY + font->ascender * sizePx, false))
                return false;
            penX += glyph->unpackAdvance() * sizePx;
            prevCodepoint = codepoint;
        }
        return true;
    }

    static inline bool computeTextLayoutBox(const char *s, int len,
                                            const FontData *font,
                                            uint16_t sizePxU16,
                                            uint16_t weight,
                                            TextLayoutBox &out)
    {
        out = {};
        if (!font || sizePxU16 == 0)
            return false;
        if (len <= 0)
            return true;

        const float sizePx = (float)sizePxU16;
        const float lineAdvance = font->lineHeight * sizePx;
        const float padScale = sizePx * (1.0f / 128.0f);
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = lineAdvance;
        float maxAdvanceX = 0.0f;
        uint16_t lines = 1;
        bool hasInk = false;

        forEachGlyph(s, len, font, sizePx, weight,
                     [&](const Glyph *g, float penX, float penY, bool nl) -> bool
                     {
                         if (nl)
                         {
                             if (penX > maxAdvanceX)
                                 maxAdvanceX = penX;
                             ++lines;
                             return true;
                         }
                         if (!g)
                             return true;

                         const float advanceX = penX + g->unpackAdvance() * sizePx;
                         if (advanceX > maxAdvanceX)
                             maxAdvanceX = advanceX;

                         const float gx0 = penX + (float)g->padLeft * padScale;
                         const float gx1 = penX + (float)g->padRight * padScale;
                         const float gy0 = penY - (float)g->padTop * padScale;
                         const float gy1 = penY - (float)g->padBottom * padScale;

                         if (!hasInk)
                         {
                             minX = gx0;
                             minY = gy0;
                             maxX = gx1;
                             maxY = gy1;
                             hasInk = true;
                             return true;
                         }

                         if (gx0 < minX)
                             minX = gx0;
                         if (gy0 < minY)
                             minY = gy0;
                         if (gx1 > maxX)
                             maxX = gx1;
                         if (gy1 > maxY)
                             maxY = gy1;
                         return true;
                     });

        if (maxAdvanceX > maxX)
            maxX = maxAdvanceX;
        const float lineBottom = (float)lines * lineAdvance;
        if (lineBottom > maxY)
            maxY = lineBottom;
        if (minX > 0.0f)
            minX = 0.0f;
        if (minY > 0.0f)
            minY = 0.0f;

        const int minXI = floorToInt(minX);
        const int minYI = floorToInt(minY);
        const int maxXI = ceilToInt(maxX);
        const int maxYI = ceilToInt(maxY);
        const int16_t weightExpandX = weightExpandXPxInt(weight, sizePx);
        const int16_t weightExpandY = weightExpandYPxInt(weight, sizePx);

        out.width = (int16_t)std::max(0, maxXI - minXI + weightExpandX * 2);
        out.height = (int16_t)std::max(0, maxYI - minYI + weightExpandY * 2);
        out.originX = (int16_t)(-minXI + weightExpandX);
        out.originY = (int16_t)(-minYI + weightExpandY);
        return true;
    }
}

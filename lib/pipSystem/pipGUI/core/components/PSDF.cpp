#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/WixMadeForDisplay.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/metrics.hpp>
#include <pipGUI/fonts/KronaOne/KronaOne.hpp>
#include <pipGUI/fonts/KronaOne/metrics.hpp>
#include <pipGUI/icons/icons.hpp>
#include <pipGUI/icons/metrics.hpp>

namespace pipgui
{

    static inline uint16_t swap16(uint16_t v)
    {
        return (uint16_t)((v >> 8) | (v << 8));
    }

    static inline uint8_t pgmAtIcons(int x, int y)
    {
        if ((uint32_t)x >= (uint32_t)psdf_icons::AtlasWidth || (uint32_t)y >= (uint32_t)psdf_icons::AtlasHeight)
            return 0;
        uint32_t idx = (uint32_t)y * (uint32_t)psdf_icons::AtlasWidth + (uint32_t)x;
        return (uint8_t)pgm_read_byte(&icons[idx]);
    }

    static inline uint8_t sampleBilinearIcons(float u, float v)
    {
        int x0 = (int)u;
        int y0 = (int)v;
        float fx = u - (float)x0;
        float fy = v - (float)y0;

        uint8_t s00 = pgmAtIcons(x0, y0);
        uint8_t s10 = pgmAtIcons(x0 + 1, y0);
        uint8_t s01 = pgmAtIcons(x0, y0 + 1);
        uint8_t s11 = pgmAtIcons(x0 + 1, y0 + 1);

        float a0 = (float)s00 + ((float)s10 - (float)s00) * fx;
        float a1 = (float)s01 + ((float)s11 - (float)s01) * fx;
        float a = a0 + (a1 - a0) * fy;
        int ia = (int)(a + 0.5f);
        if (ia < 0)
            ia = 0;
        if (ia > 255)
            ia = 255;
        return (uint8_t)ia;
    }

    struct Glyph
    {
        uint32_t codepoint;
        float advance;
        float pl, pb, pr, pt;
        float al, ab, ar, at;
    };

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

    static const PSDFFontData fontWixMadeForDisplay = {
        ::WixMadeForDisplay,
        psdf_wixfor::AtlasWidth,
        psdf_wixfor::AtlasHeight,
        psdf_wixfor::DistanceRange,
        psdf_wixfor::NominalSizePx,
        psdf_wixfor::Ascender,
        psdf_wixfor::Descender,
        psdf_wixfor::LineHeight,
        psdf_wixfor::Glyphs,
        psdf_wixfor::GlyphCount
    };

    static const PSDFFontData fontKronaOne = {
        ::KronaOne,
        psdf_krona::AtlasWidth,
        psdf_krona::AtlasHeight,
        psdf_krona::DistanceRange,
        psdf_krona::NominalSizePx,
        psdf_krona::Ascender,
        psdf_krona::Descender,
        psdf_krona::LineHeight,
        psdf_krona::Glyphs,
        psdf_krona::GlyphCount
    };

    static const PSDFFontData *g_currentFont = &fontWixMadeForDisplay;

    void GUI::setPSDFFont(FontId fontId)
    {
        switch (fontId)
        {
        case KronaOne:
            g_currentFont = &fontKronaOne;
            break;
        default:
            g_currentFont = &fontWixMadeForDisplay;
            break;
        }
        _psdfFontId = fontId;
    }

    static inline uint32_t nextUtf8(const char *s, int &i)
    {
        uint8_t c0 = (uint8_t)s[i++];
        if ((c0 & 0x80U) == 0)
            return c0;
        if ((c0 & 0xE0U) == 0xC0U)
        {
            uint8_t c1 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x1FU) << 6) | (uint32_t)(c1 & 0x3FU);
        }
        if ((c0 & 0xF0U) == 0xE0U)
        {
            uint8_t c1 = (uint8_t)s[i++];
            uint8_t c2 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x0FU) << 12) | ((uint32_t)(c1 & 0x3FU) << 6) | (uint32_t)(c2 & 0x3FU);
        }
        if ((c0 & 0xF8U) == 0xF0U)
        {
            uint8_t c1 = (uint8_t)s[i++];
            uint8_t c2 = (uint8_t)s[i++];
            uint8_t c3 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x07U) << 18) | ((uint32_t)(c1 & 0x3FU) << 12) | ((uint32_t)(c2 & 0x3FU) << 6) | (uint32_t)(c3 & 0x3FU);
        }
        return (uint32_t)'?';
    }

    static inline const Glyph *findGlyph(uint32_t cp)
    {
        int lo = 0;
        int hi = (int)g_currentFont->glyphCount - 1;
        const Glyph *glyphs = (const Glyph *)g_currentFont->glyphs;
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

    static inline float spaceAdvanceEm()
    {
        static float s_adv = -1.0f;
        if (s_adv >= 0.0f)
            return s_adv;
        const Glyph *g = findGlyph((uint32_t)'n');
        s_adv = g ? (g->advance * 0.5f) : 0.30f;
        return s_adv;
    }

    static inline void resetSpaceAdvanceCache()
    {
        // Call this when switching fonts
    }

    static inline uint8_t pgmAt(int x, int y)
    {
        if ((uint32_t)x >= (uint32_t)g_currentFont->atlasWidth || (uint32_t)y >= (uint32_t)g_currentFont->atlasHeight)
            return 0;
        uint32_t idx = (uint32_t)y * (uint32_t)g_currentFont->atlasWidth + (uint32_t)x;
        return (uint8_t)pgm_read_byte(&g_currentFont->atlasData[idx]);
    }

    static inline uint8_t sampleBilinear(float u, float v)
    {
        int x0 = (int)u;
        int y0 = (int)v;
        float fx = u - (float)x0;
        float fy = v - (float)y0;

        uint8_t s00 = pgmAt(x0, y0);
        uint8_t s10 = pgmAt(x0 + 1, y0);
        uint8_t s01 = pgmAt(x0, y0 + 1);
        uint8_t s11 = pgmAt(x0 + 1, y0 + 1);

        float a0 = (float)s00 + ((float)s10 - (float)s00) * fx;
        float a1 = (float)s01 + ((float)s11 - (float)s01) * fx;
        float a = a0 + (a1 - a0) * fy;
        int ia = (int)(a + 0.5f);
        if (ia < 0)
            ia = 0;
        if (ia > 255)
            ia = 255;
        return (uint8_t)ia;
    }

    bool GUI::loadBuiltinPSDF()
    {
        _flags.psdfLoaded = 1;
        return true;
    }

    void GUI::configureTextStyles(uint16_t h1Px, uint16_t h2Px, uint16_t bodyPx, uint16_t captionPx)
    {
        _textStyleH1Px = h1Px;
        _textStyleH2Px = h2Px;
        _textStyleBodyPx = bodyPx;
        _textStyleCaptionPx = captionPx;
    }

    void GUI::setTextStyle(TextStyle style)
    {
        uint16_t px = _textStyleBodyPx;
        switch (style)
        {
        case H1:
            px = _textStyleH1Px;
            break;
        case H2:
            px = _textStyleH2Px;
            break;
        case Body:
            px = _textStyleBodyPx;
            break;
        case Caption:
            px = _textStyleCaptionPx;
            break;
        default:
            break;
        }

        setPSDFFontSize(px);
    }

    bool GUI::psdfFontLoaded() const
    {
        return _flags.psdfLoaded;
    }

    void GUI::enablePSDF(bool enabled)
    {
        _flags.psdfEnabled = enabled ? 1 : 0;
    }

    bool GUI::psdfEnabled() const
    {
        return _flags.psdfEnabled;
    }

    void GUI::setPSDFFontSize(uint16_t px)
    {
        _psdfSizePx = px;
    }

    uint16_t GUI::psdfFontSize() const
    {
        return _psdfSizePx;
    }

    void GUI::setPSDFWeightBias(float bias)
    {
        _psdfWeightBias = bias;
    }

    float GUI::psdfWeightBias() const
    {
        return _psdfWeightBias;
    }

    bool GUI::psdfMeasureText(const String &text, int16_t &outW, int16_t &outH) const
    {
        outW = 0;
        outH = 0;

        if (_psdfSizePx == 0)
            return false;

        float scale = (float)_psdfSizePx;
        float penX = 0.0f;
        float maxX = 0.0f;
        uint16_t lines = 1;

        const char *s = text.c_str();
        int len = (int)text.length();
        int i = 0;

        while (i < len)
        {
            uint32_t cp = nextUtf8(s, i);

            if (cp == (uint32_t)'\r')
                continue;
            if (cp == (uint32_t)'\n')
            {
                if (penX > maxX)
                    maxX = penX;
                penX = 0.0f;
                ++lines;
                continue;
            }

            if (cp == (uint32_t)' ')
            {
                penX += spaceAdvanceEm() * scale;
                continue;
            }
            if (cp == (uint32_t)'\t')
            {
                penX += spaceAdvanceEm() * scale * 4.0f;
                continue;
            }

            const Glyph *g = findGlyph(cp);
            if (!g)
                g = findGlyph((uint32_t)'?');
            if (!g)
                continue;
            penX += g->advance * scale;
        }

        if (penX > maxX)
            maxX = penX;

        float w = maxX;
        if (w < 0)
            w = 0;

        float h = (float)lines * g_currentFont->lineHeight * scale;
        if (h < 0)
            h = 0;

        outW = (int16_t)(w + 0.5f);
        outH = (int16_t)(h + 0.5f);
        return true;
    }

    void GUI::psdfDrawTextInternal(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (_psdfSizePx == 0)
            return;

        int16_t tw = 0;
        int16_t th = 0;
        if (!psdfMeasureText(text, tw, th))
            return;

        int16_t rx = x;
        int16_t ry = y;

        if (rx == -1)
            rx = AutoX((int32_t)tw);
        else if (align == AlignCenter)
            rx -= (int16_t)(tw / 2);
        else if (align == AlignRight)
            rx -= (int16_t)tw;

        if (ry == -1)
            ry = AutoY((int32_t)th);

        float sizePx = (float)_psdfSizePx;
        float baseline = (float)ry + g_currentFont->ascender * sizePx;
        float drScale = g_currentFont->distanceRange * (sizePx / g_currentFont->nominalSizePx);
        float weightBias = _psdfWeightBias;

        if (_flags.spriteEnabled && (_flags.renderToSprite && (_activeSprite ? _activeSprite->getBuffer() : _sprite.getBuffer())))
        {
            pipcore::Sprite *spr = _activeSprite ? _activeSprite : &_sprite;
            uint16_t *buf = (uint16_t *)spr->getBuffer();
            if (!buf)
                return;

            const int16_t stride = spr->width();
            const int16_t maxH = spr->height();
            if (stride <= 0 || maxH <= 0)
                return;

            auto read565 = [&](int32_t idx) -> uint16_t
            { return swap16(buf[idx]); };
            
            auto write565 = [&](int32_t idx, uint16_t c) -> void
            { buf[idx] = swap16(c); };

            const char *s = text.c_str();
            int len = (int)text.length();
            int i = 0;

            float penX = (float)rx;

            while (i < len)
            {
                uint32_t cp = nextUtf8(s, i);

                if (cp == (uint32_t)'\r')
                    continue;
                if (cp == (uint32_t)'\n')
                {
                    penX = (float)rx;
                    baseline += g_currentFont->lineHeight * sizePx;
                    continue;
                }

                if (cp == (uint32_t)' ')
                {
                    penX += spaceAdvanceEm() * sizePx;
                    continue;
                }
                if (cp == (uint32_t)'\t')
                {
                    penX += spaceAdvanceEm() * sizePx * 4.0f;
                    continue;
                }

                const Glyph *g = findGlyph(cp);
                if (!g)
                    continue;

                float gx0 = penX + g->pl * sizePx;
                float gx1 = penX + g->pr * sizePx;
                float gy0 = baseline - g->pt * sizePx;
                float gy1 = baseline - g->pb * sizePx;

                int ix0 = (int)floorf(gx0);
                int ix1 = (int)ceilf(gx1);
                int iy0 = (int)floorf(gy0);
                int iy1 = (int)ceilf(gy1);

                if (ix1 <= 0 || iy1 <= 0 || ix0 >= stride || iy0 >= maxH)
                {
                    penX += g->advance * sizePx;
                    continue;
                }

                if (ix0 < 0)
                    ix0 = 0;
                if (iy0 < 0)
                    iy0 = 0;
                if (ix1 > stride)
                    ix1 = stride;
                if (iy1 > maxH)
                    iy1 = maxH;

                float srcX0 = g->al;
                float srcX1 = g->ar;

                float syTop = g->ab;
                float syBot = g->at;

                float invW = 0.0f;
                float invH = 0.0f;
                float gw = gx1 - gx0;
                float gh = gy1 - gy0;
                if (gw != 0.0f)
                    invW = 1.0f / gw;
                if (gh != 0.0f)
                    invH = 1.0f / gh;

                for (int py = iy0; py < iy1; ++py)
                {
                    float v = ((float)py + 0.5f - gy0) * invH;
                    float sy = syTop + (syBot - syTop) * v;

                    int32_t row = (int32_t)py * (int32_t)stride;
                    for (int px = ix0; px < ix1; ++px)
                    {
                        float u = ((float)px + 0.5f - gx0) * invW;
                        float sx = srcX0 + (srcX1 - srcX0) * u;

                        uint8_t s8 = sampleBilinear(sx, sy);
                        float sd = ((float)s8 * (1.0f / 255.0f) - 0.5f) * drScale;
                        float a = sd + 0.5f + weightBias;
                        if (a <= 0.0f)
                            continue;
                        if (a >= 1.0f)
                        {
                            write565(row + px, fg565);
                            continue;
                        }

                        uint8_t alpha = (uint8_t)(a * 255.0f + 0.5f);
                        uint16_t dst = read565(row + px);
                        uint16_t out = detail::blend565(dst, fg565, alpha);
                        write565(row + px, out);
                    }
                }

                penX += g->advance * sizePx;
            }

            return;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        const char *s = text.c_str();
        int len = (int)text.length();
        int i = 0;

        float penX = (float)rx;

        while (i < len)
        {
            uint32_t cp = nextUtf8(s, i);

            if (cp == (uint32_t)'\r')
                continue;
            if (cp == (uint32_t)'\n')
            {
                penX = (float)rx;
                baseline += g_currentFont->lineHeight * sizePx;
                continue;
            }

            if (cp == (uint32_t)' ')
            {
                penX += spaceAdvanceEm() * sizePx;
                continue;
            }
            if (cp == (uint32_t)'\t')
            {
                penX += spaceAdvanceEm() * sizePx * 4.0f;
                continue;
            }

            const Glyph *g = findGlyph(cp);
            if (!g)
                continue;

            float gx0 = penX + g->pl * sizePx;
            float gx1 = penX + g->pr * sizePx;
            float gy0 = baseline - g->pt * sizePx;
            float gy1 = baseline - g->pb * sizePx;

            int ix0 = (int)floorf(gx0);
            int ix1 = (int)ceilf(gx1);
            int iy0 = (int)floorf(gy0);
            int iy1 = (int)ceilf(gy1);

            float srcX0 = g->al;
            float srcX1 = g->ar;

            float syTop = g->ab;
            float syBot = g->at;

            float invW = 0.0f;
            float invH = 0.0f;
            float gw = gx1 - gx0;
            float gh = gy1 - gy0;
            if (gw != 0.0f)
                invW = 1.0f / gw;
            if (gh != 0.0f)
                invH = 1.0f / gh;

            for (int py = iy0; py < iy1; ++py)
            {
                float v = ((float)py + 0.5f - gy0) * invH;
                float sy = syTop + (syBot - syTop) * v;

                for (int px = ix0; px < ix1; ++px)
                {
                    float u = ((float)px + 0.5f - gx0) * invW;
                    float sx = srcX0 + (srcX1 - srcX0) * u;

                    uint8_t s8 = sampleBilinear(sx, sy);
                    float sd = ((float)s8 * (1.0f / 255.0f) - 0.5f) * drScale;
                    float a = sd + 0.5f + weightBias;
                    if (a <= 0.0f)
                        continue;
                    if (a >= 1.0f)
                    {
                        t->drawPixel((int16_t)px, (int16_t)py, fg565);
                        continue;
                    }

                    uint8_t alpha = (uint8_t)(a * 255.0f + 0.5f);
                    uint16_t out = detail::blend565(bg565, fg565, alpha);
                    t->drawPixel((int16_t)px, (int16_t)py, out);
                }
            }

            penX += g->advance * sizePx;
        }
    }

    void GUI::drawPSDFText(const String &text, int16_t x, int16_t y, uint32_t color, uint32_t bgColor, TextAlign align)
    {
        if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
        {
            updatePSDFText(text, x, y, color, bgColor, align);
            return;
        }

        uint16_t fg565 = color888To565(x, y, color);
        uint16_t bg565 = color888To565(x, y, bgColor);
        psdfDrawTextInternal(text, x, y, fg565, bg565, align);
    }

    void GUI::updatePSDFText(const String &text, int16_t x, int16_t y, uint32_t color, uint32_t bgColor, TextAlign align)
    {
        int16_t tw = 0;
        int16_t th = 0;
        if (!psdfMeasureText(text, tw, th) || tw <= 0 || th <= 0)
            return;

        int16_t rx = x;
        int16_t ry = y;

        if (rx == -1)
            rx = AutoX((int32_t)tw);
        else if (align == AlignCenter)
            rx -= (int16_t)(tw / 2);
        else if (align == AlignRight)
            rx -= (int16_t)tw;

        if (ry == -1)
            ry = AutoY((int32_t)th);

        int16_t pad = 4;

        float sizePx = (float)_psdfSizePx;
        float baseline = (float)ry + g_currentFont->ascender * sizePx;

        float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
        bool any = false;

        {
            const char *s = text.c_str();
            int len = (int)text.length();
            int i = 0;
            float penX = (float)rx;

            float baselineLocal = baseline;

            while (i < len)
            {
                uint32_t cp = nextUtf8(s, i);

                if (cp == (uint32_t)'\r')
                    continue;
                if (cp == (uint32_t)'\n')
                {
                    penX = (float)rx;
                    baselineLocal += g_currentFont->lineHeight * sizePx;
                    continue;
                }

                if (cp == (uint32_t)' ')
                {
                    penX += spaceAdvanceEm() * sizePx;
                    continue;
                }
                if (cp == (uint32_t)'\t')
                {
                    penX += spaceAdvanceEm() * sizePx * 4.0f;
                    continue;
                }

                const Glyph *g = findGlyph(cp);
                if (!g)
                    continue;

                float gx0 = penX + g->pl * sizePx;
                float gx1 = penX + g->pr * sizePx;
                float gy0 = baselineLocal - g->pt * sizePx;
                float gy1 = baselineLocal - g->pb * sizePx;

                if (!any)
                {
                    minX = gx0;
                    maxX = gx1;
                    minY = gy0;
                    maxY = gy1;
                    any = true;
                }
                else
                {
                    if (gx0 < minX)
                        minX = gx0;
                    if (gy0 < minY)
                        minY = gy0;
                    if (gx1 > maxX)
                        maxX = gx1;
                    if (gy1 > maxY)
                        maxY = gy1;
                }

                penX += g->advance * sizePx;
            }
        }

        if (!any)
        {
            minX = (float)rx;
            maxX = (float)(rx + tw);
            minY = (float)ry;
            maxY = (float)(ry + th);
        }

        int16_t ix0 = (int16_t)floorf(minX);
        int16_t iy0 = (int16_t)floorf(minY);
        int16_t ix1 = (int16_t)ceilf(maxX);
        int16_t iy1 = (int16_t)ceilf(maxY);

        int16_t bx = (int16_t)(ix0 - pad);
        int16_t by = (int16_t)(iy0 - pad);
        int16_t bw = (int16_t)((ix1 - ix0) + pad * 2);
        int16_t bh = (int16_t)((iy1 - iy0) + pad * 2);

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        fillRect(bx, by, bw, bh, bgColor);

        uint16_t fg565 = color888To565(rx, ry, color);
        uint16_t bg565 = color888To565(rx, ry, bgColor);
        psdfDrawTextInternal(text, rx, ry, fg565, bg565, align);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect(bx, by, bw, bh);
        flushDirty();
    }

    static inline void iconBox(uint16_t iconId, uint16_t &outX, uint16_t &outY, uint16_t &outW, uint16_t &outH)
    {
        if (iconId >= psdf_icons::IconCount)
        {
            outX = outY = outW = outH = 0;
            return;
        }
        const psdf_icons::Icon &ic = psdf_icons::Icons[iconId];
        outX = ic.x;
        outY = ic.y;
        outW = ic.w;
        outH = ic.h;
    }

    void GUI::drawIcon(uint16_t iconId,
                       int16_t x,
                       int16_t y,
                       uint16_t sizePx,
                       uint32_t color,
                       uint32_t bgColor)
    {
        if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
        {
            updateIcon(iconId, x, y, sizePx, color, bgColor);
            return;
        }

        uint16_t fg565 = color888To565(x, y, color);
        uint16_t bg565 = color888To565(x, y, bgColor);

        uint16_t srcX = 0, srcY = 0, srcW = 0, srcH = 0;
        iconBox(iconId, srcX, srcY, srcW, srcH);
        if (srcW == 0 || srcH == 0 || sizePx == 0)
            return;

        int16_t rx = x;
        int16_t ry = y;
        if (rx == -1)
            rx = AutoX((int32_t)sizePx);
        if (ry == -1)
            ry = AutoY((int32_t)sizePx);

        float drScale = psdf_icons::DistanceRange * ((float)sizePx / psdf_icons::NominalSizePx);
        float weightBias = _psdfWeightBias;

        if (_flags.spriteEnabled && (_flags.renderToSprite && (_activeSprite ? _activeSprite->getBuffer() : _sprite.getBuffer())))
        {
            pipcore::Sprite *spr = _activeSprite ? _activeSprite : &_sprite;
            uint16_t *buf = (uint16_t *)spr->getBuffer();
            if (!buf)
                return;

            const int16_t stride = spr->width();
            const int16_t maxH = spr->height();
            if (stride <= 0 || maxH <= 0)
                return;

            auto read565 = [&](int32_t idx) -> uint16_t
            { return swap16(buf[idx]); };

            auto write565 = [&](int32_t idx, uint16_t c) -> void
            { buf[idx] = swap16(c); };

            int16_t ix0 = rx;
            int16_t iy0 = ry;
            int16_t ix1 = (int16_t)(rx + (int16_t)sizePx);
            int16_t iy1 = (int16_t)(ry + (int16_t)sizePx);
            if (ix1 <= 0 || iy1 <= 0 || ix0 >= stride || iy0 >= maxH)
                return;
            if (ix0 < 0)
            {
                ix0 = 0;
            }
            if (iy0 < 0)
            {
                iy0 = 0;
            }
            if (ix1 > stride)
                ix1 = stride;
            if (iy1 > maxH)
                iy1 = maxH;

            float invSize = 1.0f / (float)sizePx;
            for (int16_t py = iy0; py < iy1; ++py)
            {
                float v = ((float)(py - ry) + 0.5f) * invSize;
                float sy = (float)srcY + (float)srcH * v;

                int32_t row = (int32_t)py * (int32_t)stride;
                for (int16_t px = ix0; px < ix1; ++px)
                {
                    float u = ((float)(px - rx) + 0.5f) * invSize;
                    float sx = (float)srcX + (float)srcW * u;

                    uint8_t s8 = sampleBilinearIcons(sx, sy);
                    float sd = ((float)s8 * (1.0f / 255.0f) - 0.5f) * drScale;
                    float a = sd + 0.5f + weightBias;
                    if (a <= 0.0f)
                        continue;
                    if (a >= 1.0f)
                    {
                        write565(row + px, fg565);
                        continue;
                    }

                    uint8_t alpha = (uint8_t)(a * 255.0f + 0.5f);
                    uint16_t dst = read565(row + px);
                    uint16_t out = detail::blend565(dst, fg565, alpha);
                    write565(row + px, out);
                }
            }
            return;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        float invSize = 1.0f / (float)sizePx;
        for (uint16_t dy = 0; dy < sizePx; ++dy)
        {
            int16_t py = (int16_t)(ry + (int16_t)dy);
            float v = ((float)dy + 0.5f) * invSize;
            float sy = (float)srcY + (float)srcH * v;

            for (uint16_t dx = 0; dx < sizePx; ++dx)
            {
                int16_t px = (int16_t)(rx + (int16_t)dx);
                float u = ((float)dx + 0.5f) * invSize;
                float sx = (float)srcX + (float)srcW * u;

                uint8_t s8 = sampleBilinearIcons(sx, sy);
                float sd = ((float)s8 * (1.0f / 255.0f) - 0.5f) * drScale;
                float a = sd + 0.5f + weightBias;
                if (a <= 0.0f)
                    continue;
                if (a >= 1.0f)
                {
                    t->drawPixel(px, py, fg565);
                    continue;
                }

                uint8_t alpha = (uint8_t)(a * 255.0f + 0.5f);
                uint16_t out = detail::blend565(bg565, fg565, alpha);
                t->drawPixel(px, py, out);
            }
        }
    }

    void GUI::updateIcon(uint16_t iconId,
                         int16_t x,
                         int16_t y,
                         uint16_t sizePx,
                         uint32_t color,
                         uint32_t bgColor)
    {
        if (sizePx == 0)
            return;

        int16_t rx = x;
        int16_t ry = y;
        if (rx == -1)
            rx = AutoX((int32_t)sizePx);
        if (ry == -1)
            ry = AutoY((int32_t)sizePx);

        int16_t pad = 2;

        int16_t bx = (int16_t)(rx - pad);
        int16_t by = (int16_t)(ry - pad);
        int16_t bw = (int16_t)((int16_t)sizePx + pad * 2);
        int16_t bh = (int16_t)((int16_t)sizePx + pad * 2);

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        fillRect(bx, by, bw, bh, bgColor);
        drawIcon(iconId, rx, ry, sizePx, color, bgColor);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect(bx, by, bw, bh);
        flushDirty();
    }

}

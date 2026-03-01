#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/WixMadeForDisplay.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/metrics.hpp>
#include <pipGUI/fonts/KronaOne/KronaOne.hpp>
#include <pipGUI/fonts/KronaOne/metrics.hpp>

namespace pipgui
{

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
        &fontWixMadeForDisplay,
        &fontKronaOne,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    static uint8_t g_fontCount = 2;
    static const PSDFFontData *g_currentFont = g_fontRegistry[0];

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
        f->atlasData = atlasData;
        f->atlasWidth = atlasWidth;
        f->atlasHeight = atlasHeight;
        f->distanceRange = distanceRange;
        f->nominalSizePx = nominalSizePx;
        f->ascender = ascender;
        f->descender = descender;
        f->lineHeight = lineHeight;
        f->glyphs = glyphs;
        f->glyphCount = glyphCount;

        FontId newId = (FontId)g_fontCount;
        g_fontRegistry[g_fontCount] = f;
        g_fontCount++;
        return newId;
    }

    uint8_t GUI::getFontCount() const { return g_fontCount; }

    static inline uint32_t nextUtf8(const char *s, int &i, int len)
    {
        if (i >= len)
            return 0;
        uint8_t c0 = (uint8_t)s[i++];
        if ((c0 & 0x80U) == 0)
            return c0;
        if ((c0 & 0xE0U) == 0xC0U)
        {
            if (i >= len)
                return (uint32_t)'?';
            uint8_t c1 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x1FU) << 6) | (uint32_t)(c1 & 0x3FU);
        }
        if ((c0 & 0xF0U) == 0xE0U)
        {
            if (i + 1 >= len)
            {
                i = len;
                return (uint32_t)'?';
            }
            uint8_t c1 = (uint8_t)s[i++];
            uint8_t c2 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x0FU) << 12) | ((uint32_t)(c1 & 0x3FU) << 6) | (uint32_t)(c2 & 0x3FU);
        }
        if ((c0 & 0xF8U) == 0xF0U)
        {
            if (i + 2 >= len)
            {
                i = len;
                return (uint32_t)'?';
            }
            uint8_t c1 = (uint8_t)s[i++];
            uint8_t c2 = (uint8_t)s[i++];
            uint8_t c3 = (uint8_t)s[i++];
            return ((uint32_t)(c0 & 0x07U) << 18) | ((uint32_t)(c1 & 0x3FU) << 12) | ((uint32_t)(c2 & 0x3FU) << 6) | (uint32_t)(c3 & 0x3FU);
        }
        return (uint32_t)'?';
    }

    static inline float spaceAdvanceEm(const PSDFFontData *font)
    {
        const Glyph *glyphs = (const Glyph *)font->glyphs;
        int lo = 0, hi = (int)font->glyphCount - 1;
        while (lo <= hi)
        {
            int mid = (lo + hi) >> 1;
            uint32_t v = glyphs[mid].codepoint;
            if (v == (uint32_t)'n')
                return glyphs[mid].advance * 0.5f;
            if (v < (uint32_t)'n')
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return 0.30f;
    }

    static inline const Glyph *findGlyph(const PSDFFontData *font, uint32_t cp)
    {
        int lo = 0, hi = (int)font->glyphCount - 1;
        const Glyph *glyphs = (const Glyph *)font->glyphs;
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

    static inline uint8_t pgmAt(pipcore::GuiPlatform *plat, const PSDFFontData *font, int x, int y)
    {
        if ((uint32_t)x >= (uint32_t)font->atlasWidth ||
            (uint32_t)y >= (uint32_t)font->atlasHeight)
            return 0;
        return plat->readProgmemByte(&font->atlasData[(uint32_t)y * font->atlasWidth + x]);
    }

    static inline uint8_t sampleBilinear(pipcore::GuiPlatform *plat, const PSDFFontData *font, float u, float v)
    {
        int x0 = (int)u, y0 = (int)v;
        float fx = u - x0, fy = v - y0;
        float a0 = (float)pgmAt(plat, font, x0, y0) + ((float)pgmAt(plat, font, x0 + 1, y0) - (float)pgmAt(plat, font, x0, y0)) * fx;
        float a1 = (float)pgmAt(plat, font, x0, y0 + 1) + ((float)pgmAt(plat, font, x0 + 1, y0 + 1) - (float)pgmAt(plat, font, x0, y0 + 1)) * fx;
        int ia = (int)(a0 + (a1 - a0) * fy + 0.5f);
        if (ia < 0)
            ia = 0;
        if (ia > 255)
            ia = 255;
        return (uint8_t)ia;
    }

    void GUI::setPSDFFont(FontId fontId)
    {
        uint16_t id = (uint16_t)fontId;
        if (id < g_fontCount && g_fontRegistry[id])
        {
            g_currentFont = g_fontRegistry[id];
            _typo.psdfFontId = fontId;
        }
    }

    bool GUI::loadBuiltinPSDF()
    {
        _flags.psdfLoaded = 1;
        return true;
    }
    void GUI::enablePSDF(bool e) { _flags.psdfEnabled = e ? 1 : 0; }
    bool GUI::psdfFontLoaded() const { return _flags.psdfLoaded; }
    bool GUI::psdfEnabled() const { return _flags.psdfEnabled; }

    void GUI::configureTextStyles(uint16_t h1Px, uint16_t h2Px, uint16_t bodyPx, uint16_t captionPx)
    {
        _typo.h1Px = h1Px;
        _typo.h2Px = h2Px;
        _typo.bodyPx = bodyPx;
        _typo.captionPx = captionPx;
    }

    void GUI::setTextStyle(TextStyle style)
    {
        uint16_t px = _typo.bodyPx;
        switch (style)
        {
        case H1:
            px = _typo.h1Px;
            break;
        case H2:
            px = _typo.h2Px;
            break;
        case Caption:
            px = _typo.captionPx;
            break;
        default:
            break;
        }
        setPSDFFontSize(px);
    }

    void GUI::setPSDFFontSize(uint16_t px) { _typo.psdfSizePx = px; }
    FontId GUI::psdfFontId() const { return _typo.psdfFontId; }

    uint16_t GUI::psdfFontSize() const { return _typo.psdfSizePx; }

    static inline float weightToBias(uint16_t weight)
    {
        int w = (int)weight;
        if (w < 100)
            w = 100;
        if (w > 900)
            w = 900;
        float bias = (float)(w - 400) / 300.f * 0.25f;
        if (bias < -0.20f)
            bias = -0.20f;
        if (bias > 0.30f)
            bias = 0.30f;
        return bias;
    }

    void GUI::setPSDFWeight(uint16_t weight)
    {
        _typo.psdfWeight = weight;
        _typo.psdfWeightBias = weightToBias(weight);
    }
    uint16_t GUI::psdfWeight() const { return _typo.psdfWeight; }
    void GUI::setPSDFWeightBias(float bias) { _typo.psdfWeightBias = bias; }
    float GUI::psdfWeightBias() const { return _typo.psdfWeightBias; }

    template <typename Fn>
    static inline void forEachGlyph(const char *s, int len, const PSDFFontData *font, float sizePx, Fn &&callback)
    {
        const float spAdv = spaceAdvanceEm(font) * sizePx;
        int i = 0;
        float penX = 0.f;
        float penY = 0.f;
        float baseline = font->ascender * sizePx;

        while (i < len)
        {
            uint32_t cp = nextUtf8(s, i, len);
            if (cp == '\r')
                continue;
            if (cp == '\n')
            {
                callback(0, penX, penY + baseline, true);
                penX = 0.f;
                penY += font->lineHeight * sizePx;
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
            const Glyph *g = findGlyph(font, cp);
            if (!g)
                g = findGlyph(font, (uint32_t)'?');
            if (!g)
                continue;
            callback(g, penX, penY + baseline, false);
            penX += g->advance * sizePx;
        }
    }

    bool GUI::psdfMeasureText(const String &text, int16_t &outW, int16_t &outH) const
    {
        outW = outH = 0;
        if (_typo.psdfSizePx == 0)
            return false;

        const float sizePx = (float)_typo.psdfSizePx;
        uint16_t lines = 1;
        float maxX = 0.f, lastPenX = 0.f;
        const char *s = text.c_str();
        int len = (int)text.length();

        forEachGlyph(s, len, g_currentFont, sizePx,
                     [&](const Glyph *g, float penX, float, bool isNewline)
                     {
                         if (isNewline)
                         {
                             if (penX > maxX)
                                 maxX = penX;
                             ++lines;
                         }
                         else
                         {
                             lastPenX = penX + g->advance * sizePx;
                         }
                     });

        if (lastPenX > maxX)
            maxX = lastPenX;

        outW = (int16_t)(maxX + 0.5f);
        outH = (int16_t)((float)lines * g_currentFont->lineHeight * sizePx + 0.5f);
        return true;
    }

    void GUI::psdfDrawTextDirect(const String &text, int16_t rx, int16_t ry, int16_t tw, int16_t th,
                                 uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (_typo.psdfSizePx == 0 || tw <= 0 || th <= 0)
            return;

        const float sizePx = (float)_typo.psdfSizePx;
        const float baseline = (float)ry + g_currentFont->ascender * sizePx;
        const float drScale = g_currentFont->distanceRange * (sizePx / g_currentFont->nominalSizePx);
        const float weightBias = _typo.psdfWeightBias;
        const PSDFFontData *font = g_currentFont;
        const float spAdv = spaceAdvanceEm(font) * sizePx;

        pipcore::Sprite *spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        const char *s = text.c_str();
        int len = (int)text.length();
        int i = 0;
        float penX = (float)rx;
        float bl = baseline;

        while (i < len)
        {
            uint32_t cp = nextUtf8(s, i, len);
            if (cp == '\r')
                continue;
            if (cp == '\n')
            {
                penX = (float)rx;
                bl += font->lineHeight * sizePx;
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

            if ((int)penX >= stride)
            {
                penX += spAdv;
                continue;
            }

            const Glyph *g = findGlyph(font, cp);
            if (!g)
                g = findGlyph(font, (uint32_t)'?');
            if (!g)
                continue;

            float gx0 = penX + g->pl * sizePx;
            float gx1 = penX + g->pr * sizePx;
            float gy0 = bl - g->pt * sizePx;
            float gy1 = bl - g->pb * sizePx;

            int ix0 = (int)floorf(gx0), ix1 = (int)ceilf(gx1);
            int iy0 = (int)floorf(gy0), iy1 = (int)ceilf(gy1);

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

            float gw = gx1 - gx0, gh = gy1 - gy0;
            float invW = (gw != 0.f) ? 1.f / gw : 0.f;
            float invH = (gh != 0.f) ? 1.f / gh : 0.f;

            for (int py = iy0; py < iy1; ++py)
            {
                float v = ((float)py + 0.5f - gy0) * invH;
                float sy = g->ab + (g->at - g->ab) * v;
                int32_t row = (int32_t)py * stride;

                for (int px = ix0; px < ix1; ++px)
                {
                    float u = ((float)px + 0.5f - gx0) * invW;
                    float sx = g->al + (g->ar - g->al) * u;

                    uint8_t s8 = sampleBilinear(platform(), font, sx, sy);
                    float a = ((float)s8 * (1.f / 255.f) - 0.5f) * drScale + 0.5f + weightBias;

                    if (a <= 0.f)
                        continue;

                    int32_t idx = row + px;
                    if (a >= 1.f)
                    {
                        buf[idx] = pipcore::Sprite::swap16(fg565);
                    }
                    else
                    {
                        uint8_t alpha = (uint8_t)(a * 255.f + 0.5f);
                        uint16_t dst = pipcore::Sprite::swap16(buf[idx]);
                        buf[idx] = pipcore::Sprite::swap16(detail::blend565(dst, fg565, alpha));
                    }
                }
            }
            penX += g->advance * sizePx;
        }
    }

    void GUI::psdfDrawTextInternal(const String &text, int16_t x, int16_t y,
                                   uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        int16_t tw = 0, th = 0;
        if (!psdfMeasureText(text, tw, th) || tw <= 0 || th <= 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)tw) : x;
        if (align == AlignCenter)
            rx -= tw / 2;
        else if (align == AlignRight)
            rx -= tw;

        int16_t ry = (y == -1) ? AutoY((int32_t)th) : y;

        psdfDrawTextDirect(text, rx, ry, tw, th, fg565, bg565, align);
    }

    void GUI::drawPSDFText(const String &text, int16_t x, int16_t y,
                           uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.renderToSprite)
        {
            updatePSDFText(text, x, y, fg565, bg565, align);
            return;
        }
        psdfDrawTextInternal(text, x, y, fg565, bg565, align);
    }

    void GUI::updatePSDFText(const String &text, int16_t x, int16_t y,
                             uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (_typo.psdfSizePx == 0)
            return;

        const float sizePx = (float)_typo.psdfSizePx;
        const PSDFFontData *font = g_currentFont;

        const char *s = text.c_str();
        int len = (int)text.length();
        float maxX = 0.f, lastPenX = 0.f;
        float minX = 0.f, minY = 0.f, maxY = 0.f;
        uint16_t lines = 1;
        bool any = false;
        float baseline = font->ascender * sizePx;

        forEachGlyph(s, len, font, sizePx,
                     [&](const Glyph *g, float penX, float penY, bool isNewline)
                     {
                         if (isNewline)
                         {
                             if (penX > maxX)
                                 maxX = penX;
                             ++lines;
                             return;
                         }
                         float gx0 = penX + g->pl * sizePx;
                         float gx1 = penX + g->pr * sizePx;
                         float gy0 = penY - g->pt * sizePx;
                         float gy1 = penY - g->pb * sizePx;
                         lastPenX = penX + g->advance * sizePx;

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
                             if (gx1 > maxX)
                                 maxX = gx1;
                             if (gy0 < minY)
                                 minY = gy0;
                             if (gy1 > maxY)
                                 maxY = gy1;
                         }
                     });

        if (lastPenX > maxX)
            maxX = lastPenX;

        int16_t tw = (int16_t)(maxX + 0.5f);
        int16_t th = (int16_t)((float)lines * font->lineHeight * sizePx + 0.5f);

        if (tw <= 0 || th <= 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)tw) : x;
        if (align == AlignCenter)
            rx -= tw / 2;
        else if (align == AlignRight)
            rx -= tw;

        int16_t ry = (y == -1) ? AutoY((int32_t)th) : y;

        if (!any)
        {
            minX = 0;
            maxX = (float)tw;
            minY = 0;
            maxY = (float)th;
        }
        else
        {
            minX += (float)rx;
            maxX += (float)rx;
            minY += (float)ry - baseline + font->ascender * sizePx;
            maxY += (float)ry - baseline + font->ascender * sizePx;
        }

        const int16_t pad = 4;
        int16_t bx = (int16_t)floorf(minX) - pad;
        int16_t by = (int16_t)floorf(minY) - pad;
        int16_t bw = (int16_t)(ceilf(maxX) - floorf(minX)) + pad * 2;
        int16_t bh = (int16_t)(ceilf(maxY) - floorf(minY)) + pad * 2;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.renderToSprite = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().at(bx, by).size(bw, bh).color(bg565).draw();

        psdfDrawTextDirect(text, rx, ry, tw, th, fg565, bg565, align);

        _flags.renderToSprite = prevRender;
        _render.activeSprite = prevActive;

        invalidateRect(bx, by, bw, bh);
        flushDirty();
    }

    void DrawTextFluent::draw()
    {
        if (_consumed || !_gui || _text.length() == 0)
            return;
        _consumed = true;

        FontId prevFont = _gui->psdfFontId();
        uint16_t prevSize = _gui->psdfFontSize();

        if (_fontId != (FontId)0)
            _gui->setPSDFFont(_fontId);

        if (_sizePx != 0)
            _gui->setPSDFFontSize(_sizePx);

        _gui->drawPSDFText(_text, _x, _y, _fg565, _bg565, _align);
        _gui->setPSDFFont(prevFont);
        _gui->setPSDFFontSize(prevSize);
    }

}
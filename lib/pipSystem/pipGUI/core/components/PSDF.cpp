#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/WixMadeForDisplay.hpp>

#include <pipGUI/fonts/WixMadeForDisplay/metrics.hpp>

namespace pipgui
{

    static inline uint16_t to565(uint32_t c)
    {
        uint8_t r = (uint8_t)((c >> 16) & 0xFF);
        uint8_t g = (uint8_t)((c >> 8) & 0xFF);
        uint8_t b = (uint8_t)(c & 0xFF);
        return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
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

    static inline const psdf::Glyph *findGlyph(uint32_t cp)
    {
        int lo = 0;
        int hi = (int)psdf::GlyphCount - 1;
        while (lo <= hi)
        {
            int mid = (lo + hi) >> 1;
            uint32_t v = psdf::Glyphs[mid].codepoint;
            if (v == cp)
                return &psdf::Glyphs[mid];
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
        const psdf::Glyph *g = findGlyph((uint32_t)'n');
        s_adv = g ? (g->advance * 0.5f) : 0.30f;
        return s_adv;
    }

    static inline uint8_t pgmAt(int x, int y)
    {
        if ((uint32_t)x >= (uint32_t)psdf::AtlasWidth || (uint32_t)y >= (uint32_t)psdf::AtlasHeight)
            return 0;
        uint32_t idx = (uint32_t)y * (uint32_t)psdf::AtlasWidth + (uint32_t)x;
        return (uint8_t)pgm_read_byte(&WixMadeForDisplay[idx]);
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

            const psdf::Glyph *g = findGlyph(cp);
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

        float h = (float)lines * psdf::LineHeight * scale;
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
        float baseline = (float)ry + psdf::Ascender * sizePx;
        float drScale = psdf::DistanceRange * (sizePx / psdf::NominalSizePx);
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

            auto swap16 = [](uint16_t v) -> uint16_t
            { return (uint16_t)((v >> 8) | (v << 8)); };
            
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
                    baseline += psdf::LineHeight * sizePx;
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

                const psdf::Glyph *g = findGlyph(cp);
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

                float syTop = (float)psdf::AtlasHeight - g->at;
                float syBot = (float)psdf::AtlasHeight - g->ab;

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
                baseline += psdf::LineHeight * sizePx;
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

            const psdf::Glyph *g = findGlyph(cp);
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

            float syTop = (float)psdf::AtlasHeight - g->at;
            float syBot = (float)psdf::AtlasHeight - g->ab;

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

        psdfDrawTextInternal(text, x, y, to565(color), to565(bgColor), align);
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
        float baseline = (float)ry + psdf::Ascender * sizePx;

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
                    baselineLocal += psdf::LineHeight * sizePx;
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

                const psdf::Glyph *g = findGlyph(cp);
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

        uint16_t bg = to565(bgColor);
        _sprite.fillRect(bx, by, bw, bh, bg);

        psdfDrawTextInternal(text, rx, ry, to565(color), bg, align);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect(bx, by, bw, bh);
        flushDirty();
    }

}

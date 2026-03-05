#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipGUI/core/Render/Draw/Blend.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/WixMadeForDisplay.hpp>
#include <pipGUI/fonts/WixMadeForDisplay/metrics.hpp>
#include <pipGUI/fonts/KronaOne/KronaOne.hpp>
#include <pipGUI/fonts/KronaOne/metrics.hpp>

namespace pipgui
{
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
        
        __attribute__((always_inline)) float unpackAdvance() const {
            return advance * (1.0f / 256.0f);
        }
        __attribute__((always_inline)) float unpackPadLeft() const {
            return padLeft * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadBottom() const {
            return padBottom * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadRight() const {
            return padRight * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackPadTop() const {
            return padTop * (1.0f / 128.0f);
        }
        __attribute__((always_inline)) float unpackAtlasLeft() const {
            return (float)atlasLeft;
        }
        __attribute__((always_inline)) float unpackAtlasBottom() const {
            return (float)atlasBottom;
        }
        __attribute__((always_inline)) float unpackAtlasRight() const {
            return (float)atlasRight;
        }
        __attribute__((always_inline)) float unpackAtlasTop() const {
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

    static inline uint8_t sampleBilinear(pipcore::GuiPlatform *plat,
                                         const PSDFFontData *font,
                                         float u, float v)
    {
        const int32_t u16 = (int32_t)(u * 65536.f);
        const int32_t v16 = (int32_t)(v * 65536.f);
        const int aw = (int)font->atlasWidth, ah = (int)font->atlasHeight;
        auto rd = [&](int x, int y) -> int32_t
        {
            if ((unsigned)x >= (unsigned)aw || (unsigned)y >= (unsigned)ah)
                return 0;
            return (int32_t)plat->readProgmemByte(&font->atlasData[(uint32_t)y * aw + x]);
        };
        return sampleBilinearSDF(rd, u16, v16, aw, ah);
    }

    bool GUI::setFont(FontId fontId)
    {
        uint16_t id = (uint16_t)fontId;
        if (id < g_fontCount && g_fontRegistry[id])
        {
            g_currentFont = g_fontRegistry[id];
            return true;
        }
        return false;
    }

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
    uint16_t GUI::fontSize() const { return _typo.psdfSizePx; }

    void GUI::setFontWeight(uint16_t weight)
    {
        _typo.psdfWeight = weight;
        int w = (int)weight;
        if (w < 100)
            w = 100;
        else if (w > 900)
            w = 900;
        float bias = (float)(w - 400) / 300.f * 0.25f;
        if (bias < -0.20f)
            bias = -0.20f;
        else if (bias > 0.30f)
            bias = 0.30f;
        _typo.psdfWeightBias = bias;
    }
    uint16_t GUI::fontWeight() const { return _typo.psdfWeight; }

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
        if (!_typo.psdfSizePx || !g_currentFont)
            return false;
        int len = (int)text.length();
        if (!len)
            return true;

        const float sizePx = (float)_typo.psdfSizePx;
        uint16_t lines = 1;
        float maxX = 0.f, curX = 0.f;

        forEachGlyph(text.c_str(), len, g_currentFont, sizePx,
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
        outH = (int16_t)ceilf((float)lines * g_currentFont->lineHeight * sizePx);
        return true;
    }

    static inline uint8_t sampleBilinear(pipcore::GuiPlatform *plat,
                                         const PSDFFontData *font,
                                         int32_t u16, int32_t v16)
    {
        const int aw = (int)font->atlasWidth, ah = (int)font->atlasHeight;
        auto rd = [&](int x, int y) -> int32_t
        {
            if ((unsigned)x >= (unsigned)aw || (unsigned)y >= (unsigned)ah)
                return 0;
            return (int32_t)plat->readProgmemByte(&font->atlasData[(uint32_t)y * aw + x]);
        };
        return sampleBilinearSDF(rd, u16, v16, aw, ah);
    }

    void GUI::drawTextImmediate(const String &text, int16_t rx, int16_t ry,
                                int16_t, int16_t,
                                uint16_t fg565, uint16_t, TextAlign)
    {
        if (!_typo.psdfSizePx)
            return;

        pipcore::Sprite *spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width(), maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        const PSDFFontData *font = g_currentFont;
        const float sizePx = (float)_typo.psdfSizePx;
        const float distanceScale = font->distanceRange * (sizePx / font->nominalSizePx);
        const float biasOffset = 0.5f + _typo.psdfWeightBias;
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = biasOffset - distanceScale * 0.5f;
        const uint16_t fgNative = pipcore::Sprite::swap16(fg565);

        float dthr = (0.f - kOffset) / kScale;
        uint8_t s8Min = (dthr > 0.f && dthr < 255.f) ? (uint8_t)dthr : 0;

        pipcore::GuiPlatform *plat = platform();

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

                         if (ix1 <= 0 || iy1 <= 0 || ix0 >= stride || iy0 >= maxH)
                             return true;
                         if (ix0 < 0)
                             ix0 = 0;
                         if (iy0 < 0)
                             iy0 = 0;
                         if (ix1 > stride)
                             ix1 = stride;
                         if (iy1 > maxH)
                             iy1 = maxH;
                         if (ix0 >= ix1 || iy0 >= iy1)
                             return true;

                         const float gx0 = absPenX + g->unpackPadLeft() * sizePx;
                         const float gy0 = absPenY - g->unpackPadTop() * sizePx;
                         const float gw = (absPenX + g->unpackPadRight() * sizePx) - gx0;
                         const float gh = (absPenY - g->unpackPadBottom() * sizePx) - gy0;
                         const float invW = (gw != 0.f) ? 1.f / gw : 0.f;
                         const float invH = (gh != 0.f) ? 1.f / gh : 0.f;

                         const float atlasW = g->unpackAtlasRight() - g->unpackAtlasLeft();
                         const float atlasH = g->unpackAtlasTop() - g->unpackAtlasBottom();

                         const int32_t atlasDu = (int32_t)(atlasW * invW * 65536.f);
                         const int32_t atlasDv = (int32_t)(atlasH * invH * 65536.f);
                         const int32_t atlasU0 = (int32_t)((g->unpackAtlasLeft() + atlasW * ((float)ix0 + 0.5f - gx0) * invW) * 65536.f);
                         const int32_t atlasV0 = (int32_t)((g->unpackAtlasBottom() + atlasH * ((float)iy0 + 0.5f - gy0) * invH) * 65536.f);

                         int32_t atlasV = atlasV0;
                         for (int py = iy0; py < iy1; ++py, atlasV += atlasDv)
                         {
                             const int32_t row = (int32_t)py * stride;
                             int32_t atlasU = atlasU0;

                             for (int px = ix0; px < ix1; ++px, atlasU += atlasDu)
                             {
                                 uint8_t s8 = sampleBilinear(plat, font, atlasU, atlasV);
                                 if (s8 <= s8Min)
                                     continue;

                                 const float a = (float)s8 * kScale + kOffset;
                                 const int32_t idx = row + px;
                                 if (a >= 1.f)
                                 {
                                     buf[idx] = fgNative;
                                 }
                                 else
                                 {
                                     const uint8_t alpha = (uint8_t)(a * 255.f + 0.5f);
                                     const uint16_t dst = pipcore::Sprite::swap16(buf[idx]);
                                     buf[idx] = pipcore::Sprite::swap16(detail::blend565(dst, fg565, alpha));
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
        drawTextImmediate(text, rx, ry, tw, th, fg565, bg565, align);
    }

    void GUI::drawText(const String &text, int16_t x, int16_t y,
                       uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (!_flags.spriteEnabled || _flags.renderToSprite || !_disp.display)
            return drawTextAligned(text, x, y, fg565, bg565, align);
        updateText(text, x, y, fg565, bg565, align);
    }

    void GUI::updateText(const String &text, int16_t x, int16_t y,
                         uint16_t fg565, uint16_t bg565, TextAlign align)
    {
        if (!_typo.psdfSizePx || !g_currentFont)
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

        constexpr int16_t pad = 4;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.renderToSprite = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().at(rx - pad, ry - pad).size(tw + pad * 2, th + pad * 2).color(bg565).draw();
        drawTextImmediate(text, rx, ry, tw, th, fg565, bg565, align);

        _flags.renderToSprite = prevRender;
        _render.activeSprite = prevActive;

        invalidateRect(rx - pad, ry - pad, tw + pad * 2, th + pad * 2);
        flushDirty();
    }

    void DrawTextFluent::draw()
    {
        if (_consumed || !_gui || _text.length() == 0)
            return;
        _consumed = true;

        struct FontGuard
        {
            GUI *gui;
            FontId prevId;
            uint16_t prevSz;
            bool hasPrev;
            FontGuard(GUI *g) : gui(g), prevSz(g->fontSize()), hasPrev(false)
            {
                for (uint16_t i = 0; i < g_fontCount; ++i)
                    if (g_fontRegistry[i] == g_currentFont)
                    {
                        prevId = (FontId)i;
                        hasPrev = true;
                        break;
                    }
            }
            ~FontGuard()
            {
                if (hasPrev)
                    gui->setFont(prevId);
                gui->setFontSize(prevSz);
            }
        } guard(_gui);

        if (_fontId != (FontId)0 && !_gui->setFont(_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        _gui->drawText(_text, _x, _y, _fg565, _bg565, _align);
    }
}
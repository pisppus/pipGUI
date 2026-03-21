#include "Internal.hpp"

namespace pipgui
{
    namespace
    {
        inline constexpr uint16_t kStyleH1Px = 24;
        inline constexpr uint16_t kStyleH2Px = 18;
        inline constexpr uint16_t kStyleBodyPx = 14;
        inline constexpr uint16_t kStyleCaptionPx = 12;
    }

    static const FontData fontWixMadeForDisplay = {
        ::WixMadeForDisplay,
        psdf_wixfor::AtlasWidth, psdf_wixfor::AtlasHeight,
        psdf_wixfor::DistanceRange, psdf_wixfor::NominalSizePx,
        psdf_wixfor::Ascender, psdf_wixfor::Descender,
        psdf_wixfor::LineHeight,
        psdf_wixfor::Glyphs, psdf_wixfor::GlyphCount,
        psdf_wixfor::KerningPairs, psdf_wixfor::KerningPairCount,
        psdf_wixfor::Tracking128};

    static const FontData fontKronaOne = {
        ::KronaOne,
        psdf_krona::AtlasWidth, psdf_krona::AtlasHeight,
        psdf_krona::DistanceRange, psdf_krona::NominalSizePx,
        psdf_krona::Ascender, psdf_krona::Descender,
        psdf_krona::LineHeight,
        psdf_krona::Glyphs, psdf_krona::GlyphCount,
        psdf_krona::KerningPairs, psdf_krona::KerningPairCount,
        psdf_krona::Tracking128};

    static const FontData *g_fontRegistry[8] = {
        &fontWixMadeForDisplay, &fontKronaOne,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    static uint8_t g_fontCount = 2;

    const FontData *fontDataForId(FontId fontId)
    {
        const uint16_t id = (uint16_t)fontId;
        return (id < g_fontCount) ? g_fontRegistry[id] : nullptr;
    }

    FontId GUI::registerFont(const uint8_t *atlasData,
                             uint16_t atlasWidth, uint16_t atlasHeight,
                             float distanceRange, float nominalSizePx,
                             float ascender, float descender, float lineHeight,
                             const void *glyphs, uint16_t glyphCount,
                             const void *kerningPairs, uint16_t kerningPairCount,
                             int8_t tracking128)
    {
        if (g_fontCount >= 8)
            return (FontId)255;

        static FontData registry[8];
        FontData *font = &registry[g_fontCount - 2];
        *font = {atlasData, atlasWidth, atlasHeight, distanceRange, nominalSizePx,
                 ascender, descender, lineHeight, glyphs, glyphCount,
                 kerningPairs, kerningPairCount, tracking128};

        const FontId newId = (FontId)g_fontCount;
        g_fontRegistry[g_fontCount++] = font;
        return newId;
    }

    bool GUI::setFont(FontId fontId)
    {
        if (!fontDataForId(fontId))
            return false;
        _typo.currentFontId = fontId;
        return true;
    }

    FontId GUI::fontId() const noexcept { return _typo.currentFontId; }

    void GUI::setTextStyle(TextStyle style)
    {
        uint16_t px = kStyleBodyPx;
        if (style == H1)
            px = kStyleH1Px;
        else if (style == H2)
            px = kStyleH2Px;
        else if (style == Caption)
            px = kStyleCaptionPx;
        setFontSize(px);
    }

    void GUI::setFontSize(uint16_t px) { _typo.psdfSizePx = px; }
    uint16_t GUI::fontSize() const noexcept { return _typo.psdfSizePx; }

    void GUI::setFontWeight(uint16_t weight)
    {
        _typo.psdfWeight = clampFontWeight(weight);
    }

    uint16_t GUI::fontWeight() const noexcept { return _typo.psdfWeight; }
}

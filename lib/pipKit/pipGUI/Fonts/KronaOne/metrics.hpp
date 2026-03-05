#pragma once
#include <cstdint>

namespace pipgui
{
namespace psdf_krona
{
static constexpr uint16_t AtlasWidth = 100;
static constexpr uint16_t AtlasHeight = 100;
static constexpr float DistanceRange = 8.0f;
static constexpr float NominalSizePx = 48.0f;
static constexpr float Ascender = 0.991210938f;
static constexpr float Descender = -0.258789062f;
static constexpr float LineHeight = 1.25f;

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
};

static constexpr uint16_t GlyphCount = 4;

static const Glyph Glyphs[GlyphCount] =
{
    {73u, 85, 0, -12, 43, 111, 1, 48, 17, 94},
    {80u, 234, 3, -12, 120, 111, 52, 47, 96, 93},
    {83u, 256, -4, -15, 127, 111, 1, 1, 52, 48},
    {85u, 234, -1, -12, 119, 111, 52, 1, 97, 47},
};

}
}

#include <pipGUI/core/api/pipGUI.hpp>
static inline pipgui::FontId registerFont_KronaOne(pipgui::GUI &gui){
    return gui.registerFont(
        ::KronaOne,
        pipgui::psdf_krona::AtlasWidth,
        pipgui::psdf_krona::AtlasHeight,
        pipgui::psdf_krona::DistanceRange,
        pipgui::psdf_krona::NominalSizePx,
        pipgui::psdf_krona::Ascender,
        pipgui::psdf_krona::Descender,
        pipgui::psdf_krona::LineHeight,
        pipgui::psdf_krona::Glyphs,
        pipgui::psdf_krona::GlyphCount);
}

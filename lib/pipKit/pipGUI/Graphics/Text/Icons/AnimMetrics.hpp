#pragma once
#include <cstdint>

extern const uint8_t animIcons[];

namespace pipgui
{
namespace psdf_anim
{
inline constexpr uint16_t AtlasWidth = 48;
inline constexpr uint16_t AtlasHeight = 48;
inline constexpr float DistanceRange = 8.0f;
inline constexpr float NominalSizePx = 48.0f;
inline constexpr int16_t TransformScale = 64;

enum AnimatedIconId : uint16_t
{
    AnimatedSetting = 0
};

struct Glyph { uint16_t x; uint16_t y; uint16_t w; uint16_t h; };
struct FrameTransform { int16_t posX; int16_t posY; int16_t anchorX; int16_t anchorY; int16_t scaleX; int16_t scaleY; int16_t rotation; uint8_t opacity; };
struct Layer { uint16_t glyphIndex; uint16_t frameOffset; };
struct AnimatedIcon { uint16_t width; uint16_t height; uint16_t frameRateX100; uint16_t frameCount; uint16_t firstLayer; uint8_t layerCount; };

inline constexpr uint16_t GlyphCount = 1;
inline constexpr Glyph Glyphs[GlyphCount] =
{
    {0u, 0u, 48u, 48u}
};

inline constexpr FrameTransform Frames[] =
{
    {768, 768, 768, 768, 6400, 6400, 0, 255u},
    {768, 768, 768, 768, 6400, 6400, 411, 255u},
    {768, 768, 768, 768, 6400, 6400, 823, 255u},
    {768, 768, 768, 768, 6400, 6400, 1234, 255u},
    {768, 768, 768, 768, 6400, 6400, 1646, 255u},
    {768, 768, 768, 768, 6400, 6400, 2057, 255u},
    {768, 768, 768, 768, 6400, 6400, 2469, 255u},
    {768, 768, 768, 768, 6400, 6400, 2880, 255u},
    {768, 768, 768, 768, 6400, 6400, 3291, 255u},
    {768, 768, 768, 768, 6400, 6400, 3703, 255u},
    {768, 768, 768, 768, 6400, 6400, 4114, 255u},
    {768, 768, 768, 768, 6400, 6400, 4526, 255u},
    {768, 768, 768, 768, 6400, 6400, 4937, 255u},
    {768, 768, 768, 768, 6400, 6400, 5349, 255u},
    {768, 768, 768, 768, 6400, 6400, 5760, 255u},
    {768, 768, 768, 768, 6400, 6400, 6171, 255u},
    {768, 768, 768, 768, 6400, 6400, 6583, 255u},
    {768, 768, 768, 768, 6400, 6400, 6994, 255u},
    {768, 768, 768, 768, 6400, 6400, 7406, 255u},
    {768, 768, 768, 768, 6400, 6400, 7817, 255u},
    {768, 768, 768, 768, 6400, 6400, 8229, 255u},
    {768, 768, 768, 768, 6400, 6400, 8640, 255u},
    {768, 768, 768, 768, 6400, 6400, 9051, 255u},
    {768, 768, 768, 768, 6400, 6400, 9463, 255u},
    {768, 768, 768, 768, 6400, 6400, 9874, 255u},
    {768, 768, 768, 768, 6400, 6400, 10286, 255u},
    {768, 768, 768, 768, 6400, 6400, 10697, 255u},
    {768, 768, 768, 768, 6400, 6400, 11109, 255u},
    {768, 768, 768, 768, 6400, 6400, 11520, 255u}
};

inline constexpr Layer Layers[] =
{
    {0u, 0u}
};

inline constexpr uint16_t AnimatedIconCount = 1;
inline constexpr AnimatedIcon Icons[AnimatedIconCount] =
{
    {24u, 24u, 2400u, 28u, 0u, 1u}
};
}
}

namespace pipgui
{
using AnimatedIconId = ::pipgui::psdf_anim::AnimatedIconId;
inline constexpr AnimatedIconId AnimatedSetting = ::pipgui::psdf_anim::AnimatedSetting;
}

inline constexpr pipgui::AnimatedIconId setting_anim = ::pipgui::psdf_anim::AnimatedSetting;

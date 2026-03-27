#pragma once
#include <cstdint>

extern const uint8_t KronaOne[];

namespace pipgui
{
namespace psdf_krona
{
inline constexpr uint16_t AtlasWidth = 100;
inline constexpr uint16_t AtlasHeight = 100;
inline constexpr float DistanceRange = 8.0f;
inline constexpr float NominalSizePx = 48.0f;
inline constexpr float Ascender = 0.991210938f;
inline constexpr float Descender = -0.258789062f;
inline constexpr float LineHeight = 1.25f;

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

struct KerningPair
{
    uint32_t left;
    uint32_t right;
    int16_t adjust;
};

inline constexpr uint16_t GlyphCount = 4;

inline constexpr Glyph Glyphs[GlyphCount] =
{
    {73u, 85, 0, -12, 43, 111, 1, 48, 17, 94},
    {80u, 234, 3, -12, 120, 111, 52, 47, 96, 93},
    {83u, 256, -4, -15, 127, 111, 1, 1, 52, 48},
    {85u, 234, -1, -12, 119, 111, 52, 1, 97, 47},
};

inline constexpr int8_t Tracking128 = 1;

inline constexpr uint16_t KerningPairCount = 0;

inline constexpr const KerningPair *KerningPairs = nullptr;

}
}

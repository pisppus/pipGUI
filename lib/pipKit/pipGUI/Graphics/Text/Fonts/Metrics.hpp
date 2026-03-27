#pragma once
#include <cstdint>

namespace pipgui
{
namespace psdf_fonts
{
enum FontId : uint16_t
{
    FontWixMadeForDisplay = 0,
    FontKronaOne = 1
};

inline constexpr uint16_t FontCount = 2;
}
}

namespace pipgui
{
using FontId = ::pipgui::psdf_fonts::FontId;
inline constexpr FontId FontWixMadeForDisplay = ::pipgui::psdf_fonts::FontWixMadeForDisplay;
inline constexpr FontId FontKronaOne = ::pipgui::psdf_fonts::FontKronaOne;
}

inline constexpr pipgui::FontId WixMadeForDisplay = ::pipgui::psdf_fonts::FontWixMadeForDisplay;
inline constexpr pipgui::FontId KronaOne = ::pipgui::psdf_fonts::FontKronaOne;

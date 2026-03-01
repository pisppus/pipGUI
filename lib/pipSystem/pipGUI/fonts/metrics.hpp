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

static constexpr uint16_t FontCount = 2;
}
}

namespace pipgui
{
using FontId = ::pipgui::psdf_fonts::FontId;
static constexpr FontId FontWixMadeForDisplay = ::pipgui::psdf_fonts::FontWixMadeForDisplay;
static constexpr FontId FontKronaOne = ::pipgui::psdf_fonts::FontKronaOne;
}

constexpr pipgui::FontId WixMadeForDisplay = ::pipgui::psdf_fonts::FontWixMadeForDisplay;
constexpr pipgui::FontId KronaOne = ::pipgui::psdf_fonts::FontKronaOne;

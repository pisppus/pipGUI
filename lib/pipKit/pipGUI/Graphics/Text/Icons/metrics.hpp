#pragma once
#include <cstdint>

extern const uint8_t icons[];

namespace pipgui
{
namespace psdf_icons
{
static constexpr uint16_t AtlasWidth = 48;
static constexpr uint16_t AtlasHeight = 336;
static constexpr float DistanceRange = 8.0f;
static constexpr float NominalSizePx = 48.0f;

enum IconId : uint16_t
{
    IconArrow = 0,
    IconBatteryL0 = 1,
    IconBatteryL1 = 2,
    IconBatteryL2 = 3,
    IconCheckmark = 4,
    IconError = 5,
    IconWarning = 6
};

struct Icon
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

static constexpr uint16_t IconCount = 7;

static constexpr Icon Icons[IconCount] =
{
    {0u, 0u, 48u, 48u},
    {0u, 48u, 48u, 48u},
    {0u, 96u, 48u, 48u},
    {0u, 144u, 48u, 48u},
    {0u, 192u, 48u, 48u},
    {0u, 240u, 48u, 48u},
    {0u, 288u, 48u, 48u}
};

}
}

namespace pipgui
{
using IconId = ::pipgui::psdf_icons::IconId;
static constexpr IconId IconArrow = ::pipgui::psdf_icons::IconArrow;
static constexpr IconId IconBatteryL0 = ::pipgui::psdf_icons::IconBatteryL0;
static constexpr IconId IconBatteryL1 = ::pipgui::psdf_icons::IconBatteryL1;
static constexpr IconId IconBatteryL2 = ::pipgui::psdf_icons::IconBatteryL2;
static constexpr IconId IconCheckmark = ::pipgui::psdf_icons::IconCheckmark;
static constexpr IconId IconError = ::pipgui::psdf_icons::IconError;
static constexpr IconId IconWarning = ::pipgui::psdf_icons::IconWarning;
}

constexpr pipgui::IconId arrow = ::pipgui::psdf_icons::IconArrow;
constexpr pipgui::IconId battery_l0 = ::pipgui::psdf_icons::IconBatteryL0;
constexpr pipgui::IconId battery_l1 = ::pipgui::psdf_icons::IconBatteryL1;
constexpr pipgui::IconId battery_l2 = ::pipgui::psdf_icons::IconBatteryL2;
constexpr pipgui::IconId checkmark = ::pipgui::psdf_icons::IconCheckmark;
constexpr pipgui::IconId error = ::pipgui::psdf_icons::IconError;
constexpr pipgui::IconId warning = ::pipgui::psdf_icons::IconWarning;

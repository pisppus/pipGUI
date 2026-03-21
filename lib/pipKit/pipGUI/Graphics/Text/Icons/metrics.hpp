#pragma once
#include <cstdint>

extern const uint8_t icons[];

namespace pipgui
{
namespace psdf_icons
{
static constexpr uint16_t AtlasWidth = 48;
static constexpr uint16_t AtlasHeight = 240;
static constexpr float DistanceRange = 8.0f;
static constexpr float NominalSizePx = 48.0f;

enum IconId : uint16_t
{
    IconBatteryL0 = 0,
    IconBatteryL1 = 1,
    IconBatteryL2 = 2,
    IconError = 3,
    IconWarning = 4
};

struct Icon
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

static constexpr uint16_t IconCount = 5;

static constexpr Icon Icons[IconCount] =
{
    {0u, 0u, 48u, 48u},
    {0u, 48u, 48u, 48u},
    {0u, 96u, 48u, 48u},
    {0u, 144u, 48u, 48u},
    {0u, 192u, 48u, 48u}
};

}
}

namespace pipgui
{
using IconId = ::pipgui::psdf_icons::IconId;
static constexpr IconId IconBatteryL0 = ::pipgui::psdf_icons::IconBatteryL0;
static constexpr IconId IconBatteryL1 = ::pipgui::psdf_icons::IconBatteryL1;
static constexpr IconId IconBatteryL2 = ::pipgui::psdf_icons::IconBatteryL2;
static constexpr IconId IconError = ::pipgui::psdf_icons::IconError;
static constexpr IconId IconWarning = ::pipgui::psdf_icons::IconWarning;
}

constexpr pipgui::IconId battery_l0 = ::pipgui::psdf_icons::IconBatteryL0;
constexpr pipgui::IconId battery_l1 = ::pipgui::psdf_icons::IconBatteryL1;
constexpr pipgui::IconId battery_l2 = ::pipgui::psdf_icons::IconBatteryL2;
constexpr pipgui::IconId error = ::pipgui::psdf_icons::IconError;
constexpr pipgui::IconId warning = ::pipgui::psdf_icons::IconWarning;

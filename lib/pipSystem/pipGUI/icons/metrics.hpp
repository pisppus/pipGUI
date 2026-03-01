#pragma once
#include <cstdint>

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
    IconBatteryLayer0 = 0,
    IconBatteryLayer1 = 1,
    IconBatteryLayer2 = 2,
    IconErrorLayer0 = 3,
    IconWarningLayer0 = 4
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
static constexpr IconId IconBatteryLayer0 = ::pipgui::psdf_icons::IconBatteryLayer0;
static constexpr IconId IconBatteryLayer1 = ::pipgui::psdf_icons::IconBatteryLayer1;
static constexpr IconId IconBatteryLayer2 = ::pipgui::psdf_icons::IconBatteryLayer2;
static constexpr IconId IconErrorLayer0 = ::pipgui::psdf_icons::IconErrorLayer0;
static constexpr IconId IconWarningLayer0 = ::pipgui::psdf_icons::IconWarningLayer0;
}

constexpr pipgui::IconId BatteryLayer0 = ::pipgui::psdf_icons::IconBatteryLayer0;
constexpr pipgui::IconId battery_layer0 = ::pipgui::psdf_icons::IconBatteryLayer0;
constexpr pipgui::IconId BatteryLayer1 = ::pipgui::psdf_icons::IconBatteryLayer1;
constexpr pipgui::IconId battery_layer1 = ::pipgui::psdf_icons::IconBatteryLayer1;
constexpr pipgui::IconId BatteryLayer2 = ::pipgui::psdf_icons::IconBatteryLayer2;
constexpr pipgui::IconId battery_layer2 = ::pipgui::psdf_icons::IconBatteryLayer2;
constexpr pipgui::IconId ErrorLayer0 = ::pipgui::psdf_icons::IconErrorLayer0;
constexpr pipgui::IconId error_layer0 = ::pipgui::psdf_icons::IconErrorLayer0;
constexpr pipgui::IconId WarningLayer0 = ::pipgui::psdf_icons::IconWarningLayer0;
constexpr pipgui::IconId warning_layer0 = ::pipgui::psdf_icons::IconWarningLayer0;

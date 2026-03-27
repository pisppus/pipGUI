#pragma once
#include <cstdint>

extern const uint8_t icons[];

namespace pipgui
{
namespace psdf_icons
{
inline constexpr uint16_t AtlasWidth = 48;
inline constexpr uint16_t AtlasHeight = 336;
inline constexpr float DistanceRange = 8.0f;
inline constexpr float NominalSizePx = 48.0f;

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

inline constexpr uint16_t IconCount = 7;

inline constexpr Icon Icons[IconCount] =
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
inline constexpr IconId IconArrow = ::pipgui::psdf_icons::IconArrow;
inline constexpr IconId IconBatteryL0 = ::pipgui::psdf_icons::IconBatteryL0;
inline constexpr IconId IconBatteryL1 = ::pipgui::psdf_icons::IconBatteryL1;
inline constexpr IconId IconBatteryL2 = ::pipgui::psdf_icons::IconBatteryL2;
inline constexpr IconId IconCheckmark = ::pipgui::psdf_icons::IconCheckmark;
inline constexpr IconId IconError = ::pipgui::psdf_icons::IconError;
inline constexpr IconId IconWarning = ::pipgui::psdf_icons::IconWarning;
}

inline constexpr pipgui::IconId arrow = ::pipgui::psdf_icons::IconArrow;
inline constexpr pipgui::IconId battery_l0 = ::pipgui::psdf_icons::IconBatteryL0;
inline constexpr pipgui::IconId battery_l1 = ::pipgui::psdf_icons::IconBatteryL1;
inline constexpr pipgui::IconId battery_l2 = ::pipgui::psdf_icons::IconBatteryL2;
inline constexpr pipgui::IconId checkmark = ::pipgui::psdf_icons::IconCheckmark;
inline constexpr pipgui::IconId error = ::pipgui::psdf_icons::IconError;
inline constexpr pipgui::IconId warning = ::pipgui::psdf_icons::IconWarning;

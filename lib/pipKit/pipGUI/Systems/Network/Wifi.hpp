#pragma once

#include <pipGUI/Core/Common.hpp>
#include <pipCore/Network/Wifi.hpp>

namespace pipgui::net
{
    using WifiState = pipcore::net::WifiState;

    void wifiRequest(bool enabled) noexcept;
    void wifiService() noexcept;

    [[nodiscard]] WifiState wifiState() noexcept;
    [[nodiscard]] bool wifiConnected() noexcept;
    [[nodiscard]] uint32_t wifiLocalIpV4() noexcept;
}

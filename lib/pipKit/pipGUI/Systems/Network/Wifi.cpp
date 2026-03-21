#include <pipGUI/Core/Config/Select.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>

namespace pipgui::net
{
    namespace
    {
        inline void configureOnce() noexcept
        {
            static bool configured = false;
            if (configured)
                return;
            pipcore::net::WifiConfig cfg;
            cfg.ssid = PIPGUI_WIFI_SSID;
            cfg.password = PIPGUI_WIFI_PASSWORD;
            pipcore::net::wifiConfigure(cfg);
            configured = true;
        }
    }

    void wifiRequest(bool enabled) noexcept
    {
        if (enabled)
            configureOnce();
        pipcore::net::wifiRequest(enabled);
    }

    void wifiService() noexcept
    {
#if PIPGUI_WIFI
        configureOnce();
#endif
        pipcore::net::wifiService();
    }

    WifiState wifiState() noexcept
    {
        return pipcore::net::wifiState();
    }

    bool wifiConnected() noexcept
    {
        return pipcore::net::wifiConnected();
    }

    uint32_t wifiLocalIpV4() noexcept
    {
        return pipcore::net::wifiLocalIpV4();
    }
}

#include <pipCore/Systems/Network/Wifi.hpp>

#include <pipCore/Platforms/Select.hpp>

namespace pipcore::net
{
    void wifiConfigure(const WifiConfig &cfg) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->wifiConfigure(cfg);
    }

    void wifiRequest(bool enabled) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->wifiRequest(enabled);
    }

    void wifiService() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->wifiService();
    }

    WifiState wifiState() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            return p->wifiState();
        return WifiState::Unsupported;
    }

    bool wifiConnected() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            return p->wifiConnected();
        return false;
    }

    uint32_t wifiLocalIpV4() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            return p->wifiLocalIpV4();
        return 0;
    }
}


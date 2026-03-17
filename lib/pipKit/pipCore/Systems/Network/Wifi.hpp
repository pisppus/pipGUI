#pragma once

#include <cstdint>

namespace pipcore::net
{
    enum class WifiState : uint8_t
    {
        Off = 0,
        Connecting = 1,
        Connected = 2,
        Failed = 3,
        Unsupported = 4,
    };

    struct WifiConfig
    {
        const char *ssid = nullptr;
        const char *password = nullptr;

        bool disableSleep = false;
        bool fullScan = false;
        bool autoReconnect = true;

        uint32_t connectTimeoutMs = 15'000;
        uint32_t retryDelayMs = 2'500;
        uint32_t noSsidGraceMs = 1'000;
    };

    void wifiConfigure(const WifiConfig &cfg) noexcept;
    void wifiRequest(bool enabled) noexcept;
    void wifiService() noexcept;

    [[nodiscard]] WifiState wifiState() noexcept;
    [[nodiscard]] bool wifiConnected() noexcept;
    [[nodiscard]] uint32_t wifiLocalIpV4() noexcept;
}

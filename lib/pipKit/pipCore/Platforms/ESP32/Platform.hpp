#pragma once

#include <pipCore/Platform.hpp>

#if !defined(ESP32)
#error "pipcore::esp32::Platform requires ESP32"
#endif

#include <pipCore/Displays/Select.hpp>
#include <pipCore/Platforms/ESP32/Services/Backlight.hpp>
#include <pipCore/Platforms/ESP32/Services/Gpio.hpp>
#include <pipCore/Platforms/ESP32/Services/Heap.hpp>
#include <pipCore/Platforms/ESP32/Services/Prefs.hpp>
#include <pipCore/Platforms/ESP32/Services/Time.hpp>
#include <pipCore/Platforms/ESP32/Services/Wifi.hpp>
#include <pipCore/Platforms/ESP32/Services/Ota.hpp>
#include <pipCore/Platforms/ESP32/Transports/St7789Spi.hpp>

namespace pipcore::esp32
{
    class Platform final : public pipcore::Platform
    {
    public:
        Platform()
        {
            _ota.bindWifi(&_wifi);
        }
        ~Platform() override = default;

        void pinModeInput(uint8_t pin, InputMode mode) noexcept override;
        [[nodiscard]] bool digitalRead(uint8_t pin) noexcept override;

        void configureBacklightPin(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12) noexcept override;

        [[nodiscard]] uint32_t nowMs() noexcept override;
        [[nodiscard]] uint8_t loadMaxBrightnessPercent() noexcept override;
        void storeMaxBrightnessPercent(uint8_t percent) noexcept override;

        void setBacklightPercent(uint8_t percent) noexcept override;

        void *alloc(size_t bytes, AllocCaps caps = AllocCaps::Default) noexcept override;
        void free(void *ptr) noexcept override;

        [[nodiscard]] bool configureDisplay(const DisplayConfig &cfg) noexcept override;
        [[nodiscard]] bool beginDisplay(uint8_t rotation) noexcept override;
        [[nodiscard]] pipcore::Display *display() noexcept override;

        [[nodiscard]] uint32_t freeHeapTotal() noexcept override;
        [[nodiscard]] uint32_t freeHeapInternal() noexcept override;
        [[nodiscard]] uint32_t largestFreeBlock() noexcept override;
        [[nodiscard]] uint32_t minFreeHeap() noexcept override;
        [[nodiscard]] PlatformError lastError() const noexcept override;
        [[nodiscard]] const char *lastErrorText() const noexcept override;

        [[nodiscard]] uint8_t readProgmemByte(const void *addr) noexcept override;

        void wifiConfigure(const pipcore::net::WifiConfig &cfg) noexcept override;
        void wifiRequest(bool enabled) noexcept override;
        void wifiService() noexcept override;
        [[nodiscard]] pipcore::net::WifiState wifiState() noexcept override;
        [[nodiscard]] bool wifiConnected() noexcept override;
        [[nodiscard]] uint32_t wifiLocalIpV4() noexcept override;

        void otaConfigure(const char *manifestUrl,
                          const pipcore::ota::Options &opt,
                          void (*cb)(const pipcore::ota::Status &, void *),
                          void *user) noexcept override;

        void otaConfigureChannels(const char *stableUrl,
                                  const char *betaUrl,
                                  pipcore::ota::Channel initial,
                                  const pipcore::ota::Options &opt,
                                  void (*cb)(const pipcore::ota::Status &, void *),
                                  void *user) noexcept override;

        void otaSetChannel(pipcore::ota::Channel ch) noexcept override;
        [[nodiscard]] pipcore::ota::Channel otaChannel() noexcept override;
        void otaRequestCheck(pipcore::ota::CheckMode mode) noexcept override;
        void otaRequestInstall() noexcept override;
        void otaRequestRollback() noexcept override;
        void otaCancel() noexcept override;
        void otaService() noexcept override;
        [[nodiscard]] const pipcore::ota::Status &otaStatus() noexcept override;
        void otaMarkAppValid() noexcept override;

    private:
        services::Time _time;
        services::Gpio _gpio;
        services::Backlight _backlight;
        services::Heap _heap;
        services::Prefs _prefs;
        services::Wifi _wifi;
        services::Ota _ota;
        St7789Spi _transport;
        SelectedDisplay _display;
        bool _displayConfigured = false;
        bool _displayReady = false;
        PlatformError _lastError = PlatformError::None;
    };
}

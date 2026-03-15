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
#include <pipCore/Platforms/ESP32/Transports/St7789Spi.hpp>

namespace pipcore::esp32
{
    class Platform final : public pipcore::Platform
    {
    public:
        Platform() = default;
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

    private:
        services::Time _time;
        services::Gpio _gpio;
        services::Backlight _backlight;
        services::Heap _heap;
        services::Prefs _prefs;
        St7789Spi _transport;
        SelectedDisplay _display;
        bool _displayConfigured = false;
        bool _displayReady = false;
        PlatformError _lastError = PlatformError::None;
    };
}

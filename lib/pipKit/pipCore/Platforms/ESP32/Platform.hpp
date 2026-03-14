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

        void pinModeInput(uint8_t pin, InputMode mode) override;
        bool digitalRead(uint8_t pin) override;

        void configureBacklightPin(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12) override;

        uint32_t nowMs() override;
        uint8_t loadMaxBrightnessPercent() override;
        void storeMaxBrightnessPercent(uint8_t percent) override;

        void setBacklightPercent(uint8_t percent) override;

        void *alloc(size_t bytes, AllocCaps caps = AllocCaps::Default) override;
        void free(void *ptr) override;

        bool configureDisplay(const DisplayConfig &cfg) override;
        bool beginDisplay(uint8_t rotation) override;
        pipcore::Display *display() override;

        uint32_t freeHeapTotal() override;
        uint32_t freeHeapInternal() override;
        uint32_t largestFreeBlock() override;
        uint32_t minFreeHeap() override;
        PlatformError lastError() const override;
        const char *lastErrorText() const override;

        uint8_t readProgmemByte(const void *addr) override;

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

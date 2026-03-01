#pragma once

#include <pipCore/Platforms/GuiPlatform.hpp>

#if !defined(ESP32)
#error "pipcore::ESP32Platform requires ESP32"
#endif

#define PIPCORE_PLATFORM_SELECTED

#include <Arduino.h>
#include <Preferences.h>
#include <pipCore/Displays/DisplayFactory.hpp>
#include <pipCore/Platforms/ESP32/Transports/St7789Spi.hpp>

namespace pipcore
{
    class ESP32Platform final : public GuiPlatform
    {
    public:
        ESP32Platform() = default;
        ~ESP32Platform();

        void ioPinModeInput(uint8_t pin, bool pullup) override;
        bool ioDigitalRead(uint8_t pin) override;

        void configureBacklightPin(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12) override;

        uint32_t nowMs() override;
        uint8_t loadMaxBrightnessPercent() override;
        void storeMaxBrightnessPercent(uint8_t percent) override;

        void setBacklightPercent(uint8_t percent) override;

        void *guiAlloc(size_t bytes, GuiAllocCaps caps = GuiAllocCaps::Default) override;
        void guiFree(void *ptr) override;

        bool guiConfigureDisplay(const GuiDisplayConfig &cfg) override;
        bool guiBeginDisplay(uint8_t rotation) override;
        GuiDisplay *guiDisplay() override;

        uint8_t readProgmemByte(const void *addr) override;

    private:
        Preferences _prefs;
        bool _prefsInited = false;

        bool _backlightConfigured = false;
        uint8_t _backlightChannel = 0;
        uint8_t _backlightResolution = 12;

        Esp32St7789Spi *_transport = nullptr;
        bool _displayConfigured = false;
    };
}

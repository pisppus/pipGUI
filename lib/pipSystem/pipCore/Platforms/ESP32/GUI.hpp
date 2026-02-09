#pragma once

#include <pipCore/Platforms/GUIPlatform.hpp>

#if !defined(ESP32)
#error "pipcore::Esp32GuiPlatform requires ESP32"
#endif

#include <Arduino.h>
#include <Preferences.h>

#include <pipCore/Displays/ST7789/Display.hpp>

namespace pipcore
{
    class Esp32GuiPlatform final : public GuiPlatform
    {
    public:
        Esp32GuiPlatform() = default;

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

    private:
        Preferences _prefs;
        bool _prefsInited = false;

        bool _backlightConfigured = false;
        uint8_t _backlightChannel = 0;
        uint8_t _backlightResolution = 12;

        ST7789Display _display;
        bool _displayConfigured = false;
    };
}

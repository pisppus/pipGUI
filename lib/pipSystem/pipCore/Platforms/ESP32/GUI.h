#pragma once

#include <pipCore/Platforms/GUIPlatform.h>

#if !defined(ESP32)
#error "pipcore::Esp32GuiPlatform requires ESP32"
#endif

#include <Arduino.h>
#include <Preferences.h>

#include <pipCore/Platforms/ESP32/ST7789.h>

namespace pipcore
{
    class Esp32GuiPlatform final : public GuiPlatform
    {
    public:
        Esp32GuiPlatform() = default;

        void configureBacklightPin(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12) override;

        uint32_t nowMs() override;
        uint8_t loadMaxBrightnessPercent() override;
        void storeMaxBrightnessPercent(uint8_t percent) override;

        void setBacklightPercent(uint8_t percent) override;

        void *guiAlloc(size_t bytes, GuiAllocCaps caps = GuiAllocCaps::Default) override;
        void guiFree(void *ptr) override;

        bool guiConfigureDisplay(const GuiDisplayConfig &cfg) override;
        bool guiBeginDisplay(uint8_t rotation) override;
        lgfx::LovyanGFX *guiDisplay() override;

    private:
        Preferences _prefs;
        bool _prefsInited = false;

        bool _backlightConfigured = false;
        uint8_t _backlightChannel = 0;
        uint8_t _backlightResolution = 12;

        Esp32St7789Display _display;
        bool _displayConfigured = false;
    };
}

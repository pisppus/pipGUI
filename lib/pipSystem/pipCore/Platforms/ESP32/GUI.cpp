#include <pipCore/Platforms/ESP32/GUI.hpp>
#include <esp_heap_caps.h>

namespace pipcore
{
    ESP32Platform::~ESP32Platform()
    {
        if (_transport)
        {
            delete _transport;
            _transport = nullptr;
        }
    }

    void ESP32Platform::ioPinModeInput(uint8_t pin, bool pullup)
    {
        pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
    }

    bool ESP32Platform::ioDigitalRead(uint8_t pin)
    {
        return digitalRead(pin) != 0;
    }

    void ESP32Platform::configureBacklightPin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        const uint8_t resolvedBits = resolutionBits > 16 ? 12 : resolutionBits;
        ledcSetup(channel, freqHz, resolvedBits);
        ledcAttachPin(pin, channel);
        _backlightConfigured = true;
        _backlightChannel = channel;
        _backlightResolution = resolvedBits;
    }

    uint32_t ESP32Platform::nowMs()
    {
        return millis();
    }

    uint8_t ESP32Platform::loadMaxBrightnessPercent()
    {
        if (!_prefsInited)
        {
            _prefs.begin("pipgui", false);
            _prefsInited = true;
        }

        uint16_t raw = _prefs.getUShort("bmax", 100);
        if (raw > 100)
            raw = 100;
        return static_cast<uint8_t>(raw);
    }

    void ESP32Platform::storeMaxBrightnessPercent(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;

        if (!_prefsInited)
        {
            _prefs.begin("pipgui", false);
            _prefsInited = true;
        }

        _prefs.putUShort("bmax", percent);
    }

    void ESP32Platform::setBacklightPercent(uint8_t percent)
    {
        if (!_backlightConfigured)
            return;

        if (percent > 100)
            percent = 100;

        const uint32_t dutyMax = (1U << _backlightResolution) - 1U;
        const uint32_t duty = (dutyMax * (uint32_t)percent + 50U) / 100U;
        ledcWrite(_backlightChannel, dutyMax - duty);
    }

    void *ESP32Platform::guiAlloc(size_t bytes, GuiAllocCaps caps)
    {
        if (bytes == 0)
            return nullptr;

        if (caps == GuiAllocCaps::PreferExternal)
        {
            void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (p)
                return p;
        }

        (void)caps;
        return heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    }

    void ESP32Platform::guiFree(void *ptr)
    {
        heap_caps_free(ptr);
    }

    bool ESP32Platform::guiConfigureDisplay(const GuiDisplayConfig &cfg)
    {
        if (cfg.width == 0 || cfg.height == 0)
            return false;

        if (_transport)
        {
            delete _transport;
            _transport = nullptr;
        }

        _transport = new Esp32St7789Spi(
            cfg.mosi, cfg.sclk, cfg.cs, cfg.dc, cfg.rst, cfg.hz);

        bool ok = _display.configure(_transport, cfg.width, cfg.height, cfg.order, cfg.invert, cfg.swap, cfg.xOffset, cfg.yOffset);
        if (!ok)
        {
            delete _transport;
            _transport = nullptr;
            return false;
        }

        _displayConfigured = true;
        return true;
    }

    bool ESP32Platform::guiBeginDisplay(uint8_t rotation)
    {
        if (!_displayConfigured)
            return false;

        return _display.begin(rotation);
    }

    GuiDisplay *ESP32Platform::guiDisplay()
    {
        if (!_displayConfigured)
            return nullptr;
        return &_display;
    }
}

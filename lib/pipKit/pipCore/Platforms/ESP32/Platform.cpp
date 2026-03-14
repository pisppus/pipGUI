#include <pipCore/Platforms/ESP32/Platform.hpp>

namespace pipcore::esp32
{
    void Platform::pinModeInput(uint8_t pin, InputMode mode)
    {
        _gpio.pinModeInput(pin, mode);
    }

    bool Platform::digitalRead(uint8_t pin)
    {
        return _gpio.digitalRead(pin);
    }

    void Platform::configureBacklightPin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        _backlight.configurePin(pin, channel, freqHz, resolutionBits);
    }

    uint32_t Platform::nowMs()
    {
        return _time.nowMs();
    }

    uint8_t Platform::loadMaxBrightnessPercent()
    {
        uint8_t percent = 100;
        if (!_prefs.loadMaxBrightnessPercent(percent))
        {
            _lastError = PlatformError::PrefsOpenFailed;
            return 100;
        }
        _lastError = PlatformError::None;
        return percent;
    }

    void Platform::storeMaxBrightnessPercent(uint8_t percent)
    {
        if (!_prefs.storeMaxBrightnessPercent(percent))
        {
            _lastError = PlatformError::PrefsOpenFailed;
            return;
        }
        _lastError = PlatformError::None;
    }

    void Platform::setBacklightPercent(uint8_t percent)
    {
        _backlight.setPercent(percent);
    }

    void *Platform::alloc(size_t bytes, AllocCaps caps)
    {
        return _heap.alloc(bytes, caps);
    }

    void Platform::free(void *ptr)
    {
        _heap.free(ptr);
    }

    bool Platform::configureDisplay(const DisplayConfig &cfg)
    {
        _lastError = PlatformError::None;
        _displayConfigured = false;
        _displayReady = false;
        if (cfg.width == 0 || cfg.height == 0)
        {
            _lastError = PlatformError::InvalidDisplayConfig;
            return false;
        }

        _display.reset();
        _transport.configure(cfg.mosi, cfg.sclk, cfg.cs, cfg.dc, cfg.rst, cfg.hz);

        const bool ok = _display.configure(this, &_transport, cfg.width, cfg.height, cfg.order, cfg.invert, cfg.swap, cfg.xOffset, cfg.yOffset);
        if (!ok)
        {
            _lastError = PlatformError::DisplayConfigureFailed;
            return false;
        }

        _displayConfigured = true;
        return true;
    }

    bool Platform::beginDisplay(uint8_t rotation)
    {
        _displayReady = false;
        if (!_displayConfigured)
        {
            _lastError = PlatformError::InvalidDisplayConfig;
            return false;
        }

        if (!_display.begin(rotation))
        {
            _lastError = PlatformError::DisplayBeginFailed;
            return false;
        }

        _lastError = PlatformError::None;
        _displayReady = true;
        return true;
    }

    pipcore::Display *Platform::display()
    {
        if (!_displayConfigured || !_displayReady)
            return nullptr;
        return &_display;
    }

    uint32_t Platform::freeHeapTotal()
    {
        return _heap.freeHeapTotal();
    }

    uint32_t Platform::freeHeapInternal()
    {
        return _heap.freeHeapInternal();
    }

    uint32_t Platform::largestFreeBlock()
    {
        return _heap.largestFreeBlock();
    }

    uint32_t Platform::minFreeHeap()
    {
        return _heap.minFreeHeap();
    }

    PlatformError Platform::lastError() const
    {
        if (_lastError != PlatformError::None)
            return _lastError;

        return _display.lastError() == st7789::IoError::None ? PlatformError::None : PlatformError::DisplayIoFailed;
    }

    const char *Platform::lastErrorText() const
    {
        if (_lastError == PlatformError::DisplayConfigureFailed ||
            _lastError == PlatformError::DisplayBeginFailed ||
            _lastError == PlatformError::DisplayIoFailed)
        {
            if (_display.lastError() != st7789::IoError::None)
                return _display.lastErrorText();
        }

        if (_lastError != PlatformError::None)
            return platformErrorText(_lastError);

        if (_display.lastError() != st7789::IoError::None)
            return _display.lastErrorText();

        return platformErrorText(PlatformError::None);
    }

    uint8_t Platform::readProgmemByte(const void *addr)
    {
        return pgm_read_byte(addr);
    }
}

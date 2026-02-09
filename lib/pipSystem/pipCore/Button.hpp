#pragma once

#include <stdint.h>

#include <pipCore/Platforms/GUIPlatform.hpp>

namespace pipcore
{
    class Button
    {
    public:
        Button(uint8_t pin, bool usePullup = true, bool activeLow = true, uint16_t debounceMs = 30)
            : _platform(nullptr),
              _pin(pin),
              _usePullup(usePullup),
              _activeLow(activeLow),
              _debounceMs(debounceMs),
              _stableState(false),
              _lastReadState(false),
              _lastChangeMs(0),
              _pressedFlag(false)
        {
        }

        Button(GuiPlatform *platform, uint8_t pin, bool usePullup = true, bool activeLow = true, uint16_t debounceMs = 30)
            : _platform(platform),
              _pin(pin),
              _usePullup(usePullup),
              _activeLow(activeLow),
              _debounceMs(debounceMs),
              _stableState(false),
              _lastReadState(false),
              _lastChangeMs(0),
              _pressedFlag(false)
        {
        }

        void begin()
        {
            GuiPlatform *plat = platform();
            if (plat)
                plat->ioPinModeInput(_pin, _usePullup);
            _stableState = _lastReadState = readRaw();
            _lastChangeMs = nowMs();
            _pressedFlag = false;
        }

        void update()
        {
            bool raw = readRaw();
            uint32_t now = nowMs();
            if (raw != _lastReadState)
            {
                _lastReadState = raw;
                _lastChangeMs = now;
            }
            if ((uint32_t)(now - _lastChangeMs) >= _debounceMs)
            {
                if (_stableState != _lastReadState)
                {
                    _stableState = _lastReadState;
                    if (_stableState)
                        _pressedFlag = true;
                }
            }
        }

        bool wasPressed()
        {
            update();
            if (_pressedFlag)
            {
                _pressedFlag = false;
                return true;
            }
            return false;
        }

        bool isDown() const { return _stableState; }

    private:
        GuiPlatform *platform() const { return _platform ? _platform : GuiPlatform::defaultPlatform(); }

        uint32_t nowMs() const
        {
            GuiPlatform *plat = platform();
            if (plat)
                return plat->nowMs();
            return 0;
        }

        bool readRaw() const
        {
            bool v = false;
            GuiPlatform *plat = platform();
            if (plat)
                v = plat->ioDigitalRead(_pin);
            return _activeLow ? !v : v;
        }

        GuiPlatform *_platform;
        uint8_t _pin;
        bool _usePullup;
        bool _activeLow;
        uint16_t _debounceMs;
        bool _stableState;
        bool _lastReadState;
        uint32_t _lastChangeMs;
        bool _pressedFlag;
    };
}

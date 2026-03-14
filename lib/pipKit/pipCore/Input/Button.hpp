#pragma once

#include <pipCore/Platform.hpp>
#include <pipCore/Platforms/Select.hpp>
#include <cstdint>

namespace pipcore
{
    enum PullMode : uint8_t
    {
        Pullup,
        Pulldown
    };

    class Button
    {
    public:
        Button(uint8_t pin, PullMode pull = Pullup)
            : _platform(nullptr),
              _pin(pin),
              _pull(pull),
              _activeLow(true),
              _rawState(false),
              _stableState(false),
              _pressedFlag(false),
              _lastRawChangeMs(0)
        {
            applyPullDefaults();
        }

        Button(Platform *platform, uint8_t pin, PullMode pull = Pullup)
            : _platform(platform),
              _pin(pin),
              _pull(pull),
              _activeLow(true),
              _rawState(false),
              _stableState(false),
              _pressedFlag(false),
              _lastRawChangeMs(0)
        {
            applyPullDefaults();
        }

        void begin()
        {
            Platform *plat = platform();
            if (plat)
            {
                const InputMode inputMode = (_pull == Pullup)
                                                ? InputMode::Pullup
                                                : InputMode::Pulldown;
                plat->pinModeInput(_pin, inputMode);
            }

            bool raw = readRaw();

            _rawState = raw;
            _stableState = raw;
            _pressedFlag = false;
            _lastRawChangeMs = nowMs();
        }

        void update()
        {
            const bool raw = readRaw();
            const uint32_t now = nowMs();

            if (raw != _rawState)
            {
                _rawState = raw;
                _lastRawChangeMs = now;
            }

            if (_stableState != _rawState && (uint32_t)(now - _lastRawChangeMs) >= debounceMs())
            {
                const bool prev = _stableState;
                _stableState = _rawState;
                if (!prev && _stableState)
                    _pressedFlag = true;
            }
        }

        bool wasPressed()
        {
            if (_pressedFlag)
            {
                _pressedFlag = false;
                return true;
            }
            return false;
        }

        bool isDown() const { return _stableState; }

        uint8_t pin() const { return _pin; }
        PullMode pullMode() const { return _pull; }

    private:
        Platform *platform() const { return _platform ? _platform : GetPlatform(); }

        uint32_t nowMs() const
        {
            Platform *plat = platform();
            return plat ? plat->nowMs() : 0;
        }

        static constexpr uint32_t debounceMs()
        {
            return 16;
        }

        void applyPullDefaults()
        {
            if (_pull == Pullup)
                _activeLow = true;
            else if (_pull == Pulldown)
                _activeLow = false;
        }

        bool readRaw() const
        {
            bool v = false;
            Platform *plat = platform();
            if (plat)
                v = plat->digitalRead(_pin);
            return _activeLow ? !v : v;
        }

        Platform *_platform;
        uint8_t _pin;
        PullMode _pull;
        bool _activeLow;
        bool _rawState;
        bool _stableState;
        bool _pressedFlag;
        uint32_t _lastRawChangeMs;
    };
}

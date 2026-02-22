#pragma once

#include <pipCore/Platforms/GuiPlatform.hpp>
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
              _stableState(false),
              _pressedFlag(false),
              _vc0(0xFF),
              _vc1(0xFF)
        {
            applyPullDefaults();
        }

        Button(GuiPlatform *platform, uint8_t pin, PullMode pull = Pullup)
            : _platform(platform),
              _pin(pin),
              _pull(pull),
              _activeLow(true),
              _stableState(false),
              _pressedFlag(false),
              _vc0(0xFF),
              _vc1(0xFF)
        {
            applyPullDefaults();
        }

        void begin()
        {
            GuiPlatform *plat = platform();
            if (plat)
            {
                const bool enablePullup = (_pull == Pullup);
                plat->ioPinModeInput(_pin, enablePullup);
            }

            bool raw = readRaw();

            _stableState = raw;
            _pressedFlag = false;

            _vc0 = 0xFF;
            _vc1 = 0xFF;
        }

        void update()
        {
            const uint8_t sample = readRaw() ? 0x01 : 0x00;
            const uint8_t deb = _stableState ? 0x01 : 0x00;
            const uint8_t delta = (uint8_t)(sample ^ deb);

            _vc0 = (uint8_t)~(_vc0 & delta);
            _vc1 = (uint8_t)(_vc0 ^ (_vc1 & delta));
            const uint8_t toggles = (uint8_t)(delta & _vc0 & _vc1);

            if (toggles)
            {
                const bool prev = _stableState;
                _stableState = !_stableState;
                if (!prev && _stableState)
                    _pressedFlag = true;
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

        uint8_t pin() const { return _pin; }
        PullMode pullMode() const { return _pull; }

    private:
        GuiPlatform *platform() const { return _platform ? _platform : GuiPlatform::defaultPlatform(); }

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
            GuiPlatform *plat = platform();
            if (plat)
                v = plat->ioDigitalRead(_pin);
            return _activeLow ? !v : v;
        }

        GuiPlatform *_platform;
        uint8_t _pin;
        PullMode _pull;
        bool _activeLow;
        bool _stableState;
        bool _pressedFlag;
        uint8_t _vc0;
        uint8_t _vc1;
    };
}

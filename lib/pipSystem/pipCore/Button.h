#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace pipcore
{
    class Button
    {
    public:
        Button(uint8_t pin, bool usePullup = true, bool activeLow = true, uint16_t debounceMs = 30)
            : _pin(pin), _usePullup(usePullup), _activeLow(activeLow), _debounceMs(debounceMs),
              _stableState(false), _lastReadState(false), _lastChangeMs(0), _pressedFlag(false)
        {
        }

        void begin()
        {
            pinMode(_pin, _usePullup ? INPUT_PULLUP : INPUT);
            _stableState = _lastReadState = readRaw();
            _lastChangeMs = millis();
            _pressedFlag = false;
        }

        void update()
        {
            bool raw = readRaw();
            uint32_t now = millis();
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
        bool readRaw() const
        {
            return _activeLow ? !digitalRead(_pin) : digitalRead(_pin);
        }

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

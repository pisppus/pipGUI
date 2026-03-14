#pragma once

#include <pipCore/Platform.hpp>

namespace pipcore::esp32::services
{
    class Gpio
    {
    public:
        void pinModeInput(uint8_t pin, InputMode mode) const;
        bool digitalRead(uint8_t pin) const;
    };
}

#pragma once

#include <pipCore/Platform.hpp>

namespace pipcore::esp32::services
{
    class Gpio
    {
    public:
        void pinModeInput(uint8_t pin, InputMode mode) const noexcept;
        [[nodiscard]] bool digitalRead(uint8_t pin) const noexcept;
    };
}

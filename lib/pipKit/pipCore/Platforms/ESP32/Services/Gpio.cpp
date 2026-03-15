#include <pipCore/Platforms/ESP32/Services/Gpio.hpp>
#include <Arduino.h>

namespace pipcore::esp32::services
{
    void Gpio::pinModeInput(uint8_t pin, InputMode mode) const noexcept
    {
        uint8_t arduinoMode = INPUT;

        if (mode == InputMode::Pullup)
            arduinoMode = INPUT_PULLUP;
#ifdef INPUT_PULLDOWN
        else if (mode == InputMode::Pulldown)
            arduinoMode = INPUT_PULLDOWN;
#endif

        pinMode(pin, arduinoMode);
    }

    bool Gpio::digitalRead(uint8_t pin) const noexcept
    {
        return ::digitalRead(pin) != 0;
    }
}

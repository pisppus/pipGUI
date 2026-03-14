#include <pipCore/Platforms/ESP32/Services/Backlight.hpp>
#include <Arduino.h>

namespace pipcore::esp32::services
{
    void Backlight::configurePin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        const uint8_t resolvedBits = resolutionBits > 16 ? 12 : resolutionBits;
        ledcSetup(channel, freqHz, resolvedBits);
        ledcAttachPin(pin, channel);
        _configured = true;
        _channel = channel;
        _resolutionBits = resolvedBits;
    }

    void Backlight::setPercent(uint8_t percent)
    {
        if (!_configured)
            return;

        if (percent > 100)
            percent = 100;

        const uint32_t dutyMax = (1U << _resolutionBits) - 1U;
        const uint32_t duty = (dutyMax * (uint32_t)percent + 50U) / 100U;
        ledcWrite(_channel, dutyMax - duty);
    }
}

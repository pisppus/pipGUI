#pragma once

#include <cstdint>

namespace pipcore::esp32::services
{
    class Backlight
    {
    public:
        void configurePin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits);
        void setPercent(uint8_t percent);

    private:
        bool _configured = false;
        uint8_t _channel = 0;
        uint8_t _resolutionBits = 12;
    };
}

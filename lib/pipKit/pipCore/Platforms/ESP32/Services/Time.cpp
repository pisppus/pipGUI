#include <pipCore/Platforms/ESP32/Services/Time.hpp>
#include <Arduino.h>

namespace pipcore::esp32::services
{
    uint32_t Time::nowMs() const noexcept
    {
        return millis();
    }
}

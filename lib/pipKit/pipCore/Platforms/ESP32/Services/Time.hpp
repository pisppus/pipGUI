#pragma once

#include <cstdint>

namespace pipcore::esp32::services
{
    class Time
    {
    public:
        uint32_t nowMs() const;
    };
}

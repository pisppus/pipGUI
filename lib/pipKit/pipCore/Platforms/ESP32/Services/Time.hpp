#pragma once

#include <cstdint>

namespace pipcore::esp32::services
{
    class Time
    {
    public:
        [[nodiscard]] uint32_t nowMs() const noexcept;
    };
}

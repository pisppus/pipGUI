#pragma once

#include <cstdint>

namespace pipgui
{
    namespace detail
    {
        static inline uint32_t lighten888(uint32_t c, uint8_t amount)
        {
            uint8_t r = (uint8_t)((c >> 16) & 0xFF);
            uint8_t g = (uint8_t)((c >> 8) & 0xFF);
            uint8_t b = (uint8_t)(c & 0xFF);
            uint16_t ar = (uint16_t)r + amount;
            if (ar > 255)
                ar = 255;
            uint16_t ag = (uint16_t)g + (amount * 2) / 3;
            if (ag > 255)
                ag = 255;
            uint16_t ab = (uint16_t)b + amount;
            if (ab > 255)
                ab = 255;
            return (uint32_t)((ar << 16) | (ag << 8) | ab);
        }

        static inline uint32_t autoTextColorForBg(uint32_t bg)
        {
            uint8_t r = (uint8_t)((bg >> 16) & 0xFF);
            uint8_t g = (uint8_t)((bg >> 8) & 0xFF);
            uint8_t b = (uint8_t)(bg & 0xFF);
            uint16_t lum = (uint16_t)((r * 30U + g * 59U + b * 11U) / 100U);
            return (lum > 140U) ? 0x000000 : 0xFFFFFF;
        }
    }
}

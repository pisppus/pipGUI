#pragma once

#include <cstdint>

namespace pipgui
{
    namespace detail
    {
        static inline uint16_t lighten565(uint16_t c, uint8_t amount)
        {
            uint8_t r = (c >> 11) & 0x1F;
            uint8_t g = (c >> 5) & 0x3F;
            uint8_t b = c & 0x1F;
            uint16_t ar = (uint16_t)r + amount;
            if (ar > 31)
                ar = 31;
            uint16_t ag = (uint16_t)g + (amount * 2) / 3;
            if (ag > 63)
                ag = 63;
            uint16_t ab = (uint16_t)b + amount;
            if (ab > 31)
                ab = 31;
            return (uint16_t)((ar << 11) | (ag << 5) | ab);
        }

        static inline uint16_t autoTextColorForBg(uint16_t bg)
        {
            uint8_t r5 = (bg >> 11) & 0x1F;
            uint8_t g6 = (bg >> 5) & 0x3F;
            uint8_t b5 = bg & 0x1F;
            uint16_t r8 = (uint16_t)((r5 * 255U) / 31U);
            uint16_t g8 = (uint16_t)((g6 * 255U) / 63U);
            uint16_t b8 = (uint16_t)((b5 * 255U) / 31U);
            uint16_t lum = (uint16_t)((r8 * 30U + g8 * 59U + b8 * 11U) / 100U);
            return (lum > 140U) ? 0x0000 : 0xFFFF;
        }
    }
}

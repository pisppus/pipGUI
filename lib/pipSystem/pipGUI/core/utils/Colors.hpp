#pragma once

#include <cstdint>
#include <algorithm>

namespace pipgui
{
    namespace detail
    {
        static inline uint32_t color565To888(uint16_t c)
        {
            uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
            uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
            uint8_t b5 = (uint8_t)(c & 0x1F);
            uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        }

        static inline uint16_t color888To565(uint32_t c)
        {
            uint8_t r = (uint8_t)((c >> 16) & 0xFF);
            uint8_t g = (uint8_t)((c >> 8) & 0xFF);
            uint8_t b = (uint8_t)(c & 0xFF);
            return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
        }

        static inline uint32_t lerp888(uint32_t a, uint32_t b, float t)
        {
            if (t <= 0.0f)
                return a;
            if (t >= 1.0f)
                return b;

            uint8_t ar = (uint8_t)((a >> 16) & 0xFF);
            uint8_t ag = (uint8_t)((a >> 8) & 0xFF);
            uint8_t ab = (uint8_t)(a & 0xFF);

            uint8_t br = (uint8_t)((b >> 16) & 0xFF);
            uint8_t bg = (uint8_t)((b >> 8) & 0xFF);
            uint8_t bb = (uint8_t)(b & 0xFF);

            uint8_t cr = (uint8_t)(ar + (uint8_t)((br - ar) * t));
            uint8_t cg = (uint8_t)(ag + (uint8_t)((bg - ag) * t));
            uint8_t cb = (uint8_t)(ab + (uint8_t)((bb - ab) * t));

            return ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;
        }

        static inline uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha)
        {
            uint32_t a = alpha + (alpha >> 7);

            uint32_t bg_g = bg & 0x07E0;
            uint32_t fg_g = fg & 0x07E0;
            uint32_t g = bg_g + (((fg_g - bg_g) * a) >> 8);
            g &= 0x07E0;

            uint32_t bg_r = bg >> 11;
            uint32_t fg_r = fg >> 11;
            uint32_t r = bg_r + (((fg_r - bg_r) * a) >> 8);

            uint32_t bg_b = bg & 0x1F;
            uint32_t fg_b = fg & 0x1F;
            uint32_t b = bg_b + (((fg_b - bg_b) * a) >> 8);

            return (uint16_t)((r << 11) | g | b);
        }

        static inline uint16_t blend565WithWhite(uint16_t c, uint8_t alpha)
        {
            uint32_t a = alpha + (alpha >> 7);

            uint32_t cg = c & 0x07E0;
            uint32_t g = cg + (((0x07E0 - cg) * a) >> 8);
            g &= 0x07E0;

            uint32_t cr = c >> 11;
            uint32_t r = cr + (((0x1F - cr) * a) >> 8);

            uint32_t cb = c & 0x1F;
            uint32_t b = cb + (((0x1F - cb) * a) >> 8);

            return (uint16_t)((r << 11) | g | b);
        }

        static inline uint32_t blend888(uint32_t bg, uint32_t fg, uint8_t alpha)
        {
            uint32_t a = alpha + (alpha >> 7);

            uint8_t bg_r = (uint8_t)((bg >> 16) & 0xFF);
            uint8_t bg_g = (uint8_t)((bg >> 8) & 0xFF);
            uint8_t bg_b = (uint8_t)(bg & 0xFF);

            uint8_t fg_r = (uint8_t)((fg >> 16) & 0xFF);
            uint8_t fg_g = (uint8_t)((fg >> 8) & 0xFF);
            uint8_t fg_b = (uint8_t)(fg & 0xFF);

            uint8_t r = (uint8_t)(bg_r + (((fg_r - bg_r) * a) >> 8));
            uint8_t g = (uint8_t)(bg_g + (((fg_g - bg_g) * a) >> 8));
            uint8_t b = (uint8_t)(bg_b + (((fg_b - bg_b) * a) >> 8));

            return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }

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

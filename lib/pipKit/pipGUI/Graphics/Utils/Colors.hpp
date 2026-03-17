#pragma once

#include <cstdint>

namespace pipgui
{
    namespace detail
    {
        [[nodiscard]] inline constexpr uint32_t color565To888(uint16_t c) noexcept
        {
            uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
            uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
            uint8_t b5 = (uint8_t)(c & 0x1F);
            uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        }

        [[nodiscard]] inline constexpr uint16_t color888To565(uint32_t c) noexcept
        {
            uint8_t r = (uint8_t)((c >> 16) & 0xFF);
            uint8_t g = (uint8_t)((c >> 8) & 0xFF);
            uint8_t b = (uint8_t)(c & 0xFF);
            return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
        }

        [[nodiscard]] inline uint32_t lerp888(uint32_t a, uint32_t b, float t) noexcept
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

        [[nodiscard]] inline constexpr uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha) noexcept
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

        [[nodiscard]] inline constexpr uint16_t blend565WithWhite(uint16_t c, uint8_t alpha) noexcept
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

        [[nodiscard]] inline constexpr uint32_t blend888(uint32_t bg, uint32_t fg, uint8_t alpha) noexcept
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

        [[nodiscard]] inline uint8_t fadeEdgeAlpha(int32_t pos, int32_t start, int32_t end, int32_t fadePx) noexcept
        {
            if (pos < start || pos >= end)
                return 0;
            if (fadePx <= 0 || end <= start)
                return 255;

            float alpha = 1.0f;
            if (pos < start + fadePx)
                alpha = (float)(pos - start + 1) / (float)(fadePx + 1);
            if (pos >= end - fadePx)
            {
                const float rightAlpha = (float)(end - pos) / (float)(fadePx + 1);
                if (rightAlpha < alpha)
                    alpha = rightAlpha;
            }

            if (alpha <= 0.0f)
                return 0;
            if (alpha >= 1.0f)
                return 255;

            alpha = alpha * alpha * (3.0f - 2.0f * alpha);
            return (uint8_t)(alpha * 255.0f + 0.5f);
        }

        [[nodiscard]] inline constexpr uint32_t lighten888(uint32_t c, uint8_t amount) noexcept
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

        [[nodiscard]] inline constexpr uint16_t autoTextColor(uint16_t bg565, uint8_t threshold = 128) noexcept
        {
            uint8_t r5 = (uint8_t)((bg565 >> 11) & 0x1F);
            uint8_t g6 = (uint8_t)((bg565 >> 5) & 0x3F);
            uint8_t b5 = (uint8_t)(bg565 & 0x1F);

            uint32_t r = (uint32_t)r5 * 255U / 31U;
            uint32_t g = (uint32_t)g6 * 255U / 63U;
            uint32_t b = (uint32_t)b5 * 255U / 31U;

            uint32_t lum = r * 213U + g * 715U + b * 72U;
            lum /= 1000U;

            return (lum > (uint32_t)threshold) ? 0x0000U : 0xFFFFU;
        }
    }
}

#include "Internal.hpp"
namespace pipgui
{
    namespace
    {
        struct GlowLayer
        {
            int16_t offset;
            uint16_t color;
        };

        static inline uint16_t computeGlowStrength(uint8_t glowStrength, GlowAnim anim,
                                                   uint16_t pulsePeriodMs, uint32_t now)
        {
            if (anim != Pulse || pulsePeriodMs == 0)
                return glowStrength;

            const uint32_t PI_Q16 = 205887;
            const uint32_t TWO_PI_Q16 = 411774;
            uint32_t angle = (uint32_t)(((uint64_t)(now % pulsePeriodMs) * TWO_PI_Q16) / pulsePeriodMs);
            bool neg = (angle > PI_Q16);
            uint32_t x = neg ? (angle - PI_Q16) : angle;
            uint32_t xpm = (uint32_t)((uint64_t)x * (PI_Q16 - x) >> 16);
            uint32_t den = (uint32_t)((5ULL * PI_Q16 * PI_Q16) >> 16) - 4 * xpm;
            uint32_t abs_sin = den ? (uint32_t)(((uint64_t)(16 * xpm) << 16) / den) : 0;
            int32_t sin_val = neg ? -(int32_t)abs_sin : (int32_t)abs_sin;
            uint32_t p = 32768 + (sin_val >> 1);
            uint32_t base = 19661 + (uint32_t)((uint64_t)45875 * p >> 16);
            uint32_t res = (uint32_t)(((uint64_t)glowStrength << 8) * base >> 24);
            return (uint16_t)(res > 255 ? 255 : res);
        }

        static inline uint8_t glowAlphaForOffset(uint32_t inv, uint16_t strength, int16_t off)
        {
            const uint32_t n = (uint32_t)off * inv;
            const uint32_t curve = (uint32_t)((uint64_t)(n * n >> 16) * n >> 16);
            return (uint8_t)min(255U, (uint32_t)strength * curve >> 16);
        }

        static inline uint8_t buildGlowLayers(uint16_t bg, uint16_t glow,
                                              uint8_t glowSize, uint16_t strength,
                                              GlowLayer *layers)
        {
            uint8_t count = 0;
            const uint32_t inv = 65535U / glowSize;
            for (int16_t off = glowSize; off > 0; --off)
            {
                const uint8_t alpha = glowAlphaForOffset(inv, strength, (int16_t)(glowSize - off + 1));
                if (alpha < 2)
                    continue;
                const uint16_t color = detail::blend565(bg, glow, alpha);
                if (count && layers[count - 1].color == color)
                    continue;
                layers[count++] = {off, color};
            }
            return count;
        }
    }

    void GUI::drawGlowCircle(int16_t x, int16_t y, int16_t r,
                             uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                             uint8_t glowSize, uint8_t glowStrength,
                             GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (r <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        const int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        const uint16_t bg = detail::resolveOptionalColor565(bgColor, _render.bgColor565);
        const uint16_t glow = glowColor >= 0 ? (uint16_t)glowColor : detail::blend565WithWhite(fillColor, 80);
        const uint16_t strength = computeGlowStrength(glowStrength, anim, pulsePeriodMs, nowMs());

        if (glowSize == 0 || strength < 2)
        {
            fillCircle(x, y, r, fillColor);
            return;
        }

        GlowLayer layers[255];
        const uint8_t layerCount = buildGlowLayers(bg, glow, glowSize, strength, layers);
        for (uint8_t i = 0; i < layerCount; ++i)
            fillCircle(x, y, (int16_t)(r + layers[i].offset), layers[i].color);
        fillCircle(x, y, r, fillColor);
    }

    void GUI::updateGlowCircle(int16_t x, int16_t y, int16_t r,
                               uint16_t fillColor, int16_t bgColor, int16_t glowColor,
                               uint8_t glowSize, uint8_t glowStrength,
                               GlowAnim anim, uint16_t pulsePeriodMs)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            return;
        }

        const int32_t outerR = r + glowSize, diam = outerR * 2;
        if (x == center)
            x = (int16_t)(AutoX(diam) + diam / 2);
        if (y == center)
            y = (int16_t)(AutoY(diam) + diam / 2);

        const uint16_t bg = detail::resolveOptionalColor565(bgColor, _render.bgColor565);
        constexpr int16_t pad = 2;
        renderToSpriteAndInvalidate(
            (int16_t)(x - outerR - pad), (int16_t)(y - outerR - pad),
            (int16_t)(diam + pad * 2), (int16_t)(diam + pad * 2),
            [&]
            { drawGlowCircle(x, y, r, fillColor, (int16_t)bg, glowColor, glowSize, glowStrength, anim, pulsePeriodMs); });
    }

}

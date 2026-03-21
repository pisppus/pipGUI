#pragma once

#include <cstdint>

namespace pipgui
{
    namespace detail
    {
        namespace motion
        {
            [[nodiscard]] inline float cubicBezierPointUnchecked(float t, float p1, float p2) noexcept
            {
                const float u = 1.0f - t;
                const float tt = t * t;
                const float uu = u * u;
                return (3.0f * uu * t * p1) + (3.0f * u * tt * p2) + (tt * t);
            }

            [[nodiscard]] inline float clamp01(float t) noexcept
            {
                if (t <= 0.0f)
                    return 0.0f;
                if (t >= 1.0f)
                    return 1.0f;
                return t;
            }

            [[nodiscard]] inline float easeOutCubic(float t) noexcept
            {
                t = clamp01(t);
                const float u = 1.0f - t;
                return 1.0f - u * u * u;
            }

            [[nodiscard]] inline float easeInOutCubic(float t) noexcept
            {
                t = clamp01(t);
                if (t < 0.5f)
                    return 4.0f * t * t * t;
                const float f = 2.0f * t - 2.0f;
                return 0.5f * f * f * f + 1.0f;
            }

            [[nodiscard]] inline float easeInOutQuint(float t) noexcept
            {
                t = clamp01(t);
                if (t < 0.5f)
                    return 16.0f * t * t * t * t * t;
                t -= 1.0f;
                return 1.0f + 16.0f * t * t * t * t * t;
            }

            [[nodiscard]] inline float smoothstep(float t) noexcept
            {
                t = clamp01(t);
                return t * t * (3.0f - 2.0f * t);
            }

            [[nodiscard]] inline float cubicBezier1D(float t, float p1, float p2) noexcept
            {
                t = clamp01(t);
                return cubicBezierPointUnchecked(t, p1, p2);
            }

            [[nodiscard]] inline float cubicBezierEase(float t, float x1, float y1, float x2, float y2) noexcept
            {
                t = clamp01(t);
                float lo = 0.0f;
                float hi = 1.0f;
                float u = t;
                for (uint8_t i = 0; i < 10; ++i)
                {
                    const float x = cubicBezierPointUnchecked(u, x1, x2);
                    if (x < t)
                        lo = u;
                    else
                        hi = u;
                    u = (lo + hi) * 0.5f;
                }
                return cubicBezierPointUnchecked(u, y1, y2);
            }
        }
    }
}

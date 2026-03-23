#include "Internal.hpp"
#include <cstdlib>

namespace pipgui
{
    namespace
    {
        constexpr float kArcDegToRad = 0.01745329252f;
        constexpr float kArcRadToDeg = 57.2957795f;
        constexpr int kArcSubSamples = 2;
        constexpr int kArcSubSampleCount = kArcSubSamples * kArcSubSamples;
        constexpr float kArcCrossEpsilon = 0.0001f;

        struct ArcSweepInfo
        {
            float startDeg;
            float endDeg;
            float sweepDeg;
            float startDirX;
            float startDirY;
            float endDirX;
            float endDirY;
            bool full;
            bool wide;
        };

        static inline float normalizeArcDeg(float deg) noexcept
        {
            while (deg < 0.0f)
                deg += 360.0f;
            while (deg >= 360.0f)
                deg -= 360.0f;
            return deg;
        }

        static inline bool angleInArcSweep(float deg, float startDeg, float endDeg) noexcept
        {
            deg = normalizeArcDeg(deg);
            startDeg = normalizeArcDeg(startDeg);
            endDeg = normalizeArcDeg(endDeg);
            if (startDeg <= endDeg)
                return deg >= startDeg && deg <= endDeg;
            return deg >= startDeg || deg <= endDeg;
        }

        static inline ArcSweepInfo makeArcSweepInfo(bool fullRing, float startDeg, float endDeg) noexcept
        {
            ArcSweepInfo info{};
            info.startDeg = normalizeArcDeg(startDeg);
            info.endDeg = normalizeArcDeg(endDeg);
            float sweep = info.endDeg - info.startDeg;
            if (sweep < 0.0f)
                sweep += 360.0f;
            info.sweepDeg = sweep;
            info.full = fullRing || sweep >= 359.5f;
            info.wide = sweep > 180.0f;

            const float startRad = info.startDeg * kArcDegToRad;
            const float endRad = info.endDeg * kArcDegToRad;
            info.startDirX = sinf(startRad);
            info.startDirY = cosf(startRad);
            info.endDirX = sinf(endRad);
            info.endDirY = cosf(endRad);
            return info;
        }

        static inline bool pointInArcSweep(const ArcSweepInfo &info, float dx, float dy) noexcept
        {
            if (info.full)
                return true;

            const float px = dx;
            const float py = -dy;
            const float crossStart = info.startDirX * py - info.startDirY * px;
            const float crossEnd = px * info.endDirY - py * info.endDirX;

            if (!info.wide)
                return crossStart <= kArcCrossEpsilon && crossEnd <= kArcCrossEpsilon;
            return crossStart <= kArcCrossEpsilon || crossEnd <= kArcCrossEpsilon;
        }

        template <typename ColorFunc>
        static void rasterThickArcAA(pipcore::Sprite *spr,
                                     int16_t cx, int16_t cy,
                                     int16_t r,
                                     uint8_t thickness,
                                     bool fullRing,
                                     float startDeg,
                                     float endDeg,
                                     ColorFunc colorAtAngle,
                                     bool needsColorAngle)
        {
            if (!spr || r <= 0 || thickness < 1)
                return;

            Surface565 s;
            if (!getSurface565(spr, s))
                return;

            const float outerR = static_cast<float>(r) + 0.5f;
            const float innerR = static_cast<float>(r - thickness) + 0.5f;
            const float outerR2 = outerR * outerR;
            const float innerR2 = innerR > 0.0f ? innerR * innerR : 0.0f;
            const int16_t outerBound = static_cast<int16_t>(ceilf(outerR)) + 1;
            int32_t minX = static_cast<int32_t>(cx - outerBound);
            int32_t maxX = static_cast<int32_t>(cx + outerBound);
            int32_t minY = static_cast<int32_t>(cy - outerBound);
            int32_t maxY = static_cast<int32_t>(cy + outerBound);
            if (minX < s.clipX)
                minX = s.clipX;
            if (maxX > s.clipR)
                maxX = s.clipR;
            if (minY < s.clipY)
                minY = s.clipY;
            if (maxY > s.clipB)
                maxY = s.clipB;
            if (minX > maxX || minY > maxY)
                return;

            const float subStep = 1.0f / static_cast<float>(kArcSubSamples);
            const float subBias = 0.5f * subStep;
            const uint8_t *gamma = gammaTable();
            const ArcSweepInfo sweepInfo = makeArcSweepInfo(fullRing, startDeg, endDeg);

            for (int32_t py = minY; py <= maxY; ++py)
            {
                for (int32_t px = minX; px <= maxX; ++px)
                {
                    int covered = 0;
                    for (int sy = 0; sy < kArcSubSamples; ++sy)
                    {
                        const float dy = (static_cast<float>(py - cy) - 0.5f) + subBias + static_cast<float>(sy) * subStep;
                        for (int sx = 0; sx < kArcSubSamples; ++sx)
                        {
                            const float dx = (static_cast<float>(px - cx) - 0.5f) + subBias + static_cast<float>(sx) * subStep;
                            const float d2 = dx * dx + dy * dy;
                            if (d2 > outerR2 || d2 < innerR2)
                                continue;
                            if (!pointInArcSweep(sweepInfo, dx, dy))
                                continue;
                            ++covered;
                        }
                    }

                    if (covered == 0)
                        continue;

                    float colorDeg = 0.0f;
                    if (needsColorAngle && !sweepInfo.full)
                    {
                        colorDeg = normalizeArcDeg(atan2f(static_cast<float>(py - cy), static_cast<float>(px - cx)) * kArcRadToDeg + 90.0f);
                        if (!angleInArcSweep(colorDeg, sweepInfo.startDeg, sweepInfo.endDeg))
                            colorDeg = normalizeArcDeg((sweepInfo.startDeg + sweepInfo.sweepDeg * 0.5f));
                    }

                    const Color565 c = makeColor565(colorAtAngle(colorDeg));
                    uint16_t *dst = s.buf + static_cast<int32_t>(py) * s.stride + px;
                    if (covered >= kArcSubSampleCount)
                        *dst = c.fg;
                    else
                        blendStore(dst, c, gamma[(covered * 255) / kArcSubSampleCount]);
                }
            }
        }

        static void rasterArcCapAA(pipcore::Sprite *spr,
                                   float cx, float cy,
                                   float radius,
                                   uint16_t color)
        {
            if (!spr || radius <= 0.0f)
                return;

            Surface565 s;
            if (!getSurface565(spr, s))
                return;

            int32_t minX = static_cast<int32_t>(floorf(cx - radius - 1.0f));
            int32_t maxX = static_cast<int32_t>(ceilf(cx + radius + 1.0f));
            int32_t minY = static_cast<int32_t>(floorf(cy - radius - 1.0f));
            int32_t maxY = static_cast<int32_t>(ceilf(cy + radius + 1.0f));
            if (minX < s.clipX)
                minX = s.clipX;
            if (maxX > s.clipR)
                maxX = s.clipR;
            if (minY < s.clipY)
                minY = s.clipY;
            if (maxY > s.clipB)
                maxY = s.clipB;
            if (minX > maxX || minY > maxY)
                return;
            const float radius2 = radius * radius;
            const float subStep = 1.0f / static_cast<float>(kArcSubSamples);
            const float subBias = 0.5f * subStep;
            const uint8_t *gamma = gammaTable();
            const Color565 c = makeColor565(color);

            for (int32_t py = minY; py <= maxY; ++py)
            {
                for (int32_t px = minX; px <= maxX; ++px)
                {
                    int covered = 0;
                    for (int sy = 0; sy < kArcSubSamples; ++sy)
                    {
                        const float dy = (static_cast<float>(py) - cy - 0.5f) + subBias + static_cast<float>(sy) * subStep;
                        for (int sx = 0; sx < kArcSubSamples; ++sx)
                        {
                            const float dx = (static_cast<float>(px) - cx - 0.5f) + subBias + static_cast<float>(sx) * subStep;
                            if (dx * dx + dy * dy <= radius2)
                                ++covered;
                        }
                    }

                    if (covered == 0)
                        continue;

                    uint16_t *dst = s.buf + static_cast<int32_t>(py) * s.stride + px;
                    if (covered >= kArcSubSampleCount)
                        *dst = c.fg;
                    else
                        blendStore(dst, c, gamma[(covered * 255) / kArcSubSampleCount]);
                }
            }
        }

        template <typename ColorFunc>
        static void drawThickArcCaps(pipcore::Sprite *spr,
                                     int16_t cx, int16_t cy,
                                     int16_t r,
                                     uint8_t thickness,
                                     float startDeg,
                                     float endDeg,
                                     ColorFunc colorAtAngle)
        {
            if (!spr || r <= 0 || thickness < 1)
                return;

            const float capRadius = static_cast<float>(thickness) * 0.5f;
            float centerRadius = static_cast<float>(r) - capRadius + 0.5f;
            if (centerRadius < capRadius)
                centerRadius = capRadius;

            auto drawCap = [&](float deg)
            {
                const float rad = (deg - 90.0f) * kArcDegToRad;
                const float px = static_cast<float>(cx) + cosf(rad) * centerRadius;
                const float py = static_cast<float>(cy) + sinf(rad) * centerRadius;
                rasterArcCapAA(spr, px, py, capRadius, colorAtAngle(normalizeArcDeg(deg)));
            };

            drawCap(startDeg);
            if (fabsf(endDeg - startDeg) > 0.01f)
                drawCap(endDeg);
        }

        static uint16_t solidArcColorAtAngle(const void *ctx, float)
        {
            return *static_cast<const uint16_t *>(ctx);
        }

        static void rasterThickLineAA(pipcore::Sprite *spr,
                                      int16_t x0, int16_t y0,
                                      int16_t x1, int16_t y1,
                                      uint8_t thickness,
                                      uint16_t color565,
                                      bool roundStart,
                                      bool roundEnd)
        {
            if (!spr || thickness < 1)
                return;

            Surface565 s;
            if (!getSurface565(spr, s))
                return;

            float radius = static_cast<float>(thickness) * 0.5f;
            if (thickness > 1)
                radius -= 0.125f;
            if (radius < 0.5f)
                radius = 0.5f;
            const float radius2 = radius * radius;
            int32_t minX = static_cast<int32_t>(floorf((x0 < x1 ? x0 : x1) - radius - 1.0f));
            int32_t maxX = static_cast<int32_t>(ceilf((x0 > x1 ? x0 : x1) + radius + 1.0f));
            int32_t minY = static_cast<int32_t>(floorf((y0 < y1 ? y0 : y1) - radius - 1.0f));
            int32_t maxY = static_cast<int32_t>(ceilf((y0 > y1 ? y0 : y1) + radius + 1.0f));
            if (minX < s.clipX)
                minX = s.clipX;
            if (maxX > s.clipR)
                maxX = s.clipR;
            if (minY < s.clipY)
                minY = s.clipY;
            if (maxY > s.clipB)
                maxY = s.clipB;
            if (minX > maxX || minY > maxY)
                return;

            const float vx = static_cast<float>(x1 - x0);
            const float vy = static_cast<float>(y1 - y0);
            const float len2 = vx * vx + vy * vy;
            if (len2 <= 0.0f)
            {
                rasterArcCapAA(spr, static_cast<float>(x0), static_cast<float>(y0), radius, color565);
                return;
            }
            const float invLen2 = 1.0f / len2;

            const float subStep = 1.0f / static_cast<float>(kArcSubSamples);
            const float subBias = 0.5f * subStep;
            const uint8_t *gamma = gammaTable();
            const Color565 c = makeColor565(color565);

            for (int32_t py = minY; py <= maxY; ++py)
            {
                for (int32_t px = minX; px <= maxX; ++px)
                {
                    int covered = 0;
                    for (int sy = 0; sy < kArcSubSamples; ++sy)
                    {
                        const float sampleY = (static_cast<float>(py) - 0.5f) + subBias + static_cast<float>(sy) * subStep;
                        for (int sx = 0; sx < kArcSubSamples; ++sx)
                        {
                            const float sampleX = (static_cast<float>(px) - 0.5f) + subBias + static_cast<float>(sx) * subStep;
                            float t = ((sampleX - static_cast<float>(x0)) * vx + (sampleY - static_cast<float>(y0)) * vy) * invLen2;
                            if (t < 0.0f)
                            {
                                if (!roundStart)
                                    continue;
                                t = 0.0f;
                            }
                            else if (t > 1.0f)
                            {
                                if (!roundEnd)
                                    continue;
                                t = 1.0f;
                            }
                            const float dx = sampleX - (static_cast<float>(x0) + vx * t);
                            const float dy = sampleY - (static_cast<float>(y0) + vy * t);
                            if (dx * dx + dy * dy <= radius2)
                                ++covered;
                        }
                    }

                    if (covered == 0)
                        continue;

                    uint16_t *dst = s.buf + static_cast<int32_t>(py) * s.stride + px;
                    if (covered >= kArcSubSampleCount)
                        *dst = c.fg;
                    else
                        blendStore(dst, c, gamma[(covered * 255) / kArcSubSampleCount]);
                }
            }
        }
        template <bool Fill>
        static inline const uint8_t *tinyCircleMask(uint8_t r, uint8_t &outSize)
        {
            static uint8_t *fillMasks[6] = {};
            static uint8_t *drawMasks[6] = {};
            static bool ready[2][6] = {};

            if (r == 0 || r > 5)
            {
                outSize = 0;
                return nullptr;
            }

            uint8_t **masks = Fill ? fillMasks : drawMasks;
            bool *state = ready[Fill ? 1 : 0];
            if (!state[r])
            {
                if (!masks[r])
                {
                    masks[r] = static_cast<uint8_t *>(malloc(11 * 11));
                    if (!masks[r])
                    {
                        outSize = 0;
                        return nullptr;
                    }
                }
                constexpr int kSub = 8;
                constexpr float kInvSubArea = 255.0f / (float)(kSub * kSub);
                const int size = r * 2 + 1;
                const float outerR = (float)r + 0.5f;
                const float innerR = Fill ? -1.0f : std::max(0.0f, (float)r - 0.5f);
                const float subStep = 1.0f / (float)kSub;
                const float subBias = 0.5f * subStep;

                for (int py = 0; py < size; ++py)
                {
                    for (int px = 0; px < size; ++px)
                    {
                        int covered = 0;
                        for (int sy = 0; sy < kSub; ++sy)
                        {
                            const float y = ((float)py - (float)r - 0.5f) + subBias + (float)sy * subStep;
                            for (int sx = 0; sx < kSub; ++sx)
                            {
                                const float x = ((float)px - (float)r - 0.5f) + subBias + (float)sx * subStep;
                                const float d2 = x * x + y * y;
                                if constexpr (Fill)
                                {
                                    if (d2 <= outerR * outerR)
                                        ++covered;
                                }
                                else
                                {
                                    if (d2 <= outerR * outerR && d2 >= innerR * innerR)
                                        ++covered;
                                }
                            }
                        }
                        masks[r][py * 11 + px] = (uint8_t)(covered * kInvSubArea + 0.5f);
                    }
                }
                state[r] = true;
            }

            outSize = (uint8_t)(r * 2 + 1);
            return masks[r];
        }
    }

    void GUI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, uint16_t color)
    {
        drawLineCore(x0, y0, x1, y1, thickness, color, true, true, true);
    }

    void GUI::drawLineCore(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           uint8_t thickness, uint16_t color,
                           bool roundStart, bool roundEnd, bool invalidate)
    {
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;

        if (thickness < 1)
            thickness = 1;

        if (thickness > 1)
        {
            rasterThickLineAA(spr, x0, y0, x1, y1, thickness, color, roundStart, roundEnd);
            if (invalidate && _disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            {
                const int16_t pad = static_cast<int16_t>(thickness / 2 + 2);
                invalidateRect((int16_t)(std::min(x0, x1) - pad), (int16_t)(std::min(y0, y1) - pad),
                               (int16_t)(std::abs(x1 - x0) + pad * 2 + 1), (int16_t)(std::abs(y1 - y0) + pad * 2 + 1));
            }
            return;
        }

        if (x0 == x1 && y0 == y1)
        {
            spr->drawPixel(x0, y0, color);
            return;
        }
        if (y0 == y1)
        {
            if (x1 < x0)
                std::swap(x0, x1);
            fillRect(x0, y0, (int16_t)(x1 - x0 + 1), 1, color);
            return;
        }
        if (x0 == x1)
        {
            if (y1 < y0)
                std::swap(y0, y1);
            fillRect(x0, y0, 1, (int16_t)(y1 - y0 + 1), color);
            return;
        }

        auto *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;
        const int16_t stride = spr->width();
        if (stride <= 0 || spr->height() <= 0)
            return;

        int32_t clipX = 0;
        int32_t clipY = 0;
        int32_t clipW = stride;
        int32_t clipH = spr->height();
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;
        const int32_t clipR = clipX + clipW - 1;
        const int32_t clipB = clipY + clipH - 1;

        const int16_t origX0 = x0;
        const int16_t origY0 = y0;
        const int16_t origX1 = x1;
        const int16_t origY1 = y1;

        const int dx0 = x1 - x0;
        const int dy0 = y1 - y0;
        const int adx0 = abs(dx0);
        const int ady0 = abs(dy0);
        const bool steep = ady0 > adx0;
        if (steep)
        {
            if (y0 > y1)
            {
                std::swap(x0, x1);
                std::swap(y0, y1);
            }
        }
        else
        {
            if (x0 > x1)
            {
                std::swap(x0, x1);
                std::swap(y0, y1);
            }
        }

        const int dx = x1 - x0;
        const int dy = y1 - y0;
        const int adx = abs(dx);
        const int ady = abs(dy);
        const uint32_t major = (uint32_t)(steep ? ady : adx);
        const uint32_t len = isqrt32((uint32_t)(dx * dx + dy * dy));
        if (major == 0 || len == 0)
            return;

        const int minX = (x0 < x1 ? x0 : x1), maxX = (x0 > x1 ? x0 : x1);
        const int minY = (y0 < y1 ? y0 : y1), maxY = (y0 > y1 ? y0 : y1);
        if (maxX < clipX - 1 || minX > clipR + 1 || maxY < clipY - 1 || minY > clipB + 1)
            return;

        const bool noClip = (minX >= clipX + 1 && maxX <= clipR - 1 && minY >= clipY + 1 && maxY <= clipB - 1);

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const uint8_t *curve = coverageTable();
        const uint32_t w256 = std::max<uint32_t>(1u, ((major << 8) + (len >> 1)) / len);

        auto blendFastPtr = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            blendStore(ptr, c, alpha);
        };

        auto blendFastClip = [&](int px, int py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
                blendFastPtr(buf + py * stride + px, alpha);
        };
        if (steep)
        {
            const int32_t step = (int32_t)(((int64_t)dx << 16) / (int32_t)ady);
            if (noClip)
                rasterAALine<true>(y0, y1, x0, step, w256, curve, gamma,
                                   [&](int32_t px, int32_t py, uint8_t alpha)
                                   { blendFastPtr(buf + py * stride + px, alpha); });
            else
                rasterAALine<true>(y0, y1, x0, step, w256, curve, gamma, blendFastClip);
        }
        else
        {
            const int32_t step = (int32_t)(((int64_t)dy << 16) / (int32_t)adx);
            if (noClip)
                rasterAALine<false>(x0, x1, y0, step, w256, curve, gamma,
                                    [&](int32_t px, int32_t py, uint8_t alpha)
                                    { blendFastPtr(buf + py * stride + px, alpha); });
            else
                rasterAALine<false>(x0, x1, y0, step, w256, curve, gamma, blendFastClip);
        }

        if (invalidate && _disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(std::min(origX0, origX1), std::min(origY0, origY1),
                           std::abs(origX1 - origX0) + 1, std::abs(origY1 - origY0) + 1);
    }

    void GUI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);
        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr || !spr->getBuffer())
            return;

        int32_t clipX = 0;
        int32_t clipY = 0;
        int32_t clipW = 0;
        int32_t clipH = 0;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        int32_t x0 = x;
        int32_t y0 = y;
        int32_t x1 = x0 + w;
        int32_t y1 = y0 + h;
        const int32_t clipR = clipX + clipW;
        const int32_t clipB = clipY + clipH;

        if (x0 < clipX)
            x0 = clipX;
        if (y0 < clipY)
            y0 = clipY;
        if (x1 > clipR)
            x1 = clipR;
        if (y1 > clipB)
            y1 = clipB;
        if (x0 >= x1 || y0 >= y1)
            return;

        const int16_t drawX = (int16_t)x0;
        const int16_t drawY = (int16_t)y0;
        const int16_t drawW = (int16_t)(x1 - x0);
        const int16_t drawH = (int16_t)(y1 - y0);

        spr->fillRect(drawX, drawY, drawW, drawH, color);

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(drawX, drawY, drawW, drawH);
    }

    void GUI::fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color565)
    {
        auto spr = getDrawTarget();
        if (!_flags.spriteEnabled || r <= 0 || !spr)
            return;
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx - r > s.clipR || cx + r < s.clipX || cy - r > s.clipB || cy + r < s.clipY)
            return;
        const Color565 c = makeColor565(color565);
        const bool noClip = (cx - r >= s.clipX && cx + r <= s.clipR && cy - r >= s.clipY && cy + r <= s.clipB);
        const uint8_t *gamma = gammaTable();

        if (r <= 5)
        {
            uint8_t maskSize = 0;
            const uint8_t *mask = tinyCircleMask<true>((uint8_t)r, maskSize);
            if (mask)
            {
                const int16_t x0 = (int16_t)(cx - r);
                const int16_t y0 = (int16_t)(cy - r);
                for (uint8_t py = 0; py < maskSize; ++py)
                {
                    const int16_t y = (int16_t)(y0 + py);
                    if (y < s.clipY || y > s.clipB)
                        continue;
                    uint16_t *row = s.buf + (int32_t)y * s.stride;
                    for (uint8_t px = 0; px < maskSize; ++px)
                    {
                        const uint8_t alpha = mask[(uint16_t)py * 11 + px];
                        if (!alpha)
                            continue;
                        const int16_t x = (int16_t)(x0 + px);
                        if (x < s.clipX || x > s.clipR)
                            continue;
                        if (alpha == 255)
                            row[x] = c.fg;
                        else
                            blendStore(row + x, c, gamma[alpha]);
                    }
                }
                if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
                    invalidateRect((int16_t)(cx - r), (int16_t)(cy - r),
                                   (int16_t)(r * 2 + 1), (int16_t)(r * 2 + 1));
                return;
            }
        }

        if (noClip)
            fillSpanFast(s, cy, cx - r, cx + r, c);
        else
            fillSpanClip(s, cy, cx - r, cx + r, c);

        if (noClip)
            rasterAAFillRoundCore(cx, cy, r, 0, 0, [&](int16_t px, int16_t py, uint8_t alpha)
                                  { blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha); }, [&](int16_t py, int16_t x0, int16_t x1)
                                  { fillSpanFast(s, py, x0, x1, c); });
        else
            rasterAAFillRoundCore(cx, cy, r, 0, 0, [&](int16_t px, int16_t py, uint8_t alpha)
                                  { plotBlendClip(s, c, px, py, alpha); }, [&](int16_t py, int16_t x0, int16_t x1)
                                  { fillSpanClip(s, py, x0, x1, c); });

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect((int16_t)(cx - r), (int16_t)(cy - r),
                           (int16_t)(r * 2 + 1), (int16_t)(r * 2 + 1));
    }

    void GUI::drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0)
            return;
        auto spr = getDrawTarget();
        Surface565 s;
        if (spr && getSurface565(spr, s) && r <= 5)
        {
            uint8_t maskSize = 0;
            const uint8_t *mask = tinyCircleMask<false>((uint8_t)r, maskSize);
            if (mask)
            {
                const Color565 c = makeColor565(color);
                const uint8_t *gamma = gammaTable();
                const int16_t x0 = (int16_t)(cx - r);
                const int16_t y0 = (int16_t)(cy - r);
                for (uint8_t py = 0; py < maskSize; ++py)
                {
                    const int16_t y = (int16_t)(y0 + py);
                    if (y < s.clipY || y > s.clipB)
                        continue;
                    uint16_t *row = s.buf + (int32_t)y * s.stride;
                    for (uint8_t px = 0; px < maskSize; ++px)
                    {
                        const uint8_t alpha = mask[(uint16_t)py * 11 + px];
                        if (!alpha)
                            continue;
                        const int16_t x = (int16_t)(x0 + px);
                        if (x < s.clipX || x > s.clipR)
                            continue;
                        if (alpha == 255)
                            row[x] = c.fg;
                        else
                            blendStore(row + x, c, gamma[alpha]);
                    }
                }
                if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
                    invalidateRect((int16_t)(cx - r), (int16_t)(cy - r),
                                   (int16_t)(r * 2 + 1), (int16_t)(r * 2 + 1));
                return;
            }
        }
        const uint8_t rr = (r > 255) ? (uint8_t)255 : (uint8_t)r;
        const int16_t d = (int16_t)(rr * 2 + 1);
        drawRoundRect((int16_t)(cx - rr), (int16_t)(cy - rr), d, d, rr, color);
    }

    void GUI::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radiusTL, uint8_t radiusTR,
                            uint8_t radiusBR, uint8_t radiusBL, uint16_t color565)
    {
        if (!_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        const int16_t maxR = (w < h ? w : h) / 2;
        const uint8_t rTL = (radiusTL > maxR) ? (uint8_t)maxR : radiusTL;
        const uint8_t rTR = (radiusTR > maxR) ? (uint8_t)maxR : radiusTR;
        const uint8_t rBR = (radiusBR > maxR) ? (uint8_t)maxR : radiusBR;
        const uint8_t rBL = (radiusBL > maxR) ? (uint8_t)maxR : radiusBL;

        if (rTL == 0 && rTR == 0 && rBR == 0 && rBL == 0)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (x > s.clipR || x + w - 1 < s.clipX || y > s.clipB || y + h - 1 < s.clipY)
            return;
        const bool noClip = (x >= s.clipX && y >= s.clipY && x + w - 1 <= s.clipR && y + h - 1 <= s.clipB);
        const Color565 c = makeColor565(color565);
        const uint8_t *gamma = gammaTable();
        auto fillSpan = [&](int16_t py, int16_t x0, int16_t x1) __attribute__((always_inline))
        {
            if (noClip)
                fillSpanFast(s, py, x0, x1, c);
            else
                fillSpanClip(s, py, x0, x1, c);
        };

        if (rTL == rTR && rTR == rBR && rBR == rBL && h >= (int16_t)(rTL * 2))
        {
            int32_t rr = (int32_t)rTL;
            if (rr > w / 2)
                rr = w / 2;
            if (rr > h / 2)
                rr = h / 2;

            const int16_t yy = (int16_t)(y + rr);
            const int16_t hh = (int16_t)(h - 2 * rr);
            if (hh > 0)
                spr->fillRect(x, yy, w, hh, color565);
            else
            {
                const int16_t midY = static_cast<int16_t>(y + rr - 1);
                const int16_t midH = static_cast<int16_t>((h & 1) ? 1 : 2);
                if (midH > 0)
                    spr->fillRect(x, midY, w, midH, color565);
            }

            const int32_t hx = (hh > 0) ? hh - 1 : 0;
            const int32_t xx = x + rr;
            const int32_t ww = (w - 2 * rr - 1 > 0) ? w - 2 * rr - 1 : 0;

            if (noClip)
                rasterAAFillRoundCore(xx, yy, rr, ww, hx, [&](int16_t px, int16_t py, uint8_t alpha)
                                      { blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha); }, fillSpan);
            else
                rasterAAFillRoundCore(xx, yy, rr, ww, hx, [&](int16_t px, int16_t py, uint8_t alpha)
                                      { plotBlendClip(s, c, px, py, alpha); }, fillSpan);
        }
        else
        {
            struct FillRoundEdge
            {
                uint8_t topRadius = 0;
                uint8_t bottomRadius = 0;
                int16_t topCenterX = 0;
                int16_t bottomCenterX = 0;
                int16_t topEndY = 0;
                int16_t bottomStartY = 0;
            };

            const int16_t py0 = (y < s.clipY) ? (int16_t)s.clipY : y;
            const int16_t py1 = (y + h - 1 > s.clipB) ? (int16_t)s.clipB : (int16_t)(y + h - 1);
            const FillRoundEdge leftEdge = {
                rTL,
                rBL,
                (int16_t)(x + rTL),
                (int16_t)(x + rBL),
                (int16_t)(y + rTL - 1),
                (int16_t)(y + h - rBL)};
            const FillRoundEdge rightEdge = {
                rTR,
                rBR,
                (int16_t)(x + w - rTR - 1),
                (int16_t)(x + w - rBR - 1),
                (int16_t)(y + rTR - 1),
                (int16_t)(y + h - rBR)};
            auto applyEdge = [&](uint8_t rEdge, int16_t ccx, int32_t dy, int8_t dir,
                                 int16_t &solidX, uint16_t *row) __attribute__((always_inline))
            {
                if (!rEdge)
                    return;
                const int32_t r2 = (int32_t)rEdge * rEdge;
                const int32_t dy2 = dy * dy;
                if (dy2 > r2)
                    return;
                const int16_t solid_dx = (int16_t)isqrt32((uint32_t)(r2 - dy2));
                solidX = ccx + dir * solid_dx;
                const uint8_t frac = fracAlphaFromResidual((int64_t)r2 - ((int64_t)solid_dx * solid_dx + dy2),
                                                           (int64_t)(2 * solid_dx + 1));
                if (!frac)
                    return;
                const int16_t px = (int16_t)(ccx + dir * (solid_dx + 1));
                if (noClip || (px >= s.clipX && px <= s.clipR))
                    blendStoreGamma(row + px, c, gamma, frac);
            };

            for (int16_t py = py0; py <= py1; ++py)
            {
                uint16_t *row = s.buf + (int32_t)py * s.stride;
                int16_t solid_x0 = x, solid_x1 = x + w - 1;

                if (leftEdge.topRadius && py <= leftEdge.topEndY)
                    applyEdge(leftEdge.topRadius, leftEdge.topCenterX, (int32_t)(leftEdge.topCenterX - x - (py - y)), -1, solid_x0, row);
                else if (leftEdge.bottomRadius && py >= leftEdge.bottomStartY)
                    applyEdge(leftEdge.bottomRadius, leftEdge.bottomCenterX, (int32_t)(py - (y + h - leftEdge.bottomRadius - 1)), -1, solid_x0, row);

                if (rightEdge.topRadius && py <= rightEdge.topEndY)
                    applyEdge(rightEdge.topRadius, rightEdge.topCenterX, (int32_t)(rightEdge.topCenterX - (x + w - rightEdge.topRadius - 1) + (y + rightEdge.topRadius - py)), 1, solid_x1, row);
                else if (rightEdge.bottomRadius && py >= rightEdge.bottomStartY)
                    applyEdge(rightEdge.bottomRadius, rightEdge.bottomCenterX, (int32_t)(py - (y + h - rightEdge.bottomRadius - 1)), 1, solid_x1, row);
                fillSpan(py, solid_x0, solid_x1);
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(x, y, w, h);
    }

    void GUI::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radius, uint16_t color565)
    {
        fillRoundRect(x, y, w, h, radius, radius, radius, radius, color565);
    }

    void GUI::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radiusTL, uint8_t radiusTR,
                            uint8_t radiusBR, uint8_t radiusBL, uint16_t color565)
    {
        if (!_flags.spriteEnabled || w <= 0 || h <= 0)
            return;
        if (w <= 2 || h <= 2)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (x > s.clipR || x + w - 1 < s.clipX || y > s.clipB || y + h - 1 < s.clipY)
            return;
        const bool noClip = (x >= s.clipX && y >= s.clipY && x + w - 1 <= s.clipR && y + h - 1 <= s.clipB);

        const int16_t maxR = (w < h ? w : h) / 2;
        const uint8_t rTL = (radiusTL > maxR) ? (uint8_t)maxR : radiusTL;
        const uint8_t rTR = (radiusTR > maxR) ? (uint8_t)maxR : radiusTR;
        const uint8_t rBR = (radiusBR > maxR) ? (uint8_t)maxR : radiusBR;
        const uint8_t rBL = (radiusBL > maxR) ? (uint8_t)maxR : radiusBL;

        const Color565 c = makeColor565(color565);
        const uint8_t *gamma = gammaTable();
        auto fillSpan = [&](int16_t py, int16_t x0, int16_t x1) __attribute__((always_inline))
        {
            if (noClip)
                fillSpanFast(s, py, x0, x1, c);
            else
                fillSpanClip(s, py, x0, x1, c);
        };
        auto fillVLine = [&](int16_t px, int16_t py0, int16_t py1) __attribute__((always_inline))
        {
            if (noClip)
                fillVLineFast(s, px, py0, py1, c.fg);
            else
                fillVLineClip(s, px, py0, py1, c.fg);
        };

        const int16_t top_x1 = x + rTL, top_x2 = x + w - rTR - 1;
        const int16_t bot_x1 = x + rBL, bot_x2 = x + w - rBR - 1;
        if (top_x1 <= top_x2)
            fillSpan(y, top_x1, top_x2);
        if (bot_x1 <= bot_x2)
            fillSpan((int16_t)(y + h - 1), bot_x1, bot_x2);

        const int16_t left_y1 = y + (rTL > 0 ? rTL : 1), left_y2 = y + h - (rBL > 0 ? rBL : 1) - 1;
        const int16_t right_y1 = y + (rTR > 0 ? rTR : 1), right_y2 = y + h - (rBR > 0 ? rBR : 1) - 1;
        if (left_y1 <= left_y2)
            fillVLine(x, left_y1, left_y2);
        if (right_y1 <= right_y2)
            fillVLine((int16_t)(x + w - 1), right_y1, right_y2);

        if (rTL == rTR && rTR == rBL && rBL == rBR && rTL > 0)
        {
            int32_t r = (int32_t)rTL;
            const int32_t ww = (w - 2 * r > 0) ? w - 2 * r : 0;
            const int32_t hh = (h - 2 * r > 0) ? h - 2 * r : 0;
            const int32_t x0 = x + r, y0 = y + r;
            const int16_t t = (r > 1) ? 2 : 1;

            if (noClip)
                rasterAARingRoundCore(x0, y0, r, ww, hh, t, [&](int16_t px, int16_t py, uint8_t alpha)
                                      { blendStore(s.buf + (int32_t)py * s.stride + px, c, gamma[alpha]); }, fillSpan, fillVLine);
            else
                rasterAARingRoundCore(x0, y0, r, ww, hh, t, [&](int16_t px, int16_t py, uint8_t alpha)
                                      { plotBlendClipGamma(s, c, gamma, px, py, alpha); }, fillSpan, fillVLine);
        }
        else
        {
            auto drawCorner = [&](int16_t ccx, int16_t ccy,
                                  int16_t px_s, int16_t px_e,
                                  int16_t py_s, int16_t py_e, uint8_t r)
            {
                if (r == 0)
                    return;
                const bool leftSide = px_s <= ccx;
                if (px_s < s.clipX)
                    px_s = s.clipX;
                if (px_e > s.clipR)
                    px_e = s.clipR;
                if (py_s < s.clipY)
                    py_s = s.clipY;
                if (py_e > s.clipB)
                    py_e = s.clipB;
                if (px_s > px_e || py_s > py_e)
                    return;
                const float fr = (float)r;
                const float r_out2 = (fr + 0.5f) * (fr + 0.5f), inv_2r_out = 0.5f / (fr + 0.5f);
                const float r_in2 = (fr - 0.5f) * (fr - 0.5f), inv_2r_in = 0.5f / (fr - 0.5f);

                for (int16_t py = py_s; py <= py_e; ++py)
                {
                    uint16_t *row = s.buf + (int32_t)py * s.stride;
                    const float dy2 = (float)((ccy - py) * (ccy - py));

                    for (int16_t px = px_s; px <= px_e; ++px)
                    {
                        const float dx = (float)(ccx - px);
                        const float S = dx * dx + dy2;
                        const float d_out = (r_out2 - S) * inv_2r_out;
                        const float d_in = (r_in2 - S) * inv_2r_in;

                        if (leftSide)
                        {
                            if (d_out <= -0.5f)
                                continue;
                            if (d_in >= 0.5f)
                                break;
                        }
                        else
                        {
                            if (d_in >= 0.5f)
                                continue;
                            if (d_out <= -0.5f)
                                break;
                        }

                        uint8_t a_out = 255;
                        if (d_out < 0.5f)
                        {
                            const float t = d_out + 0.5f;
                            a_out = (uint8_t)(t * t * (765.0f - 510.0f * t));
                        }
                        uint8_t a_in = 0;
                        if (d_in > -0.5f)
                        {
                            const float t = d_in + 0.5f;
                            a_in = (uint8_t)(t * t * (765.0f - 510.0f * t));
                        }

                        const uint8_t alpha = (a_out > a_in) ? (a_out - a_in) : 0;
                        if (alpha == 255)
                            row[px] = c.fg;
                        else if (alpha > 0)
                            blendStore(row + px, c, gamma[alpha]);
                    }
                }
            };

            drawCorner(x + rTL, y + rTL, x, x + rTL - 1, y, y + rTL - 1, rTL);
            drawCorner(x + w - rTR - 1, y + rTR, x + w - rTR, x + w - 1, y, y + rTR - 1, rTR);
            drawCorner(x + rBL, y + h - rBL - 1, x, x + rBL - 1, y + h - rBL, y + h - 1, rBL);
            drawCorner(x + w - rBR - 1, y + h - rBR - 1, x + w - rBR, x + w - 1, y + h - rBR, y + h - 1, rBR);
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(x, y, w, h);
    }

    void GUI::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radius, uint16_t color)
    {
        drawRoundRect(x, y, w, h, radius, radius, radius, radius, color);
    }

    void GUI::drawArc(int16_t cx, int16_t cy, int16_t r,
                      uint8_t thickness,
                      float startDeg, float endDeg, uint16_t color)
    {
        drawArcShaded(cx, cy, r, thickness, startDeg, endDeg, solidArcColorAtAngle, &color, false, true);
    }

    void GUI::drawArcShaded(int16_t cx, int16_t cy, int16_t r,
                            uint8_t thickness,
                            float startDeg, float endDeg,
                            uint16_t (*shader)(const void *, float),
                            const void *shaderCtx,
                            bool needsColorAngle,
                            bool invalidate)
    {
        if (r <= 0 || !_flags.spriteEnabled || !shader)
            return;

        const float rawSweep = fabsf(endDeg - startDeg);
        const bool fullSweep = rawSweep >= 359.5f;
        if (thickness < 1)
            thickness = 1;

        auto spr = getDrawTarget();
        if (!spr)
            return;
        if (fullSweep)
            rasterThickArcAA(spr, cx, cy, r, thickness, true, 0.0f, 360.0f, [&](float deg) -> uint16_t
                             { return shader(shaderCtx, deg); }, needsColorAngle);
        else
        {
            rasterThickArcAA(spr, cx, cy, r, thickness, false, startDeg, endDeg, [&](float deg) -> uint16_t
                             { return shader(shaderCtx, deg); }, needsColorAngle);
            drawThickArcCaps(spr, cx, cy, r, thickness, startDeg, endDeg,
                             [&](float deg) -> uint16_t
                             { return shader(shaderCtx, deg); });
        }

        if (invalidate && _disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 1, cy - r - 1, r * 2 + 3, r * 2 + 3);
    }
}

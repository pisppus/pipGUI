#include "Internal.hpp"

namespace pipgui
{
    namespace
    {
        static inline uint8_t clampSquircleRadius(int16_t w, int16_t h, uint8_t radius) noexcept
        {
            const int16_t maxRadius = ((w < h) ? w : h) / 2;
            return (radius > maxRadius) ? static_cast<uint8_t>(maxRadius) : radius;
        }

        static inline bool isFullSquareSquircleRect(int16_t w, int16_t h,
                                                    uint8_t rTL, uint8_t rTR, uint8_t rBR, uint8_t rBL) noexcept
        {
            return w == h &&
                   rTL == rTR && rTR == rBR && rBR == rBL &&
                   w == static_cast<int16_t>(rTL * 2 + 1);
        }

        static inline void fillSurfaceRect(const Surface565 &s, const Color565 &c,
                                           int16_t x, int16_t y, int16_t w, int16_t h) noexcept
        {
            if (w <= 0 || h <= 0)
                return;
            if (x > s.clipR || x + w - 1 < s.clipX || y > s.clipB || y + h - 1 < s.clipY)
                return;

            const bool noClip = (x >= s.clipX && y >= s.clipY &&
                                 x + w - 1 <= s.clipR && y + h - 1 <= s.clipB);
            if (noClip)
            {
                for (int16_t py = y, pyEnd = static_cast<int16_t>(y + h - 1); py <= pyEnd; ++py)
                    fillSpanFast(s, py, x, static_cast<int16_t>(x + w - 1), c);
                return;
            }

            const int16_t py0 = (y < s.clipY) ? static_cast<int16_t>(s.clipY) : y;
            const int16_t py1 = (y + h - 1 > s.clipB) ? static_cast<int16_t>(s.clipB) : static_cast<int16_t>(y + h - 1);
            for (int16_t py = py0; py <= py1; ++py)
                fillSpanClip(s, py, x, static_cast<int16_t>(x + w - 1), c);
        }

        template <typename AccT>
        static inline void fillSquircleCorner(const Surface565 &s, const Color565 &c, const uint8_t *gamma,
                                              int16_t cx, int16_t cy, uint8_t radius,
                                              int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH)
        {
            if (radius == 0 || clipW <= 0 || clipH <= 0)
                return;

            Surface565 corner = s;
            corner.clipX = clipX;
            corner.clipY = clipY;
            corner.clipR = static_cast<int32_t>(clipX + clipW - 1);
            corner.clipB = static_cast<int32_t>(clipY + clipH - 1);
            squircleRaster<true, AccT>(corner, c, cx, cy, radius, gamma, false);
        }

        template <typename AccT>
        static inline void drawSquircleCorner(const Surface565 &s, const Color565 &c, const uint8_t *gamma,
                                              int16_t cx, int16_t cy, uint8_t radius,
                                              int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH)
        {
            if (radius == 0 || clipW <= 0 || clipH <= 0)
                return;

            Surface565 corner = s;
            corner.clipX = clipX;
            corner.clipY = clipY;
            corner.clipR = static_cast<int32_t>(clipX + clipW - 1);
            corner.clipB = static_cast<int32_t>(clipY + clipH - 1);
            squircleRaster<false, AccT>(corner, c, cx, cy, radius, gamma, false);
        }

        static inline float distanceSqToSegmentProjected(float dx, float dy,
                                                         float ex, float ey,
                                                         float invLen,
                                                         float dot)
        {
            float t = dot * invLen;
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            dx -= ex * t;
            dy -= ey * t;
            return dx * dx + dy * dy;
        }

        struct DistSqAlphaLut
        {
            uint8_t radius = 0xFF;
            float minSq = 0.0f;
            float maxSq = 1.0f;
            float scale = 0.0f;
            uint8_t table[513] = {};
        };

        static inline uint8_t sampleDistSqAlpha(const DistSqAlphaLut &lut, float dSq)
        {
            if (dSq <= lut.minSq)
                return lut.table[0];
            if (dSq >= lut.maxSq)
                return lut.table[512];

            int32_t idx = (int32_t)((dSq - lut.minSq) * lut.scale + 0.5f);
            if (idx < 0)
                idx = 0;
            else if (idx > 512)
                idx = 512;
            return lut.table[idx];
        }

        template <typename AlphaFn>
        static inline const DistSqAlphaLut &buildRoundTriangleLut(DistSqAlphaLut &lut,
                                                                  uint8_t radius,
                                                                  float minRadius,
                                                                  float maxRadius,
                                                                  AlphaFn alphaFn)
        {
            if (lut.radius == radius)
                return lut;

            lut.radius = radius;
            lut.minSq = minRadius * minRadius;
            lut.maxSq = maxRadius * maxRadius;
            lut.scale = (lut.maxSq > lut.minSq) ? (512.0f / (lut.maxSq - lut.minSq)) : 0.0f;

            const float step = (lut.maxSq - lut.minSq) * (1.0f / 512.0f);
            for (int32_t i = 0; i <= 512; ++i)
            {
                const float dSq = lut.minSq + step * i;
                lut.table[i] = alphaFn(dSq);
            }
            return lut;
        }

        static inline const DistSqAlphaLut &roundTriangleOutlineLut(uint8_t radius)
        {
            static DistSqAlphaLut lut;
            const float bandMin = std::max(0.0f, radius - 1.0f);
            const float bandMax = radius + 1.0f;
            return buildRoundTriangleLut(
                lut, radius, bandMin, bandMax,
                [&](float dSq)
                {
                    const float edgeDist = fabsf(sqrtf(dSq) - radius) - 0.5f;
                    return alphaSdfAA(edgeDist);
                });
        }

        static inline const DistSqAlphaLut &roundTriangleFillLut(uint8_t radius)
        {
            static DistSqAlphaLut lut;
            const float innerRadius = std::max(0.0f, radius - 0.5f);
            const float outerRadius = radius + 1.0f;
            return buildRoundTriangleLut(
                lut, radius, innerRadius, outerRadius,
                [&](float dSq)
                { return alphaSdfAA(sqrtf(dSq) - radius); });
        }
    }

    void GUI::fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx + rx < s.clipX || cx - rx > s.clipR || cy + ry < s.clipY || cy - ry > s.clipB)
            return;

        const int64_t rx2 = (int64_t)rx * rx;
        const int64_t ry2 = (int64_t)ry * ry;
        const int64_t rhs = rx2 * ry2;
        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const bool noClip = (cx - rx - 1 >= s.clipX && cx + rx + 1 <= s.clipR &&
                             cy - ry >= s.clipY && cy + ry <= s.clipB);
        auto raster = [&](auto fillSpan, auto blendSide) __attribute__((always_inline))
        {
            int32_t xi = rx;
            int64_t xTerm = (int64_t)xi * xi * ry2;
            int64_t yTerm = 0;
            for (int16_t dy = 0; dy <= ry; ++dy)
            {
                while (xi > 0 && xTerm + yTerm > rhs)
                {
                    xTerm -= (int64_t)(xi * 2 - 1) * ry2;
                    --xi;
                }

                const int16_t py0 = (int16_t)(cy - dy), py1 = (int16_t)(cy + dy);
                const int16_t x0 = (int16_t)(cx - xi), x1 = (int16_t)(cx + xi);
                const uint8_t ag = gamma[fracAlphaFromResidual(rhs - (xTerm + yTerm), (int64_t)(2 * xi + 1) * ry2)];
                fillSpan(py0, x0, x1);
                blendSide((int16_t)(x1 + 1), py0, ag);
                blendSide((int16_t)(x0 - 1), py0, ag);
                if (dy)
                {
                    fillSpan(py1, x0, x1);
                    blendSide((int16_t)(x1 + 1), py1, ag);
                    blendSide((int16_t)(x0 - 1), py1, ag);
                }

                yTerm += (int64_t)(dy * 2 + 1) * rx2;
            }
        };

        if (noClip)
            raster([&](int16_t py, int16_t x0, int16_t x1)
                   { fillSpanFast(s, py, x0, x1, c); },
                   [&](int16_t px, int16_t py, uint8_t alpha)
                   {
                       if (alpha)
                           blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha);
                   });
        else
            raster([&](int16_t py, int16_t x0, int16_t x1)
                   { fillSpanClip(s, py, x0, x1, c); },
                   [&](int16_t px, int16_t py, uint8_t alpha)
                   {
                       if (alpha)
                           plotBlendClip(s, c, px, py, alpha);
                   });

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - rx, cy - ry, rx * 2 + 1, ry * 2 + 1);
    }

    void GUI::drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx + rx + 1 < s.clipX || cx - rx - 1 > s.clipR ||
            cy + ry + 1 < s.clipY || cy - ry - 1 > s.clipB)
            return;

        const int64_t rx2 = (int64_t)rx * rx;
        const int64_t ry2 = (int64_t)ry * ry;
        const int64_t rhs = rx2 * ry2;
        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const bool noClip = (cx - rx - 1 >= s.clipX && cx + rx + 1 <= s.clipR &&
                             cy - ry - 1 >= s.clipY && cy + ry + 1 <= s.clipB);
        auto raster = [&](auto plot) __attribute__((always_inline))
        {
            auto plot4 = [&](int16_t px0, int16_t px1, int16_t py0, int16_t py1, uint8_t alpha) __attribute__((always_inline))
            {
                plot(px0, py0, alpha);
                plot(px1, py0, alpha);
                plot(px0, py1, alpha);
                plot(px1, py1, alpha);
            };

            const uint32_t diag = isqrt32((uint32_t)(rx2 + ry2));
            int32_t yi = ry;
            int64_t yiTerm = (int64_t)yi * yi * rx2;
            const int16_t qx = (diag > 0) ? (int16_t)((rx2 + (diag >> 1)) / diag) : 0;
            int64_t xTerm = 0;
            for (int16_t dx = 0; dx <= qx; ++dx)
            {
                while (yi > 0 && xTerm + yiTerm > rhs)
                {
                    yiTerm -= (int64_t)(yi * 2 - 1) * rx2;
                    --yi;
                }

                const uint8_t frac = fracAlphaFromResidual(rhs - (xTerm + yiTerm), (int64_t)(2 * yi + 1) * rx2);
                const uint8_t a0 = gamma[255 - frac], a1 = gamma[frac];
                const int16_t x0 = (int16_t)(cx + dx), x1 = (int16_t)(cx - dx);
                const int16_t y0 = (int16_t)(cy + yi), y1 = (int16_t)(cy - yi);
                plot4(x0, x1, y0, y1, a0);
                plot4(x0, x1, (int16_t)(y0 + 1), (int16_t)(y1 - 1), a1);
                xTerm += (int64_t)(dx * 2 + 1) * ry2;
            }

            int32_t xi = rx;
            int64_t xiTerm = (int64_t)xi * xi * ry2;
            const int16_t qy = (diag > 0) ? (int16_t)((ry2 + (diag >> 1)) / diag) : 0;
            int64_t yTerm = 0;
            for (int16_t dy = 0; dy <= qy; ++dy)
            {
                while (xi > 0 && xiTerm + yTerm > rhs)
                {
                    xiTerm -= (int64_t)(xi * 2 - 1) * ry2;
                    --xi;
                }

                const uint8_t frac = fracAlphaFromResidual(rhs - (xiTerm + yTerm), (int64_t)(2 * xi + 1) * ry2);
                const uint8_t a0 = gamma[255 - frac], a1 = gamma[frac];
                const int16_t px0 = (int16_t)(cx + xi), px1 = (int16_t)(cx - xi);
                const int16_t py0 = (int16_t)(cy + dy), py1 = (int16_t)(cy - dy);
                plot(px0, py0, a0);
                plot((int16_t)(px0 + 1), py0, a1);
                plot(px1, py0, a0);
                plot((int16_t)(px1 - 1), py0, a1);
                plot(px0, py1, a0);
                plot((int16_t)(px0 + 1), py1, a1);
                plot(px1, py1, a0);
                plot((int16_t)(px1 - 1), py1, a1);
                yTerm += (int64_t)(dy * 2 + 1) * rx2;
            }
        };

        if (noClip)
            raster([&](int16_t px, int16_t py, uint8_t alpha)
                   { blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha); });
        else
            raster([&](int16_t px, int16_t py, uint8_t alpha)
                   { plotBlendClip(s, c, px, py, alpha); });

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - rx - 1, cy - ry - 1, rx * 2 + 3, ry * 2 + 3);
    }

    void GUI::drawTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2, uint16_t color)
    {
        drawLine(x0, y0, x1, y1, 1, color);
        drawLine(x1, y1, x2, y2, 1, color);
        drawLine(x2, y2, x0, y0, 1, color);
    }

    void GUI::fillTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2, uint16_t color)
    {
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;

        const int32_t cross = (int32_t)(x1 - x0) * (y2 - y0) - (int32_t)(x2 - x0) * (y1 - y0);
        if (cross == 0)
            return;

        if (y0 > y1)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        if (y0 > y2)
        {
            std::swap(x0, x2);
            std::swap(y0, y2);
        }
        if (y1 > y2)
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        if (y2 < s.clipY || y0 > s.clipB)
            return;

        const Color565 c = makeColor565(color);
        const int16_t minX = std::min({x0, x1, x2});
        const int16_t maxX = std::max({x0, x1, x2});
        const bool noClip = (minX >= s.clipX && maxX <= s.clipR && y0 >= s.clipY && y2 <= s.clipB);
        const int32_t dy02 = y2 - y0;
        if (dy02 == 0)
        {
            if (noClip)
                fillSpanFast(s, y0, minX, maxX, c);
            else
                fillSpanClip(s, y0, minX, maxX, c);
            if (_disp.display && !_flags.inSpritePass)
                invalidateRect(minX, y0, maxX - minX + 1, 1);
            return;
        }

        const int32_t step02 = ((int32_t)(x2 - x0) << 16) / dy02;
        const int32_t dy01 = y1 - y0;
        const int32_t dy12 = y2 - y1;
        const int32_t step01 = (dy01 > 0) ? (((int32_t)(x1 - x0) << 16) / dy01) : 0;
        const int32_t step12 = (dy12 > 0) ? (((int32_t)(x2 - x1) << 16) / dy12) : 0;

        const int32_t xLongAtY1 = ((int32_t)x0 << 16) + step02 * dy01;
        const bool longOnLeft = xLongAtY1 < ((int32_t)x1 << 16);

        auto rasterHalf = [&](int16_t yStart, int16_t yEnd,
                              int32_t xa, int32_t dxa,
                              int32_t xb, int32_t dxb) __attribute__((always_inline))
        {
            if (yStart < s.clipY)
            {
                const int32_t skip = s.clipY - yStart;
                xa += dxa * skip;
                xb += dxb * skip;
                yStart = s.clipY;
            }
            if (yEnd > s.clipB)
                yEnd = s.clipB;
            for (int16_t py = yStart; py <= yEnd; ++py)
            {
                int32_t xl = xa;
                int32_t xr = xb;
                if (xl > xr)
                    std::swap(xl, xr);
                const int16_t left = (int16_t)((xl + 0xFFFF) >> 16);
                const int16_t right = (int16_t)(xr >> 16);
                if (noClip)
                    fillSpanFast(s, py, left, right, c);
                else
                    fillSpanClip(s, py, left, right, c);
                xa += dxa;
                xb += dxb;
            }
        };

        if (dy01 > 0)
        {
            if (longOnLeft)
                rasterHalf(y0, (int16_t)(y1 - 1), (int32_t)x0 << 16, step02, (int32_t)x0 << 16, step01);
            else
                rasterHalf(y0, (int16_t)(y1 - 1), (int32_t)x0 << 16, step01, (int32_t)x0 << 16, step02);
        }
        if (dy12 > 0)
        {
            const int32_t xLong = ((int32_t)x0 << 16) + step02 * dy01;
            if (longOnLeft)
                rasterHalf(y1, y2, xLong, step02, (int32_t)x1 << 16, step12);
            else
                rasterHalf(y1, y2, (int32_t)x1 << 16, step12, xLong, step02);
        }

        drawTriangle(x0, y0, x1, y1, x2, y2, color);

        if (_disp.display && !_flags.inSpritePass)
        {
            invalidateRect((int16_t)(minX - 1), (int16_t)(y0 - 1),
                           (int16_t)(maxX - minX + 3), (int16_t)(y2 - y0 + 3));
        }
    }

    void GUI::drawRoundTriangle(int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2,
                                uint8_t radius, uint16_t color)
    {
        if (radius == 0)
        {
            drawTriangle(x0, y0, x1, y1, x2, y2, color);
            return;
        }
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = spr->height();
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1, clipB = clipY + clipH - 1;

        const int16_t minX = std::min({x0, x1, x2}) - radius - 2;
        const int16_t maxX = std::max({x0, x1, x2}) + radius + 2;
        const int16_t minY = std::min({y0, y1, y2}) - radius - 2;
        const int16_t maxY = std::max({y0, y1, y2}) + radius + 2;
        if (minX > clipR || maxX < clipX || minY > clipB || maxY < clipY)
            return;

        const float v0x = x0, v0y = y0, v1x = x1, v1y = y1, v2x = x2, v2y = y2;
        const float e0x = v1x - v0x, e0y = v1y - v0y, inv_len0 = 1.0f / (e0x * e0x + e0y * e0y);
        const float e1x = v2x - v1x, e1y = v2y - v1y, inv_len1 = 1.0f / (e1x * e1x + e1y * e1y);
        const float e2x = v0x - v2x, e2y = v0y - v2y, inv_len2 = 1.0f / (e2x * e2x + e2y * e2y);
        const float sign = (e0x * (v2y - v0y) - e0y * (v2x - v0x) < 0.0f) ? -1.0f : 1.0f;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const int16_t xStart = (minX < clipX) ? (int16_t)clipX : minX;
        const int16_t xEnd = (maxX > clipR) ? (int16_t)clipR : maxX;
        const int16_t yStart = (minY < clipY) ? (int16_t)clipY : minY;
        const int16_t yEnd = (maxY > clipB) ? (int16_t)clipB : maxY;
        const DistSqAlphaLut &aaLut = roundTriangleOutlineLut(radius);
        const float bandMin2 = aaLut.minSq;
        const float bandMax2 = aaLut.maxSq;

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            const float py_f = py + 0.5f;
            const float pd0y = py_f - v0y, pd1y = py_f - v1y, pd2y = py_f - v2y;
            uint16_t *row = buf + py * stride;
            float pd0x = (float)xStart + 0.5f - v0x;
            float pd1x = (float)xStart + 0.5f - v1x;
            float pd2x = (float)xStart + 0.5f - v2x;
            float c0 = e0x * pd0y - e0y * pd0x;
            float c1 = e1x * pd1y - e1y * pd1x;
            float c2 = e2x * pd2y - e2y * pd2x;
            float dot0 = pd0x * e0x + pd0y * e0y;
            float dot1 = pd1x * e1x + pd1y * e1y;
            float dot2 = pd2x * e2x + pd2y * e2y;
            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                const float side0 = c0 * sign;
                const float side1 = c1 * sign;
                const float side2 = c2 * sign;
                const bool inside = (side0 >= 0.0f) && (side1 >= 0.0f) && (side2 >= 0.0f);
                if (!inside)
                {
                    float min_d_sq = 0.0f;
                    bool hasDistance = false;
                    if (side0 < 0.0f)
                    {
                        min_d_sq = distanceSqToSegmentProjected(pd0x, pd0y, e0x, e0y, inv_len0, dot0);
                        hasDistance = true;
                    }
                    if (side1 < 0.0f)
                    {
                        const float d1_sq = distanceSqToSegmentProjected(pd1x, pd1y, e1x, e1y, inv_len1, dot1);
                        if (!hasDistance || d1_sq < min_d_sq)
                            min_d_sq = d1_sq;
                        hasDistance = true;
                    }
                    if (side2 < 0.0f)
                    {
                        const float d2_sq = distanceSqToSegmentProjected(pd2x, pd2y, e2x, e2y, inv_len2, dot2);
                        if (!hasDistance || d2_sq < min_d_sq)
                            min_d_sq = d2_sq;
                    }

                    if (min_d_sq >= bandMin2 && min_d_sq <= bandMax2)
                    {
                        const uint8_t aa = sampleDistSqAlpha(aaLut, min_d_sq);
                        if (aa > 0)
                            blendStore(row + px, c, gamma[aa]);
                    }
                }

                pd0x += 1.0f;
                pd1x += 1.0f;
                pd2x += 1.0f;
                c0 -= e0y;
                c1 -= e1y;
                c2 -= e2y;
                dot0 += e0x;
                dot1 += e1x;
                dot2 += e2x;
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    void GUI::fillRoundTriangle(int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2,
                                uint8_t radius, uint16_t color)
    {
        if (radius == 0)
        {
            fillTriangle(x0, y0, x1, y1, x2, y2, color);
            return;
        }
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = spr->height();
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1, clipB = clipY + clipH - 1;

        const int16_t minX = std::min({x0, x1, x2}) - radius - 1;
        const int16_t maxX = std::max({x0, x1, x2}) + radius + 1;
        const int16_t minY = std::min({y0, y1, y2}) - radius - 1;
        const int16_t maxY = std::max({y0, y1, y2}) + radius + 1;
        if (minX > clipR || maxX < clipX || minY > clipB || maxY < clipY)
            return;

        const float v0x = x0, v0y = y0, v1x = x1, v1y = y1, v2x = x2, v2y = y2;
        const float e0x = v1x - v0x, e0y = v1y - v0y, inv_len0 = 1.0f / (e0x * e0x + e0y * e0y);
        const float e1x = v2x - v1x, e1y = v2y - v1y, inv_len1 = 1.0f / (e1x * e1x + e1y * e1y);
        const float e2x = v0x - v2x, e2y = v0y - v2y, inv_len2 = 1.0f / (e2x * e2x + e2y * e2y);
        const float sign = (e0x * (v2y - v0y) - e0y * (v2x - v0x) < 0.0f) ? -1.0f : 1.0f;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const int16_t xStart = (minX < clipX) ? (int16_t)clipX : minX;
        const int16_t xEnd = (maxX > clipR) ? (int16_t)clipR : maxX;
        const int16_t yStart = (minY < clipY) ? (int16_t)clipY : minY;
        const int16_t yEnd = (maxY > clipB) ? (int16_t)clipB : maxY;
        const DistSqAlphaLut &aaLut = roundTriangleFillLut(radius);
        const float innerRadius2 = aaLut.minSq;
        const float outerRadius2 = aaLut.maxSq;

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            const float py_f = py + 0.5f;
            const float dy0 = py_f - v0y, dy1 = py_f - v1y, dy2 = py_f - v2y;
            const int32_t row_offset = py * stride;
            float dx0 = (float)xStart + 0.5f - v0x;
            float dx1 = (float)xStart + 0.5f - v1x;
            float dx2 = (float)xStart + 0.5f - v2x;
            float c0 = e0x * dy0 - e0y * dx0;
            float c1 = e1x * dy1 - e1y * dx1;
            float c2 = e2x * dy2 - e2y * dx2;
            float dot0 = dx0 * e0x + dy0 * e0y;
            float dot1 = dx1 * e1x + dy1 * e1y;
            float dot2 = dx2 * e2x + dy2 * e2y;

            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                const float side0 = c0 * sign;
                const float side1 = c1 * sign;
                const float side2 = c2 * sign;
                if (side0 >= 0.0f && side1 >= 0.0f && side2 >= 0.0f)
                {
                    buf[row_offset + px] = c.fg;
                }
                else
                {
                    float d_sq = 0.0f;
                    bool hasDistance = false;
                    if (side0 < 0.0f)
                    {
                        d_sq = distanceSqToSegmentProjected(dx0, dy0, e0x, e0y, inv_len0, dot0);
                        hasDistance = true;
                    }
                    if (side1 < 0.0f)
                    {
                        const float d1_sq = distanceSqToSegmentProjected(dx1, dy1, e1x, e1y, inv_len1, dot1);
                        if (!hasDistance || d1_sq < d_sq)
                            d_sq = d1_sq;
                        hasDistance = true;
                    }
                    if (side2 < 0.0f)
                    {
                        const float d2_sq = distanceSqToSegmentProjected(dx2, dy2, e2x, e2y, inv_len2, dot2);
                        if (!hasDistance || d2_sq < d_sq)
                            d_sq = d2_sq;
                    }

                    if (d_sq <= innerRadius2)
                    {
                        buf[row_offset + px] = c.fg;
                    }
                    else if (d_sq <= outerRadius2)
                    {
                        const uint8_t a = sampleDistSqAlpha(aaLut, d_sq);
                        if (a == 255)
                            buf[row_offset + px] = c.fg;
                        else if (a > 0)
                            blendStore(buf + row_offset + px, c, gamma[a]);
                    }
                }

                dx0 += 1.0f;
                dx1 += 1.0f;
                dx2 += 1.0f;
                c0 -= e0y;
                c1 -= e1y;
                c2 -= e2y;
                dot0 += e0x;
                dot1 += e1x;
                dot2 += e2x;
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    void GUI::fillSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color)
    {
        fillSquircleRect(x, y, w, h, radius, radius, radius, radius, color);
    }

    void GUI::fillSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                               uint16_t color)
    {
        if (!_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (x > s.clipR || x + w - 1 < s.clipX || y > s.clipB || y + h - 1 < s.clipY)
            return;

        const uint8_t rTL = clampSquircleRadius(w, h, radiusTL);
        const uint8_t rTR = clampSquircleRadius(w, h, radiusTR);
        const uint8_t rBR = clampSquircleRadius(w, h, radiusBR);
        const uint8_t rBL = clampSquircleRadius(w, h, radiusBL);

        if (rTL <= 1 && rTR <= 1 && rBR <= 1 && rBL <= 1)
        {
            fillRect(x, y, w, h, color);
            return;
        }

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        if (isFullSquareSquircleRect(w, h, rTL, rTR, rBR, rBL))
        {
            const int16_t r = rTL;
            const int16_t cx = static_cast<int16_t>(x + r);
            const int16_t cy = static_cast<int16_t>(y + r);
            const bool noClip = (cx - r - 1 >= s.clipX && cx + r + 1 <= s.clipR &&
                                 cy - r - 1 >= s.clipY && cy + r + 1 <= s.clipB);
            squircleRaster<true, uint32_t>(s, c, cx, cy, r, gamma, noClip);
            if (_disp.display && !_flags.inSpritePass)
                invalidateRect(cx - r - 1, cy - r - 1, r * 2 + 3, r * 2 + 3);
            return;
        }

        const int16_t leftInset = (rTL > rBL) ? rTL : rBL;
        const int16_t rightInset = (rTR > rBR) ? rTR : rBR;
        const int16_t topInset = (rTL > rTR) ? rTL : rTR;
        const int16_t bottomInset = (rBL > rBR) ? rBL : rBR;

        fillSurfaceRect(s, c, static_cast<int16_t>(x + leftInset), y,
                        static_cast<int16_t>(w - leftInset - rightInset), h);
        fillSurfaceRect(s, c, static_cast<int16_t>(x + rTL), y,
                        static_cast<int16_t>(w - rTL - rTR), topInset);
        fillSurfaceRect(s, c, static_cast<int16_t>(x + rBL), static_cast<int16_t>(y + h - bottomInset),
                        static_cast<int16_t>(w - rBL - rBR), bottomInset);
        fillSurfaceRect(s, c, x, static_cast<int16_t>(y + rTL),
                        leftInset, static_cast<int16_t>(h - rTL - rBL));
        fillSurfaceRect(s, c, static_cast<int16_t>(x + w - rightInset), static_cast<int16_t>(y + rTR),
                        rightInset, static_cast<int16_t>(h - rTR - rBR));

        if (rTL > 0)
            fillSquircleCorner<uint32_t>(s, c, gamma, static_cast<int16_t>(x + rTL), static_cast<int16_t>(y + rTL),
                                         rTL, x, y, rTL, rTL);
        if (rTR > 0)
        {
            const int16_t cx = static_cast<int16_t>(x + w - rTR - 1);
            fillSquircleCorner<uint32_t>(s, c, gamma, cx, static_cast<int16_t>(y + rTR),
                                         rTR, static_cast<int16_t>(x + w - rTR), y, rTR, rTR);
        }
        if (rBR > 0)
        {
            const int16_t cx = static_cast<int16_t>(x + w - rBR - 1);
            const int16_t cy = static_cast<int16_t>(y + h - rBR - 1);
            fillSquircleCorner<uint32_t>(s, c, gamma, cx, cy,
                                         rBR, static_cast<int16_t>(x + w - rBR), static_cast<int16_t>(y + h - rBR), rBR, rBR);
        }
        if (rBL > 0)
        {
            const int16_t cy = static_cast<int16_t>(y + h - rBL - 1);
            fillSquircleCorner<uint32_t>(s, c, gamma, static_cast<int16_t>(x + rBL), cy,
                                         rBL, x, static_cast<int16_t>(y + h - rBL), rBL, rBL);
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(x, y, w, h);
    }

    void GUI::drawSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color)
    {
        drawSquircleRect(x, y, w, h, radius, radius, radius, radius, color);
    }

    void GUI::drawSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                               uint16_t color)
    {
        if (!_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (x > s.clipR || x + w - 1 < s.clipX || y > s.clipB || y + h - 1 < s.clipY)
            return;

        const uint8_t rTL = clampSquircleRadius(w, h, radiusTL);
        const uint8_t rTR = clampSquircleRadius(w, h, radiusTR);
        const uint8_t rBR = clampSquircleRadius(w, h, radiusBR);
        const uint8_t rBL = clampSquircleRadius(w, h, radiusBL);

        if (rTL <= 1 && rTR <= 1 && rBR <= 1 && rBL <= 1)
        {
            drawRoundRect(x, y, w, h, 0, color);
            return;
        }

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        if (isFullSquareSquircleRect(w, h, rTL, rTR, rBR, rBL))
        {
            const int16_t r = rTL;
            const int16_t cx = static_cast<int16_t>(x + r);
            const int16_t cy = static_cast<int16_t>(y + r);
            const bool noClip = (cx - r - 1 >= s.clipX && cx + r + 1 <= s.clipR &&
                                 cy - r - 1 >= s.clipY && cy + r + 1 <= s.clipB);
            squircleRaster<false, uint32_t>(s, c, cx, cy, r, gamma, noClip);
            if (_disp.display && !_flags.inSpritePass)
                invalidateRect(cx - r - 2, cy - r - 2, r * 2 + 5, r * 2 + 5);
            return;
        }

        fillSurfaceRect(s, c, static_cast<int16_t>(x + rTL), y, static_cast<int16_t>(w - rTL - rTR), 1);
        fillSurfaceRect(s, c, static_cast<int16_t>(x + rBL), static_cast<int16_t>(y + h - 1), static_cast<int16_t>(w - rBL - rBR), 1);
        fillSurfaceRect(s, c, x, static_cast<int16_t>(y + rTL), 1, static_cast<int16_t>(h - rTL - rBL));
        fillSurfaceRect(s, c, static_cast<int16_t>(x + w - 1), static_cast<int16_t>(y + rTR), 1, static_cast<int16_t>(h - rTR - rBR));

        if (rTL > 0)
            drawSquircleCorner<uint32_t>(s, c, gamma, static_cast<int16_t>(x + rTL), static_cast<int16_t>(y + rTL),
                                         rTL, x, y, rTL + 1, rTL + 1);
        if (rTR > 0)
        {
            const int16_t cx = static_cast<int16_t>(x + w - rTR - 1);
            drawSquircleCorner<uint32_t>(s, c, gamma, cx, static_cast<int16_t>(y + rTR),
                                         rTR, static_cast<int16_t>(x + w - rTR - 1), y, rTR + 1, rTR + 1);
        }
        if (rBR > 0)
        {
            const int16_t cx = static_cast<int16_t>(x + w - rBR - 1);
            const int16_t cy = static_cast<int16_t>(y + h - rBR - 1);
            drawSquircleCorner<uint32_t>(s, c, gamma, cx, cy,
                                         rBR, static_cast<int16_t>(x + w - rBR - 1), static_cast<int16_t>(y + h - rBR - 1), rBR + 1, rBR + 1);
        }
        if (rBL > 0)
        {
            const int16_t cy = static_cast<int16_t>(y + h - rBL - 1);
            drawSquircleCorner<uint32_t>(s, c, gamma, static_cast<int16_t>(x + rBL), cy,
                                         rBL, x, static_cast<int16_t>(y + h - rBL - 1), rBL + 1, rBL + 1);
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(x - 1, y - 1, w + 2, h + 2);
    }

}

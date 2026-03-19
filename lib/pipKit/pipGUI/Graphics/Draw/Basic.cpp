#include "Internal.hpp"
#include <cstdlib>

namespace pipgui
{
    namespace
    {
        struct ArcQuadrantRange
        {
            uint32_t lower;
            uint32_t upper;
            bool enabled;
            bool full;
        };

        static inline uint16_t normalizeAngleDeg(float degrees)
        {
            int32_t angle = (int32_t)degrees;
            angle %= 360;
            if (angle < 0)
                angle += 360;
            return (uint16_t)angle;
        }

        static inline uint16_t normalizeArcSlopeAngle(uint16_t angle)
        {
            angle %= 180;
            return angle > 90 ? (uint16_t)(180 - angle) : angle;
        }

        static inline const uint32_t *arcSlopeTable()
        {
            static uint32_t table[91];
            static bool ready = false;
            if (!ready)
            {
                constexpr float deg2rad = 0.0174532925f;
                constexpr float minDivisor = 1.0f / 0x8000;
                for (uint32_t angle = 0; angle <= 90; ++angle)
                {
                    const float radians = (float)angle * deg2rad;
                    const float absCos = fabsf(cosf(radians));
                    const float absSin = fabsf(sinf(radians));
                    table[angle] = (uint32_t)((absCos / (absSin + minDivisor)) * (float)(1UL << 16));
                }
                ready = true;
            }
            return table;
        }

        static inline bool slopeInRange(uint32_t slopeY, uint32_t dx,
                                        uint32_t lower, uint32_t upper)
        {
            if (dx == 0)
                return lower == 0 || upper == 0xFFFFFFFFu;

            const uint64_t scaled = (uint64_t)dx;
            if (upper != 0xFFFFFFFFu && (uint64_t)slopeY > (uint64_t)upper * scaled)
                return false;
            if (lower != 0 && (uint64_t)slopeY < (uint64_t)lower * scaled)
                return false;
            return true;
        }

        static inline ArcQuadrantRange makeArcQuadrantRange(uint32_t lower, uint32_t upper)
        {
            const bool enabled = !(upper == 0 || lower == 0xFFFFFFFFu);
            return {lower, upper, enabled, enabled && lower == 0 && upper == 0xFFFFFFFFu};
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

    void GUI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
    {
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;

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

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
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

        if (rTL == rTR && rTR == rBR && rBR == rBL && h > (int16_t)(rTL * 2))
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
                      float startDeg, float endDeg, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
            return;

        const uint32_t startAngle = normalizeAngleDeg(startDeg);
        const uint32_t endAngle = normalizeAngleDeg(endDeg);
        if (startAngle == endAngle)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx - r > s.clipR || cx + r < s.clipX || cy - r > s.clipB || cy + r < s.clipY)
            return;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const bool noClip = (cx - r >= s.clipX && cx + r <= s.clipR && cy - r >= s.clipY && cy + r <= s.clipB);

        int16_t ir = r - 1;
        const uint32_t r2 = (uint32_t)r * r;
        r++;
        const uint32_t r1 = (uint32_t)r * r;
        const int16_t w = r - ir;
        const uint32_t r3 = (uint32_t)ir * ir;
        ir--;
        const uint32_t r4 = (uint32_t)ir * ir;

        uint32_t startSlope[4] = {0, 0, 0xFFFFFFFFu, 0};
        uint32_t endSlope[4] = {0, 0xFFFFFFFFu, 0, 0};

        const uint32_t *const slopeTable = arcSlopeTable();
        uint32_t slope = slopeTable[normalizeArcSlopeAngle((uint16_t)startAngle)];
        if (startAngle <= 90)
            startSlope[0] = slope;
        else if (startAngle <= 180)
            startSlope[1] = slope;
        else if (startAngle <= 270)
        {
            startSlope[1] = 0xFFFFFFFFu;
            startSlope[2] = slope;
        }
        else
        {
            startSlope[1] = 0xFFFFFFFFu;
            startSlope[2] = 0;
            startSlope[3] = slope;
        }

        slope = slopeTable[normalizeArcSlopeAngle((uint16_t)endAngle)];
        if (endAngle <= 90)
        {
            endSlope[0] = slope;
            endSlope[1] = 0;
            startSlope[2] = 0;
        }
        else if (endAngle <= 180)
        {
            endSlope[1] = slope;
            startSlope[2] = 0;
        }
        else if (endAngle <= 270)
            endSlope[2] = slope;
        else
            endSlope[3] = slope;

        const ArcQuadrantRange ranges[4] = {
            makeArcQuadrantRange(endSlope[0], startSlope[0]),
            makeArcQuadrantRange(startSlope[1], endSlope[1]),
            makeArcQuadrantRange(endSlope[2], startSlope[2]),
            makeArcQuadrantRange(startSlope[3], endSlope[3])};

        auto raster = [&](auto blendPixel, auto drawHLine, auto drawVLine) __attribute__((always_inline))
        {
            int32_t xs = 0;
            for (int32_t y = r - 1; y > 0; --y)
            {
                uint32_t len[4] = {0, 0, 0, 0};
                int32_t xst[4] = {-1, -1, -1, -1};
                const uint32_t dy = (uint32_t)(r - y);
                const uint32_t dy2 = dy * dy;
                const uint32_t slopeY = dy << 16;

                while ((uint32_t)(r - xs) * (uint32_t)(r - xs) + dy2 >= r1)
                    ++xs;

                for (int32_t x = xs; x < r; ++x)
                {
                    const uint32_t dxEdge = (uint32_t)(r - x);
                    const uint32_t hyp = dxEdge * dxEdge + dy2;
                    uint8_t alpha = 0;

                    if (hyp > r2)
                    {
                        alpha = ~sqrtU8(hyp);
                    }
                    else if (hyp >= r3)
                    {
                        if (ranges[0].enabled && (ranges[0].full || slopeInRange(slopeY, dxEdge, ranges[0].lower, ranges[0].upper)))
                        {
                            xst[0] = x;
                            ++len[0];
                        }
                        if (ranges[1].enabled && (ranges[1].full || slopeInRange(slopeY, dxEdge, ranges[1].lower, ranges[1].upper)))
                        {
                            xst[1] = x;
                            ++len[1];
                        }
                        if (ranges[2].enabled && (ranges[2].full || slopeInRange(slopeY, dxEdge, ranges[2].lower, ranges[2].upper)))
                        {
                            xst[2] = x;
                            ++len[2];
                        }
                        if (ranges[3].enabled && (ranges[3].full || slopeInRange(slopeY, dxEdge, ranges[3].lower, ranges[3].upper)))
                        {
                            xst[3] = x;
                            ++len[3];
                        }
                        continue;
                    }
                    else
                    {
                        if (hyp <= r4)
                            break;
                        alpha = sqrtU8(hyp);
                    }

                    if (alpha < 16)
                        continue;
                    if (ranges[0].enabled && (ranges[0].full || slopeInRange(slopeY, dxEdge, ranges[0].lower, ranges[0].upper)))
                        blendPixel(cx + x - r, cy - y + r, gamma[alpha]);
                    if (ranges[1].enabled && (ranges[1].full || slopeInRange(slopeY, dxEdge, ranges[1].lower, ranges[1].upper)))
                        blendPixel(cx + x - r, cy + y - r, gamma[alpha]);
                    if (ranges[2].enabled && (ranges[2].full || slopeInRange(slopeY, dxEdge, ranges[2].lower, ranges[2].upper)))
                        blendPixel(cx - x + r, cy + y - r, gamma[alpha]);
                    if (ranges[3].enabled && (ranges[3].full || slopeInRange(slopeY, dxEdge, ranges[3].lower, ranges[3].upper)))
                        blendPixel(cx - x + r, cy - y + r, gamma[alpha]);
                }

                if (len[0])
                    drawHLine(cx + xst[0] - len[0] + 1 - r, cy - y + r, len[0]);
                if (len[1])
                    drawHLine(cx + xst[1] - len[1] + 1 - r, cy + y - r, len[1]);
                if (len[2])
                    drawHLine(cx - xst[2] + r, cy + y - r, len[2]);
                if (len[3])
                    drawHLine(cx - xst[3] + r, cy - y + r, len[3]);
            }

            if (startAngle == 0 || endAngle == 360)
                drawVLine(cx, cy + r - w, cy + r - 1);
            if (startAngle <= 90 && endAngle >= 90)
                drawHLine(cx - r + 1, cy, w);
            if (startAngle <= 180 && endAngle >= 180)
                drawVLine(cx, cy - r + 1, cy - r + w);
            if (startAngle <= 270 && endAngle >= 270)
                drawHLine(cx + r - w, cy, w);
        };

        if (noClip)
            raster([&](int32_t px, int32_t py, uint8_t alpha)
                   { blendStore(s.buf + py * s.stride + px, c, alpha); },
                   [&](int32_t px, int32_t py, int32_t len)
                   {
                       if (len > 0)
                           fillSpanFast(s, py, px, px + len - 1, c);
                   },
                   [&](int32_t px, int32_t py0, int32_t py1)
                   { fillVLineFast(s, px, py0, py1, c.fg); });
        else
            raster([&](int32_t px, int32_t py, uint8_t alpha)
                   { plotBlendClip(s, c, px, py, alpha); },
                   [&](int32_t px, int32_t py, int32_t len)
                   {
                       if (len > 0)
                           fillSpanClip(s, py, px, px + len - 1, c);
                   },
                   [&](int32_t px, int32_t py0, int32_t py1)
                   { fillVLineClip(s, px, py0, py1, c.fg); });

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r, cy - r, r * 2 + 1, r * 2 + 1);
    }

}

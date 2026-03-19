#pragma once

#include <pipGUI/Core/pipGUI.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <math.h>
#include "Blend.hpp"

namespace pipgui
{

    static __attribute__((always_inline)) inline void spanFill(uint16_t *dst, int16_t n, uint16_t fg, uint32_t fg32)
    {
        if (n <= 0)
            return;
        if ((uintptr_t)dst & 2)
        {
            *dst++ = fg;
            if (!--n)
                return;
        }
        auto *d32 = reinterpret_cast<uint32_t *>(dst);
        for (int16_t c = n >> 1; c--;)
            *d32++ = fg32;
        if (n & 1)
            *reinterpret_cast<uint16_t *>(d32) = fg;
    }

    static __attribute__((always_inline)) inline uint16_t blendRGB565(uint32_t fg_rb, uint32_t fg_g, uint16_t bg, uint8_t alpha)
    {
        const uint32_t inv = 255u - alpha;
        const uint32_t b_rb = ((uint32_t)(bg & 0xF800u) << 5) | (bg & 0x001Fu);
        const uint32_t b_g = bg & 0x07E0u;
        const uint32_t rb = ((fg_rb * alpha + b_rb * inv) >> 8) & 0x001F001Fu;
        const uint32_t g = ((fg_g * alpha + b_g * inv) >> 8) & 0x000007E0u;
        return (uint16_t)((rb >> 5) | rb | g);
    }

    struct Surface565
    {
        uint16_t *buf;
        int32_t stride;
        int32_t clipX;
        int32_t clipY;
        int32_t clipR;
        int32_t clipB;
    };

    struct Color565
    {
        uint16_t fg;
        uint32_t fg32;
        uint32_t fg_rb;
        uint32_t fg_g;
    };

    static __attribute__((always_inline)) inline Color565 makeColor565(uint16_t color565)
    {
        const uint16_t fg = pipcore::Sprite::swap16(color565);
        return {fg,
                ((uint32_t)fg << 16) | fg,
                ((color565 & 0xF800u) << 5) | (color565 & 0x001Fu),
                color565 & 0x07E0u};
    }

    static __attribute__((always_inline)) inline bool getSurface565(pipcore::Sprite *spr, Surface565 &s)
    {
        if (!spr)
            return false;
        s.buf = (uint16_t *)spr->getBuffer();
        s.stride = spr->width();
        const int32_t maxH = spr->height();
        if (!s.buf || s.stride <= 0 || maxH <= 0)
            return false;
        int32_t clipW = s.stride;
        int32_t clipH = maxH;
        s.clipX = 0;
        s.clipY = 0;
        spr->getClipRect(&s.clipX, &s.clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return false;
        s.clipR = s.clipX + clipW - 1;
        s.clipB = s.clipY + clipH - 1;
        return true;
    }

    static inline const uint8_t *gammaTable() noexcept
    {
        static uint8_t table[256];
        static bool ready = false;
        if (!ready)
        {
            for (uint32_t i = 0; i < 256; ++i)
            {
                float alpha = (float)gammaAA((uint8_t)i) * (1.0f / 255.0f);
                alpha = 0.5f + (alpha - 0.5f) * 1.10f;
                if (alpha < 0.0f)
                    alpha = 0.0f;
                else if (alpha > 1.0f)
                    alpha = 1.0f;
                alpha = powf(alpha, 0.94f);
                table[i] = (uint8_t)(alpha * 255.0f + 0.5f);
            }
            ready = true;
        }
        return table;
    }

    static inline const uint8_t *coverageTable() noexcept
    {
        static uint8_t table[257];
        static bool ready = false;
        if (!ready)
        {
            for (uint32_t i = 0; i <= 256; ++i)
            {
                float d = (float)i * (1.0f / 255.0f);
                if (d >= 1.0f)
                {
                    table[i] = 0;
                    continue;
                }
                d = d * (1.08f - 0.08f * d);
                float alpha = 1.0f - d * d * (3.0f - 2.0f * d);
                alpha = 0.5f + (alpha - 0.5f) * 1.06f;
                if (alpha < 0.0f)
                    alpha = 0.0f;
                else if (alpha > 1.0f)
                    alpha = 1.0f;
                table[i] = (uint8_t)(alpha * 255.0f + 0.5f);
            }
            ready = true;
        }
        return table;
    }

    static __attribute__((always_inline)) inline uint8_t coverageFromDistance(const uint8_t *curve, uint32_t d256)
    {
        return (d256 < 256u) ? curve[d256] : 0;
    }

    static __attribute__((always_inline)) inline uint8_t fracAlphaFromResidual(int64_t rem, int64_t den)
    {
        if (rem <= 0 || den <= 0)
            return 0;
        if (rem >= den)
            return 255;
        return (uint8_t)((rem * 255 + (den >> 1)) / den);
    }

    template <typename AccT>
    static __attribute__((always_inline)) inline AccT pow4i(uint32_t v)
    {
        const AccT vv = (AccT)v * v;
        return vv * vv;
    }

    template <typename AccT>
    static __attribute__((always_inline)) inline AccT pow4StepUp(uint32_t v)
    {
        const AccT vv = (AccT)v;
        const AccT v2 = vv * vv;
        return vv * (4 * v2 + 6 * vv + 4) + 1;
    }

    template <typename AccT>
    static __attribute__((always_inline)) inline AccT pow4StepDown(uint32_t v)
    {
        const AccT vv = (AccT)v;
        const AccT v2 = vv * vv;
        return vv * (4 * v2 - 6 * vv + 4) - 1;
    }

    template <typename AccT, typename FillVLineFn, typename BlendPixelFn>
    static inline void rasterFillSquircle(int16_t cx, int16_t cy, int16_t r, AccT r4,
                                          const uint8_t *gamma,
                                          FillVLineFn fillVLine,
                                          BlendPixelFn blendPixel)
    {
        int32_t yi = r;
        AccT yi4 = r4;
        AccT x4 = 0;
        for (int16_t dx = 0; dx <= r; ++dx)
        {
            while (yi > 0 && x4 + yi4 > r4)
            {
                yi4 -= pow4StepDown<AccT>((uint32_t)yi);
                --yi;
            }

            const int16_t ySpan = (int16_t)yi;
            const uint8_t ag = gamma[fracAlphaFromResidual((int64_t)(r4 - (x4 + yi4)),
                                                           (int64_t)pow4StepUp<AccT>((uint32_t)ySpan))];
            const int16_t pxL = cx - dx;
            const int16_t pxR = cx + dx;

            fillVLine(pxL, (int16_t)(cy - ySpan), (int16_t)(cy + ySpan));
            if (dx)
                fillVLine(pxR, (int16_t)(cy - ySpan), (int16_t)(cy + ySpan));

            blendPixel(pxL, (int16_t)(cy - ySpan - 1), ag);
            blendPixel(pxL, (int16_t)(cy + ySpan + 1), ag);
            if (dx)
            {
                blendPixel(pxR, (int16_t)(cy - ySpan - 1), ag);
                blendPixel(pxR, (int16_t)(cy + ySpan + 1), ag);
            }

            if (dx != r)
                x4 += pow4StepUp<AccT>((uint32_t)dx);
        }

        int32_t xi = r;
        AccT xi4 = r4;
        AccT y4 = 0;
        for (int16_t dy = 0; dy <= r; ++dy)
        {
            while (xi > 0 && xi4 + y4 > r4)
            {
                xi4 -= pow4StepDown<AccT>((uint32_t)xi);
                --xi;
            }

            const int16_t xSpan = (int16_t)xi;
            const uint8_t ag = gamma[fracAlphaFromResidual((int64_t)(r4 - (xi4 + y4)),
                                                           (int64_t)pow4StepUp<AccT>((uint32_t)xSpan))];
            const int16_t pyT = cy - dy;
            const int16_t pyB = cy + dy;
            const int16_t pxL = cx - xSpan - 1;
            const int16_t pxR = cx + xSpan + 1;

            blendPixel(pxL, pyT, ag);
            blendPixel(pxR, pyT, ag);
            if (dy)
            {
                blendPixel(pxL, pyB, ag);
                blendPixel(pxR, pyB, ag);
            }

            if (dy != r)
                y4 += pow4StepUp<AccT>((uint32_t)dy);
        }
    }

    template <typename AccT, typename BlendPixelFn>
    static inline void rasterDrawSquircle(int16_t cx, int16_t cy, int16_t r, AccT r4,
                                          const uint8_t *gamma,
                                          BlendPixelFn blendPixel)
    {
        int32_t yi = r;
        AccT yi4 = r4;
        AccT x4 = 0;
        for (int16_t dx = 0; dx <= r; ++dx)
        {
            while (yi > 0 && x4 + yi4 > r4)
            {
                yi4 -= pow4StepDown<AccT>((uint32_t)yi);
                --yi;
            }

            const int16_t yEdge = (int16_t)yi;
            const uint8_t frac = fracAlphaFromResidual((int64_t)(r4 - (x4 + yi4)),
                                                       (int64_t)pow4StepUp<AccT>((uint32_t)yEdge));
            const uint8_t a0 = gamma[255 - frac];
            const uint8_t a1 = gamma[frac];
            const int16_t pxL = cx - dx;
            const int16_t pxR = cx + dx;
            const int16_t pyT = cy - yEdge;
            const int16_t pyB = cy + yEdge;

            blendPixel(pxL, pyT, a0);
            blendPixel(pxL, pyB, a0);
            if (dx)
            {
                blendPixel(pxR, pyT, a0);
                blendPixel(pxR, pyB, a0);
            }
            blendPixel(pxL, (int16_t)(pyT - 1), a1);
            blendPixel(pxL, (int16_t)(pyB + 1), a1);
            if (dx)
            {
                blendPixel(pxR, (int16_t)(pyT - 1), a1);
                blendPixel(pxR, (int16_t)(pyB + 1), a1);
            }

            if (dx != r)
                x4 += pow4StepUp<AccT>((uint32_t)dx);
        }

        int32_t xi = r;
        AccT xi4 = r4;
        AccT y4 = 0;
        for (int16_t dy = 0; dy <= r; ++dy)
        {
            while (xi > 0 && xi4 + y4 > r4)
            {
                xi4 -= pow4StepDown<AccT>((uint32_t)xi);
                --xi;
            }

            const int16_t xEdge = (int16_t)xi;
            const uint8_t frac = fracAlphaFromResidual((int64_t)(r4 - (xi4 + y4)),
                                                       (int64_t)pow4StepUp<AccT>((uint32_t)xEdge));
            const uint8_t a0 = gamma[255 - frac];
            const uint8_t a1 = gamma[frac];
            const int16_t pyT = cy - dy;
            const int16_t pyB = cy + dy;
            const int16_t pxL = cx - xEdge;
            const int16_t pxR = cx + xEdge;

            blendPixel(pxL, pyT, a0);
            blendPixel(pxR, pyT, a0);
            if (dy)
            {
                blendPixel(pxL, pyB, a0);
                blendPixel(pxR, pyB, a0);
            }
            blendPixel((int16_t)(pxL - 1), pyT, a1);
            blendPixel((int16_t)(pxR + 1), pyT, a1);
            if (dy)
            {
                blendPixel((int16_t)(pxL - 1), pyB, a1);
                blendPixel((int16_t)(pxR + 1), pyB, a1);
            }

            if (dy != r)
                y4 += pow4StepUp<AccT>((uint32_t)dy);
        }
    }

    static __attribute__((always_inline)) inline void blendStore(uint16_t *ptr, const Color565 &c, uint8_t alpha)
    {
        if (alpha == 0)
            return;
        if (alpha == 255)
        {
            *ptr = c.fg;
            return;
        }
        *ptr = pipcore::Sprite::swap16(
            blendRGB565(c.fg_rb, c.fg_g, pipcore::Sprite::swap16(*ptr), alpha));
    }

    static __attribute__((always_inline)) inline void blendStoreGamma(uint16_t *ptr, const Color565 &c,
                                                                      const uint8_t *gamma, uint8_t alpha)
    {
        blendStore(ptr, c, gamma[alpha]);
    }

    static __attribute__((always_inline)) inline void plotBlendClip(const Surface565 &s, const Color565 &c,
                                                                    int32_t px, int32_t py, uint8_t alpha)
    {
        if (px < s.clipX || px > s.clipR || py < s.clipY || py > s.clipB)
            return;
        blendStore(s.buf + py * s.stride + px, c, alpha);
    }

    static __attribute__((always_inline)) inline void plotBlendClipGamma(const Surface565 &s, const Color565 &c,
                                                                         const uint8_t *gamma,
                                                                         int32_t px, int32_t py, uint8_t alpha)
    {
        if (px < s.clipX || px > s.clipR || py < s.clipY || py > s.clipB)
            return;
        blendStoreGamma(s.buf + py * s.stride + px, c, gamma, alpha);
    }

    static __attribute__((always_inline)) inline void fillSpanFast(const Surface565 &s, int32_t py,
                                                                   int32_t x0, int32_t x1, const Color565 &c)
    {
        if (x0 <= x1)
            spanFill(s.buf + py * s.stride + x0, (int16_t)(x1 - x0 + 1), c.fg, c.fg32);
    }

    static __attribute__((always_inline)) inline void fillSpanClip(const Surface565 &s, int32_t py,
                                                                   int32_t x0, int32_t x1, const Color565 &c)
    {
        if (py < s.clipY || py > s.clipB)
            return;
        if (x0 < s.clipX)
            x0 = s.clipX;
        if (x1 > s.clipR)
            x1 = s.clipR;
        fillSpanFast(s, py, x0, x1, c);
    }

    static __attribute__((always_inline)) inline void fillVLineFast(const Surface565 &s, int32_t px,
                                                                    int32_t py0, int32_t py1, uint16_t fg)
    {
        if (py0 > py1)
            return;
        uint16_t *dst = s.buf + py0 * s.stride + px;
        for (int32_t c = py1 - py0 + 1; c--;)
        {
            *dst = fg;
            dst += s.stride;
        }
    }

    static __attribute__((always_inline)) inline void fillVLineClip(const Surface565 &s, int32_t px,
                                                                    int32_t py0, int32_t py1, uint16_t fg)
    {
        if (px < s.clipX || px > s.clipR)
            return;
        if (py0 < s.clipY)
            py0 = s.clipY;
        if (py1 > s.clipB)
            py1 = s.clipB;
        fillVLineFast(s, px, py0, py1, fg);
    }

    template <bool Fill, typename AccT>
    static inline void squircleRaster(const Surface565 &s, const Color565 &c,
                                      int16_t cx, int16_t cy, int16_t r,
                                      const uint8_t *gamma, bool noClip)
    {
        const AccT r4 = pow4i<AccT>((uint32_t)r);
        if constexpr (Fill)
        {
            if (noClip)
            {
                rasterFillSquircle<AccT>(cx, cy, r, r4, gamma, [&](int16_t px, int16_t py0, int16_t py1)
                                         { fillVLineFast(s, px, py0, py1, c.fg); }, [&](int16_t px, int16_t py, uint8_t alpha)
                                         {
                                             if (alpha)
                                                 blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha); });
            }
            else
            {
                rasterFillSquircle<AccT>(cx, cy, r, r4, gamma, [&](int16_t px, int16_t py0, int16_t py1)
                                         { fillVLineClip(s, px, py0, py1, c.fg); }, [&](int16_t px, int16_t py, uint8_t alpha)
                                         {
                                             if (alpha && px >= s.clipX && px <= s.clipR && py >= s.clipY && py <= s.clipB)
                                                 blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha); });
            }
        }
        else
        {
            if (noClip)
            {
                rasterDrawSquircle<AccT>(cx, cy, r, r4, gamma,
                                         [&](int16_t px, int16_t py, uint8_t alpha)
                                         {
                                             if (alpha)
                                                 blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha);
                                         });
            }
            else
            {
                rasterDrawSquircle<AccT>(cx, cy, r, r4, gamma,
                                         [&](int16_t px, int16_t py, uint8_t alpha)
                                         {
                                             if (alpha && px >= s.clipX && px <= s.clipR && py >= s.clipY && py <= s.clipB)
                                                 blendStore(s.buf + (int32_t)py * s.stride + px, c, alpha);
                                         });
            }
        }
    }

    template <typename PlotFn, typename FillSpanFn>
    static inline void rasterAAFillRoundCore(int32_t cx, int32_t cy, int32_t r,
                                             int32_t extraX, int32_t extraY,
                                             PlotFn plot, FillSpanFn fillSpan)
    {
        int32_t xs = 1, cx_i = 0;
        const int32_t r1 = r * r;
        const int32_t rp = r + 1;
        const int32_t r2 = rp * rp;

        for (int32_t cy_i = rp - 1; cy_i > 0; --cy_i)
        {
            const int32_t dy = rp - cy_i;
            const int32_t dy2 = dy * dy;

            for (cx_i = xs; cx_i < rp; ++cx_i)
            {
                const int32_t dx = rp - cx_i;
                const int32_t hyp2 = dx * dx + dy2;
                if (hyp2 <= r1)
                    break;
                if (hyp2 >= r2)
                    continue;

                const uint8_t alpha = (uint8_t)~sqrtU8((uint32_t)hyp2);
                if (alpha > 246)
                    break;
                xs = cx_i;
                if (alpha < 9)
                    continue;

                const int16_t px0 = (int16_t)(cx + cx_i - rp);
                const int16_t px1 = (int16_t)(cx - cx_i + rp + extraX);
                const int16_t py0 = (int16_t)(cy + cy_i - rp);
                const int16_t py1 = (int16_t)(cy - cy_i + rp + extraY);
                plot(px0, py0, alpha);
                plot(px1, py0, alpha);
                plot(px1, py1, alpha);
                plot(px0, py1, alpha);
            }

            const int16_t span_x0 = (int16_t)(cx + cx_i - rp);
            const int16_t span_x1 = (int16_t)(cx + (rp - cx_i) + extraX);
            const int16_t py0 = (int16_t)(cy + cy_i - rp);
            const int16_t py1 = (int16_t)(cy - cy_i + rp + extraY);
            fillSpan(py0, span_x0, span_x1);
            fillSpan(py1, span_x0, span_x1);
        }
    }

    template <typename PlotFn, typename FillSpanFn, typename FillVLineFn>
    static inline void rasterAARingRoundCore(int32_t x0, int32_t y0, int32_t r,
                                             int32_t ww, int32_t hh, int16_t t,
                                             PlotFn plot, FillSpanFn fillSpan, FillVLineFn fillVLine)
    {
        int32_t ir = r - 1;
        const int32_t r2 = r * r;
        ++r;
        const int32_t r1 = r * r;
        const int32_t r3 = ir * ir;
        if (ir > 0)
            --ir;
        const int32_t r4 = (ir > 0) ? (ir * ir) : 0;
        int32_t xs = 0, cx_i = 0;

        for (int32_t cy_i = r - 1; cy_i > 0; --cy_i)
        {
            int32_t len = 0, rxst = 0;
            const int32_t dy = r - cy_i;
            const int32_t dy2 = dy * dy;

            while ((r - xs) * (r - xs) + dy2 >= r1)
                ++xs;

            for (cx_i = xs; cx_i < r; ++cx_i)
            {
                const int32_t dx = r - cx_i;
                const int32_t hyp = dx * dx + dy2;
                uint8_t alpha = 0;

                if (hyp > r2)
                    alpha = (uint8_t)~sqrtU8((uint32_t)hyp);
                else if (hyp >= r3)
                {
                    rxst = cx_i;
                    ++len;
                    continue;
                }
                else
                {
                    if (hyp <= r4)
                        break;
                    alpha = sqrtU8((uint32_t)hyp);
                }

                if (alpha < 16)
                    continue;

                plot((int16_t)(x0 + cx_i - r), (int16_t)(y0 - cy_i + r + hh), alpha);
                plot((int16_t)(x0 + cx_i - r), (int16_t)(y0 + cy_i - r), alpha);
                plot((int16_t)(x0 - cx_i + r + ww), (int16_t)(y0 + cy_i - r), alpha);
                plot((int16_t)(x0 - cx_i + r + ww), (int16_t)(y0 - cy_i + r + hh), alpha);
            }

            if (len > 0)
            {
                const int32_t lxst = rxst - len + 1;
                fillSpan((int16_t)(y0 - cy_i + r + hh), (int16_t)(x0 + lxst - r), (int16_t)(x0 + rxst - r));
                fillSpan((int16_t)(y0 + cy_i - r), (int16_t)(x0 + lxst - r), (int16_t)(x0 + rxst - r));
                fillSpan((int16_t)(y0 + cy_i - r), (int16_t)(x0 - rxst + r + ww), (int16_t)(x0 - lxst + r + ww));
                fillSpan((int16_t)(y0 - cy_i + r + hh), (int16_t)(x0 - rxst + r + ww), (int16_t)(x0 - lxst + r + ww));
            }
        }

        for (int16_t i = 0; i < t; ++i)
        {
            fillSpan((int16_t)(y0 + r - t + i + hh), (int16_t)x0, (int16_t)(x0 + ww));
            fillSpan((int16_t)(y0 - r + 1 + i), (int16_t)x0, (int16_t)(x0 + ww));
            fillVLine((int16_t)(x0 - r + 1 + i), (int16_t)y0, (int16_t)(y0 + hh));
            fillVLine((int16_t)(x0 + r - t + i + ww), (int16_t)y0, (int16_t)(y0 + hh));
        }
    }

    template <bool Steep, typename PlotFn>
    static inline void rasterAALine(int32_t major0, int32_t major1, int32_t minor0, int32_t step,
                                    uint32_t w256, const uint8_t *curve, const uint8_t *gamma,
                                    PlotFn plot)
    {
        int32_t fp = minor0 << 16;
        for (int32_t major = major0; major <= major1; ++major)
        {
            const int32_t minor = fp >> 16;
            const uint32_t frac = ((uint32_t)fp & 0xFFFFu) >> 8;
            const uint8_t a0 = coverageFromDistance(curve, (frac * w256 + 127u) >> 8);
            const uint8_t am = coverageFromDistance(curve, ((frac + 256u) * w256 + 127u) >> 8);
            const uint8_t ap = coverageFromDistance(curve, (((256u - frac) & 0x1FFu) * w256 + 127u) >> 8);
            if constexpr (Steep)
            {
                if (a0)
                    plot(minor, major, gamma[a0]);
                if (am)
                    plot(minor - 1, major, gamma[am]);
                if (ap)
                    plot(minor + 1, major, gamma[ap]);
            }
            else
            {
                if (a0)
                    plot(major, minor, gamma[a0]);
                if (am)
                    plot(major, minor - 1, gamma[am]);
                if (ap)
                    plot(major, minor + 1, gamma[ap]);
            }
            fp += step;
        }
    }

}

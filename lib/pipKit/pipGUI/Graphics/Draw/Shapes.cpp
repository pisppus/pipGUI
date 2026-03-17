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

    static inline const uint8_t *gammaTable()
    {
        static uint8_t table[256];
        static bool ready = false;
        if (!ready)
        {
            for (uint32_t i = 0; i < 256; ++i)
                table[i] = gammaAA((uint8_t)i);
            ready = true;
        }
        return table;
    }

    static inline const uint8_t *coverageTable()
    {
        static uint8_t table[257];
        static bool ready = false;
        if (!ready)
        {
            for (uint32_t i = 0; i < 256; ++i)
            {
                const float d = (float)i * (1.0f / 255.0f);
                table[i] = (d >= 1.0f) ? 0 : (uint8_t)(255.0f - d * d * (765.0f - 510.0f * d));
            }
            table[256] = 0;
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

    static __attribute__((always_inline)) inline void blendStore(uint16_t *ptr, const Color565 &c, uint8_t alpha);
    static __attribute__((always_inline)) inline void fillVLineFast(const Surface565 &s, int32_t px,
                                                                    int32_t py0, int32_t py1, uint16_t fg);
    static __attribute__((always_inline)) inline void fillVLineClip(const Surface565 &s, int32_t px,
                                                                    int32_t py0, int32_t py1, uint16_t fg);

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

    static __attribute__((always_inline)) inline uint8_t calcAlphaSDF(float dist)
    {
        if (dist <= -0.5f)
            return 255;
        if (dist >= 0.5f)
            return 0;
        const float t = dist + 0.5f;
        return (uint8_t)((1.0f - t * t * (3.0f - 2.0f * t)) * 255.0f);
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
            const int16_t py0 = (y < s.clipY) ? (int16_t)s.clipY : y;
            const int16_t py1 = (y + h - 1 > s.clipB) ? (int16_t)s.clipB : (int16_t)(y + h - 1);
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

                uint8_t rL = 0;
                int16_t cxL = 0;
                int32_t dyL = 0;
                if (rTL > 0 && py < y + rTL)
                {
                    rL = rTL;
                    cxL = x + rTL;
                    dyL = (int32_t)(y + rTL - py);
                }
                else if (rBL > 0 && py >= y + h - rBL)
                {
                    rL = rBL;
                    cxL = x + rBL;
                    dyL = (int32_t)(py - (y + h - rBL - 1));
                }
                applyEdge(rL, cxL, dyL, -1, solid_x0, row);

                uint8_t rR = 0;
                int16_t cxR = 0;
                int32_t dyR = 0;
                if (rTR > 0 && py < y + rTR)
                {
                    rR = rTR;
                    cxR = x + w - rTR - 1;
                    dyR = (int32_t)(y + rTR - py);
                }
                else if (rBR > 0 && py >= y + h - rBR)
                {
                    rR = rBR;
                    cxR = x + w - rBR - 1;
                    dyR = (int32_t)(py - (y + h - rBR - 1));
                }
                applyEdge(rR, cxR, dyR, 1, solid_x1, row);
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

    void GUI::drawArc(int16_t cx, int16_t cy, int16_t r,
                      float startDeg, float endDeg, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
            return;

        uint32_t startAngle = (uint32_t)(fmodf(startDeg, 360.0f));
        if ((int32_t)startAngle < 0)
            startAngle += 360;
        uint32_t endAngle = (uint32_t)(fmodf(endDeg, 360.0f));
        if ((int32_t)endAngle < 0)
            endAngle += 360;
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

        const float deg2rad = 0.0174532925f;
        const float minDivisor = 1.0f / 0x8000;

        float fabscos = fabsf(cosf(startAngle * deg2rad));
        float fabssin = fabsf(sinf(startAngle * deg2rad));
        uint32_t slope = (uint32_t)((fabscos / (fabssin + minDivisor)) * (float)(1UL << 16));
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

        fabscos = fabsf(cosf(endAngle * deg2rad));
        fabssin = fabsf(sinf(endAngle * deg2rad));
        slope = (uint32_t)((fabscos / (fabssin + minDivisor)) * (float)(1UL << 16));
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
                    const uint32_t hyp = (uint32_t)(r - x) * (uint32_t)(r - x) + dy2;
                    uint8_t alpha = 0;

                    if (hyp > r2)
                    {
                        alpha = ~sqrtU8(hyp);
                    }
                    else if (hyp >= r3)
                    {
                        slope = slopeY / (uint32_t)(r - x);
                        if (slope <= startSlope[0] && slope >= endSlope[0])
                        {
                            xst[0] = x;
                            ++len[0];
                        }
                        if (slope >= startSlope[1] && slope <= endSlope[1])
                        {
                            xst[1] = x;
                            ++len[1];
                        }
                        if (slope <= startSlope[2] && slope >= endSlope[2])
                        {
                            xst[2] = x;
                            ++len[2];
                        }
                        if (slope <= endSlope[3] && slope >= startSlope[3])
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
                    slope = slopeY / (uint32_t)(r - x);
                    if (slope <= startSlope[0] && slope >= endSlope[0])
                        blendPixel(cx + x - r, cy - y + r, gamma[alpha]);
                    if (slope >= startSlope[1] && slope <= endSlope[1])
                        blendPixel(cx + x - r, cy + y - r, gamma[alpha]);
                    if (slope <= startSlope[2] && slope >= endSlope[2])
                        blendPixel(cx - x + r, cy + y - r, gamma[alpha]);
                    if (slope <= endSlope[3] && slope >= startSlope[3])
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
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
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
        const int32_t dy02 = y2 - y0;
        if (dy02 == 0)
        {
            int16_t minX = std::min({x0, x1, x2});
            int16_t maxX = std::max({x0, x1, x2});
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
                fillSpanClip(s, py,
                             (int16_t)((xl + 0xFFFF) >> 16),
                             (int16_t)(xr >> 16),
                             c);
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
            const int16_t minX = std::min({x0, x1, x2}) - 1;
            const int16_t maxX = std::max({x0, x1, x2}) + 1;
            invalidateRect(minX, y0 - 1, maxX - minX + 1, y2 - y0 + 3);
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

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            const float py_f = py + 0.5f;
            const float pd0y = py_f - v0y, pd1y = py_f - v1y, pd2y = py_f - v2y;
            uint16_t *row = buf + py * stride;
            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                const float px_f = px + 0.5f;

                float pd0x = px_f - v0x;
                float t0 = (pd0x * e0x + pd0y * e0y) * inv_len0;
                t0 = t0 < 0.0f ? 0.0f : (t0 > 1.0f ? 1.0f : t0);
                float dx0 = pd0x - e0x * t0, dy0 = pd0y - e0y * t0;
                float min_d_sq = dx0 * dx0 + dy0 * dy0;

                float pd1x = px_f - v1x;
                float t1 = (pd1x * e1x + pd1y * e1y) * inv_len1;
                t1 = t1 < 0.0f ? 0.0f : (t1 > 1.0f ? 1.0f : t1);
                float dx1 = pd1x - e1x * t1, dy1 = pd1y - e1y * t1;
                float d1_sq = dx1 * dx1 + dy1 * dy1;
                if (d1_sq < min_d_sq)
                    min_d_sq = d1_sq;

                float pd2x = px_f - v2x;
                float t2 = (pd2x * e2x + pd2y * e2y) * inv_len2;
                t2 = t2 < 0.0f ? 0.0f : (t2 > 1.0f ? 1.0f : t2);
                float dx2 = pd2x - e2x * t2, dy2 = pd2y - e2y * t2;
                float d2_sq = dx2 * dx2 + dy2 * dy2;
                if (d2_sq < min_d_sq)
                    min_d_sq = d2_sq;

                const float dist = sqrtf(min_d_sq);
                const float c0 = e0x * pd0y - e0y * pd0x;
                const float c1 = e1x * pd1y - e1y * pd1x;
                const float c2 = e2x * pd2y - e2y * pd2x;
                const bool inside = (c0 * sign >= 0) && (c1 * sign >= 0) && (c2 * sign >= 0);
                const float sdf = inside ? -dist : dist;
                const float edge_dist = sdf - radius;

                if (fabsf(edge_dist) <= 1.0f)
                {
                    const uint8_t aa = calcAlphaSDF(fabsf(edge_dist) - 0.5f);
                    if (aa > 0)
                        blendStore(row + px, c, gamma[aa]);
                }
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
        const float r_lim = radius + 1.0f;
        const float r_lim2 = r_lim * r_lim;

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            const float py_f = py + 0.5f;
            const float dy0 = py_f - v0y, dy1 = py_f - v1y, dy2 = py_f - v2y;
            const int32_t row_offset = py * stride;

            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                const float px_f = px + 0.5f;
                const float dx0 = px_f - v0x, dx1 = px_f - v1x, dx2 = px_f - v2x;

                const float c0 = e0x * dy0 - e0y * dx0;
                const float c1 = e1x * dy1 - e1y * dx1;
                const float c2 = e2x * dy2 - e2y * dx2;

                if ((c0 * sign >= 0) && (c1 * sign >= 0) && (c2 * sign >= 0))
                {
                    buf[row_offset + px] = c.fg;
                    continue;
                }

                float t0 = (dx0 * e0x + dy0 * e0y) * inv_len0;
                t0 = t0 < 0.0f ? 0.0f : (t0 > 1.0f ? 1.0f : t0);
                float nx0 = dx0 - e0x * t0, ny0 = dy0 - e0y * t0;
                float d_sq = nx0 * nx0 + ny0 * ny0;

                float t1 = (dx1 * e1x + dy1 * e1y) * inv_len1;
                t1 = t1 < 0.0f ? 0.0f : (t1 > 1.0f ? 1.0f : t1);
                float nx1 = dx1 - e1x * t1, ny1 = dy1 - e1y * t1;
                float d1_sq = nx1 * nx1 + ny1 * ny1;
                if (d1_sq < d_sq)
                    d_sq = d1_sq;

                float t2 = (dx2 * e2x + dy2 * e2y) * inv_len2;
                t2 = t2 < 0.0f ? 0.0f : (t2 > 1.0f ? 1.0f : t2);
                float nx2 = dx2 - e2x * t2, ny2 = dy2 - e2y * t2;
                float d2_sq = nx2 * nx2 + ny2 * ny2;
                if (d2_sq < d_sq)
                    d_sq = d2_sq;

                if (d_sq > r_lim2)
                    continue;

                const uint8_t a = calcAlphaSDF(sqrtf(d_sq) - radius);
                if (a == 255)
                    buf[row_offset + px] = c.fg;
                else if (a > 0)
                    blendStore(buf + row_offset + px, c, gamma[a]);
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    void GUI::fillSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx + r < s.clipX || cx - r > s.clipR || cy + r < s.clipY || cy - r > s.clipB)
            return;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const bool noClip = (cx - r - 1 >= s.clipX && cx + r + 1 <= s.clipR &&
                             cy - r - 1 >= s.clipY && cy + r + 1 <= s.clipB);

        if (r <= 255)
            squircleRaster<true, uint32_t>(s, c, cx, cy, r, gamma, noClip);
        else
            squircleRaster<true, uint64_t>(s, c, cx, cy, r, gamma, noClip);

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 1, cy - r - 1, r * 2 + 3, r * 2 + 3);
    }

    void GUI::drawSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        Surface565 s;
        if (!getSurface565(spr, s))
            return;
        if (cx + r + 2 < s.clipX || cx - r - 2 > s.clipR ||
            cy + r + 2 < s.clipY || cy - r - 2 > s.clipB)
            return;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();
        const bool noClip = (cx - r - 1 >= s.clipX && cx + r + 1 <= s.clipR &&
                             cy - r - 1 >= s.clipY && cy + r + 1 <= s.clipB);

        if (r <= 255)
            squircleRaster<false, uint32_t>(s, c, cx, cy, r, gamma, noClip);
        else
            squircleRaster<false, uint64_t>(s, c, cx, cy, r, gamma, noClip);

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 2, cy - r - 2, r * 2 + 5, r * 2 + 5);
    }

}

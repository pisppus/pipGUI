#include <pipGUI/Core/API/pipGUI.hpp>
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

        int32_t xs = 1;
        int32_t cx_i = 0;
        const int32_t r1 = (int32_t)r * r;
        const int32_t rp = (int32_t)r + 1;
        const int32_t r2 = rp * rp;

        if (noClip)
        {
            for (int32_t cy_i = rp - 1; cy_i > 0; cy_i--)
            {
                const int32_t dy = rp - cy_i;
                const int32_t dy2 = dy * dy;

                for (cx_i = xs; cx_i < rp; cx_i++)
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

                    const int16_t x0 = (int16_t)(cx + cx_i - rp);
                    const int16_t x1 = (int16_t)(cx - cx_i + rp);
                    const int16_t y0 = (int16_t)(cy + cy_i - rp);
                    const int16_t y1 = (int16_t)(cy - cy_i + rp);

                    blendStore(s.buf + (int32_t)y0 * s.stride + x0, c, alpha);
                    blendStore(s.buf + (int32_t)y0 * s.stride + x1, c, alpha);
                    blendStore(s.buf + (int32_t)y1 * s.stride + x1, c, alpha);
                    blendStore(s.buf + (int32_t)y1 * s.stride + x0, c, alpha);
                }

                const int16_t span_x0 = (int16_t)(cx + cx_i - rp);
                const int16_t span_x1 = (int16_t)(cx + (rp - cx_i));
                const int16_t py0 = (int16_t)(cy + cy_i - rp);
                const int16_t py1 = (int16_t)(cy - cy_i + rp);
                fillSpanFast(s, py0, span_x0, span_x1, c);
                fillSpanFast(s, py1, span_x0, span_x1, c);
            }
        }
        else
        {
            for (int32_t cy_i = rp - 1; cy_i > 0; cy_i--)
            {
                const int32_t dy = rp - cy_i;
                const int32_t dy2 = dy * dy;

                for (cx_i = xs; cx_i < rp; cx_i++)
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

                    const int16_t x0 = (int16_t)(cx + cx_i - rp);
                    const int16_t x1 = (int16_t)(cx - cx_i + rp);
                    const int16_t y0 = (int16_t)(cy + cy_i - rp);
                    const int16_t y1 = (int16_t)(cy - cy_i + rp);

                    plotBlendClip(s, c, x0, y0, alpha);
                    plotBlendClip(s, c, x1, y0, alpha);
                    plotBlendClip(s, c, x1, y1, alpha);
                    plotBlendClip(s, c, x0, y1, alpha);
                }

                const int16_t span_x0 = (int16_t)(cx + cx_i - rp);
                const int16_t span_x1 = (int16_t)(cx + (rp - cx_i));
                const int16_t py0 = (int16_t)(cy + cy_i - rp);
                const int16_t py1 = (int16_t)(cy - cy_i + rp);
                fillSpanClip(s, py0, span_x0, span_x1, c);
                fillSpanClip(s, py1, span_x0, span_x1, c);
            }
        }

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

            int32_t xs = 0, cx_i = 0;
            const int32_t r1 = rr * rr;
            const int32_t rp = rr + 1;
            const int32_t r2 = rp * rp;
            const int32_t hx = (hh > 0) ? hh - 1 : 0;
            const int32_t xx = x + rr;
            const int32_t ww = (w - 2 * rr - 1 > 0) ? w - 2 * rr - 1 : 0;

            if (noClip)
            {
                for (int32_t cy_i = rp - 1; cy_i > 0; cy_i--)
                {
                    const int32_t dy = rp - cy_i;
                    const int32_t dy2 = dy * dy;

                    for (cx_i = xs; cx_i < rp; cx_i++)
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

                        const int16_t px0 = (int16_t)(xx + cx_i - rp);
                        const int16_t px1 = (int16_t)(xx - cx_i + rp + ww);
                        const int16_t py0 = (int16_t)(yy + cy_i - rp);
                        const int16_t py1 = (int16_t)(yy - cy_i + rp + hx);

                        blendStore(s.buf + (int32_t)py0 * s.stride + px0, c, alpha);
                        blendStore(s.buf + (int32_t)py0 * s.stride + px1, c, alpha);
                        blendStore(s.buf + (int32_t)py1 * s.stride + px1, c, alpha);
                        blendStore(s.buf + (int32_t)py1 * s.stride + px0, c, alpha);
                    }

                    const int16_t span_x0 = (int16_t)(xx + cx_i - rp);
                    const int16_t span_x1 = (int16_t)(xx + (rp - cx_i) + ww);
                    const int16_t py0 = (int16_t)(yy + cy_i - rp);
                    const int16_t py1 = (int16_t)(yy - cy_i + rp + hx);
                    fillSpanFast(s, py0, span_x0, span_x1, c);
                    fillSpanFast(s, py1, span_x0, span_x1, c);
                }
            }
            else
            {
                for (int32_t cy_i = rp - 1; cy_i > 0; cy_i--)
                {
                    const int32_t dy = rp - cy_i;
                    const int32_t dy2 = dy * dy;

                    for (cx_i = xs; cx_i < rp; cx_i++)
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

                        const int16_t px0 = (int16_t)(xx + cx_i - rp);
                        const int16_t px1 = (int16_t)(xx - cx_i + rp + ww);
                        const int16_t py0 = (int16_t)(yy + cy_i - rp);
                        const int16_t py1 = (int16_t)(yy - cy_i + rp + hx);

                        plotBlendClip(s, c, px0, py0, alpha);
                        plotBlendClip(s, c, px1, py0, alpha);
                        plotBlendClip(s, c, px1, py1, alpha);
                        plotBlendClip(s, c, px0, py1, alpha);
                    }

                    const int16_t span_x0 = (int16_t)(xx + cx_i - rp);
                    const int16_t span_x1 = (int16_t)(xx + (rp - cx_i) + ww);
                    const int16_t py0 = (int16_t)(yy + cy_i - rp);
                    const int16_t py1 = (int16_t)(yy - cy_i + rp + hx);
                    fillSpanClip(s, py0, span_x0, span_x1, c);
                    fillSpanClip(s, py1, span_x0, span_x1, c);
                }
            }
        }
        else
        {
            auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
            {
                if (dist >= 0.5f)
                    return 255;
                if (dist <= -0.5f)
                    return 0;
                const float t = dist + 0.5f;
                return (uint8_t)(t * t * (765.0f - 510.0f * t));
            };

            for (int16_t py = y; py < y + h; ++py)
            {
                if (py < s.clipY || py > s.clipB)
                    continue;
                uint16_t *row = s.buf + (int32_t)py * s.stride;

                int16_t solid_x0 = x, solid_x1 = x + w - 1;

                uint8_t rL = 0;
                int16_t cxL = 0, cyL = 0;
                float dyL = 0;
                if (rTL > 0 && py < y + rTL)
                {
                    rL = rTL;
                    cxL = x + rTL;
                    cyL = y + rTL;
                    dyL = (float)(cyL - py);
                }
                else if (rBL > 0 && py >= y + h - rBL)
                {
                    rL = rBL;
                    cxL = x + rBL;
                    cyL = y + h - rBL - 1;
                    dyL = (float)(py - cyL);
                }

                if (rL > 0)
                {
                    const float fr = (float)rL, r2 = fr * fr, inv_2r = 0.5f / fr;
                    const float dy2 = dyL * dyL, r_minus = fr - 0.5f, r_plus = fr + 0.5f;
                    const int16_t solid_dx = r_minus * r_minus >= dy2 ? (int16_t)sqrtf(r_minus * r_minus - dy2) : -1;
                    const int16_t aa_dx = r_plus * r_plus >= dy2 ? (int16_t)sqrtf(r_plus * r_plus - dy2) : -1;

                    solid_x0 = cxL - solid_dx;
                    for (int16_t dx = (solid_dx < 0 ? 0 : solid_dx + 1); dx <= aa_dx; ++dx)
                    {
                        const uint8_t a = calcAlpha((r2 - ((float)(dx * dx) + dy2)) * inv_2r);
                        const int16_t px = cxL - dx;
                        if (px >= s.clipX && px <= s.clipR)
                            blendStoreGamma(row + px, c, gamma, a);
                    }
                }

                uint8_t rR = 0;
                int16_t cxR = 0, cyR = 0;
                float dyR = 0;
                if (rTR > 0 && py < y + rTR)
                {
                    rR = rTR;
                    cxR = x + w - rTR - 1;
                    cyR = y + rTR;
                    dyR = (float)(cyR - py);
                }
                else if (rBR > 0 && py >= y + h - rBR)
                {
                    rR = rBR;
                    cxR = x + w - rBR - 1;
                    cyR = y + h - rBR - 1;
                    dyR = (float)(py - cyR);
                }

                if (rR > 0)
                {
                    const float fr = (float)rR, r2 = fr * fr, inv_2r = 0.5f / fr;
                    const float dy2 = dyR * dyR, r_minus = fr - 0.5f, r_plus = fr + 0.5f;
                    const int16_t solid_dx = r_minus * r_minus >= dy2 ? (int16_t)sqrtf(r_minus * r_minus - dy2) : -1;
                    const int16_t aa_dx = r_plus * r_plus >= dy2 ? (int16_t)sqrtf(r_plus * r_plus - dy2) : -1;

                    solid_x1 = cxR + solid_dx;
                    for (int16_t dx = (solid_dx < 0 ? 0 : solid_dx + 1); dx <= aa_dx; ++dx)
                    {
                        const uint8_t a = calcAlpha((r2 - ((float)(dx * dx) + dy2)) * inv_2r);
                        const int16_t px = cxR + dx;
                        if (px >= s.clipX && px <= s.clipR)
                            blendStoreGamma(row + px, c, gamma, a);
                    }
                }

                fillSpanClip(s, py, solid_x0, solid_x1, c);
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

        const int16_t top_x1 = x + rTL, top_x2 = x + w - rTR - 1;
        const int16_t bot_x1 = x + rBL, bot_x2 = x + w - rBR - 1;
        if (top_x1 <= top_x2)
        {
            if (noClip)
                fillSpanFast(s, y, top_x1, top_x2, c);
            else
                fillSpanClip(s, y, top_x1, top_x2, c);
        }
        if (bot_x1 <= bot_x2)
        {
            if (noClip)
                fillSpanFast(s, y + h - 1, bot_x1, bot_x2, c);
            else
                fillSpanClip(s, y + h - 1, bot_x1, bot_x2, c);
        }

        const int16_t left_y1 = y + (rTL > 0 ? rTL : 1), left_y2 = y + h - (rBL > 0 ? rBL : 1) - 1;
        const int16_t right_y1 = y + (rTR > 0 ? rTR : 1), right_y2 = y + h - (rBR > 0 ? rBR : 1) - 1;
        if (left_y1 <= left_y2)
        {
            if (noClip)
                fillVLineFast(s, x, left_y1, left_y2, c.fg);
            else
                fillVLineClip(s, x, left_y1, left_y2, c.fg);
        }
        if (right_y1 <= right_y2)
        {
            if (noClip)
                fillVLineFast(s, x + w - 1, right_y1, right_y2, c.fg);
            else
                fillVLineClip(s, x + w - 1, right_y1, right_y2, c.fg);
        }

        if (rTL == rTR && rTR == rBL && rBL == rBR && rTL > 0)
        {
            int32_t r = (int32_t)rTL;
            int32_t ir = r - 1;
            if (ir < 0)
                ir = 0;
            const int32_t ww = (w - 2 * r > 0) ? w - 2 * r : 0;
            const int32_t hh = (h - 2 * r > 0) ? h - 2 * r : 0;
            const int32_t x0 = x + r, y0 = y + r;
            const uint16_t t = (uint16_t)(r - ir + 1);
            int32_t xs = 0, cx_i = 0;
            const int32_t r2 = r * r;
            r++;
            const int32_t r1 = r * r;
            const int32_t r3 = ir * ir;
            if (ir > 0)
                ir--;
            const int32_t r4 = (ir > 0) ? (ir * ir) : 0;

            if (noClip)
            {
                for (int32_t cy_i = r - 1; cy_i > 0; cy_i--)
                {
                    int32_t len = 0, lxst = 0, rxst = 0;
                    const int32_t dy = r - cy_i;
                    const int32_t dy2 = dy * dy;

                    while ((r - xs) * (r - xs) + dy2 >= r1)
                        xs++;

                    for (cx_i = xs; cx_i < r; cx_i++)
                    {
                        const int32_t dx = r - cx_i;
                        const int32_t hyp = dx * dx + dy2;
                        uint8_t alpha = 0;

                        if (hyp > r2)
                            alpha = (uint8_t)~sqrtU8((uint32_t)hyp);
                        else if (hyp >= r3)
                        {
                            rxst = cx_i;
                            len++;
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

                        const uint8_t ag = gamma[alpha];
                        blendStore(s.buf + (y0 - cy_i + r + hh) * s.stride + (x0 + cx_i - r), c, ag);
                        blendStore(s.buf + (y0 + cy_i - r) * s.stride + (x0 + cx_i - r), c, ag);
                        blendStore(s.buf + (y0 + cy_i - r) * s.stride + (x0 - cx_i + r + ww), c, ag);
                        blendStore(s.buf + (y0 - cy_i + r + hh) * s.stride + (x0 - cx_i + r + ww), c, ag);
                    }

                    lxst = rxst - len + 1;
                    if (len > 0)
                    {
                        fillSpanFast(s, y0 - cy_i + r + hh, x0 + lxst - r, x0 + rxst - r, c);
                        fillSpanFast(s, y0 + cy_i - r, x0 + lxst - r, x0 + rxst - r, c);
                        fillSpanFast(s, y0 + cy_i - r, x0 - rxst + r + ww, x0 - lxst + r + ww, c);
                        fillSpanFast(s, y0 - cy_i + r + hh, x0 - rxst + r + ww, x0 - lxst + r + ww, c);
                    }
                }

                for (int16_t i = 0; i < (int16_t)t; ++i)
                {
                    fillSpanFast(s, y0 + r - (int32_t)t + i + hh, x0, x0 + ww, c);
                    fillSpanFast(s, y0 - r + 1 + i, x0, x0 + ww, c);
                    fillVLineFast(s, x0 - r + 1 + i, y0, y0 + hh, c.fg);
                    fillVLineFast(s, x0 + r - (int32_t)t + i + ww, y0, y0 + hh, c.fg);
                }
            }
            else
            {
                for (int32_t cy_i = r - 1; cy_i > 0; cy_i--)
                {
                    int32_t len = 0, lxst = 0, rxst = 0;
                    const int32_t dy = r - cy_i;
                    const int32_t dy2 = dy * dy;

                    while ((r - xs) * (r - xs) + dy2 >= r1)
                        xs++;

                    for (cx_i = xs; cx_i < r; cx_i++)
                    {
                        const int32_t dx = r - cx_i;
                        const int32_t hyp = dx * dx + dy2;
                        uint8_t alpha = 0;

                        if (hyp > r2)
                            alpha = (uint8_t)~sqrtU8((uint32_t)hyp);
                        else if (hyp >= r3)
                        {
                            rxst = cx_i;
                            len++;
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

                        plotBlendClipGamma(s, c, gamma, x0 + cx_i - r, y0 - cy_i + r + hh, alpha);
                        plotBlendClipGamma(s, c, gamma, x0 + cx_i - r, y0 + cy_i - r, alpha);
                        plotBlendClipGamma(s, c, gamma, x0 - cx_i + r + ww, y0 + cy_i - r, alpha);
                        plotBlendClipGamma(s, c, gamma, x0 - cx_i + r + ww, y0 - cy_i + r + hh, alpha);
                    }

                    lxst = rxst - len + 1;
                    if (len > 0)
                    {
                        fillSpanClip(s, y0 - cy_i + r + hh, x0 + lxst - r, x0 + rxst - r, c);
                        fillSpanClip(s, y0 + cy_i - r, x0 + lxst - r, x0 + rxst - r, c);
                        fillSpanClip(s, y0 + cy_i - r, x0 - rxst + r + ww, x0 - lxst + r + ww, c);
                        fillSpanClip(s, y0 - cy_i + r + hh, x0 - rxst + r + ww, x0 - lxst + r + ww, c);
                    }
                }

                for (int16_t i = 0; i < (int16_t)t; ++i)
                {
                    fillSpanClip(s, y0 + r - (int32_t)t + i + hh, x0, x0 + ww, c);
                    fillSpanClip(s, y0 - r + 1 + i, x0, x0 + ww, c);
                    fillVLineClip(s, x0 - r + 1 + i, y0, y0 + hh, c.fg);
                    fillVLineClip(s, x0 + r - (int32_t)t + i + ww, y0, y0 + hh, c.fg);
                }
            }
        }
        else
        {
            auto drawCornerClip = [&](int16_t ccx, int16_t ccy,
                                      int16_t px_s, int16_t px_e,
                                      int16_t py_s, int16_t py_e, uint8_t r)
            {
                if (r == 0)
                    return;
                const float fr = (float)r;
                const float r_out2 = (fr + 0.5f) * (fr + 0.5f), inv_2r_out = 0.5f / (fr + 0.5f);
                const float r_in2 = (fr - 0.5f) * (fr - 0.5f), inv_2r_in = 0.5f / (fr - 0.5f);

                for (int16_t py = py_s; py <= py_e; ++py)
                {
                    if (py < s.clipY || py > s.clipB)
                        continue;
                    uint16_t *row = s.buf + (int32_t)py * s.stride;
                    const float dy2 = (float)((ccy - py) * (ccy - py));

                    for (int16_t px = px_s; px <= px_e; ++px)
                    {
                        if (px < s.clipX || px > s.clipR)
                            continue;
                        const float dx = (float)(ccx - px);
                        const float S = dx * dx + dy2;
                        const float d_out = (r_out2 - S) * inv_2r_out;
                        const float d_in = (r_in2 - S) * inv_2r_in;

                        if (px_s <= ccx)
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

            if (noClip)
            {
                auto drawCornerFast = [&](int16_t ccx, int16_t ccy,
                                          int16_t px_s, int16_t px_e,
                                          int16_t py_s, int16_t py_e, uint8_t r)
                {
                    if (r == 0)
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

                            if (px_s <= ccx)
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

                drawCornerFast(x + rTL, y + rTL, x, x + rTL - 1, y, y + rTL - 1, rTL);
                drawCornerFast(x + w - rTR - 1, y + rTR, x + w - rTR, x + w - 1, y, y + rTR - 1, rTR);
                drawCornerFast(x + rBL, y + h - rBL - 1, x, x + rBL - 1, y + h - rBL, y + h - 1, rBL);
                drawCornerFast(x + w - rBR - 1, y + h - rBR - 1, x + w - rBR, x + w - 1, y + h - rBR, y + h - 1, rBR);
            }
            else
            {
                drawCornerClip(x + rTL, y + rTL, x, x + rTL - 1, y, y + rTL - 1, rTL);
                drawCornerClip(x + w - rTR - 1, y + rTR, x + w - rTR, x + w - 1, y, y + rTR - 1, rTR);
                drawCornerClip(x + rBL, y + h - rBL - 1, x, x + rBL - 1, y + h - rBL, y + h - 1, rBL);
                drawCornerClip(x + w - rBR - 1, y + h - rBR - 1, x + w - rBR, x + w - 1, y + h - rBR, y + h - 1, rBR);
            }
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
        const float len = sqrtf((float)(dx * dx) + (float)(dy * dy));
        if (len <= 0.0f)
            return;

        const int minX = (x0 < x1 ? x0 : x1), maxX = (x0 > x1 ? x0 : x1);
        const int minY = (y0 < y1 ? y0 : y1), maxY = (y0 > y1 ? y0 : y1);
        if (maxX < clipX - 1 || minX > clipR + 1 || maxY < clipY - 1 || minY > clipB + 1)
            return;

        const bool noClip = (minX >= clipX + 1 && maxX <= clipR - 1 && minY >= clipY + 1 && maxY <= clipB - 1);

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendFastPtr = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            blendStore(ptr, c, alpha);
        };

        auto blendFastClip = [&](int px, int py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
                blendFastPtr(buf + py * stride + px, alpha);
        };

        auto calcAlpha = [](float d) __attribute__((always_inline)) -> uint8_t
        {
            d = fabsf(d);
            if (d >= 1.0f)
                return 0;
            return (uint8_t)(255.0f - d * d * (765.0f - 510.0f * d));
        };

        auto drawShallow = [&](bool useClip) __attribute__((always_inline))
        {
            const float W = (float)adx / len;
            const float dy_dx = (float)dy / (float)adx;
            float y = (float)y0;
            for (int x = x0; x <= x1; ++x)
            {
                const int yi = (int)(y + 65536.5f) - 65536;
                const float dist0 = (y - (float)yi) * W;
                const uint8_t a0 = calcAlpha(dist0);
                const uint8_t am = calcAlpha(dist0 + W);
                const uint8_t ap = calcAlpha(dist0 - W);
                if (useClip)
                {
                    if (a0)
                        blendFastClip(x, yi, gamma[a0]);
                    if (am)
                        blendFastClip(x, yi - 1, gamma[am]);
                    if (ap)
                        blendFastClip(x, yi + 1, gamma[ap]);
                }
                else
                {
                    if (a0)
                        blendFastPtr(buf + (int32_t)yi * stride + x, gamma[a0]);
                    if (am)
                        blendFastPtr(buf + (int32_t)(yi - 1) * stride + x, gamma[am]);
                    if (ap)
                        blendFastPtr(buf + (int32_t)(yi + 1) * stride + x, gamma[ap]);
                }
                y += dy_dx;
            }
        };

        auto drawSteep = [&](bool useClip) __attribute__((always_inline))
        {
            const float W = (float)ady / len;
            const float dx_dy = (float)dx / (float)ady;
            float x = (float)x0;
            for (int y = y0; y <= y1; ++y)
            {
                const int xi = (int)(x + 65536.5f) - 65536;
                const float dist0 = (x - (float)xi) * W;
                const uint8_t a0 = calcAlpha(dist0);
                const uint8_t am = calcAlpha(dist0 + W);
                const uint8_t ap = calcAlpha(dist0 - W);
                if (useClip)
                {
                    if (a0)
                        blendFastClip(xi, y, gamma[a0]);
                    if (am)
                        blendFastClip(xi - 1, y, gamma[am]);
                    if (ap)
                        blendFastClip(xi + 1, y, gamma[ap]);
                }
                else
                {
                    const int32_t row = (int32_t)y * stride;
                    if (a0)
                        blendFastPtr(buf + row + xi, gamma[a0]);
                    if (am)
                        blendFastPtr(buf + row + xi - 1, gamma[am]);
                    if (ap)
                        blendFastPtr(buf + row + xi + 1, gamma[ap]);
                }
                x += dx_dy;
            }
        };

        if (noClip)
        {
            if (steep)
                drawSteep(false);
            else
                drawShallow(false);
        }
        else
        {
            if (steep)
                drawSteep(true);
            else
                drawShallow(true);
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(std::min(x0, x1), std::min(y0, y1),
                           std::abs(x1 - x0) + 1, std::abs(y1 - y0) + 1);
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
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1;
        const int32_t clipB = clipY + clipH - 1;

        if (cx - r > clipR || cx + r < clipX || cy - r > clipB || cy + r < clipY)
            return;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int32_t px, int32_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px < clipX || px > clipR || py < clipY || py > clipB)
                return;
            blendStore(buf + py * stride + px, c, alpha);
        };

        auto drawHLine = [&](int32_t px, int32_t py, int32_t len) __attribute__((always_inline))
        {
            if (py < clipY || py > clipB)
                return;
            if (px < clipX)
            {
                len -= (clipX - px);
                px = clipX;
            }
            if (px + len - 1 > clipR)
                len = clipR - px + 1;
            if (len <= 0)
                return;
            spanFill(buf + py * stride + px, (int16_t)len, c.fg, c.fg32);
        };

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

        int32_t xs = 0;
        for (int32_t y = r - 1; y > 0; y--)
        {
            uint32_t len[4] = {0, 0, 0, 0};
            int32_t xst[4] = {-1, -1, -1, -1};
            const uint32_t dy2 = (uint32_t)(r - y) * (uint32_t)(r - y);

            while ((uint32_t)(r - xs) * (uint32_t)(r - xs) + dy2 >= r1)
                xs++;

            for (int32_t x = xs; x < r; x++)
            {
                const uint32_t hyp = (uint32_t)(r - x) * (uint32_t)(r - x) + dy2;
                uint8_t alpha = 0;

                if (hyp > r2)
                {
                    alpha = ~sqrtU8(hyp);
                }
                else if (hyp >= r3)
                {
                    slope = ((uint32_t)(r - y) << 16) / (uint32_t)(r - x);
                    if (slope <= startSlope[0] && slope >= endSlope[0])
                    {
                        xst[0] = x;
                        len[0]++;
                    }
                    if (slope >= startSlope[1] && slope <= endSlope[1])
                    {
                        xst[1] = x;
                        len[1]++;
                    }
                    if (slope <= startSlope[2] && slope >= endSlope[2])
                    {
                        xst[2] = x;
                        len[2]++;
                    }
                    if (slope <= endSlope[3] && slope >= startSlope[3])
                    {
                        xst[3] = x;
                        len[3]++;
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
                slope = ((uint32_t)(r - y) << 16) / (uint32_t)(r - x);
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
            for (int16_t i = 0; i < w; i++)
                if (cy + r - w + i >= clipY && cy + r - w + i <= clipB)
                    buf[(cy + r - w + i) * stride + cx] = c.fg;
        if (startAngle <= 90 && endAngle >= 90)
            for (int16_t i = 0; i < w; i++)
                if (cx - r + 1 + i >= clipX && cx - r + 1 + i <= clipR)
                    buf[cy * stride + (cx - r + 1 + i)] = c.fg;
        if (startAngle <= 180 && endAngle >= 180)
            for (int16_t i = 0; i < w; i++)
                if (cy - r + 1 + i >= clipY && cy - r + 1 + i <= clipB)
                    buf[(cy - r + 1 + i) * stride + cx] = c.fg;
        if (startAngle <= 270 && endAngle >= 270)
            for (int16_t i = 0; i < w; i++)
                if (cx + r - w + i >= clipX && cx + r - w + i <= clipR)
                    buf[cy * stride + (cx + r - w + i)] = c.fg;

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r, cy - r, r * 2 + 1, r * 2 + 1);
    }

    void GUI::fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0 || !_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1;
        const int32_t clipB = clipY + clipH - 1;

        if (cx + rx < clipX || cx - rx > clipR || cy + ry < clipY || cy - ry > clipB)
            return;

        const float fa = (float)rx;
        const float fb2 = (float)(ry * ry);

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int32_t px, int32_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (alpha == 0 || px < clipX || px > clipR || py < clipY || py > clipB)
                return;
            blendStore(buf + py * stride + px, c, alpha);
        };

        auto drawHLine = [&](int32_t px, int32_t py, int32_t len) __attribute__((always_inline))
        {
            if (py < clipY || py > clipB)
                return;
            if (px < clipX)
            {
                len -= (clipX - px);
                px = clipX;
            }
            if (px + len - 1 > clipR)
                len = clipR - px + 1;
            if (len <= 0)
                return;
            spanFill(buf + py * stride + px, (int16_t)len, c.fg, c.fg32);
        };

        for (int16_t dy = 0; dy <= ry; ++dy)
        {
            const int32_t py0 = cy - dy, py1 = cy + dy;
            if (py0 < clipY && py1 < clipY)
                continue;
            if (py0 > clipB && py1 > clipB)
                continue;

            const float fy = (float)dy;
            float term = 1.0f - (fy * fy) / fb2;
            if (term < 0.0f)
                term = 0.0f;

            const float fx = fa * sqrtf(term);
            const int16_t xi = (int16_t)fx;
            const float frac = fx - (float)xi;

            if (py0 >= clipY && py0 <= clipB)
            {
                drawHLine(cx - xi, py0, (int32_t)xi * 2 + 1);
                const uint8_t a = (uint8_t)(frac * 255.0f);
                if (a)
                {
                    const uint8_t ag = gamma[a];
                    blendPixel(cx + xi + 1, py0, ag);
                    blendPixel(cx - xi - 1, py0, ag);
                }
            }
            if (dy != 0 && py1 >= clipY && py1 <= clipB)
            {
                drawHLine(cx - xi, py1, (int32_t)xi * 2 + 1);
                const uint8_t a = (uint8_t)(frac * 255.0f);
                if (a)
                {
                    const uint8_t ag = gamma[a];
                    blendPixel(cx + xi + 1, py1, ag);
                    blendPixel(cx - xi - 1, py1, ag);
                }
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - rx, cy - ry, rx * 2 + 1, ry * 2 + 1);
    }

    void GUI::drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0 || !_flags.spriteEnabled)
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
        const int32_t clipR = clipX + clipW - 1;
        const int32_t clipB = clipY + clipH - 1;

        if (cx + rx + 1 < clipX || cx - rx - 1 > clipR ||
            cy + ry + 1 < clipY || cy - ry - 1 > clipB)
            return;

        const float fa = (float)rx, fa2 = fa * fa;
        const float fb = (float)ry, fb2 = fb * fb;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int32_t px, int32_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (alpha == 0 || px < clipX || px > clipR || py < clipY || py > clipB)
                return;
            blendStore(buf + py * stride + px, c, alpha);
        };

        auto plot4 = [&](int16_t dx, int16_t dy, uint8_t a0, uint8_t a1) __attribute__((always_inline))
        {
            const int32_t x0 = cx + dx, x1 = cx - dx;
            const int32_t y0 = cy + dy, y1 = cy - dy;
            blendPixel(x0, y0, a0);
            blendPixel(x1, y0, a0);
            blendPixel(x0, y1, a0);
            blendPixel(x1, y1, a0);
            blendPixel(x0, y0 + 1, a1);
            blendPixel(x1, y0 + 1, a1);
            blendPixel(x0, y1 - 1, a1);
            blendPixel(x1, y1 - 1, a1);
        };

        const int16_t qx = (int16_t)((fa2) / sqrtf(fa2 + fb2) + 0.5f);
        for (int16_t dx = 0; dx <= qx; ++dx)
        {
            const float x = (float)dx;
            const float y = fb * sqrtf(1.0f - (x * x) / fa2);
            const int16_t yi = (int16_t)y;
            const float frac = y - (float)yi;
            plot4(dx, yi, gamma[(uint8_t)((1.0f - frac) * 255.0f)],
                  gamma[(uint8_t)(frac * 255.0f)]);
        }

        const int16_t qy = (int16_t)((fb2) / sqrtf(fa2 + fb2) + 0.5f);
        for (int16_t dy = 0; dy <= qy; ++dy)
        {
            const float y = (float)dy;
            const float x = fa * sqrtf(1.0f - (y * y) / fb2);
            const int16_t xi = (int16_t)x;
            const float frac = x - (float)xi;
            const uint8_t a0 = gamma[(uint8_t)((1.0f - frac) * 255.0f)];
            const uint8_t a1 = gamma[(uint8_t)(frac * 255.0f)];
            const int32_t px0 = cx + xi, px1 = cx - xi;
            const int32_t py0 = cy + dy, py1 = cy - dy;
            blendPixel(px0, py0, a0);
            blendPixel(px0 + 1, py0, a1);
            blendPixel(px1, py0, a0);
            blendPixel(px1 - 1, py0, a1);
            blendPixel(px0, py1, a0);
            blendPixel(px0 + 1, py1, a1);
            blendPixel(px1, py1, a0);
            blendPixel(px1 - 1, py1, a1);
        }

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
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = spr->height();
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1;
        const int32_t clipB = clipY + clipH - 1;

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
        if (y2 < clipY || y0 > clipB)
            return;

        static const int SUB_BITS = 4;
        static const int SUB_SCALE = 1 << SUB_BITS;

        const int32_t x0f = x0 << SUB_BITS, y0f = y0 << SUB_BITS;
        const int32_t x1f = x1 << SUB_BITS, y1f = y1 << SUB_BITS;
        const int32_t x2f = x2 << SUB_BITS, y2f = y2 << SUB_BITS;

        int32_t e0x = x2f - x1f, e0y = y2f - y1f;
        int32_t e1x = x0f - x2f, e1y = y0f - y2f;
        int32_t e2x = x1f - x0f, e2y = y1f - y0f;

        const int32_t cross = e2x * (-e1y) - e2y * (-e1x);
        if (cross == 0)
            return;
        if (cross < 0)
        {
            e0x = -e0x;
            e0y = -e0y;
            e1x = -e1x;
            e1y = -e1y;
            e2x = -e2x;
            e2y = -e2y;
        }

        int16_t minX = (int16_t)std::max((int32_t)((std::min({x0f, x1f, x2f}) >> SUB_BITS) - 1), clipX);
        int16_t maxX = (int16_t)std::min((int32_t)((std::max({x0f, x1f, x2f}) >> SUB_BITS) + 1), clipR);
        int16_t minY = (int16_t)std::max((int32_t)((std::min({y0f, y1f, y2f}) >> SUB_BITS) - 1), clipY);
        int16_t maxY = (int16_t)std::min((int32_t)((std::max({y0f, y1f, y2f}) >> SUB_BITS) + 1), clipB);
        if (minX > maxX || minY > maxY)
            return;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            blendStore(buf + py * stride + px, c, alpha);
        };

        static const int32_t ox[4] = {-(SUB_SCALE >> 2), (SUB_SCALE >> 2), -(SUB_SCALE >> 2), (SUB_SCALE >> 2)};
        static const int32_t oy[4] = {-(SUB_SCALE >> 2), -(SUB_SCALE >> 2), (SUB_SCALE >> 2), (SUB_SCALE >> 2)};

        for (int16_t py = minY; py <= maxY; ++py)
        {
            const int32_t row = py * stride;
            const int32_t py_center = (py << SUB_BITS) + (SUB_SCALE >> 1);

            for (int16_t px = minX; px <= maxX; ++px)
            {
                const int32_t px_center = (px << SUB_BITS) + (SUB_SCALE >> 1);
                int inside_count = 0;

                for (int s = 0; s < 4; ++s)
                {
                    const int32_t sx = px_center + ox[s], sy = py_center + oy[s];
                    if ((x1f - sx) * e0y - (y1f - sy) * e0x >= 0 &&
                        (x2f - sx) * e1y - (y2f - sy) * e1x >= 0 &&
                        (x0f - sx) * e2y - (y0f - sy) * e2x >= 0)
                        inside_count++;
                }

                if (inside_count == 0)
                    continue;
                if (inside_count == 4)
                    buf[row + px] = c.fg;
                else
                    blendPixel(px, py, gamma[(uint8_t)((inside_count * 255) >> 2)]);
            }
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

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
                blendStore(buf + py * stride + px, c, alpha);
        };

        for (int16_t py = minY; py <= maxY; ++py)
        {
            if (py < clipY || py > clipB)
                continue;
            for (int16_t px = minX; px <= maxX; ++px)
            {
                if (px < clipX || px > clipR)
                    continue;
                const float px_f = px + 0.5f, py_f = py + 0.5f;

                float pd0x = px_f - v0x, pd0y = py_f - v0y;
                float t0 = (pd0x * e0x + pd0y * e0y) * inv_len0;
                t0 = t0 < 0.0f ? 0.0f : (t0 > 1.0f ? 1.0f : t0);
                float dx0 = pd0x - e0x * t0, dy0 = pd0y - e0y * t0;
                float min_d_sq = dx0 * dx0 + dy0 * dy0;

                float pd1x = px_f - v1x, pd1y = py_f - v1y;
                float t1 = (pd1x * e1x + pd1y * e1y) * inv_len1;
                t1 = t1 < 0.0f ? 0.0f : (t1 > 1.0f ? 1.0f : t1);
                float dx1 = pd1x - e1x * t1, dy1 = pd1y - e1y * t1;
                float d1_sq = dx1 * dx1 + dy1 * dy1;
                if (d1_sq < min_d_sq)
                    min_d_sq = d1_sq;

                float pd2x = px_f - v2x, pd2y = py_f - v2y;
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
                        blendPixel(px, py, gamma[aa]);
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

        auto blendFast = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            blendStore(ptr, c, alpha);
        };

        for (int16_t py = minY; py <= maxY; ++py)
        {
            if (py < clipY || py > clipB)
                continue;
            const float py_f = py + 0.5f;
            const float dy0 = py_f - v0y, dy1 = py_f - v1y, dy2 = py_f - v2y;
            const int32_t row_offset = py * stride;

            for (int16_t px = minX; px <= maxX; ++px)
            {
                if (px < clipX || px > clipR)
                    continue;
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

                const float r_lim = radius + 1.0f;
                if (d_sq > r_lim * r_lim)
                    continue;

                const uint8_t a = calcAlphaSDF(sqrtf(d_sq) - radius);
                if (a == 255)
                    buf[row_offset + px] = c.fg;
                else if (a > 0)
                    blendFast(buf + row_offset + px, gamma[a]);
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
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = spr->height();
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        const int32_t clipR = clipX + clipW - 1, clipB = clipY + clipH - 1;

        if (cx + r < clipX || cx - r > clipR || cy + r < clipY || cy - r > clipB)
            return;

        const float fr = (float)r;
        const float r4 = fr * fr * fr * fr;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px < clipX || px > clipR || py < clipY || py > clipB)
                return;
            blendStore(buf + py * stride + px, c, alpha);
        };

        int16_t x_start = cx - r, x_end = cx + r;
        if (x_start < clipX)
            x_start = (int16_t)clipX;
        if (x_end > clipR)
            x_end = (int16_t)clipR;

        for (int16_t px = x_start; px <= x_end; ++px)
        {
            const float dx = (float)(px - cx);
            const float rem = r4 - dx * dx * dx * dx;
            if (rem < 0.0f)
                continue;

            const float fy = sqrtf(sqrtf(rem));
            const int16_t yi = (int16_t)fy;
            const float frac = fy - (float)yi;

            int16_t y_top = cy - yi, y_bot = cy + yi;
            if (y_top < clipY)
                y_top = (int16_t)clipY;
            if (y_bot > clipB)
                y_bot = (int16_t)clipB;

            uint16_t *row = buf + y_top * stride + px;
            for (int16_t py = y_top; py <= y_bot; ++py)
            {
                *row = c.fg;
                row += stride;
            }

            const uint8_t alpha = (uint8_t)(frac * 255.0f);
            if (alpha > 0 && alpha < 255)
            {
                const uint8_t ag = gamma[alpha];
                int16_t py_out = cy - yi - 1;
                if (py_out >= clipY && py_out <= clipB)
                    blendPixel(px, py_out, ag);
                py_out = cy + yi + 1;
                if (py_out >= clipY && py_out <= clipB)
                    blendPixel(px, py_out, ag);
            }
        }

        int16_t y_start = cy - r, y_end = cy + r;
        if (y_start < clipY)
            y_start = (int16_t)clipY;
        if (y_end > clipB)
            y_end = (int16_t)clipB;

        for (int16_t py = y_start; py <= y_end; ++py)
        {
            const float dy = (float)(py - cy);
            const float rem = r4 - dy * dy * dy * dy;
            if (rem < 0.0f)
                continue;

            const float fx = sqrtf(sqrtf(rem));
            const int16_t xi = (int16_t)fx;
            const float frac = fx - (float)xi;

            const uint8_t alpha = (uint8_t)(frac * 255.0f);
            if (alpha > 0 && alpha < 255)
            {
                const uint8_t ag = gamma[alpha];
                int16_t px_out = cx - xi - 1;
                if (px_out >= clipX && px_out <= clipR)
                    blendPixel(px_out, py, ag);
                px_out = cx + xi + 1;
                if (px_out >= clipX && px_out <= clipR)
                    blendPixel(px_out, py, ag);
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 1, cy - r - 1, r * 2 + 3, r * 2 + 3);
    }

    void GUI::drawSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
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

        if (cx + r + 2 < clipX || cx - r - 2 > clipR ||
            cy + r + 2 < clipY || cy - r - 2 > clipB)
            return;

        const float fr = (float)r;
        const float r4 = fr * fr * fr * fr;
        const float r2 = fr * fr;
        const float strokeWidth = 1.0f;

        const Color565 c = makeColor565(color);
        const uint8_t *gamma = gammaTable();

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px < clipX || px > clipR || py < clipY || py > clipB)
                return;
            blendStore(buf + py * stride + px, c, alpha);
        };

        int16_t x0 = cx - r - 2, x1 = cx + r + 2;
        int16_t y0 = cy - r - 2, y1 = cy + r + 2;
        if (x0 < clipX)
            x0 = (int16_t)clipX;
        if (x1 > clipR)
            x1 = (int16_t)clipR;
        if (y0 < clipY)
            y0 = (int16_t)clipY;
        if (y1 > clipB)
            y1 = (int16_t)clipB;

        for (int16_t py = y0; py <= y1; ++py)
        {
            const float dy = (float)(py - cy);
            const float dy2 = dy * dy;
            const float dy4 = dy2 * dy2;
            const float dy6 = dy4 * dy2;

            for (int16_t px = x0; px <= x1; ++px)
            {
                const float dx = (float)(px - cx);
                const float dx2 = dx * dx;
                const float dx4 = dx2 * dx2;
                const float dx6 = dx4 * dx2;

                const float f = dx4 + dy4 - r4;
                const float grad_mag = 4.0f * sqrtf(dx6 + dy6);
                const float dist = (grad_mag > 1e-6f) ? f / grad_mag : f / (4.0f * r2);

                const float halfWidth = strokeWidth * 0.5f;
                const float d_abs = fabsf(dist);

                if (d_abs <= halfWidth + 1.0f)
                {
                    float alpha_f = 1.0f - (d_abs - halfWidth + 0.5f);
                    if (alpha_f < 0.0f)
                        alpha_f = 0.0f;
                    if (alpha_f > 1.0f)
                        alpha_f = 1.0f;
                    const uint8_t alpha = (uint8_t)(alpha_f * 255.0f);
                    if (alpha > 0)
                        blendPixel(px, py, gamma[alpha]);
                }
            }
        }

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect(cx - r - 2, cy - r - 2, r * 2 + 5, r * 2 + 5);
    }

}

#include <pipGUI/Core/pipGUI.hpp>
#include "Blend.hpp"
#include <pipGUI/Graphics/Utils/BlueNoise.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <cstring>

namespace pipgui
{
    namespace
    {
        static inline uint16_t quantize565Swapped(Color888 color, int16_t x, int16_t y)
        {
            return pipcore::Sprite::swap16(detail::quantize565(color, x, y));
        }

        static inline void fillPattern32(uint16_t *dst, int16_t x, int16_t count, const uint16_t *pattern)
        {
            if (count <= 0)
                return;

            uint8_t patternIdx = (uint8_t)(x & 31);
            while (count > 0 && patternIdx)
            {
                *dst++ = pattern[patternIdx];
                patternIdx = (uint8_t)((patternIdx + 1) & 31);
                --count;
            }

            while (count >= 32)
            {
                std::memcpy(dst, pattern, sizeof(uint16_t) * 32);
                dst += 32;
                count -= 32;
            }

            for (uint8_t idx = 0; count > 0; --count)
                *dst++ = pattern[idx++];
        }
    }

    void GUI::fillRectGradientVertical(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t topColor, uint32_t bottomColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);
        if (x0 >= x1 || y0 >= y1)
            return;

        const int32_t r1 = (topColor >> 16) & 0xFF, g1 = (topColor >> 8) & 0xFF, b1 = topColor & 0xFF;
        const int32_t r2 = (bottomColor >> 16) & 0xFF, g2 = (bottomColor >> 8) & 0xFF, b2 = bottomColor & 0xFF;

        const int32_t div = (h > 1) ? (h - 1) : 1;
        const int32_t dr = ((r2 - r1) << 16) / div;
        const int32_t dg = ((g2 - g1) << 16) / div;
        const int32_t db = ((b2 - b1) << 16) / div;
        const int32_t doff = y0 - y;

        int32_t cr = (r1 << 16) + dr * doff;
        int32_t cg = (g1 << 16) + dg * doff;
        int32_t cb = (b1 << 16) + db * doff;

        for (int16_t py = y0; py < y1; ++py, cr += dr, cg += dg, cb += db)
        {
            const Color888 c888 = {(uint8_t)(cr >> 16), (uint8_t)(cg >> 16), (uint8_t)(cb >> 16)};
            uint16_t pattern[32];
            for (uint8_t idx = 0; idx < 32; ++idx)
                pattern[idx] = quantize565Swapped(c888, idx, py);

            uint16_t *dst = buf + py * stride + x0;
            fillPattern32(dst, x0, (int16_t)(x1 - x0), pattern);
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(x0, y0, x1 - x0, y1 - y0);
    }

    void GUI::fillRectGradientHorizontal(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t leftColor, uint32_t rightColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);
        if (x0 >= x1 || y0 >= y1)
            return;

        const int32_t r1 = (leftColor >> 16) & 0xFF, g1 = (leftColor >> 8) & 0xFF, b1 = leftColor & 0xFF;
        const int32_t r2 = (rightColor >> 16) & 0xFF, g2 = (rightColor >> 8) & 0xFF, b2 = rightColor & 0xFF;

        const int32_t div = (w > 1) ? (w - 1) : 1;
        const int32_t dr = ((r2 - r1) << 16) / div;
        const int32_t dg = ((g2 - g1) << 16) / div;
        const int32_t db = ((b2 - b1) << 16) / div;

        const int32_t cr0 = (r1 << 16) + dr * (x0 - x);
        const int32_t cg0 = (g1 << 16) + dg * (x0 - x);
        const int32_t cb0 = (b1 << 16) + db * (x0 - x);

        for (int16_t py = y0; py < y1; ++py)
        {
            uint16_t *dst = buf + py * stride + x0;
            int32_t cr = cr0, cg = cg0, cb = cb0;
            for (int16_t px = x0; px < x1; ++px, ++dst, cr += dr, cg += dg, cb += db)
            {
                const Color888 c = {(uint8_t)(cr >> 16), (uint8_t)(cg >> 16), (uint8_t)(cb >> 16)};
                *dst = quantize565Swapped(c, px, py);
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(x0, y0, x1 - x0, y1 - y0);
    }

    void GUI::fillRectGradientCorners(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);
        if (x0 >= x1 || y0 >= y1)
            return;

        const int32_t r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
        const int32_t r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
        const int32_t r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
        const int32_t r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

        const int32_t divY = (h > 1) ? (h - 1) : 1;
        const int32_t divX = (w > 1) ? (w - 1) : 1;

        const int32_t drL = ((r01 - r00) << 16) / divY, dgL = ((g01 - g00) << 16) / divY, dbL = ((b01 - b00) << 16) / divY;
        const int32_t drR = ((r11 - r10) << 16) / divY, dgR = ((g11 - g10) << 16) / divY, dbR = ((b11 - b10) << 16) / divY;

        const int32_t dy = y0 - y, dx = x0 - x;

        int32_t crL = (r00 << 16) + drL * dy, cgL = (g00 << 16) + dgL * dy, cbL = (b00 << 16) + dbL * dy;
        int32_t crR = (r10 << 16) + drR * dy, cgR = (g10 << 16) + dgR * dy, cbR = (b10 << 16) + dbR * dy;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t dr_row = (crR - crL) / divX;
            const int32_t dg_row = (cgR - cgL) / divX;
            const int32_t db_row = (cbR - cbL) / divX;

            int32_t cr = crL + dr_row * dx;
            int32_t cg = cgL + dg_row * dx;
            int32_t cb = cbL + db_row * dx;

            uint16_t *dst = buf + py * stride + x0;
            for (int16_t px = x0; px < x1; ++px, ++dst, cr += dr_row, cg += dg_row, cb += db_row)
            {
                const Color888 c888 = {(uint8_t)(cr >> 16), (uint8_t)(cg >> 16), (uint8_t)(cb >> 16)};
                *dst = quantize565Swapped(c888, px, py);
            }

            crL += drL;
            cgL += dgL;
            cbL += dbL;
            crR += drR;
            cgR += dgR;
            cbR += dbR;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::fillRectGradientDiagonal(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tlColor, uint32_t brColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);
        if (x0 >= x1 || y0 >= y1)
            return;

        const int32_t r1 = (tlColor >> 16) & 0xFF, g1 = (tlColor >> 8) & 0xFF, b1 = tlColor & 0xFF;
        const int32_t r2 = (brColor >> 16) & 0xFF, g2 = (brColor >> 8) & 0xFF, b2 = brColor & 0xFF;

        const int32_t total = (w + h > 2) ? (w + h - 2) : 1;
        const int32_t dr = ((r2 - r1) << 16) / total;
        const int32_t dg = ((g2 - g1) << 16) / total;
        const int32_t db = ((b2 - b1) << 16) / total;

        const int32_t base = (x0 - x) + (y0 - y);
        int32_t rr0 = (r1 << 16) + dr * base;
        int32_t rg0 = (g1 << 16) + dg * base;
        int32_t rb0 = (b1 << 16) + db * base;

        for (int16_t py = y0; py < y1; ++py, rr0 += dr, rg0 += dg, rb0 += db)
        {
            uint16_t *dst = buf + py * stride + x0;
            int32_t cr = rr0, cg = rg0, cb = rb0;
            for (int16_t px = x0; px < x1; ++px, ++dst, cr += dr, cg += dg, cb += db)
            {
                const Color888 c888 = {(uint8_t)(cr >> 16), (uint8_t)(cg >> 16), (uint8_t)(cb >> 16)};
                *dst = quantize565Swapped(c888, px, py);
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::fillRectGradientRadial(int16_t x, int16_t y, int16_t w, int16_t h, int16_t cx, int16_t cy, int16_t radius, uint32_t innerColor, uint32_t outerColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0 || !_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);
        if (x0 >= x1 || y0 >= y1)
            return;

        if (radius <= 0)
            radius = 1;

        const int32_t ri = (innerColor >> 16) & 0xFF, gi = (innerColor >> 8) & 0xFF, bi = innerColor & 0xFF;
        const int32_t dr = (int32_t)((outerColor >> 16) & 0xFF) - ri;
        const int32_t dg = (int32_t)((outerColor >> 8) & 0xFF) - gi;
        const int32_t db = (int32_t)(outerColor & 0xFF) - bi;
        const Color888 outer888 = {
            (uint8_t)((outerColor >> 16) & 0xFF),
            (uint8_t)((outerColor >> 8) & 0xFF),
            (uint8_t)(outerColor & 0xFF)};

        const int32_t r256 = radius;
        const int32_t radiusSq = r256 * r256;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t ddy = (int32_t)(py - cy);
            const int32_t dy2 = ddy * ddy;
            uint16_t *dst = buf + py * stride + x0;
            int32_t ddx = (int32_t)(x0 - cx);
            int32_t d2 = ddx * ddx + dy2;
            int32_t dStep = ddx * 2 + 1;
            uint16_t outerPattern[32];
            for (uint8_t idx = 0; idx < 32; ++idx)
                outerPattern[idx] = quantize565Swapped(outer888, idx, py);

            for (int16_t px = x0; px < x1; ++px, ++dst)
            {
                int32_t t;
                if (d2 == 0)
                {
                    t = 0;
                }
                else if (d2 >= radiusSq)
                {
                    t = 256;
                }
                else
                {
                    const int32_t dist = (int32_t)isqrt32((uint32_t)d2);
                    t = (dist << 8) / r256;
                }

                const Color888 c888 = {
                    (uint8_t)(ri + ((dr * t) >> 8)),
                    (uint8_t)(gi + ((dg * t) >> 8)),
                    (uint8_t)(bi + ((db * t) >> 8))};
                *dst = quantize565Swapped(c888, px, py);
                d2 += dStep;
                dStep += 2;

                if (d2 >= radiusSq && dStep >= 0 && px + 1 < x1)
                {
                    fillPattern32(dst + 1, (int16_t)(px + 1), (int16_t)(x1 - (px + 1)), outerPattern);
                    break;
                }
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.inSpritePass)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }
}

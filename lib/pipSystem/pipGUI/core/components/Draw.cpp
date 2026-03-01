#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/components/BNSD.hpp>
#include <pipGUI/core/Debug.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <math.h>

namespace pipgui
{

    static inline void applyGuiClipToSprite(pipcore::Sprite *spr,
                                            bool enabled,
                                            int16_t x,
                                            int16_t y,
                                            int16_t w,
                                            int16_t h)
    {
        if (!spr)
            return;
        if (!enabled)
        {
            spr->setClipRect(0, 0, spr->width(), spr->height());
            return;
        }
        spr->setClipRect(x, y, w, h);
    }

    uint16_t GUI::screenWidth() const { return _render.screenWidth; }
    uint16_t GUI::screenHeight() const { return _render.screenHeight; }

    uint16_t GUI::rgb(uint8_t r, uint8_t g, uint8_t b) const
    {
        return pipcore::Sprite::color565(r, g, b);
    }

    void GUI::fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color565)
    {
        auto spr = getDrawTarget();
        if (!_flags.spriteEnabled || r <= 0 || !spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        int32_t stride = spr->width(), maxH = spr->height();
        if (!buf || stride <= 0 || maxH <= 0)
            return;

        int32_t cX = 0, cY = 0, cW = stride, cH = maxH;
        spr->getClipRect(&cX, &cY, &cW, &cH);
        int32_t cR = cX + cW - 1, cB = cY + cH - 1;

        if (cx - r > cR || cx + r < cX || cy - r > cB || cy + r < cY)
            return;

        const float fr = (float)r, r2 = fr * fr, i2r = 0.5f / fr;
        const float rm2 = (fr - 0.5f) * (fr - 0.5f), rp2 = (fr + 0.5f) * (fr + 0.5f);

        const uint16_t fg = pipcore::Sprite::swap16(color565);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;
        const uint32_t fg_rb = ((color565 & 0xF800) << 5) | (color565 & 0x001F);
        const uint32_t fg_g = (color565 & 0x07E0);

        auto blendFast = [&](uint16_t bg, uint8_t alpha) __attribute__((always_inline)) -> uint16_t
        {
            uint32_t inv = 255 - alpha;
            uint32_t b_rb = ((bg & 0xF800) << 5) | (bg & 0x001F), b_g = bg & 0x07E0;
            uint32_t rb = ((fg_rb * alpha + b_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + b_g * inv) >> 8) & 0x000007E0;
            return (uint16_t)((rb >> 5) | rb | g);
        };

        int16_t sx = r, ax = r + 1;

        for (int16_t y = 0; y <= r; ++y)
        {
            float dy2 = (float)(y * y);
            while (sx >= 0 && (float)(sx * sx) + dy2 > rm2)
                sx--;
            while (ax >= 0 && (float)(ax * ax) + dy2 > rp2)
                ax--;

            int16_t pys[2];
            int num_py = 0;
            if (cy + y >= cY && cy + y <= cB)
                pys[num_py++] = cy + y;
            if (y > 0 && cy - y >= cY && cy - y <= cB)
                pys[num_py++] = cy - y;

            for (int i = 0; i < num_py; ++i)
            {
                int32_t row_off = (int32_t)pys[i] * stride;

                if (sx >= 0)
                {
                    int16_t fx0 = cx - sx < cX ? cX : cx - sx, fx1 = cx + sx > cR ? cR : cx + sx;
                    if (fx0 <= fx1)
                    {
                        uint16_t *d = buf + row_off + fx0;
                        int16_t c = fx1 - fx0 + 1;
                        if (c > 0 && ((uintptr_t)d & 2))
                        {
                            *d++ = fg;
                            c--;
                        }
                        uint32_t *d32 = (uint32_t *)d;
                        int16_t c32 = c >> 1;
                        while (c32--)
                            *d32++ = fg32;
                        if (c & 1)
                            *(uint16_t *)d32 = fg;
                    }
                }

                for (int16_t x = sx < 0 ? 0 : sx + 1; x <= ax; ++x)
                {
                    float dist = (r2 - ((float)(x * x) + dy2)) * i2r;
                    if (dist > -0.5f)
                    {
                        uint8_t a = 255;
                        if (dist < 0.5f)
                        {
                            float t = dist + 0.5f;
                            a = (uint8_t)(t * t * (765.0f - 510.0f * t));
                        }
                        if (a > 0)
                        {
                            int16_t px_r = cx + x;
                            if (px_r >= cX && px_r <= cR)
                                buf[row_off + px_r] = (a == 255) ? fg : pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[row_off + px_r]), a));

                            if (x > 0)
                            {
                                int16_t px_l = cx - x;
                                if (px_l >= cX && px_l <= cR)
                                    buf[row_off + px_l] = (a == 255) ? fg : pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[row_off + px_l]), a));
                            }
                        }
                    }
                }
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect((int16_t)(cx - r), (int16_t)(cy - r), (int16_t)(r * 2 + 1), (int16_t)(r * 2 + 1));
    }

    void GUI::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radiusTL, uint8_t radiusTR,
                            uint8_t radiusBR, uint8_t radiusBL, uint16_t color565)
    {
        if (!_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        int16_t maxR = (w < h ? w : h) / 2;
        uint8_t rTL = (radiusTL > maxR) ? (uint8_t)maxR : radiusTL;
        uint8_t rTR = (radiusTR > maxR) ? (uint8_t)maxR : radiusTR;
        uint8_t rBR = (radiusBR > maxR) ? (uint8_t)maxR : radiusBR;
        uint8_t rBL = (radiusBL > maxR) ? (uint8_t)maxR : radiusBL;

        if (rTL == 0 && rTR == 0 && rBR == 0 && rBL == 0)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (x > clipR || x + w - 1 < clipX || y > clipB || y + h - 1 < clipY)
            return;

        const uint16_t fg = pipcore::Sprite::swap16(color565);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;

        const uint32_t fg_rb = ((color565 & 0xF800) << 5) | (color565 & 0x001F);
        const uint32_t fg_g = (color565 & 0x07E0);

        auto blendFast = [&](uint16_t bg, uint8_t alpha) __attribute__((always_inline)) -> uint16_t
        {
            uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
            uint32_t bg_g = (bg & 0x07E0);
            uint32_t inv = 255 - alpha;
            uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
            return (uint16_t)((rb >> 5) | rb | g);
        };

        auto fillSpan = [&](int16_t py, int16_t x0, int16_t x1) __attribute__((always_inline))
        {
            if (x0 < clipX)
                x0 = clipX;
            if (x1 > clipR)
                x1 = clipR;
            if (x0 <= x1)
            {
                uint16_t *dst = buf + py * stride + x0;
                int16_t count = x1 - x0 + 1;
                if (count > 0 && ((uintptr_t)dst & 2))
                {
                    *dst++ = fg;
                    count--;
                }
                uint32_t *dst32 = (uint32_t *)dst;
                int16_t count32 = count >> 1;
                while (count32--)
                    *dst32++ = fg32;
                if (count & 1)
                    *(uint16_t *)dst32 = fg;
            }
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist >= 0.5f)
                return 255;
            if (dist <= -0.5f)
                return 0;
            float t = dist + 0.5f;
            return (uint8_t)(t * t * (765.0f - 510.0f * t));
        };

        if (rTL == rTR && rTR == rBR && rBR == rBL)
        {
            float fr = (float)rTL, r2 = fr * fr, inv_2r = 0.5f / fr;
            float r_minus_sq = (fr - 0.5f) * (fr - 0.5f), rp2 = (fr + 0.5f) * (fr + 0.5f);

            int16_t cxL = x + rTL, cxR = x + w - rTL - 1;
            int16_t cyT = y + rTL, cyB = y + h - rTL - 1;

            int16_t solid_dx = rTL;
            int16_t aa_dx = rTL + 1;

            for (int16_t dy_int = 1; dy_int <= rTL; ++dy_int)
            {
                float dy2 = (float)(dy_int * dy_int);

                while (solid_dx >= 0 && (float)(solid_dx * solid_dx) + dy2 > r_minus_sq)
                    solid_dx--;
                while (aa_dx >= 0 && (float)(aa_dx * aa_dx) + dy2 > rp2)
                    aa_dx--;

                int16_t py_T = cyT - dy_int, py_B = cyB + dy_int;
                bool draw_T = (py_T >= clipY && py_T <= clipB);
                bool draw_B = (py_B >= clipY && py_B <= clipB);
                if (!draw_T && !draw_B)
                    continue;

                if (draw_T)
                    fillSpan(py_T, cxL - solid_dx, cxR + solid_dx);
                if (draw_B)
                    fillSpan(py_B, cxL - solid_dx, cxR + solid_dx);

                int16_t xs = solid_dx < 0 ? 0 : solid_dx + 1;
                for (int16_t dx = xs; dx <= aa_dx; ++dx)
                {
                    uint8_t a = calcAlpha((r2 - ((float)(dx * dx) + dy2)) * inv_2r);
                    if (a > 0)
                    {
                        int16_t px_LL = cxL - dx, px_RR = cxR + dx;
                        if (draw_T)
                        {
                            if (a == 255)
                            {
                                if (px_LL >= clipX && px_LL <= clipR)
                                    buf[py_T * stride + px_LL] = fg;
                                if (px_RR >= clipX && px_RR <= clipR)
                                    buf[py_T * stride + px_RR] = fg;
                            }
                            else
                            {
                                if (px_LL >= clipX && px_LL <= clipR)
                                    buf[py_T * stride + px_LL] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py_T * stride + px_LL]), a));
                                if (px_RR >= clipX && px_RR <= clipR)
                                    buf[py_T * stride + px_RR] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py_T * stride + px_RR]), a));
                            }
                        }
                        if (draw_B)
                        {
                            if (a == 255)
                            {
                                if (px_LL >= clipX && px_LL <= clipR)
                                    buf[py_B * stride + px_LL] = fg;
                                if (px_RR >= clipX && px_RR <= clipR)
                                    buf[py_B * stride + px_RR] = fg;
                            }
                            else
                            {
                                if (px_LL >= clipX && px_LL <= clipR)
                                    buf[py_B * stride + px_LL] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py_B * stride + px_LL]), a));
                                if (px_RR >= clipX && px_RR <= clipR)
                                    buf[py_B * stride + px_RR] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py_B * stride + px_RR]), a));
                            }
                        }
                    }
                }
            }

            int16_t mid_y_start = cyT < clipY ? clipY : cyT;
            int16_t mid_y_end = cyB > clipB ? clipB : cyB;
            for (int16_t py = mid_y_start; py <= mid_y_end; ++py)
                fillSpan(py, x, x + w - 1);
        }

        else
        {
            for (int16_t py = y; py < y + h; ++py)
            {
                if (py < clipY || py > clipB)
                    continue;

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
                    float fr = (float)rL, r2 = fr * fr, inv_2r = 0.5f / fr;
                    float dy2 = dyL * dyL, r_minus = fr - 0.5f, r_plus = fr + 0.5f;
                    int16_t solid_dx = r_minus * r_minus >= dy2 ? (int16_t)sqrtf(r_minus * r_minus - dy2) : -1;
                    int16_t aa_dx = r_plus * r_plus >= dy2 ? (int16_t)sqrtf(r_plus * r_plus - dy2) : -1;

                    solid_x0 = cxL - solid_dx;
                    for (int16_t dx = (solid_dx < 0 ? 0 : solid_dx + 1); dx <= aa_dx; ++dx)
                    {
                        uint8_t a = calcAlpha((r2 - ((float)(dx * dx) + dy2)) * inv_2r);
                        int16_t px = cxL - dx;
                        if (px >= clipX && px <= clipR)
                        {
                            if (a == 255)
                                buf[py * stride + px] = fg;
                            else if (a > 0)
                                buf[py * stride + px] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py * stride + px]), a));
                        }
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
                    float fr = (float)rR, r2 = fr * fr, inv_2r = 0.5f / fr;
                    float dy2 = dyR * dyR, r_minus = fr - 0.5f, r_plus = fr + 0.5f;
                    int16_t solid_dx = r_minus * r_minus >= dy2 ? (int16_t)sqrtf(r_minus * r_minus - dy2) : -1;
                    int16_t aa_dx = r_plus * r_plus >= dy2 ? (int16_t)sqrtf(r_plus * r_plus - dy2) : -1;

                    solid_x1 = cxR + solid_dx;
                    for (int16_t dx = (solid_dx < 0 ? 0 : solid_dx + 1); dx <= aa_dx; ++dx)
                    {
                        uint8_t a = calcAlpha((r2 - ((float)(dx * dx) + dy2)) * inv_2r);
                        int16_t px = cxR + dx;
                        if (px >= clipX && px <= clipR)
                        {
                            if (a == 255)
                                buf[py * stride + px] = fg;
                            else if (a > 0)
                                buf[py * stride + px] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[py * stride + px]), a));
                        }
                    }
                }

                fillSpan(py, solid_x0, solid_x1);
            }
        }

        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(x, y, w, h);
    }

    void GUI::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color565)
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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (x > clipR || x + w - 1 < clipX || y > clipB || y + h - 1 < clipY)
            return;

        int16_t maxR = (w < h ? w : h) / 2;
        uint8_t rTL = (radiusTL > maxR) ? (uint8_t)maxR : radiusTL;
        uint8_t rTR = (radiusTR > maxR) ? (uint8_t)maxR : radiusTR;
        uint8_t rBR = (radiusBR > maxR) ? (uint8_t)maxR : radiusBR;
        uint8_t rBL = (radiusBL > maxR) ? (uint8_t)maxR : radiusBL;

        const uint16_t fg = pipcore::Sprite::swap16(color565);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;

        const uint32_t fg_rb = ((color565 & 0xF800) << 5) | (color565 & 0x001F);
        const uint32_t fg_g = (color565 & 0x07E0);

        auto blendFast = [&](uint16_t bg, uint8_t alpha) __attribute__((always_inline)) -> uint16_t
        {
            uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
            uint32_t bg_g = (bg & 0x07E0);
            uint32_t inv = 255 - alpha;
            uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
            return (uint16_t)((rb >> 5) | rb | g);
        };

        auto fillHLine = [&](int16_t py, int16_t px1, int16_t px2) __attribute__((always_inline))
        {
            if (py < clipY || py > clipB)
                return;
            if (px1 < clipX)
                px1 = clipX;
            if (px2 > clipR)
                px2 = clipR;
            if (px1 > px2)
                return;
            uint16_t *dst = buf + py * stride + px1;
            int16_t count = px2 - px1 + 1;
            if (count > 0 && ((uintptr_t)dst & 2))
            {
                *dst++ = fg;
                count--;
            }
            uint32_t *dst32 = (uint32_t *)dst;
            int16_t count32 = count >> 1;
            while (count32--)
                *dst32++ = fg32;
            if (count & 1)
                *(uint16_t *)dst32 = fg;
        };

        auto fillVLine = [&](int16_t px, int16_t py1, int16_t py2) __attribute__((always_inline))
        {
            if (px < clipX || px > clipR)
                return;
            if (py1 < clipY)
                py1 = clipY;
            if (py2 > clipB)
                py2 = clipB;
            if (py1 > py2)
                return;
            uint16_t *dst = buf + py1 * stride + px;
            int16_t count = py2 - py1 + 1;
            while (count--)
            {
                *dst = fg;
                dst += stride;
            }
        };

        auto blendPixelIdx = [&](int32_t idx, uint8_t alpha) __attribute__((always_inline))
        {
            buf[idx] = pipcore::Sprite::swap16(blendFast(pipcore::Sprite::swap16(buf[idx]), alpha));
        };

        int16_t top_x1 = x + rTL, top_x2 = x + w - rTR - 1;
        if (top_x1 <= top_x2)
            fillHLine(y, top_x1, top_x2);

        int16_t bot_x1 = x + rBL, bot_x2 = x + w - rBR - 1;
        if (bot_x1 <= bot_x2)
            fillHLine(y + h - 1, bot_x1, bot_x2);

        int16_t left_y1 = y + (rTL > 0 ? rTL : 1), left_y2 = y + h - (rBL > 0 ? rBL : 1) - 1;
        if (left_y1 <= left_y2)
            fillVLine(x, left_y1, left_y2);

        int16_t right_y1 = y + (rTR > 0 ? rTR : 1), right_y2 = y + h - (rBR > 0 ? rBR : 1) - 1;
        if (right_y1 <= right_y2)
            fillVLine(x + w - 1, right_y1, right_y2);

        if (rTL == rTR && rTR == rBL && rBL == rBR && rTL > 0)
        {
            uint8_t r = rTL;
            float fr = (float)r;
            float r_out2 = (fr + 0.5f) * (fr + 0.5f), inv_2r_out = 0.5f / (fr + 0.5f);
            float r_in2 = (fr - 0.5f) * (fr - 0.5f), inv_2r_in = 0.5f / (fr - 0.5f);

            int16_t cxL = x + r, cxR = x + w - r - 1;
            int16_t cyT = y + r, cyB = y + h - r - 1;
            int16_t dx_start = r;

            for (int16_t dy_int = 1; dy_int <= r; ++dy_int)
            {
                float dy2 = (float)(dy_int * dy_int);

                while (dx_start >= 0)
                {
                    if ((r_out2 - (float)(dx_start * dx_start) - dy2) * inv_2r_out > -0.5f)
                        break;
                    dx_start--;
                }

                int16_t py_T = cyT - dy_int, py_B = cyB + dy_int;
                bool draw_T = (py_T >= clipY && py_T <= clipB);
                bool draw_B = (py_B >= clipY && py_B <= clipB);
                if (!draw_T && !draw_B)
                    continue;

                for (int16_t dx_int = dx_start; dx_int >= 0; --dx_int)
                {
                    float S = (float)(dx_int * dx_int) + dy2;

                    float d_out = (r_out2 - S) * inv_2r_out;
                    if (d_out <= -0.5f)
                        continue;

                    float d_in = (r_in2 - S) * inv_2r_in;
                    if (d_in >= 0.5f)
                        break;

                    uint8_t a_out = 255;
                    if (d_out < 0.5f)
                    {
                        float t = d_out + 0.5f;
                        a_out = (uint8_t)(t * t * (765.0f - 510.0f * t));
                    }

                    uint8_t a_in = 0;
                    if (d_in > -0.5f)
                    {
                        float t = d_in + 0.5f;
                        a_in = (uint8_t)(t * t * (765.0f - 510.0f * t));
                    }

                    uint8_t alpha = (a_out > a_in) ? (a_out - a_in) : 0;
                    if (alpha == 0)
                        continue;

                    int16_t px_L = cxL - dx_int, px_R = cxR + dx_int;

                    if (alpha == 255)
                    {
                        if (draw_T)
                        {
                            if (px_L >= clipX && px_L <= clipR)
                                buf[py_T * stride + px_L] = fg;
                            if (px_R >= clipX && px_R <= clipR)
                                buf[py_T * stride + px_R] = fg;
                        }
                        if (draw_B)
                        {
                            if (px_L >= clipX && px_L <= clipR)
                                buf[py_B * stride + px_L] = fg;
                            if (px_R >= clipX && px_R <= clipR)
                                buf[py_B * stride + px_R] = fg;
                        }
                    }
                    else
                    {
                        if (draw_T)
                        {
                            if (px_L >= clipX && px_L <= clipR)
                                blendPixelIdx(py_T * stride + px_L, alpha);
                            if (px_R >= clipX && px_R <= clipR)
                                blendPixelIdx(py_T * stride + px_R, alpha);
                        }
                        if (draw_B)
                        {
                            if (px_L >= clipX && px_L <= clipR)
                                blendPixelIdx(py_B * stride + px_L, alpha);
                            if (px_R >= clipX && px_R <= clipR)
                                blendPixelIdx(py_B * stride + px_R, alpha);
                        }
                    }
                }
            }
        }
        else
        {
            auto drawCorner = [&](int16_t cx, int16_t cy, int16_t px_s, int16_t px_e, int16_t py_s, int16_t py_e, uint8_t r)
            {
                if (r == 0)
                    return;
                float fr = (float)r;
                float r_out2 = (fr + 0.5f) * (fr + 0.5f), inv_2r_out = 0.5f / (fr + 0.5f);
                float r_in2 = (fr - 0.5f) * (fr - 0.5f), inv_2r_in = 0.5f / (fr - 0.5f);

                for (int16_t py = py_s; py <= py_e; ++py)
                {
                    if (py < clipY || py > clipB)
                        continue;

                    float dy2 = (float)((cy - py) * (cy - py));

                    for (int16_t px = px_s; px <= px_e; ++px)
                    {
                        if (px < clipX || px > clipR)
                            continue;
                        float dx = (float)(cx - px);
                        float S = dx * dx + dy2;

                        float d_out = (r_out2 - S) * inv_2r_out;
                        float d_in = (r_in2 - S) * inv_2r_in;

                        if (px_s <= cx)
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
                            float t = d_out + 0.5f;
                            a_out = (uint8_t)(t * t * (765.0f - 510.0f * t));
                        }

                        uint8_t a_in = 0;
                        if (d_in > -0.5f)
                        {
                            float t = d_in + 0.5f;
                            a_in = (uint8_t)(t * t * (765.0f - 510.0f * t));
                        }

                        uint8_t alpha = (a_out > a_in) ? (a_out - a_in) : 0;
                        if (alpha == 255)
                            buf[py * stride + px] = fg;
                        else if (alpha > 0)
                            blendPixelIdx(py * stride + px, alpha);
                    }
                }
            };

            drawCorner(x + rTL, y + rTL, x, x + rTL - 1, y, y + rTL - 1, rTL);
            drawCorner(x + w - rTR - 1, y + rTR, x + w - rTR, x + w - 1, y, y + rTR - 1, rTR);
            drawCorner(x + rBL, y + h - rBL - 1, x, x + rBL - 1, y + h - rBL, y + h - 1, rBL);
            drawCorner(x + w - rBR - 1, y + h - rBR - 1, x + w - rBR, x + w - 1, y + h - rBR, y + h - 1, rBR);
        }

        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(x, y, w, h);
    }

    void GUI::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color)
    {
        drawRoundRect(x, y, w, h, radius, radius, radius, radius, color);
    }

    void GUI::drawLine(int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       uint16_t color)
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

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        bool steep = abs(y1 - y0) > abs(x1 - x0);
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

        int dx = x1 - x0;
        int dy = y1 - y0;
        float len = sqrtf((float)(dx * dx) + (float)(dy * dy));
        if (len <= 0.0f)
            return;

        int minX = (x0 < x1 ? x0 : x1);
        int maxX = (x0 > x1 ? x0 : x1);
        int minY = (y0 < y1 ? y0 : y1);
        int maxY = (y0 > y1 ? y0 : y1);

        if (maxX < clipX - 1 || minX > clipR + 1 || maxY < clipY - 1 || minY > clipB + 1)
            return;

        bool noClip = (minX >= clipX + 1 && maxX <= clipR - 1 && minY >= clipY + 1 && maxY <= clipB - 1);

        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFastPtr = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            uint32_t bg = pipcore::Sprite::swap16(*ptr);
            uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
            uint32_t bg_g = (bg & 0x07E0);
            uint32_t inv = 255 - alpha;
            uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
            *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
        };

        auto blendFastClip = [&](int px, int py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
            {
                blendFastPtr(buf + py * stride + px, alpha);
            }
        };

        auto calcAlpha = [](float d) __attribute__((always_inline)) -> uint8_t
        {
            d = fabsf(d);
            if (d >= 1.0f)
                return 0;
            return (uint8_t)(255.0f - d * d * (765.0f - 510.0f * d));
        };

        if (noClip)
        {
            if (!steep)
            {
                float W = (float)abs(dx) / len;
                float y = (float)y0;
                float dy_dx = (float)dy / (float)abs(dx);

                for (int x = x0; x <= x1; ++x)
                {

                    int yi = (int)(y + 65536.5f) - 65536;
                    float dist0 = (y - (float)yi) * W;

                    uint8_t a0 = calcAlpha(dist0);
                    if (a0)
                        blendFastPtr(buf + yi * stride + x, a0);

                    uint8_t am = calcAlpha(dist0 + W);
                    if (am)
                        blendFastPtr(buf + (yi - 1) * stride + x, am);

                    uint8_t ap = calcAlpha(dist0 - W);
                    if (ap)
                        blendFastPtr(buf + (yi + 1) * stride + x, ap);

                    y += dy_dx;
                }
            }
            else
            {
                float W = (float)abs(dy) / len;
                float x = (float)x0;
                float dx_dy = (float)dx / (float)abs(dy);

                for (int y = y0; y <= y1; ++y)
                {
                    int xi = (int)(x + 65536.5f) - 65536;
                    float dist0 = (x - (float)xi) * W;
                    uint32_t row = y * stride;

                    uint8_t a0 = calcAlpha(dist0);
                    if (a0)
                        blendFastPtr(buf + row + xi, a0);

                    uint8_t am = calcAlpha(dist0 + W);
                    if (am)
                        blendFastPtr(buf + row + xi - 1, am);

                    uint8_t ap = calcAlpha(dist0 - W);
                    if (ap)
                        blendFastPtr(buf + row + xi + 1, ap);

                    x += dx_dy;
                }
            }
        }
        else
        {
            if (!steep)
            {
                float W = (float)abs(dx) / len;
                float y = (float)y0;
                float dy_dx = (float)dy / (float)abs(dx);

                for (int x = x0; x <= x1; ++x)
                {
                    if (x >= clipX && x <= clipR)
                    {
                        int yi = (int)(y + 65536.5f) - 65536;
                        float dist0 = (y - (float)yi) * W;

                        uint8_t a0 = calcAlpha(dist0);
                        if (a0)
                            blendFastClip(x, yi, a0);

                        uint8_t am = calcAlpha(dist0 + W);
                        if (am)
                            blendFastClip(x, yi - 1, am);

                        uint8_t ap = calcAlpha(dist0 - W);
                        if (ap)
                            blendFastClip(x, yi + 1, ap);
                    }
                    y += dy_dx;
                }
            }
            else
            {
                float W = (float)abs(dy) / len;
                float x = (float)x0;
                float dx_dy = (float)dx / (float)abs(dy);

                for (int y = y0; y <= y1; ++y)
                {
                    if (y >= clipY && y <= clipB)
                    {
                        int xi = (int)(x + 65536.5f) - 65536;
                        float dist0 = (x - (float)xi) * W;

                        uint8_t a0 = calcAlpha(dist0);
                        if (a0)
                            blendFastClip(xi, y, a0);

                        uint8_t am = calcAlpha(dist0 + W);
                        if (am)
                            blendFastClip(xi - 1, y, am);

                        uint8_t ap = calcAlpha(dist0 - W);
                        if (ap)
                            blendFastClip(xi + 1, y, ap);
                    }
                    x += dx_dy;
                }
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
        {
            invalidateRect((int16_t)(minX - 1), (int16_t)(minY - 1), (int16_t)(maxX - minX + 3), (int16_t)(maxY - minY + 3));
        }
    }

    void GUI::drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        drawArc(cx, cy, r, 0.0f, 360.0f, color);
    }

    void GUI::drawArc(int16_t cx, int16_t cy, int16_t r, float startDeg, float endDeg, uint16_t color)
    {
        if (r <= 0 || !_flags.spriteEnabled)
            return;

        float startRad = fmodf(startDeg, 360.0f);
        if (startRad < 0.0f)
            startRad += 360.0f;
        float endRad = fmodf(endDeg, 360.0f);
        if (endRad < 0.0f)
            endRad += 360.0f;

        if (fabsf(startRad - endRad) < 0.001f || fabsf(fabsf(startDeg - endDeg) - 360.0f) < 0.001f)
        {
        }

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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (cx + r + 2 < clipX || cx - r - 2 > clipR || cy + r + 2 < clipY || cy - r - 2 > clipB)
            return;

        const float fr = (float)r;
        const float r2 = fr * fr;
        const float r_min = fr - 1.0f;
        const float r_max = fr + 1.0f;
        const float r_min2 = r_min * r_min;
        const float r_max2 = r_max * r_max;
        const float inv_2r = 0.5f / fr;

        const float deg2rad = 0.0174532925f;
        float sr = startRad * deg2rad;
        float er = endRad * deg2rad;

        float sx = cosf(sr), sy = sinf(sr);
        float ex = cosf(er), ey = sinf(er);

        float sweep = endRad - startRad;
        if (sweep < 0.0f)
            sweep += 360.0f;
        bool isLargeArc = (sweep > 180.0f);

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            uint32_t bg = pipcore::Sprite::swap16(*ptr);
            uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
            uint32_t bg_g = (bg & 0x07E0);
            uint32_t inv = 255 - alpha;
            uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
            *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
        };

        auto isAngleCorrect = [&](float dx, float dy) __attribute__((always_inline)) -> bool
        {
            float cp_s = sx * dy - sy * dx;
            float cp_e = ex * dy - ey * dx;

            if (isLargeArc)
                return (cp_s >= 0.0f || cp_e <= 0.0f);
            else
                return (cp_s >= 0.0f && cp_e <= 0.0f);
        };

        int16_t y_start = -r - 2;
        int16_t y_end = r + 2;

        for (int16_t dy_int = y_start; dy_int <= y_end; ++dy_int)
        {
            int16_t py = cy + dy_int;
            if (py < clipY || py > clipB)
                continue;

            float dy = (float)dy_int;
            float dy_sq = dy * dy;

            if (dy_sq > r_max2 + 2.0f * fr)
                continue;

            float x_outer_f = (r_max2 > dy_sq) ? sqrtf(r_max2 - dy_sq) : 0.0f;
            float x_inner_f = (r_min2 > dy_sq) ? sqrtf(r_min2 - dy_sq) : 0.0f;

            int16_t x_outer = (int16_t)(x_outer_f + 1.0f);
            int16_t x_inner = (int16_t)(x_inner_f > 1.0f ? x_inner_f - 1.0f : 0.0f);

            int16_t signs[] = {1, -1};
            for (int s = 0; s < 2; ++s)
            {
                int16_t sign = signs[s];

                for (int16_t x_off = x_inner; x_off <= x_outer; ++x_off)
                {
                    int16_t dx_int = x_off * sign;
                    int16_t px = cx + dx_int;

                    if (px < clipX || px > clipR)
                        continue;

                    float dx = (float)dx_int;
                    float dist_sq = dx * dx + dy_sq;

                    float dist_diff = fabsf(dist_sq - r2) * inv_2r;

                    if (dist_diff >= 1.0f)
                        continue;

                    if (!isAngleCorrect(dx, dy))
                        continue;

                    uint8_t alpha;
                    if (dist_diff <= 0.5f)
                    {
                        alpha = 255;
                    }
                    else
                    {
                        float t = (dist_diff - 0.5f) * 2.0f;
                        alpha = (uint8_t)((1.0f - t * t * (3.0f - 2.0f * t)) * 255.0f);
                    }

                    if (alpha == 255)
                        buf[py * stride + px] = fg;
                    else if (alpha > 0)
                        blendFast(buf + py * stride + px, alpha);
                }
            }
        }

        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(cx - r - 2, cy - r - 2, r * 2 + 5, r * 2 + 5);
    }

    void GUI::fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0)
            return;
        if (!_flags.spriteEnabled)
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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (cx + rx < clipX || cx - rx > clipR || cy + ry < clipY || cy - ry > clipB)
            return;

        const float fa = (float)rx;
        const float fb = (float)ry;
        const float fb_sq = fb * fb;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        for (int16_t dy = 0; dy <= ry; ++dy)
        {
            float fy = (float)dy;

            float term = 1.0f - (fy * fy) / fb_sq;
            if (term < 0.0f)
                term = 0.0f;

            float fx = fa * sqrtf(term);
            int16_t x_int = (int16_t)fx;

            float fraction = fx - (float)x_int;
            uint8_t alpha = (uint8_t)(fraction * 255.0f);

            int16_t py_arr[2];
            int count_y = 0;

            if (cy - dy >= clipY && cy - dy <= clipB)
                py_arr[count_y++] = cy - dy;
            if (dy != 0 && cy + dy >= clipY && cy + dy <= clipB)
                py_arr[count_y++] = cy + dy;

            for (int i = 0; i < count_y; ++i)
            {
                int16_t py = py_arr[i];
                int32_t row = py * stride;

                int16_t left = cx - x_int;
                int16_t right = cx + x_int;

                int16_t sl = (left < clipX) ? clipX : left;
                int16_t sr = (right > clipR) ? clipR : right;

                if (sl <= sr)
                {
                    uint16_t *dst = buf + row + sl;
                    int16_t count = sr - sl + 1;
                    if (count > 0 && ((uintptr_t)dst & 2))
                    {
                        *dst++ = fg;
                        count--;
                    }
                    uint32_t *dst32 = (uint32_t *)dst;
                    int16_t count32 = count >> 1;
                    while (count32--)
                        *dst32++ = fg32;
                    if (count & 1)
                        *(uint16_t *)dst32 = fg;
                }

                if (alpha > 0)
                {
                    blendFast(cx + x_int + 1, py, alpha);
                    blendFast(cx - x_int - 1, py, alpha);
                }
            }
        }
    }

    void GUI::drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color)
    {
        if (rx <= 0 || ry <= 0)
            return;
        if (!_flags.spriteEnabled)
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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (cx + rx + 2 < clipX || cx - rx - 2 > clipR || cy + ry + 2 < clipY || cy - ry - 2 > clipB)
            return;

        const float fa = (float)rx;
        const float fb = (float)ry;
        const float fa2 = fa * fa;
        const float fb2 = fb * fb;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist <= 0.0f)
                return 255;
            if (dist >= 1.0f)
                return 0;
            return (uint8_t)((1.0f - dist * dist * (3.0f - 2.0f * dist)) * 255.0f);
        };

        float split_x = (fa2) / sqrtf(fa2 + fb2);
        int16_t limit_x = (int16_t)(split_x + 2.0f);

        for (int16_t dx = 0; dx <= limit_x; ++dx)
        {
            float fx = (float)dx;

            float term = 1.0f - (fx * fx) / fa2;
            if (term < 0.0f)
                term = 0.0f;
            float fy = fb * sqrtf(term);

            int16_t y_int = (int16_t)fy;

            float m = (fb2 * fx) / (fa2 * fy);
            float aa_scale = sqrtf(1.0f + m * m);

            for (int16_t offset = -1; offset <= 1; ++offset)
            {
                int16_t dy_curr = y_int + offset;
                if (dy_curr < 0)
                    continue;

                float dist_y = fabsf((float)dy_curr - fy);
                float dist_perp = dist_y / aa_scale;

                uint8_t alpha = calcAlpha(dist_perp - 0.5f);

                if (alpha > 0)
                {
                    blendPixel(cx + dx, cy + dy_curr, alpha);
                    blendPixel(cx - dx, cy + dy_curr, alpha);
                    blendPixel(cx + dx, cy - dy_curr, alpha);
                    blendPixel(cx - dx, cy - dy_curr, alpha);
                }
            }
        }

        float split_y = (fb2) / sqrtf(fa2 + fb2);
        int16_t limit_y = (int16_t)(split_y + 2.0f);

        for (int16_t dy = 0; dy <= limit_y; ++dy)
        {
            float fy = (float)dy;

            float term = 1.0f - (fy * fy) / fb2;
            if (term < 0.0f)
                term = 0.0f;
            float fx = fa * sqrtf(term);

            int16_t x_int = (int16_t)fx;

            if (x_int < limit_x - 1)
                continue;

            float m = (fa2 * fy) / (fb2 * fx);
            float aa_scale = sqrtf(1.0f + m * m);

            for (int16_t offset = -1; offset <= 1; ++offset)
            {
                int16_t dx_curr = x_int + offset;
                if (dx_curr < 0)
                    continue;

                float dist_x = fabsf((float)dx_curr - fx);
                float dist_perp = dist_x / aa_scale;

                uint8_t alpha = calcAlpha(dist_perp - 0.5f);

                if (alpha > 0)
                {
                    blendPixel(cx + dx_curr, cy + dy, alpha);
                    blendPixel(cx - dx_curr, cy + dy, alpha);
                    blendPixel(cx + dx_curr, cy - dy, alpha);
                    blendPixel(cx - dx_curr, cy - dy, alpha);
                }
            }
        }

        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(cx - rx - 2, cy - ry - 2, rx * 2 + 5, ry * 2 + 5);
    }

    void GUI::drawTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint16_t color)
    {

        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    }

    void GUI::fillTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint16_t color)
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
        const int16_t maxH = spr->height();
        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

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

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist <= 0.0f)
                return 255;
            if (dist >= 1.0f)
                return 0;
            return (uint8_t)((1.0f - dist * dist * (3.0f - 2.0f * dist)) * 255.0f);
        };

        auto drawScanline = [&](int y_start, int y_end, float x_left, float dx_left, float x_right, float dx_right, float aa_factor_l, float aa_factor_r) __attribute__((always_inline))
        {
            for (int y = y_start; y <= y_end; ++y)
            {
                if (y >= clipY && y <= clipB)
                {

                    float xl_f = x_left;
                    float xr_f = x_right;

                    float true_aa_l = aa_factor_l;
                    float true_aa_r = aa_factor_r;

                    if (xl_f > xr_f)
                    {
                        std::swap(xl_f, xr_f);
                        std::swap(true_aa_l, true_aa_r);
                    }

                    int16_t xl = (int16_t)xl_f;
                    int16_t xr = (int16_t)xr_f;

                    int16_t solid_l = xl + 1;
                    int16_t solid_r = xr - 1;

                    int16_t draw_l = (solid_l < clipX) ? clipX : solid_l;
                    int16_t draw_r = (solid_r > clipR) ? clipR : solid_r;

                    if (draw_l <= draw_r)
                    {
                        uint16_t *dst = buf + y * stride + draw_l;
                        int16_t count = draw_r - draw_l + 1;
                        if (count > 0 && ((uintptr_t)dst & 2))
                        {
                            *dst++ = fg;
                            count--;
                        }
                        uint32_t *dst32 = (uint32_t *)dst;
                        int16_t count32 = count >> 1;
                        while (count32--)
                            *dst32++ = fg32;
                        if (count & 1)
                            *(uint16_t *)dst32 = fg;
                    }

                    float dist_l = (solid_l - xl_f) * true_aa_l;
                    blendFast(xl, y, calcAlpha((xl_f - xl) * true_aa_l - 0.5f));
                    blendFast(xr, y, calcAlpha((xr - xr_f) * true_aa_r + 0.5f));
                }
                x_left += dx_left;
                x_right += dx_right;
            }
        };

        auto getSlopeData = [](int x0, int y0, int x1, int y1, float &dx_dy, float &aa_factor)
        {
            int dy = y1 - y0;
            int dx = x1 - x0;
            if (dy == 0)
            {
                dx_dy = 0.0f;
                aa_factor = 0.0f;
                return;
            }
            dx_dy = (float)dx / (float)dy;

            aa_factor = 1.0f / sqrtf(1.0f + dx_dy * dx_dy);
        };

        float dx01, aa01, dx02, aa02, dx12, aa12;
        getSlopeData(x0, y0, x1, y1, dx01, aa01);
        getSlopeData(x0, y0, x2, y2, dx02, aa02);
        getSlopeData(x1, y1, x2, y2, dx12, aa12);

        if (y1 > y0)
        {
            drawScanline(y0, y1 - 1,
                         (float)x0, dx02,
                         (float)x0, dx01,
                         aa02, aa01);
        }

        if (y2 > y1)
        {

            float x_long = (float)x0 + dx02 * (float)(y1 - y0);
            drawScanline(y1, y2,
                         x_long, dx02,
                         (float)x1, dx12,
                         aa02, aa12);
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

        int16_t stride = spr->width();
        int16_t clipX, clipY, clipW, clipH;
        spr->getClipRect((int32_t *)&clipX, (int32_t *)&clipY, (int32_t *)&clipW, (int32_t *)&clipH);
        int16_t clipR = clipX + clipW - 1;
        int16_t clipB = clipY + clipH - 1;

        int16_t minX = std::min({x0, x1, x2}) - radius - 2;
        int16_t maxX = std::max({x0, x1, x2}) + radius + 2;
        int16_t minY = std::min({y0, y1, y2}) - radius - 2;
        int16_t maxY = std::max({y0, y1, y2}) + radius + 2;

        if (minX > clipR || maxX < clipX || minY > clipB || maxY < clipY)
            return;

        float v0x = x0, v0y = y0;
        float v1x = x1, v1y = y1;
        float v2x = x2, v2y = y2;

        float e0x = v1x - v0x, e0y = v1y - v0y;
        float len0_sq = e0x * e0x + e0y * e0y;
        float inv_len0 = 1.0f / len0_sq;
        float e1x = v2x - v1x, e1y = v2y - v1y;
        float len1_sq = e1x * e1x + e1y * e1y;
        float inv_len1 = 1.0f / len1_sq;
        float e2x = v0x - v2x, e2y = v0y - v2y;
        float len2_sq = e2x * e2x + e2y * e2y;
        float inv_len2 = 1.0f / len2_sq;

        float cp = e0x * (v2y - v0y) - e0y * (v2x - v0x);
        float sign = (cp < 0.0f) ? -1.0f : 1.0f;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist >= 0.5f)
                return 0;
            if (dist <= -0.5f)
                return 255;
            float t = dist + 0.5f;
            return (uint8_t)((1.0f - t * t * (3.0f - 2.0f * t)) * 255.0f);
        };

        for (int16_t py = minY; py <= maxY; ++py)
        {
            if (py < clipY || py > clipB)
                continue;
            for (int16_t px = minX; px <= maxX; ++px)
            {
                if (px < clipX || px > clipR)
                    continue;

                float px_f = px + 0.5f, py_f = py + 0.5f;

                float pd0x = px_f - v0x, pd0y = py_f - v0y;
                float t0 = (pd0x * e0x + pd0y * e0y) * inv_len0;
                t0 = (t0 < 0.0f) ? 0.0f : (t0 > 1.0f ? 1.0f : t0);
                float dx0 = pd0x - e0x * t0, dy0 = pd0y - e0y * t0;
                float d0_sq = dx0 * dx0 + dy0 * dy0;

                float pd1x = px_f - v1x, pd1y = py_f - v1y;
                float t1 = (pd1x * e1x + pd1y * e1y) * inv_len1;
                t1 = (t1 < 0.0f) ? 0.0f : (t1 > 1.0f ? 1.0f : t1);
                float dx1 = pd1x - e1x * t1, dy1 = pd1y - e1y * t1;
                float d1_sq = dx1 * dx1 + dy1 * dy1;

                float pd2x = px_f - v2x, pd2y = py_f - v2y;
                float t2 = (pd2x * e2x + pd2y * e2y) * inv_len2;
                t2 = (t2 < 0.0f) ? 0.0f : (t2 > 1.0f ? 1.0f : t2);
                float dx2 = pd2x - e2x * t2, dy2 = pd2y - e2y * t2;
                float d2_sq = dx2 * dx2 + dy2 * dy2;

                float min_d_sq = d0_sq;
                if (d1_sq < min_d_sq)
                    min_d_sq = d1_sq;
                if (d2_sq < min_d_sq)
                    min_d_sq = d2_sq;

                float dist = sqrtf(min_d_sq);

                float c0 = e0x * pd0y - e0y * pd0x;
                float c1 = e1x * pd1y - e1y * pd1x;
                float c2 = e2x * pd2y - e2y * pd2x;
                bool inside = ((c0 * sign >= 0) && (c1 * sign >= 0) && (c2 * sign >= 0));

                float sdf = inside ? -dist : dist;
                float edge_dist = sdf - radius;

                float dist_from_stroke = fabsf(edge_dist);
                if (dist_from_stroke <= 1.0f)
                {
                    uint8_t a = calcAlpha(dist_from_stroke - 0.5f);
                    if (a > 0)
                        blendPixel(px, py, a);
                }
            }
        }
        if (_disp.display && !_flags.renderToSprite)
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
        const int16_t maxH = spr->height();
        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        int16_t minX = std::min({x0, x1, x2}) - radius - 1;
        int16_t maxX = std::max({x0, x1, x2}) + radius + 1;
        int16_t minY = std::min({y0, y1, y2}) - radius - 1;
        int16_t maxY = std::max({y0, y1, y2}) + radius + 1;

        if (minX > clipR || maxX < clipX || minY > clipB || maxY < clipY)
            return;

        float v0x = x0, v0y = y0;
        float v1x = x1, v1y = y1;
        float v2x = x2, v2y = y2;

        float e0x = v1x - v0x, e0y = v1y - v0y;
        float len0_sq = e0x * e0x + e0y * e0y;
        float inv_len0 = 1.0f / len0_sq;
        float e1x = v2x - v1x, e1y = v2y - v1y;
        float len1_sq = e1x * e1x + e1y * e1y;
        float inv_len1 = 1.0f / len1_sq;
        float e2x = v0x - v2x, e2y = v0y - v2y;
        float len2_sq = e2x * e2x + e2y * e2y;
        float inv_len2 = 1.0f / len2_sq;

        float cp_tri = e0x * (v2y - v0y) - e0y * (v2x - v0x);
        float sign = (cp_tri < 0.0f) ? -1.0f : 1.0f;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](uint16_t *ptr, uint8_t alpha) __attribute__((always_inline))
        {
            uint32_t bg = pipcore::Sprite::swap16(*ptr);
            uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
            uint32_t bg_g = (bg & 0x07E0);
            uint32_t inv = 255 - alpha;
            uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
            uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
            *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist >= 0.5f)
                return 0;
            if (dist <= -0.5f)
                return 255;
            float t = dist + 0.5f;
            return (uint8_t)((1.0f - t * t * (3.0f - 2.0f * t)) * 255.0f);
        };

        for (int16_t py = minY; py <= maxY; ++py)
        {
            if (py < clipY || py > clipB)
                continue;

            float py_f = py + 0.5f;
            float dy0 = py_f - v0y;
            float dy1 = py_f - v1y;
            float dy2 = py_f - v2y;

            int32_t row_offset = py * stride;

            for (int16_t px = minX; px <= maxX; ++px)
            {
                if (px < clipX || px > clipR)
                    continue;

                float px_f = px + 0.5f;
                float dx0 = px_f - v0x;
                float dx1 = px_f - v1x;
                float dx2 = px_f - v2x;

                float c0 = e0x * dy0 - e0y * dx0;
                float c1 = e1x * dy1 - e1y * dx1;
                float c2 = e2x * dy2 - e2y * dx2;

                bool inside_tri = ((c0 * sign >= 0) && (c1 * sign >= 0) && (c2 * sign >= 0));

                if (inside_tri)
                {
                    buf[row_offset + px] = fg;
                    continue;
                }

                float t0 = (dx0 * e0x + dy0 * e0y) * inv_len0;
                t0 = (t0 < 0.0f) ? 0.0f : (t0 > 1.0f ? 1.0f : t0);
                float nx0 = dx0 - e0x * t0, ny0 = dy0 - e0y * t0;
                float d_sq = nx0 * nx0 + ny0 * ny0;

                float t1 = (dx1 * e1x + dy1 * e1y) * inv_len1;
                t1 = (t1 < 0.0f) ? 0.0f : (t1 > 1.0f ? 1.0f : t1);
                float nx1 = dx1 - e1x * t1, ny1 = dy1 - e1y * t1;
                float d1_sq = nx1 * nx1 + ny1 * ny1;
                if (d1_sq < d_sq)
                    d_sq = d1_sq;

                float t2 = (dx2 * e2x + dy2 * e2y) * inv_len2;
                t2 = (t2 < 0.0f) ? 0.0f : (t2 > 1.0f ? 1.0f : t2);
                float nx2 = dx2 - e2x * t2, ny2 = dy2 - e2y * t2;
                float d2_sq = nx2 * nx2 + ny2 * ny2;
                if (d2_sq < d_sq)
                    d_sq = d2_sq;

                float r_lim = radius + 1.0f;
                if (d_sq > r_lim * r_lim)
                    continue;

                float dist = sqrtf(d_sq);
                float sdf = dist - radius;

                uint8_t a = calcAlpha(sdf);
                if (a == 255)
                    buf[row_offset + px] = fg;
                else if (a > 0)
                    blendFast(buf + row_offset + px, a);
            }
        }
        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    void GUI::fillSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0)
            return;
        if (!_flags.spriteEnabled)
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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (cx + r < clipX || cx - r > clipR || cy + r < clipY || cy - r > clipB)
            return;

        const float fr = (float)r;
        const float r2 = fr * fr;
        const float r4 = r2 * r2;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg32 = ((uint32_t)fg << 16) | fg;
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        for (int16_t dy = 0; dy <= r; ++dy)
        {
            float fy = (float)dy;
            float fy2 = fy * fy;
            float fy4 = fy2 * fy2;

            float rem = r4 - fy4;
            if (rem < 0.0f)
                rem = 0.0f;

            float fx = sqrtf(sqrtf(rem));
            int16_t x_int = (int16_t)fx;

            float fraction = fx - (float)x_int;
            uint8_t alpha = (uint8_t)(fraction * 255.0f);

            int16_t py_arr[2];
            int count_y = 0;
            if (cy - dy >= clipY && cy - dy <= clipB)
                py_arr[count_y++] = cy - dy;
            if (dy != 0 && cy + dy >= clipY && cy + dy <= clipB)
                py_arr[count_y++] = cy + dy;

            for (int i = 0; i < count_y; ++i)
            {
                int16_t py = py_arr[i];
                int32_t row = py * stride;

                int16_t xl = cx - x_int;
                int16_t xr = cx + x_int;

                int16_t s_xl = (xl < clipX) ? clipX : xl;
                int16_t s_xr = (xr > clipR) ? clipR : xr;

                if (s_xl <= s_xr)
                {
                    uint16_t *dst = buf + row + s_xl;
                    int16_t count = s_xr - s_xl + 1;
                    if (count > 0 && ((uintptr_t)dst & 2))
                    {
                        *dst++ = fg;
                        count--;
                    }
                    uint32_t *dst32 = (uint32_t *)dst;
                    int16_t count32 = count >> 1;
                    while (count32--)
                        *dst32++ = fg32;
                    if (count & 1)
                        *(uint16_t *)dst32 = fg;
                }

                if (alpha > 0)
                {
                    blendFast(cx + x_int + 1, py, alpha);
                    blendFast(cx - x_int - 1, py, alpha);
                }
            }
        }
    }

    void GUI::drawSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
    {
        if (r <= 0)
            return;
        if (!_flags.spriteEnabled)
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
        int32_t clipR = clipX + clipW - 1;
        int32_t clipB = clipY + clipH - 1;

        if (cx + r + 2 < clipX || cx - r - 2 > clipR || cy + r + 2 < clipY || cy - r - 2 > clipB)
            return;

        const float fr = (float)r;
        const float r2 = fr * fr;
        const float r4 = r2 * r2;

        const uint16_t fg = pipcore::Sprite::swap16(color);
        const uint32_t fg_rb = ((color & 0xF800) << 5) | (color & 0x001F);
        const uint32_t fg_g = (color & 0x07E0);

        auto blendFast = [&](int16_t px, int16_t py, uint8_t alpha) __attribute__((always_inline))
        {
            if (px >= clipX && px <= clipR && py >= clipY && py <= clipB)
            {
                uint16_t *ptr = buf + py * stride + px;
                uint32_t bg = pipcore::Sprite::swap16(*ptr);
                uint32_t bg_rb = ((bg & 0xF800) << 5) | (bg & 0x001F);
                uint32_t bg_g = (bg & 0x07E0);
                uint32_t inv = 255 - alpha;
                uint32_t rb = ((fg_rb * alpha + bg_rb * inv) >> 8) & 0x001F001F;
                uint32_t g = ((fg_g * alpha + bg_g * inv) >> 8) & 0x000007E0;
                *ptr = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
            }
        };

        auto calcAlpha = [](float dist) __attribute__((always_inline)) -> uint8_t
        {
            if (dist <= 0.0f)
                return 255;
            if (dist >= 1.0f)
                return 0;
            return (uint8_t)((1.0f - dist * dist * (3.0f - 2.0f * dist)) * 255.0f);
        };

        float split_val = fr / 1.189207115f;
        int16_t limit = (int16_t)(split_val + 2.0f);

        for (int16_t dy = 0; dy <= limit; ++dy)
        {
            float fy = (float)dy;
            float fy2 = fy * fy;
            float fy4 = fy2 * fy2;

            float rem = r4 - fy4;
            if (rem < 0.0f)
                rem = 0.0f;
            float fx = sqrtf(sqrtf(rem));

            int16_t x_int = (int16_t)fx;

            float fx2 = fx * fx;
            float fx3 = fx2 * fx;
            float fy3 = fy2 * fy;
            float grad_mag = sqrtf(fx3 * fx3 + fy3 * fy3);
            float aa_scale = (grad_mag > 1e-4f) ? (fx3 / grad_mag) : 1.0f;

            for (int16_t offset = -1; offset <= 1; ++offset)
            {
                int16_t dx_curr = x_int + offset;
                if (dx_curr < 0)
                    continue;

                float dist_x = fabsf((float)dx_curr - fx);
                float dist_perp = dist_x * aa_scale;

                uint8_t alpha = calcAlpha(dist_perp - 0.5f);
                if (alpha > 0)
                {
                    blendFast(cx + dx_curr, cy + dy, alpha);
                    blendFast(cx - dx_curr, cy + dy, alpha);
                    blendFast(cx + dx_curr, cy - dy, alpha);
                    blendFast(cx - dx_curr, cy - dy, alpha);
                }
            }
        }

        for (int16_t dx = 0; dx <= limit; ++dx)
        {
            float fx = (float)dx;
            float fx2 = fx * fx;
            float fx4 = fx2 * fx2;

            float rem = r4 - fx4;
            if (rem < 0.0f)
                rem = 0.0f;
            float fy = sqrtf(sqrtf(rem));

            int16_t y_int = (int16_t)fy;

            if (y_int < limit - 1)
                continue;

            float fy2 = fy * fy;
            float fy3 = fy2 * fy;
            float fx3 = fx2 * fx;
            float grad_mag = sqrtf(fx3 * fx3 + fy3 * fy3);
            float aa_scale = (grad_mag > 1e-4f) ? (fy3 / grad_mag) : 1.0f;

            for (int16_t offset = -1; offset <= 1; ++offset)
            {
                int16_t dy_curr = y_int + offset;
                if (dy_curr < 0)
                    continue;

                float dist_y = fabsf((float)dy_curr - fy);
                float dist_perp = dist_y * aa_scale;

                uint8_t alpha = calcAlpha(dist_perp - 0.5f);
                if (alpha > 0)
                {
                    blendFast(cx + dx, cy + dy_curr, alpha);
                    blendFast(cx - dx, cy + dy_curr, alpha);
                    blendFast(cx + dx, cy - dy_curr, alpha);
                    blendFast(cx - dx, cy - dy_curr, alpha);
                }
            }
        }

        if (_disp.display && !_flags.renderToSprite)
            invalidateRect(cx - r - 2, cy - r - 2, r * 2 + 5, r * 2 + 5);
    }

    pipcore::Sprite *GUI::getDrawTarget()
    {
        pipcore::Sprite *spr = nullptr;
        if (_flags.renderToSprite && _flags.spriteEnabled)
            spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        else
            spr = &_render.sprite;

        applyGuiClipToSprite(spr, _clip.enabled, _clip.x, _clip.y, _clip.w, _clip.h);
        return spr;
    }

    void GUI::setClipWindow(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        _clip.enabled = true;
        _clip.x = x;
        _clip.y = y;
        _clip.w = w;
        _clip.h = h;
    }

    void GUI::resetClipWindow()
    {
        _clip.enabled = false;
        _clip.x = 0;
        _clip.y = 0;
        _clip.w = 0;
        _clip.h = 0;
    }

    void GUI::clear(uint16_t color)
    {
        _render.bgColor = detail::color565To888(color);

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                   ? (_render.activeSprite ? _render.activeSprite : &_render.sprite)
                                   : &_render.sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t w = spr->width();
        const int16_t h = spr->height();
        if (w <= 0 || h <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = w, clipH = h;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        const uint16_t v = pipcore::Sprite::swap16(color);
        for (int32_t yy = 0; yy < clipH; ++yy)
        {
            const int32_t row = (int32_t)(clipY + yy) * (int32_t)w;
            uint16_t *p = buf + row + clipX;
            for (int32_t xx = 0; xx < clipW; ++xx)
                p[xx] = v;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
    }

    int16_t GUI::AutoX(int32_t contentWidth) const
    {
        int16_t sb = statusBarHeight();
        int16_t left = (_flags.statusBarEnabled && _status.pos == Left) ? sb : 0;
        int16_t availW = _render.screenWidth - ((_flags.statusBarEnabled && (_status.pos == Left || _status.pos == Right)) ? sb : 0);

        if (contentWidth > availW)
            availW = (int16_t)contentWidth;

        return left + (availW - (int16_t)contentWidth) / 2;
    }

    int16_t GUI::AutoY(int32_t contentHeight) const
    {
        int16_t sb = statusBarHeight();
        int16_t top = (_flags.statusBarEnabled && _status.pos == Top) ? sb : 0;
        int16_t availH = _render.screenHeight - ((_flags.statusBarEnabled && (_status.pos == Top || _status.pos == Bottom)) ? sb : 0);

        if (contentHeight > availH)
            availH = (int16_t)contentHeight;

        return top + (availH - (int16_t)contentHeight) / 2;
    }

    void GUI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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
        if (clipW <= 0 || clipH <= 0)
            return;

        if (x < 0)
        {
            w += x;
            x = 0;
        }
        if (y < 0)
        {
            h += y;
            y = 0;
        }
        if (x + w > stride)
            w = (int16_t)(stride - x);
        if (y + h > maxH)
            h = (int16_t)(maxH - y);

        int16_t cx0 = (int16_t)clipX;
        int16_t cy0 = (int16_t)clipY;
        int16_t cx1 = (int16_t)(clipX + clipW);
        int16_t cy1 = (int16_t)(clipY + clipH);

        if (x < cx0)
        {
            w = (int16_t)(w - (cx0 - x));
            x = cx0;
        }
        if (y < cy0)
        {
            h = (int16_t)(h - (cy0 - y));
            y = cy0;
        }
        if (x + w > cx1)
            w = (int16_t)(cx1 - x);
        if (y + h > cy1)
            h = (int16_t)(cy1 - y);

        if (w <= 0 || h <= 0)
            return;

        const uint16_t v = pipcore::Sprite::swap16(color);
        for (int16_t yy = 0; yy < h; ++yy)
        {
            const int16_t py = (int16_t)(y + yy);
            const int32_t row = (int32_t)py * (int32_t)stride;
            uint16_t *p = buf + row + x;
            for (int16_t xx = 0; xx < w; ++xx)
                p[xx] = v;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x, y, w, h);
    }

    void GUI::fillRectAlpha(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t alpha)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;
        if (alpha == 0)
            return;
        if (alpha >= 255)
        {
            fillRect(x, y, w, h, color);
            return;
        }

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
        if (clipW <= 0 || clipH <= 0)
            return;

        int16_t x0 = x;
        int16_t y0 = y;
        int16_t x1 = (int16_t)(x + w);
        int16_t y1 = (int16_t)(y + h);

        if (x0 < (int16_t)clipX)
            x0 = (int16_t)clipX;
        if (y0 < (int16_t)clipY)
            y0 = (int16_t)clipY;
        if (x1 > (int16_t)(clipX + clipW))
            x1 = (int16_t)(clipX + clipW);
        if (y1 > (int16_t)(clipY + clipH))
            y1 = (int16_t)(clipY + clipH);

        if (x0 >= x1 || y0 >= y1)
            return;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
            {
                const int32_t idx = row + px;
                const uint16_t bg565 = pipcore::Sprite::swap16(buf[idx]);
                const uint16_t out565 = pipcore::Sprite::blend565(bg565, color, alpha);
                buf[idx] = pipcore::Sprite::swap16(out565);
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::fillRectGradientVertical(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t topColor, uint32_t bottomColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);

        if (x0 >= x1 || y0 >= y1)
            return;

        int32_t r1 = (topColor >> 16) & 0xFF;
        int32_t g1 = (topColor >> 8) & 0xFF;
        int32_t b1 = topColor & 0xFF;

        int32_t r2 = (bottomColor >> 16) & 0xFF;
        int32_t g2 = (bottomColor >> 8) & 0xFF;
        int32_t b2 = bottomColor & 0xFF;

        int32_t div = (h > 1) ? (h - 1) : 1;
        int32_t r_step = ((r2 - r1) << 16) / div;
        int32_t g_step = ((g2 - g1) << 16) / div;
        int32_t b_step = ((b2 - b1) << 16) / div;

        int32_t dy_offset = y0 - y;
        int32_t cur_r = (r1 << 16) + r_step * dy_offset;
        int32_t cur_g = (g1 << 16) + g_step * dy_offset;
        int32_t cur_b = (b1 << 16) + b_step * dy_offset;

        for (int16_t py = y0; py < y1; ++py)
        {
            uint8_t r = cur_r >> 16;
            uint8_t g = cur_g >> 16;
            uint8_t b = cur_b >> 16;

            Color888 c888 = {r, g, b};

            uint16_t *row_ptr = buf + py * stride;

            for (int16_t px = x0; px < x1; ++px)
            {
                row_ptr[px] = pipcore::Sprite::swap16(detail::quantize565(c888, px, py));
            }

            cur_r += r_step;
            cur_g += g_step;
            cur_b += b_step;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, x1 - x0, y1 - y0);
    }

    void GUI::fillRectGradientHorizontal(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t leftColor, uint32_t rightColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);

        if (x0 >= x1 || y0 >= y1)
            return;

        int32_t r1 = (leftColor >> 16) & 0xFF;
        int32_t g1 = (leftColor >> 8) & 0xFF;
        int32_t b1 = leftColor & 0xFF;

        int32_t r2 = (rightColor >> 16) & 0xFF;
        int32_t g2 = (rightColor >> 8) & 0xFF;
        int32_t b2 = rightColor & 0xFF;

        int32_t div = (w > 1) ? (w - 1) : 1;
        int32_t r_step = ((r2 - r1) << 16) / div;
        int32_t g_step = ((g2 - g1) << 16) / div;
        int32_t b_step = ((b2 - b1) << 16) / div;

        int32_t dx_offset = x0 - x;
        int32_t start_r = (r1 << 16) + r_step * dx_offset;
        int32_t start_g = (g1 << 16) + g_step * dx_offset;
        int32_t start_b = (b1 << 16) + b_step * dx_offset;

        for (int16_t py = y0; py < y1; ++py)
        {
            uint16_t *row_ptr = buf + py * stride;

            int32_t cur_r = start_r;
            int32_t cur_g = start_g;
            int32_t cur_b = start_b;

            for (int16_t px = x0; px < x1; ++px)
            {
                uint8_t r = cur_r >> 16;
                uint8_t g = cur_g >> 16;
                uint8_t b = cur_b >> 16;

                Color888 c888 = {r, g, b};

                row_ptr[px] = pipcore::Sprite::swap16(detail::quantize565(c888, px, py));

                cur_r += r_step;
                cur_g += g_step;
                cur_b += b_step;
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, x1 - x0, y1 - y0);
    }

    void GUI::fillRectGradient4(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);

        if (x0 >= x1 || y0 >= y1)
            return;

        int32_t r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
        int32_t r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
        int32_t r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
        int32_t r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

        int32_t divY = (h > 1) ? (h - 1) : 1;
        int32_t divX = (w > 1) ? (w - 1) : 1;

        int32_t dr_L = ((r01 - r00) << 16) / divY;
        int32_t dg_L = ((g01 - g00) << 16) / divY;
        int32_t db_L = ((b01 - b00) << 16) / divY;

        int32_t dr_R = ((r11 - r10) << 16) / divY;
        int32_t dg_R = ((g11 - g10) << 16) / divY;
        int32_t db_R = ((b11 - b10) << 16) / divY;

        int32_t dy = y0 - y;
        int32_t cur_rL = (r00 << 16) + dr_L * dy;
        int32_t cur_gL = (g00 << 16) + dg_L * dy;
        int32_t cur_bL = (b00 << 16) + db_L * dy;

        int32_t cur_rR = (r10 << 16) + dr_R * dy;
        int32_t cur_gR = (g10 << 16) + dg_R * dy;
        int32_t cur_bR = (b10 << 16) + db_R * dy;

        int32_t dx_start = x0 - x;

        for (int16_t py = y0; py < y1; ++py)
        {
            int32_t dr_row = (cur_rR - cur_rL) / divX;
            int32_t dg_row = (cur_gR - cur_gL) / divX;
            int32_t db_row = (cur_bR - cur_bL) / divX;

            int32_t r = cur_rL + dr_row * dx_start;
            int32_t g = cur_gL + dg_row * dx_start;
            int32_t b = cur_bL + db_row * dx_start;

            uint16_t *row_ptr = buf + py * stride;

            for (int16_t px = x0; px < x1; ++px)
            {
                Color888 c888 = {(uint8_t)(r >> 16), (uint8_t)(g >> 16), (uint8_t)(b >> 16)};
                row_ptr[px] = pipcore::Sprite::swap16(detail::quantize565(c888, px, py));

                r += dr_row;
                g += dg_row;
                b += db_row;
            }

            cur_rL += dr_L;
            cur_gL += dg_L;
            cur_bL += db_L;
            cur_rR += dr_R;
            cur_gR += dg_R;
            cur_bR += db_R;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::fillRectGradientDiagonal(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tlColor, uint32_t brColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);

        if (x0 >= x1 || y0 >= y1)
            return;

        int32_t r1 = (tlColor >> 16) & 0xFF;
        int32_t g1 = (tlColor >> 8) & 0xFF;
        int32_t b1 = tlColor & 0xFF;

        int32_t r2 = (brColor >> 16) & 0xFF;
        int32_t g2 = (brColor >> 8) & 0xFF;
        int32_t b2 = brColor & 0xFF;

        int32_t total_steps = (w + h > 2) ? (w + h - 2) : 1;

        int32_t r_step = ((r2 - r1) << 16) / total_steps;
        int32_t g_step = ((g2 - g1) << 16) / total_steps;
        int32_t b_step = ((b2 - b1) << 16) / total_steps;

        int32_t start_steps = (x0 - x) + (y0 - y);

        int32_t row_start_r = (r1 << 16) + r_step * start_steps;
        int32_t row_start_g = (g1 << 16) + g_step * start_steps;
        int32_t row_start_b = (b1 << 16) + b_step * start_steps;

        for (int16_t py = y0; py < y1; ++py)
        {
            uint16_t *row_ptr = buf + py * stride;
            int32_t cur_r = row_start_r;
            int32_t cur_g = row_start_g;
            int32_t cur_b = row_start_b;

            for (int16_t px = x0; px < x1; ++px)
            {
                Color888 c888 = {(uint8_t)(cur_r >> 16), (uint8_t)(cur_g >> 16), (uint8_t)(cur_b >> 16)};
                row_ptr[px] = pipcore::Sprite::swap16(detail::quantize565(c888, px, py));

                cur_r += r_step;
                cur_g += g_step;
                cur_b += b_step;
            }

            row_start_r += r_step;
            row_start_g += g_step;
            row_start_b += b_step;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::fillRectGradientRadial(int16_t x, int16_t y, int16_t w, int16_t h, int16_t cx, int16_t cy, int16_t radius, uint32_t innerColor, uint32_t outerColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
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

        int16_t x0 = (x < clipX) ? clipX : x;
        int16_t y0 = (y < clipY) ? clipY : y;
        int16_t x1 = (x + w > clipX + clipW) ? (clipX + clipW) : (x + w);
        int16_t y1 = (y + h > clipY + clipH) ? (clipY + clipH) : (y + h);

        if (x0 >= x1 || y0 >= y1)
            return;

        if (radius <= 0)
            radius = 1;
        const float invR = 1.0f / (float)radius;

        int32_t r1 = (innerColor >> 16) & 0xFF;
        int32_t g1 = (innerColor >> 8) & 0xFF;
        int32_t b1 = innerColor & 0xFF;

        int32_t dr = ((int32_t)((outerColor >> 16) & 0xFF) - r1);
        int32_t dg = ((int32_t)((outerColor >> 8) & 0xFF) - g1);
        int32_t db = ((int32_t)(outerColor & 0xFF) - b1);

        for (int16_t py = y0; py < y1; ++py)
        {
            const float dy = (float)(py - cy);
            const float dy2 = dy * dy;
            uint16_t *row_ptr = buf + py * stride;

            for (int16_t px = x0; px < x1; ++px)
            {
                const float dx = (float)(px - cx);

                float d = sqrtf(dx * dx + dy2) * invR;

                if (d > 1.0f)
                    d = 1.0f;

                uint8_t r = (uint8_t)(r1 + (int32_t)(dr * d));
                uint8_t g = (uint8_t)(g1 + (int32_t)(dg * d));
                uint8_t b = (uint8_t)(b1 + (int32_t)(db * d));

                Color888 c888 = {r, g, b};
                row_ptr[px] = pipcore::Sprite::swap16(detail::quantize565(c888, px, py));
            }
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565)
    {
        if (!_flags.spriteEnabled)
            return;

        clear(bg565);

        const uint16_t titlePx = _typo.logoTitleSizePx ? _typo.logoTitleSizePx : _typo.h1Px;
        const uint16_t subPx = _typo.logoSubtitleSizePx ? _typo.logoSubtitleSizePx : (uint16_t)((titlePx * 3) >> 2);
        const int16_t spacing = 4;

        int16_t titleW, titleH;
        setPSDFFontSize(titlePx);
        psdfMeasureText(title, titleW, titleH);

        int16_t totalH = titleH;
        int16_t subW = 0, subH = 0;
        const bool hasSub = (subtitle.length() > 0);

        if (hasSub)
        {
            setPSDFFontSize(subPx);
            psdfMeasureText(subtitle, subW, subH);
            totalH += spacing + subH;
        }

        int16_t topY = (_boot.y != -1) ? _boot.y : (int16_t)((_render.screenHeight - totalH) >> 1);

        int16_t cx = (_boot.x != -1) ? _boot.x : (int16_t)(_render.screenWidth >> 1);

        setPSDFFontSize(titlePx);
        psdfDrawTextInternal(title, cx, topY, fg565, bg565, AlignCenter);

        if (hasSub)
        {
            setPSDFFontSize(subPx);
            psdfDrawTextInternal(subtitle, cx, topY + titleH + spacing, fg565, bg565, AlignCenter);
        }
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_disp.display || !_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        if (Debug::dirtyRectEnabled())
            Debug::recordDirtyRect(x, y, w, h);

        int16_t x2 = x + w;
        int16_t y2 = y + h;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x2 > (int16_t)_render.screenWidth)
            x2 = (int16_t)_render.screenWidth;
        if (y2 > (int16_t)_render.screenHeight)
            y2 = (int16_t)_render.screenHeight;

        w = x2 - x;
        h = y2 - y;
        if (w <= 0 || h <= 0)
            return;

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            DirtyRect &dr = _dirty.rects[i];

            if (!(x > dr.x + dr.w || x2 < dr.x || y > dr.y + dr.h || y2 < dr.y))
            {

                int16_t nx1 = (x < dr.x) ? x : dr.x;
                int16_t ny1 = (y < dr.y) ? y : dr.y;
                int16_t nx2 = (x2 > dr.x + dr.w) ? x2 : (dr.x + dr.w);

                dr.x = nx1;
                dr.y = ny1;
                dr.w = nx2 - nx1;
                dr.h = ((y2 > dr.y + dr.h) ? y2 : (dr.y + dr.h)) - ny1;

                for (uint8_t j = i + 1; j < _dirty.count; ++j)
                {
                    DirtyRect &next = _dirty.rects[j];
                    if (!(dr.x > next.x + next.w || dr.x + dr.w < next.x ||
                          dr.y > next.y + next.h || dr.y + dr.h < next.y))
                    {

                        int16_t mx1 = (dr.x < next.x) ? dr.x : next.x;
                        int16_t my1 = (dr.y < next.y) ? dr.y : next.y;
                        int16_t mx2 = (dr.x + dr.w > next.x + next.w) ? (dr.x + dr.w) : (next.x + next.w);
                        int16_t my2 = (dr.y + dr.h > next.y + next.h) ? (dr.y + dr.h) : (next.y + next.h);

                        dr.x = mx1;
                        dr.y = my1;
                        dr.w = mx2 - mx1;
                        dr.h = my2 - my1;

                        _dirty.rects[j] = _dirty.rects[--_dirty.count];
                        j--;
                    }
                }
                return;
            }
        }

        if (_dirty.count < (uint8_t)(sizeof(_dirty.rects) / sizeof(_dirty.rects[0])))
        {
            _dirty.rects[_dirty.count++] = {x, y, w, h};
        }
        else
        {

            DirtyRect &first = _dirty.rects[0];
            int16_t total_x2 = first.x + first.w;
            int16_t total_y2 = first.y + first.h;

            for (uint8_t i = 1; i < _dirty.count; ++i)
            {
                if (_dirty.rects[i].x < first.x)
                    first.x = _dirty.rects[i].x;
                if (_dirty.rects[i].y < first.y)
                    first.y = _dirty.rects[i].y;
                int16_t r_x2 = _dirty.rects[i].x + _dirty.rects[i].w;
                int16_t r_y2 = _dirty.rects[i].y + _dirty.rects[i].h;
                if (r_x2 > total_x2)
                    total_x2 = r_x2;
                if (r_y2 > total_y2)
                    total_y2 = r_y2;
            }

            if (x < first.x)
                first.x = x;
            if (y < first.y)
                first.y = y;
            if (x2 > total_x2)
                total_x2 = x2;
            if (y2 > total_y2)
                total_y2 = y2;

            first.w = total_x2 - first.x;
            first.h = total_y2 - first.y;
            _dirty.count = 1;
        }
    }

    void GUI::flushDirty()
    {
        if (_dirty.count == 0)
            return;
        if (!_disp.display || !_flags.spriteEnabled)
        {
            _dirty.count = 0;
            return;
        }

        const int16_t sw = _render.sprite.width();
        const int16_t sh = _render.sprite.height();
        uint16_t *buf = (uint16_t *)_render.sprite.getBuffer();
        const int32_t stride = sw;

        Debug::beginRender();

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            DirtyRect r = _dirty.rects[i];
            if (r.w <= 0 || r.h <= 0)
                continue;

            int16_t x0 = r.x;
            int16_t y0 = r.y;
            int16_t w = r.w;
            int16_t h = r.h;

            if (x0 < 0)
            {
                w += x0;
                x0 = 0;
            }
            if (y0 < 0)
            {
                h += y0;
                y0 = 0;
            }
            if (w <= 0 || h <= 0)
                continue;

            if (x0 + w > sw)
                w = (int16_t)(sw - x0);
            if (y0 + h > sh)
                h = (int16_t)(sh - y0);
            if (w <= 0 || h <= 0)
                continue;

            if (Debug::dirtyRectEnabled())
            {
                Debug::drawOverlay(buf, stride, sw, sh, x0, y0, w, h);
            }

            _render.sprite.writeToDisplay(*_disp.display, x0, y0, w, h);
        }

        _dirty.count = 0;

        Debug::endRender();

        Debug::clearRects();
    }

    void GUI::tickDebugDirtyOverlay(uint32_t now)
    {
        (void)now;
    }

}
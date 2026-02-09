#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Platforms/GuiDisplay.hpp>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>

namespace pipcore
{
    namespace
    {
        inline uint16_t swap16(uint16_t v)
        {
            return __builtin_bswap16(v);
        }

        inline uint8_t u8clamp(int v)
        {
            if (v < 0)
                return 0;
            if (v > 255)
                return 255;
            return (uint8_t)v;
        }

        inline uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha)
        {
            if (alpha == 0)
                return bg;
            if (alpha >= 255)
                return fg;

            const uint16_t inv = (uint16_t)(255 - alpha);

            const uint16_t br = (uint16_t)((bg >> 11) & 0x1F);
            const uint16_t bg6 = (uint16_t)((bg >> 5) & 0x3F);
            const uint16_t bb = (uint16_t)(bg & 0x1F);

            const uint16_t fr = (uint16_t)((fg >> 11) & 0x1F);
            const uint16_t fg6 = (uint16_t)((fg >> 5) & 0x3F);
            const uint16_t fb = (uint16_t)(fg & 0x1F);

            const uint16_t r = (uint16_t)((br * inv + fr * alpha) >> 8);
            const uint16_t g = (uint16_t)((bg6 * inv + fg6 * alpha) >> 8);
            const uint16_t b = (uint16_t)((bb * inv + fb * alpha) >> 8);
            return (uint16_t)((r << 11) | (g << 5) | b);
        }
    }

    void Sprite::fillScreen(uint16_t color565)
    {
        if (!_buf)
            return;
        const size_t pixels = static_cast<size_t>(_w) * static_cast<size_t>(_h);
        const uint16_t v = swap16(color565);
        for (size_t i = 0; i < pixels; ++i)
            _buf[i] = v;
    }

    void Sprite::drawPixel(int16_t x, int16_t y, uint16_t color565)
    {
        if (!_buf)
            return;
        if (x < 0 || y < 0 || x >= _w || y >= _h)
            return;
        if (!clipTest(x, y))
            return;
        _buf[static_cast<size_t>(y) * static_cast<size_t>(_w) + static_cast<size_t>(x)] = swap16(color565);
    }

    void Sprite::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565)
    {
        if (!_buf)
            return;
        if (w <= 0 || h <= 0)
            return;

        const uint16_t v = swap16(color565);

        if (_clipW == _w && _clipH == _h && x >= 0 && y >= 0 && x + w <= _w && y + h <= _h)
        {
            const size_t rowStride = static_cast<size_t>(_w);
            uint16_t *dst = _buf + static_cast<size_t>(y) * rowStride + static_cast<size_t>(x);
            if (h == 1)
            {
                for (int16_t xx = 0; xx < w; ++xx)
                    dst[xx] = v;
                return;
            }
            for (int16_t yy = 0; yy < h; ++yy)
            {
                uint16_t *row = dst + static_cast<size_t>(yy) * rowStride;
                for (int16_t xx = 0; xx < w; ++xx)
                    row[xx] = v;
            }
            return;
        }

        int16_t x0 = x;
        int16_t y0 = y;
        int16_t x1 = static_cast<int16_t>(x + w - 1);
        int16_t y1 = static_cast<int16_t>(y + h - 1);

        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= _w)
            x1 = static_cast<int16_t>(_w - 1);
        if (y1 >= _h)
            y1 = static_cast<int16_t>(_h - 1);
        if (x0 > x1 || y0 > y1)
            return;

        int16_t cx0 = _clipX;
        int16_t cy0 = _clipY;
        int16_t cx1 = static_cast<int16_t>(_clipX + _clipW - 1);
        int16_t cy1 = static_cast<int16_t>(_clipY + _clipH - 1);

        if (_clipW == 0 || _clipH == 0)
            return;

        if (x0 < cx0)
            x0 = cx0;
        if (y0 < cy0)
            y0 = cy0;
        if (x1 > cx1)
            x1 = cx1;
        if (y1 > cy1)
            y1 = cy1;
        if (x0 > x1 || y0 > y1)
            return;

        const size_t rowStride = static_cast<size_t>(_w);
        for (int16_t yy = y0; yy <= y1; ++yy)
        {
            uint16_t *row = _buf + static_cast<size_t>(yy) * rowStride;
            for (int16_t xx = x0; xx <= x1; ++xx)
                row[xx] = v;
        }
    }

    void Sprite::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565)
    {
        if (!_buf)
            return;

        if (_clipW == 0 || _clipH == 0)
            return;

        if (y0 == y1)
        {
            if (x1 < x0)
                std::swap(x0, x1);
            fillRect(x0, y0, (int16_t)(x1 - x0 + 1), 1, color565);
            return;
        }
        if (x0 == x1)
        {
            if (y1 < y0)
                std::swap(y0, y1);
            fillRect(x0, y0, 1, (int16_t)(y1 - y0 + 1), color565);
            return;
        }

        const uint16_t fg = color565;

        auto plot = [&](int16_t x, int16_t y, uint8_t a)
        {
            if (x < 0 || y < 0 || x >= _w || y >= _h)
                return;
            if (!clipTest(x, y))
                return;
            uint16_t *p = _buf + (size_t)y * (size_t)_w + (size_t)x;
            const uint16_t bg = swap16(*p);
            *p = swap16(blend565(bg, fg, a));
        };

        auto ipart = [](float x) -> int { return (int)floorf(x); };
        auto fpart = [](float x) -> float { return x - floorf(x); };
        auto rfpart = [&](float x) -> float { return 1.0f - fpart(x); };

        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep)
        {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        const float dx = (float)(x1 - x0);
        const float dy = (float)(y1 - y0);
        const float gradient = (dx == 0.0f) ? 1.0f : (dy / dx);

        float xend = roundf((float)x0);
        float yend = (float)y0 + gradient * (xend - (float)x0);
        float xgap = rfpart((float)x0 + 0.5f);
        int xpxl1 = (int)xend;
        int ypxl1 = ipart(yend);
        if (steep)
        {
            plot((int16_t)ypxl1, (int16_t)xpxl1, u8clamp((int)(rfpart(yend) * xgap * 255.0f)));
            plot((int16_t)(ypxl1 + 1), (int16_t)xpxl1, u8clamp((int)(fpart(yend) * xgap * 255.0f)));
        }
        else
        {
            plot((int16_t)xpxl1, (int16_t)ypxl1, u8clamp((int)(rfpart(yend) * xgap * 255.0f)));
            plot((int16_t)xpxl1, (int16_t)(ypxl1 + 1), u8clamp((int)(fpart(yend) * xgap * 255.0f)));
        }
        float intery = yend + gradient;

        xend = roundf((float)x1);
        yend = (float)y1 + gradient * (xend - (float)x1);
        xgap = fpart((float)x1 + 0.5f);
        int xpxl2 = (int)xend;
        int ypxl2 = ipart(yend);
        if (steep)
        {
            plot((int16_t)ypxl2, (int16_t)xpxl2, u8clamp((int)(rfpart(yend) * xgap * 255.0f)));
            plot((int16_t)(ypxl2 + 1), (int16_t)xpxl2, u8clamp((int)(fpart(yend) * xgap * 255.0f)));
        }
        else
        {
            plot((int16_t)xpxl2, (int16_t)ypxl2, u8clamp((int)(rfpart(yend) * xgap * 255.0f)));
            plot((int16_t)xpxl2, (int16_t)(ypxl2 + 1), u8clamp((int)(fpart(yend) * xgap * 255.0f)));
        }

        for (int x = xpxl1 + 1; x < xpxl2; ++x)
        {
            int y = ipart(intery);
            const uint8_t a1 = u8clamp((int)(rfpart(intery) * 255.0f));
            const uint8_t a2 = u8clamp((int)(fpart(intery) * 255.0f));
            if (steep)
            {
                plot((int16_t)y, (int16_t)x, a1);
                plot((int16_t)(y + 1), (int16_t)x, a2);
            }
            else
            {
                plot((int16_t)x, (int16_t)y, a1);
                plot((int16_t)x, (int16_t)(y + 1), a2);
            }
            intery += gradient;
        }
    }

    void Sprite::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565)
    {
        if (w <= 0 || h <= 0)
            return;
        fillRect(x, y, w, 1, color565);
        fillRect(x, static_cast<int16_t>(y + h - 1), w, 1, color565);
        fillRect(x, y, 1, h, color565);
        fillRect(static_cast<int16_t>(x + w - 1), y, 1, h, color565);
    }

    void Sprite::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565)
    {
        if (!_buf)
            return;
        if (w <= 0 || h <= 0)
            return;

        if (radius <= 0)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

        int16_t r = radius;
        if (r * 2 > w)
            r = (int16_t)(w / 2);
        if (r * 2 > h)
            r = (int16_t)(h / 2);
        if (r <= 0)
        {
            fillRect(x, y, w, h, color565);
            return;
        }

        if (h - r * 2 > 0)
            fillRect(x, (int16_t)(y + r), w, (int16_t)(h - r * 2), color565);

        const int32_t rr = (int32_t)r * (int32_t)r;
        for (int16_t dy = 0; dy < r; ++dy)
        {
            int32_t yy = (int32_t)(r - dy);
            int32_t inside = rr - yy * yy;
            if (inside < 0)
                inside = 0;

            int32_t xx = 0;
            while ((xx + 1) * (xx + 1) <= inside)
                ++xx;

            int16_t inset = (int16_t)(r - (int16_t)xx);
            int16_t spanX = (int16_t)(x + inset);
            int16_t spanW = (int16_t)(w - inset * 2);
            if (spanW <= 0)
                continue;

            fillRect(spanX, (int16_t)(y + dy), spanW, 1, color565);
            fillRect(spanX, (int16_t)(y + h - 1 - dy), spanW, 1, color565);
        }

        const uint16_t fg = color565;
        auto blendPixel = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if (px < 0 || py < 0 || px >= _w || py >= _h)
                return;
            if (!clipTest(px, py))
                return;
            uint16_t *p = _buf + (size_t)py * (size_t)_w + (size_t)px;
            const uint16_t bg = swap16(*p);
            *p = swap16(blend565(bg, fg, a));
        };

        auto aaQuarter = [&](int16_t cx, int16_t cy, int16_t sx, int16_t sy)
        {
            const float rf = (float)r;
            const float rrF = rf * rf;

            for (int16_t dy = 0; dy <= r; ++dy)
            {
                const float yf = (float)dy;
                const float xf = sqrtf(rrF - yf * yf);
                const int16_t xi = (int16_t)floorf(xf);
                const float frac = xf - (float)xi;
                const uint8_t a = u8clamp((int)(frac * 255.0f));

                blendPixel((int16_t)(cx + sx * (xi + 1)), (int16_t)(cy + sy * dy), a);
            }

            for (int16_t dx = 0; dx <= r; ++dx)
            {
                const float xf = (float)dx;
                const float yf = sqrtf(rrF - xf * xf);
                const int16_t yi = (int16_t)floorf(yf);
                const float frac = yf - (float)yi;
                const uint8_t a = u8clamp((int)(frac * 255.0f));

                blendPixel((int16_t)(cx + sx * dx), (int16_t)(cy + sy * (yi + 1)), a);
            }
        };

        aaQuarter((int16_t)(x + r), (int16_t)(y + r), -1, -1);
        aaQuarter((int16_t)(x + w - 1 - r), (int16_t)(y + r), +1, -1);
        aaQuarter((int16_t)(x + r), (int16_t)(y + h - 1 - r), -1, +1);
        aaQuarter((int16_t)(x + w - 1 - r), (int16_t)(y + h - 1 - r), +1, +1);
    }

    void Sprite::pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels565)
    {
        if (!_buf || !pixels565)
            return;
        if (w <= 0 || h <= 0)
            return;

        int16_t sx0 = 0;
        int16_t sy0 = 0;
        int16_t dx0 = x;
        int16_t dy0 = y;
        int16_t cw = w;
        int16_t ch = h;

        if (dx0 < 0)
        {
            sx0 = static_cast<int16_t>(-dx0);
            cw = static_cast<int16_t>(cw + dx0);
            dx0 = 0;
        }
        if (dy0 < 0)
        {
            sy0 = static_cast<int16_t>(-dy0);
            ch = static_cast<int16_t>(ch + dy0);
            dy0 = 0;
        }

        if (dx0 + cw > _w)
            cw = static_cast<int16_t>(_w - dx0);
        if (dy0 + ch > _h)
            ch = static_cast<int16_t>(_h - dy0);

        if (cw <= 0 || ch <= 0)
            return;

        if (_clipW == 0 || _clipH == 0)
            return;

        int16_t cx0 = _clipX;
        int16_t cy0 = _clipY;
        int16_t cx1 = static_cast<int16_t>(_clipX + _clipW);
        int16_t cy1 = static_cast<int16_t>(_clipY + _clipH);

        int16_t dx1 = static_cast<int16_t>(dx0 + cw);
        int16_t dy1 = static_cast<int16_t>(dy0 + ch);

        if (dx0 < cx0)
        {
            sx0 = static_cast<int16_t>(sx0 + (cx0 - dx0));
            dx0 = cx0;
        }
        if (dy0 < cy0)
        {
            sy0 = static_cast<int16_t>(sy0 + (cy0 - dy0));
            dy0 = cy0;
        }
        if (dx1 > cx1)
            dx1 = cx1;
        if (dy1 > cy1)
            dy1 = cy1;

        cw = static_cast<int16_t>(dx1 - dx0);
        ch = static_cast<int16_t>(dy1 - dy0);
        if (cw <= 0 || ch <= 0)
            return;

        const size_t srcStride = static_cast<size_t>(w);
        const size_t dstStride = static_cast<size_t>(_w);

        for (int16_t yy = 0; yy < ch; ++yy)
        {
            const uint16_t *src = pixels565 + static_cast<size_t>(sy0 + yy) * srcStride + static_cast<size_t>(sx0);
            uint16_t *dst = _buf + static_cast<size_t>(dy0 + yy) * dstStride + static_cast<size_t>(dx0);
            for (int16_t xx = 0; xx < cw; ++xx)
                dst[xx] = swap16(src[xx]);
        }
    }

    void Sprite::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565)
    {
        if (w <= 0 || h <= 0)
            return;
        if (radius <= 0)
        {
            drawRect(x, y, w, h, color565);
            return;
        }

        int16_t r = radius;
        if (r * 2 > w)
            r = static_cast<int16_t>(w / 2);
        if (r * 2 > h)
            r = static_cast<int16_t>(h / 2);
        if (r <= 0)
        {
            drawRect(x, y, w, h, color565);
            return;
        }

        fillRect(static_cast<int16_t>(x + r), y, static_cast<int16_t>(w - r * 2), 1, color565);
        fillRect(static_cast<int16_t>(x + r), static_cast<int16_t>(y + h - 1), static_cast<int16_t>(w - r * 2), 1, color565);
        fillRect(x, static_cast<int16_t>(y + r), 1, static_cast<int16_t>(h - r * 2), color565);
        fillRect(static_cast<int16_t>(x + w - 1), static_cast<int16_t>(y + r), 1, static_cast<int16_t>(h - r * 2), color565);

        const int32_t rr = (int32_t)r * (int32_t)r;
        for (int16_t dy = 0; dy < r; ++dy)
        {
            int32_t yy = (int32_t)(r - dy);
            int32_t inside = rr - yy * yy;
            if (inside < 0)
                inside = 0;
            int32_t xx = 0;
            while ((xx + 1) * (xx + 1) <= inside)
                ++xx;
            int16_t inset = (int16_t)(r - (int16_t)xx);

            int16_t xl = static_cast<int16_t>(x + inset);
            int16_t xr = static_cast<int16_t>(x + w - 1 - inset);
            int16_t yt = static_cast<int16_t>(y + dy);
            int16_t yb = static_cast<int16_t>(y + h - 1 - dy);

            drawPixel(xl, yt, color565);
            drawPixel(xr, yt, color565);
            drawPixel(xl, yb, color565);
            drawPixel(xr, yb, color565);
        }
    }

    void Sprite::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color565)
    {
        if (!_buf)
            return;
        if (r <= 0)
            return;

        int32_t rr = (int32_t)r * (int32_t)r;
        for (int16_t yy = static_cast<int16_t>(y - r); yy <= static_cast<int16_t>(y + r); ++yy)
        {
            int32_t dy = (int32_t)yy - (int32_t)y;
            int32_t dy2 = dy * dy;
            int32_t rem = rr - dy2;
            if (rem < 0)
                continue;

            int32_t dx = 0;
            while ((dx + 1) * (dx + 1) <= rem)
                ++dx;

            int16_t x0 = static_cast<int16_t>(x - (int16_t)dx);
            int16_t spanW = static_cast<int16_t>((int16_t)dx * 2 + 1);
            fillRect(x0, yy, spanW, 1, color565);
        }

        const uint16_t fg = color565;
        auto blendPixel = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if (px < 0 || py < 0 || px >= _w || py >= _h)
                return;
            if (!clipTest(px, py))
                return;
            uint16_t *p = _buf + (size_t)py * (size_t)_w + (size_t)px;
            const uint16_t bg = swap16(*p);
            *p = swap16(blend565(bg, fg, a));
        };

        const float rf = (float)r;
        const float rrF = rf * rf;

        for (int16_t dy = (int16_t)-r; dy <= r; ++dy)
        {
            const float yf = (float)dy;
            const float xf = sqrtf(rrF - yf * yf);
            const int16_t xi = (int16_t)floorf(xf);
            const float frac = xf - (float)xi;
            const uint8_t a = u8clamp((int)(frac * 255.0f));

            blendPixel((int16_t)(x + xi + 1), (int16_t)(y + dy), a);
            blendPixel((int16_t)(x - xi - 1), (int16_t)(y + dy), a);
        }

        for (int16_t dx = (int16_t)-r; dx <= r; ++dx)
        {
            const float xf = (float)dx;
            const float yf = sqrtf(rrF - xf * xf);
            const int16_t yi = (int16_t)floorf(yf);
            const float frac = yf - (float)yi;
            const uint8_t a = u8clamp((int)(frac * 255.0f));

            blendPixel((int16_t)(x + dx), (int16_t)(y + yi + 1), a);
            blendPixel((int16_t)(x + dx), (int16_t)(y - yi - 1), a);
        }
    }
}

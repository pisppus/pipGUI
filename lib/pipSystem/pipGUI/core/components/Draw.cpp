#include <pipGUI/core/api/pipGUI.hpp>

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

    static inline uint16_t swap16(uint16_t v)
    {
        return (uint16_t)((v >> 8) | (v << 8));
    }

    uint16_t GUI::screenWidth() const { return _screenWidth; }
    uint16_t GUI::screenHeight() const { return _screenHeight; }

    uint32_t GUI::rgb(uint8_t r, uint8_t g, uint8_t b) const
    {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    uint16_t GUI::color888To565(int16_t x, int16_t y, uint32_t color888) const
    {
        Color888 c{
            (uint8_t)((color888 >> 16) & 0xFF),
            (uint8_t)((color888 >> 8) & 0xFF),
            (uint8_t)(color888 & 0xFF),
        };
        if (c.isBlack() || c.isWhite())
            return detail::quantize565(c, x, y, _frcSeed, FrcProfile::Off);
        if (c.r == c.g && c.g == c.b)
            return detail::quantize565(c, x, y, _frcSeed, FrcProfile::Off);
        if (((c.r & 7U) == 0U) && ((c.g & 3U) == 0U) && ((c.b & 7U) == 0U))
            return detail::quantize565(c, x, y, _frcSeed, FrcProfile::Off);
        return detail::quantize565(c, x, y, _frcSeed, _frcProfile);
    }

    void GUI::fillCircleFrc(int16_t cx, int16_t cy, int16_t r, uint32_t color888)
    {
        if (r <= 0)
            return;

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                        ? (_activeSprite ? _activeSprite : &_sprite)
                                        : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t stride = spr->width();
        int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        Color888 c = Color888::fromUint32(color888);
        FrcProfile profile = _frcProfile;
        if (c.isBlack() || c.isWhite())
            profile = FrcProfile::Off;
        else if (c.r == c.g && c.g == c.b)
            profile = FrcProfile::Off;
        else if (((c.r & 7U) == 0U) && ((c.g & 3U) == 0U) && ((c.b & 7U) == 0U))
            profile = FrcProfile::Off;

        const int16_t y0 = (int16_t)(cy - r);
        const int16_t y1 = (int16_t)(cy + r);
        const int32_t rr = (int32_t)r * (int32_t)r;

        for (int16_t yy = y0; yy <= y1; ++yy)
        {
            int32_t dy = (int32_t)yy - (int32_t)cy;
            int32_t rem = rr - dy * dy;
            if (rem < 0)
                continue;

            int32_t dx = 0;
            while ((dx + 1) * (dx + 1) <= rem)
                ++dx;

            int16_t x0 = (int16_t)(cx - (int16_t)dx);
            int16_t w = (int16_t)((int16_t)dx * 2 + 1);
            fillRect888(x0, yy, w, 1, c.r, c.g, c.b, profile);
        }

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if ((uint16_t)px >= (uint16_t)stride || (uint16_t)py >= (uint16_t)maxH)
                return;
            uint16_t *p = buf + (int32_t)py * (int32_t)stride + px;
            const uint16_t bg = swap16(*p);
            const uint16_t fg = detail::quantize565(c, px, py, _frcSeed, profile);
            *p = swap16(detail::blend565(bg, fg, a));
        };

        const float rf = (float)r;
        const float rrF = rf * rf;

        for (int16_t dy = (int16_t)-r; dy <= r; ++dy)
        {
            const float yf = (float)dy;
            const float xf = sqrtf(rrF - yf * yf);
            const int16_t xi = (int16_t)floorf(xf);
            const float frac = xf - (float)xi;
            const uint8_t a = (uint8_t)std::min(255, (int)(frac * 255.0f));

            blendPixel((int16_t)(cx + xi + 1), (int16_t)(cy + dy), a);
            blendPixel((int16_t)(cx - xi - 1), (int16_t)(cy + dy), a);
        }

        for (int16_t dx = (int16_t)-r; dx <= r; ++dx)
        {
            const float xf = (float)dx;
            const float yf = sqrtf(rrF - xf * xf);
            const int16_t yi = (int16_t)floorf(yf);
            const float frac = yf - (float)yi;
            const uint8_t a = (uint8_t)std::min(255, (int)(frac * 255.0f));

            blendPixel((int16_t)(cx + dx), (int16_t)(cy + yi + 1), a);
            blendPixel((int16_t)(cx + dx), (int16_t)(cy - yi - 1), a);
        }
    }

    void GUI::fillRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888)
    {
        if (w <= 0 || h <= 0)
            return;

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                        ? (_activeSprite ? _activeSprite : &_sprite)
                                        : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t stride = spr->width();
        int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int16_t r = (int16_t)radius;
        int16_t maxR = (w < h ? w : h) / 2;
        if (r > maxR)
            r = maxR;
        if (r < 0)
            r = 0;

        Color888 c = Color888::fromUint32(color888);
        FrcProfile profile = _frcProfile;
        if (c.isBlack() || c.isWhite())
            profile = FrcProfile::Off;

        if (r == 0)
        {
            fillRect888(x, y, w, h, c.r, c.g, c.b, profile);
            return;
        }

        if (h - r * 2 > 0)
            fillRect888(x, (int16_t)(y + r), w, (int16_t)(h - r * 2), c.r, c.g, c.b, profile);

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

            fillRect888(spanX, (int16_t)(y + dy), spanW, 1, c.r, c.g, c.b, profile);
            fillRect888(spanX, (int16_t)(y + h - 1 - dy), spanW, 1, c.r, c.g, c.b, profile);
        }

        auto blendPixel = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if ((uint16_t)px >= (uint16_t)stride || (uint16_t)py >= (uint16_t)maxH)
                return;
            uint16_t *p = buf + (int32_t)py * (int32_t)stride + px;
            const uint16_t bg = swap16(*p);
            const uint16_t fg = detail::quantize565(c, px, py, _frcSeed, profile);
            *p = swap16(detail::blend565(bg, fg, a));
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
                const uint8_t a = (uint8_t)std::min(255, (int)(frac * 255.0f));

                blendPixel((int16_t)(cx + sx * (xi + 1)), (int16_t)(cy + sy * dy), a);
            }

            for (int16_t dx = 0; dx <= r; ++dx)
            {
                const float xf = (float)dx;
                const float yf = sqrtf(rrF - xf * xf);
                const int16_t yi = (int16_t)floorf(yf);
                const float frac = yf - (float)yi;
                const uint8_t a = (uint8_t)std::min(255, (int)(frac * 255.0f));

                blendPixel((int16_t)(cx + sx * dx), (int16_t)(cy + sy * (yi + 1)), a);
            }
        };

        aaQuarter((int16_t)(x + r), (int16_t)(y + r), -1, -1);
        aaQuarter((int16_t)(x + w - 1 - r), (int16_t)(y + r), +1, -1);
        aaQuarter((int16_t)(x + r), (int16_t)(y + h - 1 - r), -1, +1);
        aaQuarter((int16_t)(x + w - 1 - r), (int16_t)(y + h - 1 - r), +1, +1);
    }

    void GUI::drawRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888)
    {
        if (w <= 0 || h <= 0)
            return;

        if (w <= 2 || h <= 2)
        {
            fillRect(x, y, w, h, color888);
            return;
        }

        fillRoundRectFrc(x, y, w, h, radius, color888);
        fillRoundRectFrc(x + 1, y + 1, w - 2, h - 2, radius > 0 ? (uint8_t)(radius - 1) : 0, _bgColor);
    }

    void GUI::blitImage565Frc(int16_t x, int16_t y, const uint16_t *bitmap565, int16_t w, int16_t h)
    {
        if (!bitmap565 || w <= 0 || h <= 0)
            return;

        auto t = getDrawTarget();
        if (!t)
            return;

        const int16_t x0 = x;
        const int16_t y0 = y;

        for (int16_t yy = 0; yy < h; ++yy)
        {
            const int16_t py = (int16_t)(y0 + yy);
            const uint16_t *row = bitmap565 + (int32_t)yy * (int32_t)w;

            for (int16_t xx = 0; xx < w; ++xx)
            {
                const int16_t px = (int16_t)(x0 + xx);
                const uint16_t c565 = row[xx];

                const uint8_t r5 = (uint8_t)((c565 >> 11) & 0x1F);
                const uint8_t g6 = (uint8_t)((c565 >> 5) & 0x3F);
                const uint8_t b5 = (uint8_t)(c565 & 0x1F);

                const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
                const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
                const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);

                const uint32_t c888 = ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
                t->drawPixel(px, py, color888To565(px, py, c888));
            }
        }
    }

    uint16_t GUI::rgb888To565Frc(uint8_t r, uint8_t g, uint8_t b, int16_t x, int16_t y, FrcProfile profile) const
    {
        Color888 c{r, g, b};
        return detail::quantize565(c, x, y, _frcSeed, profile);
    }

    void GUI::clear888(uint8_t r, uint8_t g, uint8_t b, FrcProfile profile)
    {
        _bgColor = rgb(r, g, b);

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                        ? (_activeSprite ? _activeSprite : &_sprite)
                                        : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t w = spr->width();
        int16_t h = spr->height();
        if (w <= 0 || h <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = w, clipH = h;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        if ((r == 0 && g == 0 && b == 0) || (r == 255 && g == 255 && b == 255))
            profile = FrcProfile::Off;
        else if (r == g && g == b)
            profile = FrcProfile::Off;
        else if (((r & 7U) == 0U) && ((g & 3U) == 0U) && ((b & 7U) == 0U))
            profile = FrcProfile::Off;

        if (profile == FrcProfile::Off)
        {
            uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
            uint16_t v = swap16(c565);

            for (int32_t yy = 0; yy < clipH; ++yy)
            {
                int32_t row = (int32_t)(clipY + yy) * (int32_t)w;
                uint16_t *p = buf + row + clipX;
                for (int32_t xx = 0; xx < clipW; ++xx)
                    p[xx] = v;
            }

            if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
                invalidateRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
            return;
        }

        uint16_t tile[16 * 16];
        Color888 c{r, g, b};
        for (int16_t ty = 0; ty < 16; ++ty)
        {
            for (int16_t tx = 0; tx < 16; ++tx)
                tile[(uint16_t)ty * 16U + (uint16_t)tx] = swap16(detail::quantize565(c, tx, ty, _frcSeed, profile));
        }

        for (int32_t yy = 0; yy < clipH; ++yy)
        {
            int16_t py = (int16_t)(clipY + yy);
            int32_t row = (int32_t)py * (int32_t)w;
            const uint16_t *tileRow = &tile[((uint16_t)py & 15U) * 16U];
            for (int32_t xx = 0; xx < clipW; ++xx)
            {
                int16_t px = (int16_t)(clipX + xx);
                buf[row + px] = tileRow[(uint16_t)px & 15U];
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
    }

    void GUI::fillRect888(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b, FrcProfile profile)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                        ? (_activeSprite ? _activeSprite : &_sprite)
                                        : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t stride = spr->width();
        int16_t maxH = spr->height();
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

        if ((r == 0 && g == 0 && b == 0) || (r == 255 && g == 255 && b == 255))
            profile = FrcProfile::Off;
        else if (r == g && g == b)
            profile = FrcProfile::Off;
        else if (((r & 7U) == 0U) && ((g & 3U) == 0U) && ((b & 7U) == 0U))
            profile = FrcProfile::Off;

        if (profile == FrcProfile::Off)
        {
            uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
            uint16_t v = swap16(c565);
            for (int16_t yy = 0; yy < h; ++yy)
            {
                int16_t py = (int16_t)(y + yy);
                int32_t row = (int32_t)py * (int32_t)stride;
                uint16_t *p = buf + row + x;
                for (int16_t xx = 0; xx < w; ++xx)
                    p[xx] = v;
            }

            if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
                invalidateRect(x, y, w, h);
            return;
        }

        int32_t area = (int32_t)w * (int32_t)h;
        if (area <= 256)
        {
            Color888 c{r, g, b};
            for (int16_t yy = 0; yy < h; ++yy)
            {
                int16_t py = (int16_t)(y + yy);
                int32_t row = (int32_t)py * (int32_t)stride;
                for (int16_t xx = 0; xx < w; ++xx)
                {
                    int16_t px = (int16_t)(x + xx);
                    uint16_t c565 = detail::quantize565(c, px, py, _frcSeed, profile);
                    buf[row + px] = swap16(c565);
                }
            }
        }
        else
        {
            uint16_t tile[16 * 16];
            Color888 c{r, g, b};
            for (int16_t ty = 0; ty < 16; ++ty)
            {
                for (int16_t tx = 0; tx < 16; ++tx)
                    tile[(uint16_t)ty * 16U + (uint16_t)tx] = swap16(detail::quantize565(c, tx, ty, _frcSeed, profile));
            }

            for (int16_t yy = 0; yy < h; ++yy)
            {
                int16_t py = (int16_t)(y + yy);
                int32_t row = (int32_t)py * (int32_t)stride;
                const uint16_t *tileRow = &tile[((uint16_t)py & 15U) * 16U];
                for (int16_t xx = 0; xx < w; ++xx)
                {
                    int16_t px = (int16_t)(x + xx);
                    buf[row + px] = tileRow[(uint16_t)px & 15U];
                }
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x, y, w, h);
    }

    void GUI::drawLine(int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       uint32_t color)
    {
        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;

        if (x0 == x1 && y0 == y1)
        {
            spr->drawPixel(x0, y0, color888To565(x0, y0, color));
            return;
        }

        // Fast paths: axis-aligned lines without AA
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

        auto readBg888 = [&](int16_t px, int16_t py) -> uint32_t
        {
            const uint16_t v = swap16(buf[(int32_t)py * (int32_t)stride + px]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        };

        auto blend888 = [&](uint32_t bg, uint32_t fg, uint8_t a) -> uint32_t
        {
            if (a == 0)
                return bg;
            if (a >= 255)
                return fg;

            const uint32_t inv = 255U - a;
            const uint32_t br = (bg >> 16) & 0xFFU;
            const uint32_t bgc = (bg >> 8) & 0xFFU;
            const uint32_t bb = bg & 0xFFU;
            const uint32_t fr = (fg >> 16) & 0xFFU;
            const uint32_t fgc = (fg >> 8) & 0xFFU;
            const uint32_t fb = fg & 0xFFU;

            const uint8_t r = (uint8_t)((br * inv + fr * a) >> 8);
            const uint8_t g = (uint8_t)((bgc * inv + fgc * a) >> 8);
            const uint8_t b = (uint8_t)((bb * inv + fb * a) >> 8);
            return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        };

        auto blendPixel888Frc = [&](int16_t px, int16_t py, uint8_t a)
        {
            if (a == 0)
                return;
            if ((uint16_t)px >= (uint16_t)stride || (uint16_t)py >= (uint16_t)maxH)
                return;
            if (px < (int16_t)clipX || py < (int16_t)clipY || px >= (int16_t)(clipX + clipW) || py >= (int16_t)(clipY + clipH))
                return;

            const uint32_t bg888 = readBg888(px, py);
            const uint32_t out888 = (a >= 255) ? color : blend888(bg888, color, a);
            buf[(int32_t)py * (int32_t)stride + px] = swap16(color888To565(px, py, out888));
        };

        auto intensityCubic = [&](float rAbs) -> uint8_t
        {
            // Cubic falloff in [0..1]. Outside -> 0.
            if (rAbs <= 0.0f)
                return 255;
            if (rAbs >= 1.0f)
                return 0;
            // 1 - smoothstep(0,1,r)  where smoothstep = r^2(3-2r)
            const float t = rAbs;
            const float s = t * t * (3.0f - 2.0f * t);
            const float v = 1.0f - s;
            int aa = (int)(v * 255.0f + 0.5f);
            if (aa < 0)
                aa = 0;
            if (aa > 255)
                aa = 255;
            return (uint8_t)aa;
        };

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

        const int dx = (int)x1 - (int)x0;
        const int dyAbs = abs((int)y1 - (int)y0);
        const int yStep = (y1 >= y0) ? 1 : -1;

        const float length = sqrtf((float)dx * (float)dx + (float)dyAbs * (float)dyAbs);
        if (length <= 0.0f)
            return;

        const float sinT = (float)dx / length;
        const float cosT = (float)dyAbs / length;

        int x = x0;
        int y = y0;
        int d = 2 * dyAbs - dx;
        float D = 0.0f;

        while (x <= x1)
        {
            if (steep)
            {
                blendPixel888Frc((int16_t)(y - 1), (int16_t)x, intensityCubic(fabsf(D + cosT)));
                blendPixel888Frc((int16_t)y, (int16_t)x, intensityCubic(fabsf(D)));
                blendPixel888Frc((int16_t)(y + 1), (int16_t)x, intensityCubic(fabsf(D - cosT)));
            }
            else
            {
                blendPixel888Frc((int16_t)x, (int16_t)(y - 1), intensityCubic(fabsf(D + cosT)));
                blendPixel888Frc((int16_t)x, (int16_t)y, intensityCubic(fabsf(D)));
                blendPixel888Frc((int16_t)x, (int16_t)(y + 1), intensityCubic(fabsf(D - cosT)));
            }

            ++x;
            if (d <= 0)
            {
                D += sinT;
                d += 2 * dyAbs;
            }
            else
            {
                D += sinT - cosT;
                d += 2 * (dyAbs - dx);
                y += yStep;
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
        {
            // Conservative invalidate: bounding box expanded by 1px due to AA.
            int16_t minX = (x0 < x1) ? x0 : x1;
            int16_t maxX = (x0 > x1) ? x0 : x1;
            int16_t minY = (y0 < y1) ? y0 : y1;
            int16_t maxY = (y0 > y1) ? y0 : y1;
            if (steep)
            {
                std::swap(minX, minY);
                std::swap(maxX, maxY);
            }
            invalidateRect((int16_t)(minX - 1), (int16_t)(minY - 1), (int16_t)(maxX - minX + 3), (int16_t)(maxY - minY + 3));
        }
    }

    void GUI::fillCircle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
    {
        fillCircleFrc(cx, cy, r, color);
        drawCircle(cx, cy, r, color);
    }

    void GUI::drawCircle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
    {
        drawArc(cx, cy, r, 0.0f, 360.0f, color);
    }

    void GUI::drawArc(int16_t cx, int16_t cy, int16_t r, float startDeg, float endDeg, uint32_t color)
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
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        const float a0 = startDeg * 3.14159265f / 180.0f;
        const float a1 = endDeg * 3.14159265f / 180.0f;
        const float da = a1 - a0;
        if (da == 0.0f)
            return;

        auto norm2pi = [&](float a) -> float
        {
            const float twoPi = 6.28318530f;
            a = fmodf(a, twoPi);
            if (a < 0.0f)
                a += twoPi;
            return a;
        };

        const float na0 = norm2pi(a0);
        const float na1 = norm2pi(a1);
        const bool forward = (da > 0.0f);

        auto angleInSweep = [&](float a) -> bool
        {
            a = norm2pi(a);
            if (forward)
            {
                if (na0 <= na1)
                    return a >= na0 && a <= na1;
                return a >= na0 || a <= na1;
            }
            else
            {
                if (na1 <= na0)
                    return a >= na1 && a <= na0;
                return a >= na1 || a <= na0;
            }
        };

        auto readBg888 = [&](int16_t px, int16_t py) -> uint32_t
        {
            const uint16_t v = swap16(buf[(int32_t)py * (int32_t)stride + px]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        };

        auto blend888Local = [&](uint32_t bg, uint32_t fg, uint8_t a) -> uint32_t
        {
            if (a == 0)
                return bg;
            if (a >= 255)
                return fg;

            const uint32_t inv = 255U - a;
            const uint32_t br = (bg >> 16) & 0xFFU;
            const uint32_t bgc = (bg >> 8) & 0xFFU;
            const uint32_t bb = bg & 0xFFU;
            const uint32_t fr = (fg >> 16) & 0xFFU;
            const uint32_t fgc = (fg >> 8) & 0xFFU;
            const uint32_t fb = fg & 0xFFU;

            const uint8_t rr = (uint8_t)((br * inv + fr * a) >> 8);
            const uint8_t gg = (uint8_t)((bgc * inv + fgc * a) >> 8);
            const uint8_t bb8 = (uint8_t)((bb * inv + fb * a) >> 8);
            return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb8;
        };

        auto intensityCubicLocal = [&](float t01) -> uint8_t
        {
            if (t01 <= 0.0f)
                return 0;
            if (t01 >= 1.0f)
                return 255;
            const float s = t01 * t01 * (3.0f - 2.0f * t01);
            return (uint8_t)(s * 255.0f + 0.5f);
        };

        int16_t x0 = (int16_t)(cx - r - 1);
        int16_t y0 = (int16_t)(cy - r - 1);
        int16_t x1 = (int16_t)(cx + r + 1);
        int16_t y1 = (int16_t)(cy + r + 1);

        if (x0 < (int16_t)clipX)
            x0 = (int16_t)clipX;
        if (y0 < (int16_t)clipY)
            y0 = (int16_t)clipY;
        if (x1 > (int16_t)(clipX + clipW - 1))
            x1 = (int16_t)(clipX + clipW - 1);
        if (y1 > (int16_t)(clipY + clipH - 1))
            y1 = (int16_t)(clipY + clipH - 1);

        if (x0 > x1 || y0 > y1)
            return;

        const float fr = (float)r;
        for (int16_t py = y0; py <= y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            const float dy = ((float)py + 0.5f) - (float)cy;
            for (int16_t px = x0; px <= x1; ++px)
            {
                const float dx = ((float)px + 0.5f) - (float)cx;
                const float ang = atan2f(dy, dx);
                if (!angleInSweep(ang))
                    continue;

                const float d = sqrtf(dx * dx + dy * dy);
                const float dist = fabsf(d - fr);
                const float cov = 1.0f - dist;
                if (cov <= 0.0f)
                    continue;

                const uint8_t a = intensityCubicLocal(cov);
                const uint32_t bg888 = readBg888(px, py);
                const uint32_t out888 = blend888Local(bg888, color, a);
                buf[row + px] = swap16(color888To565(px, py, out888));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0 + 1), (int16_t)(y1 - y0 + 1));
    }

    void GUI::fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint32_t color)
    {
        if (rx <= 0 || ry <= 0)
            return;

        const float frx = (float)rx;
        const float fry = (float)ry;
        const float invRy2 = 1.0f / (fry * fry);

        for (int16_t dy = (int16_t)-ry; dy <= ry; ++dy)
        {
            const float y = (float)dy;
            const float t = 1.0f - (y * y) * invRy2;
            if (t <= 0.0f)
                continue;
            const float xf = frx * sqrtf(t);
            const int16_t xi = (int16_t)floorf(xf);
            fillRect((int16_t)(cx - xi), (int16_t)(cy + dy), (int16_t)(xi * 2 + 1), 1, color);
        }

        drawEllipse(cx, cy, rx, ry, color);
    }

    void GUI::drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint32_t color)
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
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        auto readBg888 = [&](int16_t px, int16_t py) -> uint32_t
        {
            const uint16_t v = swap16(buf[(int32_t)py * (int32_t)stride + px]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        };

        auto blend888Local = [&](uint32_t bg, uint32_t fg, uint8_t a) -> uint32_t
        {
            if (a == 0)
                return bg;
            if (a >= 255)
                return fg;
            const uint32_t inv = 255U - a;
            const uint32_t br = (bg >> 16) & 0xFFU;
            const uint32_t bgc = (bg >> 8) & 0xFFU;
            const uint32_t bb = bg & 0xFFU;
            const uint32_t fr = (fg >> 16) & 0xFFU;
            const uint32_t fgc = (fg >> 8) & 0xFFU;
            const uint32_t fb = fg & 0xFFU;
            const uint8_t rr = (uint8_t)((br * inv + fr * a) >> 8);
            const uint8_t gg = (uint8_t)((bgc * inv + fgc * a) >> 8);
            const uint8_t bb8 = (uint8_t)((bb * inv + fb * a) >> 8);
            return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb8;
        };

        auto intensityCubicLocal = [&](float t01) -> uint8_t
        {
            if (t01 <= 0.0f)
                return 0;
            if (t01 >= 1.0f)
                return 255;
            const float s = t01 * t01 * (3.0f - 2.0f * t01);
            return (uint8_t)(s * 255.0f + 0.5f);
        };

        int16_t x0 = (int16_t)(cx - rx - 1);
        int16_t y0 = (int16_t)(cy - ry - 1);
        int16_t x1 = (int16_t)(cx + rx + 1);
        int16_t y1 = (int16_t)(cy + ry + 1);

        if (x0 < (int16_t)clipX)
            x0 = (int16_t)clipX;
        if (y0 < (int16_t)clipY)
            y0 = (int16_t)clipY;
        if (x1 > (int16_t)(clipX + clipW - 1))
            x1 = (int16_t)(clipX + clipW - 1);
        if (y1 > (int16_t)(clipY + clipH - 1))
            y1 = (int16_t)(clipY + clipH - 1);

        if (x0 > x1 || y0 > y1)
            return;

        const float a = (float)rx;
        const float b = (float)ry;
        const float invA2 = 1.0f / (a * a);
        const float invB2 = 1.0f / (b * b);

        for (int16_t py = y0; py <= y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            const float y = ((float)py + 0.5f) - (float)cy;
            for (int16_t px = x0; px <= x1; ++px)
            {
                const float x = ((float)px + 0.5f) - (float)cx;

                const float xx = x * x;
                const float yy = y * y;
                const float f = (xx * invA2) + (yy * invB2) - 1.0f;

                const float gx = (2.0f * x) * invA2;
                const float gy = (2.0f * y) * invB2;
                const float g = sqrtf(gx * gx + gy * gy);
                if (g <= 1e-6f)
                    continue;

                const float dist = fabsf(f) / g;
                const float cov = 1.0f - dist;
                if (cov <= 0.0f)
                    continue;

                const uint8_t alpha = intensityCubicLocal(cov);
                const uint32_t bg888 = readBg888(px, py);
                const uint32_t out888 = blend888Local(bg888, color, alpha);
                buf[row + px] = swap16(color888To565(px, py, out888));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0 + 1), (int16_t)(y1 - y0 + 1));
    }

    void GUI::drawTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint32_t color)
    {
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    }

    void GUI::fillTriangle(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint32_t color)
    {
        // Sort by y
        if (y1 < y0)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        if (y2 < y0)
        {
            std::swap(x0, x2);
            std::swap(y0, y2);
        }
        if (y2 < y1)
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        if (y0 == y2)
        {
            int16_t minX = std::min<int16_t>(x0, std::min<int16_t>(x1, x2));
            int16_t maxX = std::max<int16_t>(x0, std::max<int16_t>(x1, x2));
            fillRect(minX, y0, (int16_t)(maxX - minX + 1), 1, color);
            return;
        }

        auto edgeX = [&](int16_t y, int16_t xa, int16_t ya, int16_t xb, int16_t yb) -> float
        {
            if (yb == ya)
                return (float)xa;
            return (float)xa + (float)(xb - xa) * ((float)(y - ya) / (float)(yb - ya));
        };

        for (int16_t y = y0; y <= y2; ++y)
        {
            bool upper = (y < y1) || (y1 == y0);
            float xa = edgeX(y, x0, y0, x2, y2);
            float xb = upper ? edgeX(y, x0, y0, x1, y1) : edgeX(y, x1, y1, x2, y2);
            if (xa > xb)
                std::swap(xa, xb);
            int16_t ix0 = (int16_t)ceilf(xa - 0.5f);
            int16_t ix1 = (int16_t)floorf(xb + 0.5f);
            if (ix1 >= ix0)
                fillRect(ix0, y, (int16_t)(ix1 - ix0 + 1), 1, color);
        }

        // AA edges
        drawTriangle(x0, y0, x1, y1, x2, y2, color);
    }

    void GUI::drawSquircle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
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
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = stride, clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        auto readBg888 = [&](int16_t px, int16_t py) -> uint32_t
        {
            const uint16_t v = swap16(buf[(int32_t)py * (int32_t)stride + px]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        };

        auto blend888Local = [&](uint32_t bg, uint32_t fg, uint8_t a) -> uint32_t
        {
            if (a == 0)
                return bg;
            if (a >= 255)
                return fg;
            const uint32_t inv = 255U - a;
            const uint32_t br = (bg >> 16) & 0xFFU;
            const uint32_t bgc = (bg >> 8) & 0xFFU;
            const uint32_t bb = bg & 0xFFU;
            const uint32_t fr = (fg >> 16) & 0xFFU;
            const uint32_t fgc = (fg >> 8) & 0xFFU;
            const uint32_t fb = fg & 0xFFU;
            const uint8_t rr = (uint8_t)((br * inv + fr * a) >> 8);
            const uint8_t gg = (uint8_t)((bgc * inv + fgc * a) >> 8);
            const uint8_t bb8 = (uint8_t)((bb * inv + fb * a) >> 8);
            return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb8;
        };

        auto intensityCubicLocal = [&](float t01) -> uint8_t
        {
            if (t01 <= 0.0f)
                return 0;
            if (t01 >= 1.0f)
                return 255;
            const float s = t01 * t01 * (3.0f - 2.0f * t01);
            return (uint8_t)(s * 255.0f + 0.5f);
        };

        int16_t x0 = (int16_t)(cx - r - 1);
        int16_t y0 = (int16_t)(cy - r - 1);
        int16_t x1 = (int16_t)(cx + r + 1);
        int16_t y1 = (int16_t)(cy + r + 1);

        if (x0 < (int16_t)clipX)
            x0 = (int16_t)clipX;
        if (y0 < (int16_t)clipY)
            y0 = (int16_t)clipY;
        if (x1 > (int16_t)(clipX + clipW - 1))
            x1 = (int16_t)(clipX + clipW - 1);
        if (y1 > (int16_t)(clipY + clipH - 1))
            y1 = (int16_t)(clipY + clipH - 1);

        if (x0 > x1 || y0 > y1)
            return;

        const float rr = (float)r;
        const float r4 = rr * rr * rr * rr;

        for (int16_t py = y0; py <= y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            const float y = ((float)py + 0.5f) - (float)cy;
            const float ay = fabsf(y);
            const float y2 = ay * ay;
            const float y3 = y2 * ay;

            for (int16_t px = x0; px <= x1; ++px)
            {
                const float x = ((float)px + 0.5f) - (float)cx;
                const float ax = fabsf(x);
                const float x2 = ax * ax;
                const float x3 = x2 * ax;

                const float f = (x2 * x2) + (y2 * y2) - r4;
                const float gx = 4.0f * x3;
                const float gy = 4.0f * y3;
                const float g = sqrtf(gx * gx + gy * gy);
                if (g <= 1e-6f)
                    continue;

                const float dist = fabsf(f) / g;
                const float cov = 1.0f - dist;
                if (cov <= 0.0f)
                    continue;

                const uint8_t alpha = intensityCubicLocal(cov);
                const uint32_t bg888 = readBg888(px, py);
                const uint32_t out888 = blend888Local(bg888, color, alpha);
                buf[row + px] = swap16(color888To565(px, py, out888));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0 + 1), (int16_t)(y1 - y0 + 1));
    }

    void GUI::fillSquircle(int16_t cx, int16_t cy, int16_t r, uint32_t color)
    {
        if (r <= 0)
            return;

        const float rr = (float)r;
        const float r4 = rr * rr * rr * rr;

        for (int16_t dy = (int16_t)-r; dy <= r; ++dy)
        {
            float y = (float)dy;
            float y4 = y * y;
            y4 *= y4;
            float rem = r4 - y4;
            if (rem <= 0.0f)
                continue;
            float xf = powf(rem, 0.25f);
            int16_t xi = (int16_t)floorf(xf);
            fillRect((int16_t)(cx - xi), (int16_t)(cy + dy), (int16_t)(xi * 2 + 1), 1, color);
        }

        drawSquircle(cx, cy, r, color);
    }

    pipcore::Sprite *GUI::getDrawTarget()
    {
        pipcore::Sprite *spr = nullptr;
        if (_flags.renderToSprite && _flags.spriteEnabled)
            spr = _activeSprite ? _activeSprite : &_sprite;
        else
            spr = &_sprite;

        applyGuiClipToSprite(spr, _clipEnabled, _clipX, _clipY, _clipW, _clipH);
        return spr;
    }

    void GUI::setClipWindow(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        _clipEnabled = true;
        _clipX = x;
        _clipY = y;
        _clipW = w;
        _clipH = h;
    }

    void GUI::resetClipWindow()
    {
        _clipEnabled = false;
        _clipX = 0;
        _clipY = 0;
        _clipW = 0;
        _clipH = 0;
    }

    void GUI::clear(uint32_t color)
    {
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        clear888(r, g, b, _frcProfile);
    }

    int16_t GUI::AutoX(int32_t contentWidth) const
    {
        int16_t left = (_flags.statusBarEnabled && _statusBarPos == Left) ? _statusBarHeight : 0;
        int16_t availW = _screenWidth - ((_flags.statusBarEnabled && (_statusBarPos == Left || _statusBarPos == Right)) ? _statusBarHeight : 0);

        if (contentWidth > availW)
            availW = (int16_t)contentWidth;

        return left + (availW - (int16_t)contentWidth) / 2;
    }

    int16_t GUI::AutoY(int32_t contentHeight) const
    {
        int16_t top = (_flags.statusBarEnabled && _statusBarPos == Top) ? _statusBarHeight : 0;
        int16_t availH = _screenHeight - ((_flags.statusBarEnabled && (_statusBarPos == Top || _statusBarPos == Bottom)) ? _statusBarHeight : 0);

        if (contentHeight > availH)
            availH = (int16_t)contentHeight;

        return top + (availH - (int16_t)contentHeight) / 2;
    }

    void GUI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color)
    {
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        fillRect888(x, y, w, h, r, g, b, _frcProfile);
    }

    void GUI::fillRectAlpha(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color, uint8_t alpha)
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

        auto readBg888 = [&](int16_t px, int16_t py) -> uint32_t
        {
            const uint16_t v = swap16(buf[(int32_t)py * (int32_t)stride + px]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
            const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
            const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
            return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
        };

        auto blend888Local = [&](uint32_t bg, uint32_t fg, uint8_t a) -> uint32_t
        {
            if (a == 0)
                return bg;
            if (a >= 255)
                return fg;

            const uint32_t inv = 255U - a;
            const uint32_t br = (bg >> 16) & 0xFFU;
            const uint32_t bgc = (bg >> 8) & 0xFFU;
            const uint32_t bb = bg & 0xFFU;
            const uint32_t fr = (fg >> 16) & 0xFFU;
            const uint32_t fgc = (fg >> 8) & 0xFFU;
            const uint32_t fb = fg & 0xFFU;

            const uint8_t rr = (uint8_t)((br * inv + fr * a) >> 8);
            const uint8_t gg = (uint8_t)((bgc * inv + fgc * a) >> 8);
            const uint8_t bb8 = (uint8_t)((bb * inv + fb * a) >> 8);
            return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb8;
        };

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
            {
                const uint32_t bg888 = readBg888(px, py);
                const uint32_t out888 = blend888Local(bg888, color, alpha);
                buf[row + px] = swap16(color888To565(px, py, out888));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    static inline uint8_t u8lerp(uint8_t a, uint8_t b, uint16_t t255)
    {
        return (uint8_t)((((uint16_t)a * (uint16_t)(255U - t255)) + ((uint16_t)b * t255) + 127U) / 255U);
    }

    static inline uint32_t lerp888(uint32_t a, uint32_t b, uint16_t t255)
    {
        const uint8_t ar = (uint8_t)((a >> 16) & 0xFF);
        const uint8_t ag = (uint8_t)((a >> 8) & 0xFF);
        const uint8_t ab = (uint8_t)(a & 0xFF);
        const uint8_t br = (uint8_t)((b >> 16) & 0xFF);
        const uint8_t bg = (uint8_t)((b >> 8) & 0xFF);
        const uint8_t bb = (uint8_t)(b & 0xFF);
        const uint8_t rr = u8lerp(ar, br, t255);
        const uint8_t gg = u8lerp(ag, bg, t255);
        const uint8_t bb8 = u8lerp(ab, bb, t255);
        return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb8;
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

        const int16_t fullY0 = y;
        const int16_t fullH = h;
        const int denom = (fullH > 1) ? (fullH - 1) : 1;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int relY = (int)py - (int)fullY0;
            const uint16_t t = (uint16_t)((relY <= 0) ? 0 : (relY >= denom ? 255 : (relY * 255) / denom));
            const uint32_t c = lerp888(topColor, bottomColor, t);
            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
                buf[row + px] = swap16(color888To565(px, py, c));
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
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

        const int16_t fullX0 = x;
        const int16_t fullW = w;
        const int denom = (fullW > 1) ? (fullW - 1) : 1;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
            {
                const int relX = (int)px - (int)fullX0;
                const uint16_t t = (uint16_t)((relX <= 0) ? 0 : (relX >= denom ? 255 : (relX * 255) / denom));
                const uint32_t c = lerp888(leftColor, rightColor, t);
                buf[row + px] = swap16(color888To565(px, py, c));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
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

        const int denomX = (w > 1) ? (w - 1) : 1;
        const int denomY = (h > 1) ? (h - 1) : 1;
        const int fullX0 = x;
        const int fullY0 = y;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int relY = (int)py - fullY0;
            const uint16_t v = (uint16_t)((relY <= 0) ? 0 : (relY >= denomY ? 255 : (relY * 255) / denomY));

            const uint32_t left = lerp888(c00, c01, v);
            const uint32_t right = lerp888(c10, c11, v);

            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
            {
                const int relX = (int)px - fullX0;
                const uint16_t u = (uint16_t)((relX <= 0) ? 0 : (relX >= denomX ? 255 : (relX * 255) / denomX));
                const uint32_t c = lerp888(left, right, u);
                buf[row + px] = swap16(color888To565(px, py, c));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
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

        const int fullX0 = x;
        const int fullY0 = y;
        const int denom = ((w + h) > 2) ? ((w + h) - 2) : 1;

        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = x0; px < x1; ++px)
            {
                const int rel = ((int)px - fullX0) + ((int)py - fullY0);
                const uint16_t t = (uint16_t)((rel <= 0) ? 0 : (rel >= denom ? 255 : (rel * 255) / denom));
                const uint32_t c = lerp888(tlColor, brColor, t);
                buf[row + px] = swap16(color888To565(px, py, c));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
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

        if (radius <= 0)
            radius = 1;

        const float invR = 1.0f / (float)radius;
        for (int16_t py = y0; py < y1; ++py)
        {
            const int32_t row = (int32_t)py * (int32_t)stride;
            const float dy = (float)((int32_t)py - (int32_t)cy);
            for (int16_t px = x0; px < x1; ++px)
            {
                const float dx = (float)((int32_t)px - (int32_t)cx);
                float d = sqrtf(dx * dx + dy * dy) * invR;
                if (d < 0.0f)
                    d = 0.0f;
                if (d > 1.0f)
                    d = 1.0f;
                const uint16_t t = (uint16_t)(d * 255.0f + 0.5f);
                const uint32_t c = lerp888(innerColor, outerColor, t);
                buf[row + px] = swap16(color888To565(px, py, c));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x0, y0, (int16_t)(x1 - x0), (int16_t)(y1 - y0));
    }

    void GUI::drawCenteredTitle(const String &title, const String &subtitle, uint32_t fg, uint32_t bg)
    {
        auto t = getDrawTarget();

        clear(bg);

        uint16_t titlePx = _logoTitleSizePx ? _logoTitleSizePx : _textStyleH1Px;
        uint16_t subPx = _logoSubtitleSizePx ? _logoSubtitleSizePx : (uint16_t)((titlePx * 3U) / 4U);

        int16_t titleW = 0;
        int16_t titleH = 0;
        setPSDFFontSize(titlePx);
        psdfMeasureText(title, titleW, titleH);

        bool hasSub = subtitle.length() > 0;
        int16_t subW = 0;
        int16_t subH = 0;
        if (hasSub)
        {
            setPSDFFontSize(subPx);
            psdfMeasureText(subtitle, subW, subH);
        }

        int16_t spacing = 4;

        int16_t totalH = titleH;
        if (hasSub)
            totalH = (int16_t)(titleH + spacing + subH);

        int16_t topY = (_bootY != -1) ? _bootY : (_screenHeight - totalH) / 2;
        int16_t cx = (_bootX != -1) ? _bootX : (int16_t)(_screenWidth / 2);

        setPSDFFontSize(titlePx);
        uint16_t fg565Title = color888To565(cx, topY, fg);
        uint16_t bg565Title = color888To565(cx, topY, bg);
        psdfDrawTextInternal(title, cx, topY, fg565Title, bg565Title, AlignCenter);

        if (hasSub)
        {
            int16_t subY = (int16_t)(topY + titleH + spacing);
            setPSDFFontSize(subPx);
            uint16_t fg565Sub = color888To565(cx, subY, fg);
            uint16_t bg565Sub = color888To565(cx, subY, bg);
            psdfDrawTextInternal(subtitle, cx, subY, fg565Sub, bg565Sub, AlignCenter);
        }
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_display || !_flags.spriteEnabled)
            return;
        if (w <= 0 || h <= 0)
            return;

        if (_debugShowDirtyRects)
        {
            x = (int16_t)(x - 2);
            y = (int16_t)(y - 2);
            w = (int16_t)(w + 4);
            h = (int16_t)(h + 4);
        }

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
        if (w <= 0 || h <= 0)
            return;

        if (x + w > (int16_t)_screenWidth)
            w = (int16_t)_screenWidth - x;
        if (y + h > (int16_t)_screenHeight)
            h = (int16_t)_screenHeight - y;

        if (w <= 0 || h <= 0)
            return;

        if (_debugShowDirtyRects)
        {
            if (_debugRectCount < (uint8_t)(sizeof(_debugRects) / sizeof(_debugRects[0])))
            {
                _debugRects[_debugRectCount++] = {x, y, w, h};
            }
            else
            {
                for (uint8_t i = 1; i < _debugRectCount; ++i)
                    _debugRects[i - 1] = _debugRects[i];
                _debugRects[_debugRectCount - 1] = {x, y, w, h};
            }
        }

        auto intersectsOrTouches = [](const DirtyRect &a, int16_t bx, int16_t by, int16_t bw, int16_t bh) -> bool
        {
            int16_t ax2 = (int16_t)(a.x + a.w);
            int16_t ay2 = (int16_t)(a.y + a.h);
            int16_t bx2 = (int16_t)(bx + bw);
            int16_t by2 = (int16_t)(by + bh);
            return !(bx2 < a.x || bx > ax2 || by2 < a.y || by > ay2);
        };

        auto mergeInto = [](DirtyRect &a, int16_t bx, int16_t by, int16_t bw, int16_t bh)
        {
            int16_t x1 = (a.x < bx) ? a.x : bx;
            int16_t y1 = (a.y < by) ? a.y : by;
            int16_t x2 = ((int16_t)(a.x + a.w) > (int16_t)(bx + bw)) ? (int16_t)(a.x + a.w) : (int16_t)(bx + bw);
            int16_t y2 = ((int16_t)(a.y + a.h) > (int16_t)(by + bh)) ? (int16_t)(a.y + a.h) : (int16_t)(by + bh);
            a.x = x1;
            a.y = y1;
            a.w = (int16_t)(x2 - x1);
            a.h = (int16_t)(y2 - y1);
        };

        for (uint8_t i = 0; i < _dirtyCount; ++i)
        {
            if (intersectsOrTouches(_dirty[i], x, y, w, h))
            {
                mergeInto(_dirty[i], x, y, w, h);
                for (uint8_t j = 0; j < _dirtyCount; ++j)
                {
                    if (j == i)
                        continue;
                    if (intersectsOrTouches(_dirty[i], _dirty[j].x, _dirty[j].y, _dirty[j].w, _dirty[j].h))
                    {
                        mergeInto(_dirty[i], _dirty[j].x, _dirty[j].y, _dirty[j].w, _dirty[j].h);
                        _dirty[j] = _dirty[_dirtyCount - 1];
                        --_dirtyCount;
                        --j;
                    }
                }
                return;
            }
        }

        if (_dirtyCount < (uint8_t)(sizeof(_dirty) / sizeof(_dirty[0])))
        {
            _dirty[_dirtyCount++] = {x, y, w, h};
            return;
        }

        if (_dirtyCount > 0)
        {
            mergeInto(_dirty[0], x, y, w, h);
            for (uint8_t i = 1; i < _dirtyCount; ++i)
                mergeInto(_dirty[0], _dirty[i].x, _dirty[i].y, _dirty[i].w, _dirty[i].h);
            _dirtyCount = 1;
        }
    }

    void GUI::flushDirty()
    {
        if (_dirtyCount == 0)
            return;
        if (!_display || !_flags.spriteEnabled)
        {
            _dirtyCount = 0;
            return;
        }

        const int16_t sw = _sprite.width();
        const int16_t sh = _sprite.height();
        uint16_t *buf = (uint16_t *)_sprite.getBuffer();
        const int32_t stride = sw;

        for (uint8_t i = 0; i < _dirtyCount; ++i)
        {
            DirtyRect r = _dirty[i];
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

            if (_debugShowDirtyRects && buf && sw > 0 && sh > 0)
            {
                if (w > 0 && h > 0)
                {
                    const uint32_t perim = (uint32_t)(w * 2) + (uint32_t)((h > 2) ? ((h - 2) * 2) : 0);
                    uint16_t *saved = (uint16_t *)detail::guiAlloc(platform(), perim * sizeof(uint16_t), pipcore::GuiAllocCaps::Default);
                    if (saved)
                    {
                        uint32_t idx = 0;

                        const int32_t top = (int32_t)y0 * stride + x0;
                        for (int16_t xx = 0; xx < w; ++xx)
                            saved[idx++] = buf[top + xx];

                        if (h > 1)
                        {
                            const int32_t bot = (int32_t)(y0 + h - 1) * stride + x0;
                            for (int16_t xx = 0; xx < w; ++xx)
                                saved[idx++] = buf[bot + xx];
                        }

                        if (h > 2)
                        {
                            for (int16_t yy = 1; yy < h - 1; ++yy)
                                saved[idx++] = buf[((int32_t)(y0 + yy) * stride) + x0];
                            if (w > 1)
                            {
                                for (int16_t yy = 1; yy < h - 1; ++yy)
                                    saved[idx++] = buf[((int32_t)(y0 + yy) * stride) + (x0 + w - 1)];
                            }
                        }
                        else if (w > 1)
                        {
                            for (int16_t yy = 0; yy < h; ++yy)
                                saved[idx++] = buf[((int32_t)(y0 + yy) * stride) + (x0 + w - 1)];
                        }

                        const uint16_t col = swap16(_debugDirtyRectActiveColor);

                        for (int16_t xx = 0; xx < w; ++xx)
                            buf[top + xx] = col;

                        if (h > 1)
                        {
                            const int32_t bot = (int32_t)(y0 + h - 1) * stride + x0;
                            for (int16_t xx = 0; xx < w; ++xx)
                                buf[bot + xx] = col;
                        }

                        for (int16_t yy = 1; yy < h - 1; ++yy)
                        {
                            buf[((int32_t)(y0 + yy) * stride) + x0] = col;
                            if (w > 1)
                                buf[((int32_t)(y0 + yy) * stride) + (x0 + w - 1)] = col;
                        }

                        _sprite.writeToDisplay(*_display, x0, y0, w, h);

                        idx = 0;
                        for (int16_t xx = 0; xx < w; ++xx)
                            buf[top + xx] = saved[idx++];

                        if (h > 1)
                        {
                            const int32_t bot = (int32_t)(y0 + h - 1) * stride + x0;
                            for (int16_t xx = 0; xx < w; ++xx)
                                buf[bot + xx] = saved[idx++];
                        }

                        if (h > 2)
                        {
                            for (int16_t yy = 1; yy < h - 1; ++yy)
                                buf[((int32_t)(y0 + yy) * stride) + x0] = saved[idx++];
                            if (w > 1)
                            {
                                for (int16_t yy = 1; yy < h - 1; ++yy)
                                    buf[((int32_t)(y0 + yy) * stride) + (x0 + w - 1)] = saved[idx++];
                            }
                        }
                        else if (w > 1)
                        {
                            for (int16_t yy = 0; yy < h; ++yy)
                                buf[((int32_t)(y0 + yy) * stride) + (x0 + w - 1)] = saved[idx++];
                        }

                        detail::guiFree(platform(), saved);
                        continue;
                    }
                }
            }

            _sprite.writeToDisplay(*_display, x0, y0, w, h);
        }

        _dirtyCount = 0;
    }

    void GUI::tickDebugDirtyOverlay(uint32_t now)
    {
        if (!_debugShowDirtyRects || !_display)
            return;
        if (_debugRectCount == 0)
            return;
        if (_flags.bootActive || _flags.errorActive)
            return;
    }

}

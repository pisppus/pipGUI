#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/Debug.hpp>
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

    void GUI::fillRoundRectCornersFrc(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint8_t radiusTL, uint8_t radiusTR,
                                      uint8_t radiusBR, uint8_t radiusBL,
                                      uint32_t color888)
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

        // Clamp radii to half of min(w, h)
        int16_t maxR = (w < h ? w : h) / 2;
        uint8_t rTL = (radiusTL > maxR) ? (uint8_t)maxR : radiusTL;
        uint8_t rTR = (radiusTR > maxR) ? (uint8_t)maxR : radiusTR;
        uint8_t rBR = (radiusBR > maxR) ? (uint8_t)maxR : radiusBR;
        uint8_t rBL = (radiusBL > maxR) ? (uint8_t)maxR : radiusBL;

        Color888 c = Color888::fromUint32(color888);
        FrcProfile profile = _frcProfile;
        if (c.isBlack() || c.isWhite())
            profile = FrcProfile::Off;

        // Fast path: all radii zero -> plain rect
        if (rTL == 0 && rTR == 0 && rBR == 0 && rBL == 0)
        {
            fillRect888(x, y, w, h, c.r, c.g, c.b, profile);
            return;
        }

        // For each scanline compute left/right inset from corners and draw horizontal span.
        for (int16_t iy = 0; iy < h; ++iy)
        {
            int16_t insetL = 0;
            int16_t insetR = 0;

            // Top-left corner
            if (rTL > 0 && iy < rTL)
            {
                int16_t dy = (int16_t)(rTL - 1 - iy);
                int32_t rr = (int32_t)rTL * (int32_t)rTL;
                int32_t inside = rr - (int32_t)dy * (int32_t)dy;
                if (inside < 0)
                    inside = 0;
                int32_t xx = 0;
                while ((xx + 1) * (xx + 1) <= inside)
                    ++xx;
                int16_t inset = (int16_t)(rTL - (int16_t)xx);
                if (inset > insetL)
                    insetL = inset;
            }

            // Bottom-left corner
            if (rBL > 0 && iy >= h - rBL)
            {
                int16_t rowFromBottom = (int16_t)(h - 1 - iy);
                int16_t dy = (int16_t)(rBL - 1 - rowFromBottom);
                int32_t rr = (int32_t)rBL * (int32_t)rBL;
                int32_t inside = rr - (int32_t)dy * (int32_t)dy;
                if (inside < 0)
                    inside = 0;
                int32_t xx = 0;
                while ((xx + 1) * (xx + 1) <= inside)
                    ++xx;
                int16_t inset = (int16_t)(rBL - (int16_t)xx);
                if (inset > insetL)
                    insetL = inset;
            }

            // Top-right corner
            if (rTR > 0 && iy < rTR)
            {
                int16_t dy = (int16_t)(rTR - 1 - iy);
                int32_t rr = (int32_t)rTR * (int32_t)rTR;
                int32_t inside = rr - (int32_t)dy * (int32_t)dy;
                if (inside < 0)
                    inside = 0;
                int32_t xx = 0;
                while ((xx + 1) * (xx + 1) <= inside)
                    ++xx;
                int16_t inset = (int16_t)(rTR - (int16_t)xx);
                if (inset > insetR)
                    insetR = inset;
            }

            // Bottom-right corner
            if (rBR > 0 && iy >= h - rBR)
            {
                int16_t rowFromBottom = (int16_t)(h - 1 - iy);
                int16_t dy = (int16_t)(rBR - 1 - rowFromBottom);
                int32_t rr = (int32_t)rBR * (int32_t)rBR;
                int32_t inside = rr - (int32_t)dy * (int32_t)dy;
                if (inside < 0)
                    inside = 0;
                int32_t xx = 0;
                while ((xx + 1) * (xx + 1) <= inside)
                    ++xx;
                int16_t inset = (int16_t)(rBR - (int16_t)xx);
                if (inset > insetR)
                    insetR = inset;
            }

            int16_t spanX = (int16_t)(x + insetL);
            int16_t spanW = (int16_t)(w - insetL - insetR);
            if (spanW <= 0)
                continue;

            fillRect888(spanX, (int16_t)(y + iy), spanW, 1, c.r, c.g, c.b, profile);
        }
    }

    void GUI::drawRoundRectCornersFrc(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint8_t radiusTL, uint8_t radiusTR,
                                      uint8_t radiusBR, uint8_t radiusBL,
                                      uint32_t color888)
    {
        if (w <= 0 || h <= 0)
            return;

        if (w <= 2 || h <= 2)
        {
            fillRectImpl(x, y, w, h, color888);
            return;
        }

        fillRoundRectCornersFrc(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL, color888);

        // Inner stroke: shrink rect by 1px and radii by 1 where non-zero, fill with background color.
        uint8_t inTL = (radiusTL > 0) ? (uint8_t)(radiusTL - 1) : 0;
        uint8_t inTR = (radiusTR > 0) ? (uint8_t)(radiusTR - 1) : 0;
        uint8_t inBR = (radiusBR > 0) ? (uint8_t)(radiusBR - 1) : 0;
        uint8_t inBL = (radiusBL > 0) ? (uint8_t)(radiusBL - 1) : 0;
        if (w > 2 && h > 2)
            fillRoundRectCornersFrc((int16_t)(x + 1), (int16_t)(y + 1), (int16_t)(w - 2), (int16_t)(h - 2),
                                    inTL, inTR, inBR, inBL, _bgColor);
    }

    void GUI::drawRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888)
    {
        if (w <= 0 || h <= 0)
            return;

        if (w <= 2 || h <= 2)
        {
            fillRectImpl(x, y, w, h, color888);
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

    void GUI::drawLineImpl(int16_t x0, int16_t y0,
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
            fillRectImpl(x0, y0, (int16_t)(x1 - x0 + 1), 1, color);
            return;
        }
        if (x0 == x1)
        {
            if (y1 < y0)
                std::swap(y0, y1);
            fillRectImpl(x0, y0, 1, (int16_t)(y1 - y0 + 1), color);
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

    void GUI::fillCircleImpl(int16_t cx, int16_t cy, int16_t r, uint32_t color)
    {
        fillCircleFrc(cx, cy, r, color);
        drawCircleImpl(cx, cy, r, color);
    }

    void GUI::drawCircleImpl(int16_t cx, int16_t cy, int16_t r, uint32_t color)
    {
        drawArcImpl(cx, cy, r, 0.0f, 360.0f, color);
    }

    void GUI::drawArcImpl(int16_t cx, int16_t cy, int16_t r, float startDeg, float endDeg, uint32_t color)
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

    void GUI::fillEllipseImpl(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint32_t color)
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
            fillRectImpl((int16_t)(cx - xi), (int16_t)(cy + dy), (int16_t)(xi * 2 + 1), 1, color);
        }

        drawEllipseImpl(cx, cy, rx, ry, color);
    }

    void GUI::drawEllipseImpl(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint32_t color)
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

    void GUI::drawTriangleImpl(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2,
                           uint32_t color)
    {
        drawLineImpl(x0, y0, x1, y1, color);
        drawLineImpl(x1, y1, x2, y2, color);
        drawLineImpl(x2, y2, x0, y0, color);
    }

    void GUI::fillTriangleImpl(int16_t x0, int16_t y0,
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
            fillRectImpl(minX, y0, (int16_t)(maxX - minX + 1), 1, color);
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
                fillRectImpl(ix0, y, (int16_t)(ix1 - ix0 + 1), 1, color);
        }
    }

    void GUI::drawRoundTriangleImpl(int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2,
                                uint8_t radius, uint32_t color)
    {
        if (radius == 0)
        {
            drawTriangleImpl(x0, y0, x1, y1, x2, y2, color);
            return;
        }

        if (!_flags.spriteEnabled)
            return;

        auto spr = getDrawTarget();
        if (!spr)
            return;

        // SDF-based rounded triangle outline (thin stroke around rounded triangle).
        const float r = (float)radius;

        struct Vec2
        {
            float x, y;
        };
        auto make = [](float x, float y) -> Vec2 { Vec2 v{x, y}; return v; };
        auto sub = [](const Vec2 &a, const Vec2 &b) -> Vec2 { return Vec2{a.x - b.x, a.y - b.y}; };
        auto dot2f = [](const Vec2 &v) -> float { return v.x * v.x + v.y * v.y; };
        auto dotf = [](const Vec2 &a, const Vec2 &b) -> float { return a.x * b.x + a.y * b.y; };
        auto cross2 = [](const Vec2 &a, const Vec2 &b) -> float { return a.x * b.y - a.y * b.x; };

        auto distToSegment = [&](const Vec2 &p, const Vec2 &a, const Vec2 &b) -> float
        {
            Vec2 pa = sub(p, a);
            Vec2 ba = sub(b, a);
            float h = 0.0f;
            float len2 = dot2f(ba);
            if (len2 > 0.0f)
            {
                h = dotf(pa, ba) / len2;
                if (h < 0.0f)
                    h = 0.0f;
                else if (h > 1.0f)
                    h = 1.0f;
            }
            Vec2 v = Vec2{pa.x - ba.x * h, pa.y - ba.y * h};
            return sqrtf(dot2f(v));
        };

        Vec2 v0 = make((float)x0, (float)y0);
        Vec2 v1 = make((float)x1, (float)y1);
        Vec2 v2 = make((float)x2, (float)y2);

        Vec2 e0 = sub(v1, v0);
        Vec2 e1 = sub(v2, v1);
        Vec2 e2 = sub(v0, v2);
        Vec2 v02 = sub(v2, v0);
        float ori = cross2(e0, v02);

        int16_t minX = std::min<int16_t>(x0, std::min<int16_t>(x1, x2));
        int16_t maxX = std::max<int16_t>(x0, std::max<int16_t>(x1, x2));
        int16_t minY = std::min<int16_t>(y0, std::min<int16_t>(y1, y2));
        int16_t maxY = std::max<int16_t>(y0, std::max<int16_t>(y1, y2));

        int16_t pad = (int16_t)radius + 2;
        int16_t xStart = (int16_t)(minX - pad);
        int16_t xEnd   = (int16_t)(maxX + pad);
        int16_t yStart = (int16_t)(minY - pad);
        int16_t yEnd   = (int16_t)(maxY + pad);

        if (xStart > xEnd || yStart > yEnd)
            return;

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                Vec2 p = make((float)px + 0.5f, (float)py + 0.5f);

                Vec2 pv0 = sub(p, v0);
                Vec2 pv1 = sub(p, v1);
                Vec2 pv2 = sub(p, v2);

                float c0 = cross2(e0, pv0);
                float c1 = cross2(e1, pv1);
                float c2 = cross2(e2, pv2);
                if (ori < 0.0f)
                {
                    c0 = -c0;
                    c1 = -c1;
                    c2 = -c2;
                }
                bool inside = (c0 >= 0.0f && c1 >= 0.0f && c2 >= 0.0f);

                float d0 = distToSegment(p, v0, v1);
                float d1 = distToSegment(p, v1, v2);
                float d2 = distToSegment(p, v2, v0);
                float d = d0;
                if (d1 < d) d = d1;
                if (d2 < d) d = d2;

                float sd = (inside ? -d : d) + r;

                // Thin stroke around rounded triangle boundary.
                if (fabsf(sd) <= 0.75f)
                    spr->drawPixel(px, py, color888To565(px, py, color));
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
        {
            int16_t bx = xStart;
            int16_t by = yStart;
            int16_t bw = (int16_t)(xEnd - xStart + 1);
            int16_t bh = (int16_t)(yEnd - yStart + 1);
            invalidateRect(bx, by, bw, bh);
        }
    }

    void GUI::fillRoundTriangleImpl(int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2,
                                uint8_t radius, uint32_t color)
    {
        if (radius == 0)
        {
            fillTriangleImpl(x0, y0, x1, y1, x2, y2, color);
            return;
        }

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

        Color888 c = Color888::fromUint32(color);
        FrcProfile profile = _frcProfile;
        if (c.isBlack() || c.isWhite())
            profile = FrcProfile::Off;
        else if (c.r == c.g && c.g == c.b)
            profile = FrcProfile::Off;
        else if (((c.r & 7U) == 0U) && ((c.g & 3U) == 0U) && ((c.b & 7U) == 0U))
            profile = FrcProfile::Off;

        const float r = (float)radius;

        struct Vec2
        {
            float x, y;
        };
        auto make = [](float x, float y) -> Vec2 { Vec2 v{x, y}; return v; };
        auto sub = [](const Vec2 &a, const Vec2 &b) -> Vec2 { return Vec2{a.x - b.x, a.y - b.y}; };
        auto dot2f = [](const Vec2 &v) -> float { return v.x * v.x + v.y * v.y; };
        auto dotf = [](const Vec2 &a, const Vec2 &b) -> float { return a.x * b.x + a.y * b.y; };
        auto cross2 = [](const Vec2 &a, const Vec2 &b) -> float { return a.x * b.y - a.y * b.x; };

        auto distToSegment = [&](const Vec2 &p, const Vec2 &a, const Vec2 &b) -> float
        {
            Vec2 pa = sub(p, a);
            Vec2 ba = sub(b, a);
            float h = 0.0f;
            float len2 = dot2f(ba);
            if (len2 > 0.0f)
            {
                h = dotf(pa, ba) / len2;
                if (h < 0.0f)
                    h = 0.0f;
                else if (h > 1.0f)
                    h = 1.0f;
            }
            Vec2 v = Vec2{pa.x - ba.x * h, pa.y - ba.y * h};
            return sqrtf(dot2f(v));
        };

        Vec2 v0 = make((float)x0, (float)y0);
        Vec2 v1 = make((float)x1, (float)y1);
        Vec2 v2 = make((float)x2, (float)y2);

        Vec2 e0 = sub(v1, v0);
        Vec2 e1 = sub(v2, v1);
        Vec2 e2 = sub(v0, v2);
        Vec2 v02 = sub(v2, v0);
        float ori = cross2(e0, v02);

        int16_t minX = std::min<int16_t>(x0, std::min<int16_t>(x1, x2));
        int16_t maxX = std::max<int16_t>(x0, std::max<int16_t>(x1, x2));
        int16_t minY = std::min<int16_t>(y0, std::min<int16_t>(y1, y2));
        int16_t maxY = std::max<int16_t>(y0, std::max<int16_t>(y1, y2));

        int16_t pad = (int16_t)radius + 2;
        int16_t xStart = (int16_t)(minX - pad);
        int16_t xEnd   = (int16_t)(maxX + pad);
        int16_t yStart = (int16_t)(minY - pad);
        int16_t yEnd   = (int16_t)(maxY + pad);

        if (xStart > xEnd || yStart > yEnd)
            return;

        // Clamp to clip rect
        if (xStart < (int16_t)clipX) xStart = (int16_t)clipX;
        if (yStart < (int16_t)clipY) yStart = (int16_t)clipY;
        if (xEnd > (int16_t)(clipX + clipW - 1)) xEnd = (int16_t)(clipX + clipW - 1);
        if (yEnd > (int16_t)(clipY + clipH - 1)) yEnd = (int16_t)(clipY + clipH - 1);
        if (xStart > xEnd || yStart > yEnd)
            return;

        for (int16_t py = yStart; py <= yEnd; ++py)
        {
            int32_t row = (int32_t)py * (int32_t)stride;
            for (int16_t px = xStart; px <= xEnd; ++px)
            {
                Vec2 p = make((float)px + 0.5f, (float)py + 0.5f);

                Vec2 pv0 = sub(p, v0);
                Vec2 pv1 = sub(p, v1);
                Vec2 pv2 = sub(p, v2);

                float c0 = cross2(e0, pv0);
                float c1 = cross2(e1, pv1);
                float c2 = cross2(e2, pv2);
                if (ori < 0.0f)
                {
                    c0 = -c0;
                    c1 = -c1;
                    c2 = -c2;
                }
                bool inside = (c0 >= 0.0f && c1 >= 0.0f && c2 >= 0.0f);

                float d0 = distToSegment(p, v0, v1);
                float d1 = distToSegment(p, v1, v2);
                float d2 = distToSegment(p, v2, v0);
                float d = d0;
                if (d1 < d) d = d1;
                if (d2 < d) d = d2;

                float sd = (inside ? -d : d) + r;

                if (sd <= 0.0f)
                {
                    uint16_t c565 = detail::quantize565(c, px, py, _frcSeed, profile);
                    buf[row + px] = swap16(c565);
                }
            }
        }

        if (_display && _flags.spriteEnabled && !_flags.renderToSprite)
        {
            int16_t bx = xStart;
            int16_t by = yStart;
            int16_t bw = (int16_t)(xEnd - xStart + 1);
            int16_t bh = (int16_t)(yEnd - yStart + 1);
            invalidateRect(bx, by, bw, bh);
        }
    }

    void GUI::drawSquircleImpl(int16_t cx, int16_t cy, int16_t r, uint32_t color)
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

    void GUI::fillSquircleImpl(int16_t cx, int16_t cy, int16_t r, uint32_t color)
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
            fillRectImpl((int16_t)(cx - xi), (int16_t)(cy + dy), (int16_t)(xi * 2 + 1), 1, color);
        }

        drawSquircleImpl(cx, cy, r, color);
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
        int16_t sb = statusBarHeight();
        int16_t left = (_flags.statusBarEnabled && _statusBarPos == Left) ? sb : 0;
        int16_t availW = _screenWidth - ((_flags.statusBarEnabled && (_statusBarPos == Left || _statusBarPos == Right)) ? sb : 0);

        if (contentWidth > availW)
            availW = (int16_t)contentWidth;

        return left + (availW - (int16_t)contentWidth) / 2;
    }

    int16_t GUI::AutoY(int32_t contentHeight) const
    {
        int16_t sb = statusBarHeight();
        int16_t top = (_flags.statusBarEnabled && _statusBarPos == Top) ? sb : 0;
        int16_t availH = _screenHeight - ((_flags.statusBarEnabled && (_statusBarPos == Top || _statusBarPos == Bottom)) ? sb : 0);

        if (contentHeight > availH)
            availH = (int16_t)contentHeight;

        return top + (availH - (int16_t)contentHeight) / 2;
    }

    void GUI::fillRectImpl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color)
    {
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        // For axis-aligned rectangles we intentionally disable FRC/dithering
        // to keep edges and fills perfectly clean (no \"smoothing\" noise).
        fillRect888(x, y, w, h, r, g, b, FrcProfile::Off);
    }

    void GUI::fillRectAlphaImpl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color, uint8_t alpha)
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
            fillRectImpl(x, y, w, h, color);
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

    void GUI::fillRectGradientVerticalImpl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t topColor, uint32_t bottomColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;

        // Gradients are one of the main places where FRC/dithering is beneficial.
        FrcProfile prevProfile = _frcProfile;
        _frcProfile = FrcProfile::BlueNoise;

        auto spr = getDrawTarget();
        if (!spr)
        {
            _frcProfile = prevProfile;
            return;
        }

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
        {
            _frcProfile = prevProfile;
            return;
        }

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
        {
            _frcProfile = prevProfile;
            return;
        }

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

        _frcProfile = prevProfile;
    }

    void GUI::fillRectGradientHorizontalImpl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t leftColor, uint32_t rightColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;

        FrcProfile prevProfile = _frcProfile;
        _frcProfile = FrcProfile::BlueNoise;

        auto spr = getDrawTarget();
        if (!spr)
        {
            _frcProfile = prevProfile;
            return;
        }

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
        {
            _frcProfile = prevProfile;
            return;
        }

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
        {
            _frcProfile = prevProfile;
            return;
        }

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

        _frcProfile = prevProfile;
    }

    void GUI::fillRectGradient4Impl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;

        FrcProfile prevProfile = _frcProfile;
        _frcProfile = FrcProfile::BlueNoise;

        auto spr = getDrawTarget();
        if (!spr)
        {
            _frcProfile = prevProfile;
            return;
        }

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
        {
            _frcProfile = prevProfile;
            return;
        }

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
        {
            _frcProfile = prevProfile;
            return;
        }

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

        _frcProfile = prevProfile;
    }

    void GUI::fillRectGradientDiagonalImpl(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tlColor, uint32_t brColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;

        FrcProfile prevProfile = _frcProfile;
        _frcProfile = FrcProfile::BlueNoise;

        auto spr = getDrawTarget();
        if (!spr)
        {
            _frcProfile = prevProfile;
            return;
        }

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
        {
            _frcProfile = prevProfile;
            return;
        }

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
        {
            _frcProfile = prevProfile;
            return;
        }

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

        _frcProfile = prevProfile;
    }

    void GUI::fillRectGradientRadialImpl(int16_t x, int16_t y, int16_t w, int16_t h, int16_t cx, int16_t cy, int16_t radius, uint32_t innerColor, uint32_t outerColor)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (w <= 0 || h <= 0)
            return;
        if (!_flags.spriteEnabled)
            return;

        FrcProfile prevProfile = _frcProfile;
        _frcProfile = FrcProfile::BlueNoise;

        auto spr = getDrawTarget();
        if (!spr)
        {
            _frcProfile = prevProfile;
            return;
        }

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
        {
            _frcProfile = prevProfile;
            return;
        }

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
        {
            _frcProfile = prevProfile;
            return;
        }

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

        _frcProfile = prevProfile;
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

        // Record original rect for debug overlay
        if (Debug::dirtyRectEnabled())
        {
            Debug::recordDirtyRect(x, y, w, h);
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

        auto intersects = [](const DirtyRect &a, int16_t bx, int16_t by, int16_t bw, int16_t bh) -> bool
        {
            int16_t ax2 = (int16_t)(a.x + a.w);
            int16_t ay2 = (int16_t)(a.y + a.h);
            int16_t bx2 = (int16_t)(bx + bw);
            int16_t by2 = (int16_t)(by + bh);
            // Use half-open rects [x, x+w) to avoid merging rects that only touch edges.
            return !(bx2 <= a.x || bx >= ax2 || by2 <= a.y || by >= ay2);
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
            if (intersects(_dirty[i], x, y, w, h))
            {
                mergeInto(_dirty[i], x, y, w, h);
                for (uint8_t j = 0; j < _dirtyCount; ++j)
                {
                    if (j == i)
                        continue;
                    if (intersects(_dirty[i], _dirty[j].x, _dirty[j].y, _dirty[j].w, _dirty[j].h))
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

        // Mark render start for CPU measurement
        Debug::beginRender();

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

            // Draw overlay if enabled (modifies buffer in-place)
            if (Debug::dirtyRectEnabled())
            {
                Debug::drawOverlay(buf, stride, sw, sh, x0, y0, w, h);
            }

            // Always write to display
            _sprite.writeToDisplay(*_display, x0, y0, w, h);
        }

        _dirtyCount = 0; // Reset dirty count after flush
        
        // Mark render end for CPU measurement
        Debug::endRender();
        
        // Clear debug rects after display update for next frame
        Debug::clearRects();
    }

    void GUI::tickDebugDirtyOverlay(uint32_t now)
    {
        // Logic moved to Debug class - called automatically during flushDirty
        // This method is kept for API compatibility but does nothing now
        (void)now;
    }

}

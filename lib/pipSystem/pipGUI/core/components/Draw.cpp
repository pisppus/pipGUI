#include <pipGUI/core/api/pipGUI.h>

namespace pipgui
{

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
        return detail::quantize565(c, x, y, _frcSeed, _frcProfile);
    }

    void GUI::fillCircleFrc(int16_t cx, int16_t cy, int16_t r, uint32_t color888)
    {
        if (r <= 0)
            return;

        auto t = getDrawTarget();
        if (!t)
            return;

        int16_t x0 = (int16_t)(cx - r);
        int16_t y0 = (int16_t)(cy - r);
        int16_t x1 = (int16_t)(cx + r);
        int16_t y1 = (int16_t)(cy + r);

        int32_t rr = (int32_t)r * (int32_t)r;
        for (int16_t y = y0; y <= y1; ++y)
        {
            int32_t dy = (int32_t)y - (int32_t)cy;
            int32_t dy2 = dy * dy;
            for (int16_t x = x0; x <= x1; ++x)
            {
                int32_t dx = (int32_t)x - (int32_t)cx;
                if (dx * dx + dy2 <= rr)
                    t->drawPixel(x, y, color888To565(x, y, color888));
            }
        }
    }

    void GUI::fillRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888)
    {
        if (w <= 0 || h <= 0)
            return;

        auto t = getDrawTarget();
        if (!t)
            return;

        int16_t r = (int16_t)radius;
        int16_t maxR = (w < h ? w : h) / 2;
        if (r > maxR)
            r = maxR;
        if (r < 0)
            r = 0;

        int16_t x0 = x;
        int16_t y0 = y;
        int16_t x1 = (int16_t)(x + w - 1);
        int16_t y1 = (int16_t)(y + h - 1);

        int16_t cxL = (int16_t)(x0 + r);
        int16_t cxR = (int16_t)(x1 - r);
        int16_t cyT = (int16_t)(y0 + r);
        int16_t cyB = (int16_t)(y1 - r);

        int32_t rr = (int32_t)r * (int32_t)r;

        for (int16_t py = y0; py <= y1; ++py)
        {
            for (int16_t px = x0; px <= x1; ++px)
            {
                bool inside = false;

                if (r == 0)
                {
                    inside = true;
                }
                else if (px >= cxL && px <= cxR)
                {
                    inside = true;
                }
                else if (py >= cyT && py <= cyB)
                {
                    inside = true;
                }
                else
                {
                    int16_t ccx = (px < cxL) ? cxL : cxR;
                    int16_t ccy = (py < cyT) ? cyT : cyB;
                    int32_t dx = (int32_t)px - (int32_t)ccx;
                    int32_t dy = (int32_t)py - (int32_t)ccy;
                    inside = (dx * dx + dy * dy) <= rr;
                }

                if (inside)
                    t->drawPixel(px, py, color888To565(px, py, color888));
            }
        }
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
        {
            if (_tft)
            {
                if (profile == FrcProfile::Off)
                {
                    uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
                    _tft->fillScreen(c565);
                }
                else
                {
                    uint16_t tile[16 * 16];
                    Color888 c{r, g, b};
                    for (int16_t ty = 0; ty < 16; ++ty)
                    {
                        for (int16_t tx = 0; tx < 16; ++tx)
                            tile[(uint16_t)ty * 16U + (uint16_t)tx] = detail::quantize565(c, tx, ty, _frcSeed, profile);
                    }

                    for (int16_t yy = 0; yy < (int16_t)_screenHeight; ++yy)
                    {
                        const uint16_t *tileRow = &tile[((uint16_t)yy & 15U) * 16U];
                        for (int16_t xx = 0; xx < (int16_t)_screenWidth; ++xx)
                        {
                            _tft->drawPixel(xx, yy, tileRow[(uint16_t)xx & 15U]);
                        }
                    }
                }
            }
            return;
        }

        lgfx::LGFX_Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                     ? (_activeSprite ? _activeSprite : &_sprite)
                                     : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t w = spr->width();
        int16_t h = spr->height();
        if (w <= 0 || h <= 0)
            return;

        if (profile == FrcProfile::Off)
        {
            uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
            uint16_t v = swap16(c565);
            for (int16_t yy = 0; yy < h; ++yy)
            {
                int32_t row = (int32_t)yy * (int32_t)w;
                for (int16_t xx = 0; xx < w; ++xx)
                    buf[row + xx] = v;
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
                int32_t row = (int32_t)yy * (int32_t)w;
                const uint16_t *tileRow = &tile[((uint16_t)yy & 15U) * 16U];
                for (int16_t xx = 0; xx < w; ++xx)
                    buf[row + xx] = tileRow[(uint16_t)xx & 15U];
            }
        }

        if (_tft && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(0, 0, (int16_t)_screenWidth, (int16_t)_screenHeight);
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
        {
            if (_tft)
            {
                if (profile == FrcProfile::Off)
                {
                    uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
                    _tft->fillRect(x, y, w, h, c565);
                }
                else
                {
                    Color888 c{r, g, b};
                    for (int16_t yy = 0; yy < h; ++yy)
                    {
                        int16_t py = (int16_t)(y + yy);
                        for (int16_t xx = 0; xx < w; ++xx)
                        {
                            int16_t px = (int16_t)(x + xx);
                            uint16_t c565 = detail::quantize565(c, px, py, _frcSeed, profile);
                            _tft->drawPixel(px, py, c565);
                        }
                    }
                }
            }
            return;
        }

        lgfx::LGFX_Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                     ? (_activeSprite ? _activeSprite : &_sprite)
                                     : &_sprite;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        int16_t stride = spr->width();
        int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
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

        if (w <= 0 || h <= 0)
            return;

        if (profile == FrcProfile::Off)
        {
            uint16_t c565 = (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
            uint16_t v = swap16(c565);
            for (int16_t yy = 0; yy < h; ++yy)
            {
                int16_t py = (int16_t)(y + yy);
                int32_t row = (int32_t)py * (int32_t)stride;
                for (int16_t xx = 0; xx < w; ++xx)
                    buf[row + (int32_t)x + xx] = v;
            }
        }
        else
        {
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
        }

        if (_tft && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect(x, y, w, h);
    }

    LovyanGFX *GUI::getDrawTarget()
    {
        if (_flags.renderToSprite && _flags.spriteEnabled)
            return _activeSprite ? _activeSprite : &_sprite;
        return _tft;
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

    void GUI::drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg, uint16_t bg)
    {
        auto t = getDrawTarget();

        t->fillScreen(bg);

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
        psdfDrawTextInternal(title, cx, topY, fg, bg, AlignCenter);

        if (hasSub)
        {
            int16_t subY = (int16_t)(topY + titleH + spacing);
            setPSDFFontSize(subPx);
            psdfDrawTextInternal(subtitle, cx, subY, fg, bg, AlignCenter);
        }
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_tft || !_flags.spriteEnabled)
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
        if (!_tft || !_flags.spriteEnabled)
        {
            _dirtyCount = 0;
            return;
        }

        uint32_t now = nowMs();

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _tft->getClipRect(&clipX, &clipY, &clipW, &clipH);

        bool debugRectsSame = false;
        if (_debugShowDirtyRects)
        {
            uint8_t cap = (uint8_t)(sizeof(_debugRects) / sizeof(_debugRects[0]));
            uint8_t newCount = (_dirtyCount < cap) ? _dirtyCount : cap;
            if (_debugRectCount == newCount)
            {
                debugRectsSame = true;
                for (uint8_t i = 0; i < newCount; ++i)
                {
                    if (_debugRects[i].x != _dirty[i].x ||
                        _debugRects[i].y != _dirty[i].y ||
                        _debugRects[i].w != _dirty[i].w ||
                        _debugRects[i].h != _dirty[i].h)
                    {
                        debugRectsSame = false;
                        break;
                    }
                }
            }
        }

        if (_debugShowDirtyRects && _debugRectCount > 0 && _debugOverlayDrawnState != 0 && !debugRectsSame)
        {
            for (uint8_t i = 0; i < _debugRectCount; ++i)
            {
                DirtyRect r = _debugRects[i];
                if (r.w <= 0 || r.h <= 0)
                    continue;

                r.x = (int16_t)(r.x - 2);
                r.y = (int16_t)(r.y - 2);
                r.w = (int16_t)(r.w + 4);
                r.h = (int16_t)(r.h + 4);

                if (r.x < 0)
                {
                    r.w += r.x;
                    r.x = 0;
                }
                if (r.y < 0)
                {
                    r.h += r.y;
                    r.y = 0;
                }
                if (r.w <= 0 || r.h <= 0)
                    continue;

                if (r.x + r.w > (int16_t)_screenWidth)
                    r.w = (int16_t)_screenWidth - r.x;
                if (r.y + r.h > (int16_t)_screenHeight)
                    r.h = (int16_t)_screenHeight - r.y;
                if (r.w <= 0 || r.h <= 0)
                    continue;

                _tft->setClipRect(r.x, r.y, r.w, r.h);
                _sprite.pushSprite(_tft, 0, 0);
            }

            _tft->setClipRect(clipX, clipY, clipW, clipH);
        }

        if (_debugShowDirtyRects)
        {
            uint8_t cap = (uint8_t)(sizeof(_debugRects) / sizeof(_debugRects[0]));
            _debugRectCount = (_dirtyCount < cap) ? _dirtyCount : cap;
            for (uint8_t i = 0; i < _debugRectCount; ++i)
                _debugRects[i] = _dirty[i];

            _debugHighlightUntilMs = now + 160;
            _debugOverlayDrawnState = 2;
            _debugLastFlushMs = now;
        }

        for (uint8_t i = 0; i < _dirtyCount; ++i)
        {
            DirtyRect r = _dirty[i];
            if (r.w <= 0 || r.h <= 0)
                continue;

            _tft->setClipRect(r.x, r.y, r.w, r.h);
            _sprite.pushSprite(_tft, 0, 0);

            if (_debugShowDirtyRects)
            {
                _tft->setClipRect(clipX, clipY, clipW, clipH);
                _tft->drawRect(r.x, r.y, r.w, r.h, _debugDirtyRectActiveColor);
            }
        }

        _tft->setClipRect(clipX, clipY, clipW, clipH);

        _dirtyCount = 0;
    }

    void GUI::tickDebugDirtyOverlay(uint32_t now)
    {
        if (!_debugShowDirtyRects || !_tft)
            return;
        if (_debugRectCount == 0)
            return;
        if (_flags.bootActive || _flags.errorActive)
            return;

        uint8_t desired = (now < _debugHighlightUntilMs) ? 2 : 1;
        if (desired == _debugOverlayDrawnState)
            return;

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _tft->getClipRect(&clipX, &clipY, &clipW, &clipH);
        _tft->setClipRect(clipX, clipY, clipW, clipH);

        uint16_t color = (desired == 2) ? _debugDirtyRectActiveColor : _debugDirtyRectColor;
        for (uint8_t i = 0; i < _debugRectCount; ++i)
        {
            DirtyRect r = _debugRects[i];
            if (r.w <= 0 || r.h <= 0)
                continue;
            _tft->drawRect(r.x, r.y, r.w, r.h, color);
        }

        _tft->setClipRect(clipX, clipY, clipW, clipH);
        _debugOverlayDrawnState = desired;
    }

}

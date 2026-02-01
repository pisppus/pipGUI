#include <pipGUI/core/api/pipGUI.h>

namespace pipgui
{

    static inline uint16_t swap16(uint16_t v)
    {
        return (uint16_t)((v >> 8) | (v << 8));
    }

    uint16_t GUI::screenWidth() const { return _screenWidth; }
    uint16_t GUI::screenHeight() const { return _screenHeight; }

    uint16_t GUI::rgb(uint8_t r, uint8_t g, uint8_t b) const
    {
        if (_tft)
            return _tft->color565(r, g, b);
        return (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | ((uint16_t)(b & 0xF8) >> 3));
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
                _tft->fillScreen(_bgColor);
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
                _tft->fillRect(x, y, w, h, rgb(r, g, b));
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

    void GUI::clear(uint16_t color)
    {
        _bgColor = color;
        lgfx::LGFX_Sprite *spr = _activeSprite;
        if (_flags.renderToSprite && _flags.spriteEnabled && spr)
            spr->fillSprite(color);
        else if (_tft)
            _tft->fillScreen(color);
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

    void GUI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (x == -1)
            x = AutoX(w);
        if (y == -1)
            y = AutoY(h);

        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;
            getDrawTarget()->fillRect(x, y, w, h, color);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;

            invalidateRect(x, y, w, h);
            return;
        }

        getDrawTarget()->fillRect(x, y, w, h, color);
    }

    void GUI::drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg, uint16_t bg)
    {
        auto t = getDrawTarget();

        t->fillScreen(bg);

        if (!_flags.ttfLoaded)
            return;

        _ttf.setDrawer(*t);
        _ttf.setFontColor(fg, bg);

        uint16_t titlePx = _logoTitleTTFSize ? _logoTitleTTFSize : _textStyleH1Px;
        uint16_t subPx = _logoSubtitleTTFSize ? _logoSubtitleTTFSize : (uint16_t)((titlePx * 3U) / 4U);

        _ttf.setFontSize(titlePx);
        int16_t titleW = (int16_t)_ttf.getTextWidth("%s", title.c_str());
        int16_t titleH = (int16_t)_ttf.getTextHeight("%s", title.c_str());

        bool hasSub = subtitle.length() > 0;
        int16_t subW = 0, subH = 0;
        if (hasSub)
        {
            _ttf.setFontSize(subPx);
            subW = (int16_t)_ttf.getTextWidth("%s", subtitle.c_str());
            subH = (int16_t)_ttf.getTextHeight("%s", subtitle.c_str());
        }

        int16_t spacing = 4;
        int16_t totalH = hasSub ? (titleH + spacing + subH) : titleH;
        int16_t topY = (_bootY != -1) ? _bootY : (_screenHeight - totalH) / 2;
        int16_t cx = (_bootX != -1) ? _bootX : _screenWidth / 2;

        _ttf.setFontSize(titlePx);
        _ttf.drawString(title.c_str(), cx - titleW / 2, topY, fg, bg, Layout::Horizontal);

        if (hasSub)
        {
            int16_t subY = topY + titleH + spacing;
            _ttf.setFontSize(subPx);
            _ttf.drawString(subtitle.c_str(), cx - subW / 2, subY, fg, bg, Layout::Horizontal);
        }
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
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

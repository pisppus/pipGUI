#include <pipGUI/core/api/pipGUI.h>
#include <math.h>
#include <algorithm>
#include <cstdint>

namespace pipgui
{
    namespace
    {
        float fastEase(float t) { return (t < 0.5f) ? 2 * t * t : 2 * (t - 0.5f) * (0.5f - (t - 0.5f)) + 0.5f; }

        void drawScaledSprite(lgfx::LGFX_Sprite &dest, lgfx::LGFX_Sprite &src, int w, int h, int x, int y)
        {
            if (w <= 0 || h <= 0)
                return;
            uint16_t *srcBuf = (uint16_t *)src.getBuffer();
            uint16_t *dstBuf = (uint16_t *)dest.getBuffer();
            if (!srcBuf || !dstBuf)
                return;

            if (((uintptr_t)srcBuf & 1) || ((uintptr_t)dstBuf & 1))
                return;

            int srcW = src.width(), srcH = src.height();
            int dstW = dest.width(), dstH = dest.height();
            if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
                return;

            uint32_t xr = (srcW << 16) / w, yr = (srcH << 16) / h;

            int x_start = std::max(0, x), y_start = std::max(0, y);
            int x_end = std::min((int)dstW, x + w), y_end = std::min((int)dstH, y + h);

            uint32_t srcY_acc = (y < 0) ? (uint32_t)(-y) * yr : 0;

            for (int iy = y_start; iy < y_end; ++iy)
            {
                int srcRow = std::min(srcH - 1, (int)(srcY_acc >> 16));
                uint32_t srcRowOffset = (uint32_t)srcRow * (uint32_t)srcW;
                uint32_t dstRowOffset = (uint32_t)iy * (uint32_t)dstW;
                uint32_t srcX_acc = (x < 0) ? (uint32_t)(-x) * xr : 0;

                for (int ix = x_start; ix < x_end; ++ix)
                {
                    int srcCol = (int)(srcX_acc >> 16);
                    if (srcCol < 0)
                        srcCol = 0;
                    else if (srcCol >= srcW)
                        srcCol = srcW - 1;
                    dstBuf[dstRowOffset + ix] = srcBuf[srcRowOffset + srcCol];
                    srcX_acc += xr;
                }
                srcY_acc += yr;
            }
        }
    }

    void GUI::regScreen(uint8_t id, ScreenCallback cb)
    {
        ensureScreenStorage(id);
        if (id < _screenCapacity && _screens)
            _screens[id] = cb;
    }

    void GUI::setScreen(uint8_t id)
    {
        ensureScreenStorage(id);
        if (id < _screenCapacity)
            _currentScreen = id;
        else
            _currentScreen = 255;
        _flags.needRedraw = 1;
    }

    uint8_t GUI::currentScreen() const { return _currentScreen; }

    void GUI::requestRedraw() { _flags.needRedraw = 1; }

    void GUI::nextScreen()
    {
        if (_flags.screenTransition)
            return;
        if (_screenCapacity == 0 || !_screens)
            return;
        uint16_t cap = _screenCapacity;
        uint16_t idx = (_currentScreen < cap) ? _currentScreen : 0;
        for (uint16_t i = 0; i < cap; i++)
        {
            idx = (idx + 1) % cap;
            if (_screens[idx])
            {
                if (_screenAnim == Slide && _flags.spriteEnabled && ensureScreenTransitionSprites())
                {
                    _screenFrom = _currentScreen;
                    _screenTo = (uint8_t)idx;
                    _screenTransDir = 1;
                    _screenAnimStartMs = nowMs();

                    ScreenCallback fromCb = _screens[_screenFrom];
                    ScreenCallback toCb = _screens[_screenTo];

                    prepareScreenTransition(fromCb, toCb);

                    _flags.screenTransition = 1;
                    return;
                }
                _currentScreen = (uint8_t)idx;
                _flags.needRedraw = 1;
                return;
            }
        }
    }

    void GUI::prevScreen()
    {
        if (_flags.screenTransition)
            return;
        if (_screenCapacity == 0 || !_screens)
            return;
        uint16_t cap = _screenCapacity;
        uint16_t idx = (_currentScreen < cap) ? _currentScreen : 0;
        for (uint16_t i = 0; i < cap; i++)
        {
            idx = (idx + cap - 1) % cap;
            if (_screens[idx])
            {
                if (_screenAnim == Slide && _flags.spriteEnabled && ensureScreenTransitionSprites())
                {
                    _screenFrom = _currentScreen;
                    _screenTo = (uint8_t)idx;
                    _screenTransDir = -1;
                    _screenAnimStartMs = nowMs();

                    ScreenCallback fromCb = _screens[_screenFrom];
                    ScreenCallback toCb = _screens[_screenTo];

                    prepareScreenTransition(fromCb, toCb);

                    _flags.screenTransition = 1;
                    return;
                }
                _currentScreen = (uint8_t)idx;
                _flags.needRedraw = 1;
                return;
            }
        }
    }

    void GUI::prepareScreenTransition(ScreenCallback fromCb, ScreenCallback toCb)
    {
        lgfx::LGFX_Sprite *prevActive = _activeSprite;
        bool prevRender = _flags.renderToSprite;

        _activeSprite = &_sprite;
        _flags.renderToSprite = 1;

        _sprite.fillScreen(_bgColor);
        if (fromCb)
            fromCb(*this);

        int16_t fw = _screenFromSprite.width();
        int16_t fh = _screenFromSprite.height();
        _screenFromSprite.fillScreen(_bgColor);
        drawScaledSprite(_screenFromSprite, _sprite, fw, fh, 0, 0);

        _sprite.fillScreen(_bgColor);
        if (toCb)
            toCb(*this);

        int16_t tw = _screenToSprite.width();
        int16_t th = _screenToSprite.height();
        _screenToSprite.fillScreen(_bgColor);
        drawScaledSprite(_screenToSprite, _sprite, tw, th, 0, 0);

        _activeSprite = prevActive;
        _flags.renderToSprite = prevRender;
    }

    bool GUI::ensureScreenTransitionSprites()
    {
        if (!_flags.spriteEnabled || !_tft)
            return false;

        if (_screenFromSprite.getBuffer() && _screenToSprite.getBuffer())
        {
            _flags.screenSpritesEnabled = 1;
            return true;
        }

        _screenFromSprite.deleteSprite();
        _screenToSprite.deleteSprite();

        bool fromOk = _screenFromSprite.createSprite(_screenWidth, _screenHeight);
        bool toOk = fromOk && _screenToSprite.createSprite(_screenWidth, _screenHeight);

        if (!(fromOk && toOk))
        {
            _screenFromSprite.deleteSprite();
            _screenToSprite.deleteSprite();

            int16_t halfW = _screenWidth / 2;
            int16_t halfH = _screenHeight / 2;
            if (halfW > 0 && halfH > 0)
            {
                fromOk = _screenFromSprite.createSprite(halfW, halfH);
                toOk = fromOk && _screenToSprite.createSprite(halfW, halfH);

                if (!(fromOk && toOk))
                {
                    _screenFromSprite.deleteSprite();
                    _screenToSprite.deleteSprite();
                }
            }
        }

        _flags.screenSpritesEnabled = fromOk && toOk;
        return _flags.screenSpritesEnabled;
    }

    void GUI::freeScreenTransitionSprites()
    {
        _screenFromSprite.deleteSprite();
        _screenToSprite.deleteSprite();
        _flags.screenSpritesEnabled = 0;
    }

    void GUI::renderScreenTransition(uint32_t now)
    {
        if (!_flags.screenTransition)
            return;
        if (_screenTo >= _screenCapacity || !_flags.spriteEnabled || !_screens)
        {
            _currentScreen = _screenTo;
            _flags.needRedraw = 1;
            _flags.screenTransition = 0;
            freeScreenTransitionSprites();
            return;
        }

        uint32_t el = now - _screenAnimStartMs, dur = _screenAnimDurationMs ? _screenAnimDurationMs : 1;
        if (el > dur)
            el = dur;
        float p = fastEase((float)el / dur);

        _sprite.fillScreen(_bgColor);
        int dir = (_screenTransDir > 0) ? -1 : 1;
        float scale = 1.0f - p * 0.15f;
        int w = round(_screenWidth * scale), h = round(_screenHeight * scale);
        int offY = round(p * 45.0f * dir);

        drawScaledSprite(_sprite, _screenFromSprite, w, h, (_screenWidth - w) / 2, offY);

        int slideY = round(p * _screenHeight);
        int destY = (dir == -1) ? (_screenHeight - slideY) : (slideY - _screenHeight);
        _screenToSprite.pushSprite(&_sprite, 0, destY);
        if (_tft)
            _sprite.pushSprite(_tft, 0, 0);

        if (el >= dur)
        {
            _flags.screenTransition = 0;
            _currentScreen = _screenTo;
            _flags.needRedraw = 0;
            freeScreenTransitionSprites();
        }
    }

    void GUI::loop()
    {
        uint32_t now = nowMs();

        tickRecovery(now);
        tickDebugDirtyOverlay(now);

        if (_flags.bootActive)
        {
            renderBootFrame(now);
            return;
        }
        if (_flags.errorActive)
        {
            renderErrorFrame(now);
            if (_flags.notifActive)
                renderNotificationOverlay();
            return;
        }
        if (_flags.screenTransition)
        {
            renderScreenTransition(now);
            if (_flags.notifActive)
                renderNotificationOverlay();

            tickDebugDirtyOverlay(now);
            return;
        }

        if (_flags.notifActive && _flags.spriteEnabled)
        {
            if (_currentScreen < _screenCapacity && _screens && _screens[_currentScreen])
            {
                _flags.renderToSprite = 1;
                _sprite.fillScreen(_bgColor);
                _screens[_currentScreen](*this);

                _flags.renderToSprite = 0;
                renderStatusBar(true);
            }
            renderNotificationOverlay();

            tickDebugDirtyOverlay(now);
            return;
        }

        if (_flags.needRedraw && _currentScreen < _screenCapacity && _screens && _screens[_currentScreen])
        {
            _flags.needRedraw = 0;
            if (_flags.spriteEnabled)
            {
                if (_tft)
                {
                    ListMenuState *lm = getListMenu(_currentScreen);
                    if (lm && lm->configured && lm->itemCount > 0)
                    {
                        bool more = updateListMenu(_currentScreen);
                        updateStatusBar();
                        if (more)
                            _flags.needRedraw = 1;
                        return;
                    }

                    _flags.renderToSprite = 1;
                    _sprite.fillScreen(_bgColor);
                    _screens[_currentScreen](*this);
                    _flags.renderToSprite = 0;

                    renderStatusBar(true);
                    if (_tft)
                        _sprite.pushSprite(_tft, 0, 0);
                    _dirtyCount = 0;

                    tickDebugDirtyOverlay(now);
                }
                else
                {
                    _screens[_currentScreen](*this);
                    renderStatusBar(false);
                }
            }
            else
            {
                _screens[_currentScreen](*this);
                renderStatusBar(false);
            }
        }

        if (_flags.notifActive && !_flags.spriteEnabled)
        {
            renderNotificationOverlay();
            tickDebugDirtyOverlay(now);
        }
    }

    void GUI::loopWithInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();

        setNotificationButtonDown(prev.isDown());
        setErrorButtonDown(prev.isDown());

        loop();
    }
}
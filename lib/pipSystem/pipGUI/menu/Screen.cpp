#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>
#include <algorithm>
#include <cstdint>

namespace pipgui
{
    namespace
    {
        float fastEase(float t) { return (t < 0.5f) ? 2 * t * t : 2 * (t - 0.5f) * (0.5f - (t - 0.5f)) + 0.5f; }

        void drawScaledSprite(pipcore::Sprite &dest, pipcore::Sprite &src, int w, int h, int x, int y)
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
        if (id < _screen.capacity && _screen.callbacks)
            _screen.callbacks[id] = cb;
    }

    void GUI::setScreen(uint8_t id)
    {
        ensureScreenStorage(id);
        if (id < _screen.capacity)
            _screen.current = id;
        else
            _screen.current = 255;
        _flags.needRedraw = 1;
    }

    uint8_t GUI::currentScreen() const { return _screen.current; }

    void GUI::requestRedraw() { _flags.needRedraw = 1; }

    void GUI::nextScreen()
    {
        if (_flags.screenTransition)
            return;
        if (_screen.capacity == 0 || !_screen.callbacks)
            return;
        uint16_t cap = _screen.capacity;
        uint16_t idx = (_screen.current < cap) ? _screen.current : 0;
        for (uint16_t i = 0; i < cap; i++)
        {
            idx = (idx + 1) % cap;
            if (_screen.callbacks[idx])
            {
                if (_screen.anim == Slide && _flags.spriteEnabled && ensureScreenTransitionSprites())
                {
                    _screen.from = _screen.current;
                    _screen.to = (uint8_t)idx;
                    _screen.transDir = 1;
                    _screen.animStartMs = nowMs();

                    ScreenCallback fromCb = _screen.callbacks[_screen.from];
                    ScreenCallback toCb = _screen.callbacks[_screen.to];

                    prepareScreenTransition(fromCb, toCb);

                    _flags.screenTransition = 1;
                    return;
                }
                _screen.current = (uint8_t)idx;
                _flags.needRedraw = 1;
                return;
            }
        }
    }

    void GUI::prevScreen()
    {
        if (_flags.screenTransition)
            return;
        if (_screen.capacity == 0 || !_screen.callbacks)
            return;
        uint16_t cap = _screen.capacity;
        uint16_t idx = (_screen.current < cap) ? _screen.current : 0;
        for (uint16_t i = 0; i < cap; i++)
        {
            idx = (idx + cap - 1) % cap;
            if (_screen.callbacks[idx])
            {
                if (_screen.anim == Slide && _flags.spriteEnabled && ensureScreenTransitionSprites())
                {
                    _screen.from = _screen.current;
                    _screen.to = (uint8_t)idx;
                    _screen.transDir = -1;
                    _screen.animStartMs = nowMs();

                    ScreenCallback fromCb = _screen.callbacks[_screen.from];
                    ScreenCallback toCb = _screen.callbacks[_screen.to];

                    prepareScreenTransition(fromCb, toCb);

                    _flags.screenTransition = 1;
                    return;
                }
                _screen.current = (uint8_t)idx;
                _flags.needRedraw = 1;
                return;
            }
        }
    }

    void GUI::prepareScreenTransition(ScreenCallback fromCb, ScreenCallback toCb)
    {
        pipcore::Sprite *prevActive = _render.activeSprite;
        bool prevRender = _flags.renderToSprite;

        _render.activeSprite = &_render.sprite;
        _flags.renderToSprite = 1;

        clear(_render.bgColor);
        if (fromCb)
            fromCb(*this);

        int16_t fw = _render.screenFromSprite.width();
        int16_t fh = _render.screenFromSprite.height();
        {
            pipcore::Sprite *prev = _render.activeSprite;
            _render.activeSprite = &_render.screenFromSprite;
            fillRect()
                .at(0, 0)
                .size(fw, fh)
                .color(detail::color888To565(_render.bgColor))
                .draw();
            _render.activeSprite = prev;
        }
        drawScaledSprite(_render.screenFromSprite, _render.sprite, fw, fh, 0, 0);

        clear(_render.bgColor);
        if (toCb)
            toCb(*this);

        int16_t tw = _render.screenToSprite.width();
        int16_t th = _render.screenToSprite.height();
        {
            pipcore::Sprite *prev = _render.activeSprite;
            _render.activeSprite = &_render.screenToSprite;
            fillRect()
                .at(0, 0)
                .size(tw, th)
                .color(detail::color888To565(_render.bgColor))
                .draw();
            _render.activeSprite = prev;
        }
        drawScaledSprite(_render.screenToSprite, _render.sprite, tw, th, 0, 0);

        _render.activeSprite = prevActive;
        _flags.renderToSprite = prevRender;
    }

    bool GUI::ensureScreenTransitionSprites()
    {
        if (!_flags.spriteEnabled || !_disp.display)
            return false;

        if (_render.screenFromSprite.getBuffer() && _render.screenToSprite.getBuffer())
        {
            _flags.screenSpritesEnabled = 1;
            return true;
        }

        _render.screenFromSprite.deleteSprite();
        _render.screenToSprite.deleteSprite();

        bool fromOk = _render.screenFromSprite.createSprite(_render.screenWidth, _render.screenHeight);
        bool toOk = fromOk && _render.screenToSprite.createSprite(_render.screenWidth, _render.screenHeight);

        if (!(fromOk && toOk))
        {
            _render.screenFromSprite.deleteSprite();
            _render.screenToSprite.deleteSprite();

            int16_t halfW = _render.screenWidth / 2;
            int16_t halfH = _render.screenHeight / 2;
            if (halfW > 0 && halfH > 0)
            {
                fromOk = _render.screenFromSprite.createSprite(halfW, halfH);
                toOk = fromOk && _render.screenToSprite.createSprite(halfW, halfH);

                if (!(fromOk && toOk))
                {
                    _render.screenFromSprite.deleteSprite();
                    _render.screenToSprite.deleteSprite();
                }
            }
        }

        _flags.screenSpritesEnabled = fromOk && toOk;
        return _flags.screenSpritesEnabled;
    }

    void GUI::freeScreenTransitionSprites()
    {
        _render.screenFromSprite.deleteSprite();
        _render.screenToSprite.deleteSprite();
        _flags.screenSpritesEnabled = 0;
    }

    void GUI::renderScreenTransition(uint32_t now)
    {
        if (!_flags.screenTransition)
            return;
        if (_screen.to >= _screen.capacity || !_flags.spriteEnabled || !_screen.callbacks)
        {
            _screen.current = _screen.to;
            _flags.needRedraw = 1;
            _flags.screenTransition = 0;
            freeScreenTransitionSprites();
            return;
        }

        uint32_t el = now - _screen.animStartMs, dur = _screen.animDurationMs ? _screen.animDurationMs : 1;
        if (el > dur)
            el = dur;
        float p = fastEase((float)el / dur);

        {
            pipcore::Sprite *prevActive = _render.activeSprite;
            bool prevRender = _flags.renderToSprite;
            _render.activeSprite = &_render.sprite;
            _flags.renderToSprite = 1;
            clear(_render.bgColor);
            _render.activeSprite = prevActive;
            _flags.renderToSprite = prevRender;
        }
        int dir = (_screen.transDir > 0) ? -1 : 1;
        float scale = 1.0f - p * 0.15f;
        int w = round(_render.screenWidth * scale), h = round(_render.screenHeight * scale);
        int offY = round(p * 45.0f * dir);

        drawScaledSprite(_render.sprite, _render.screenFromSprite, w, h, (_render.screenWidth - w) / 2, offY);

        int slideY = round(p * _render.screenHeight);
        int destY = (dir == -1) ? (_render.screenHeight - slideY) : (slideY - _render.screenHeight);
        _render.screenToSprite.pushSprite(&_render.sprite, 0, destY);
        if (_disp.display)
            _render.sprite.writeToDisplay(*_disp.display, 0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);

        if (el >= dur)
        {
            _flags.screenTransition = 0;
            _screen.current = _screen.to;
            _flags.needRedraw = 0;
            freeScreenTransitionSprites();
        }
    }

    void GUI::loop()
    {
        uint32_t now = nowMs();

        // Update debug metrics periodically
        Debug::update();
        tickDebugDirtyOverlay(now);

        if (_flags.bootActive)
        {
            Debug::beginRender();
            renderBootFrame(now);
            Debug::endRender();
            return;
        }
        if (_flags.errorActive)
        {
            Debug::beginRender();
            renderErrorFrame(now);
            if (_flags.notifActive)
                renderNotificationOverlay();
            if (_flags.toastActive)
                renderToastOverlay();
            Debug::endRender();
            return;
        }
        if (_flags.screenTransition)
        {
            Debug::beginRender();
            renderScreenTransition(now);
            if (_flags.notifActive)
                renderNotificationOverlay();
            if (_flags.toastActive)
                renderToastOverlay();
            Debug::endRender();
            return;
        }

        if (_flags.notifActive && _flags.spriteEnabled)
        {
            if (_screen.current < _screen.capacity && _screen.callbacks && _screen.callbacks[_screen.current])
            {
                Debug::beginRender();
                _flags.renderToSprite = 1;
                clear(_render.bgColor);
                _screen.callbacks[_screen.current](*this);

                _flags.renderToSprite = 0;
                renderStatusBar(true);
                Debug::endRender();
            }
            renderNotificationOverlay();
            if (_flags.toastActive)
                renderToastOverlay();
            return;
        }

        if (_flags.needRedraw && _screen.current < _screen.capacity && _screen.callbacks && _screen.callbacks[_screen.current])
        {
            _flags.needRedraw = 0;
            if (_flags.spriteEnabled)
            {
                if (_disp.display)
                {
                    ListMenuState *lm = getListMenu(_screen.current);
                    if (lm && lm->configured && lm->itemCount > 0)
                    {
                        Debug::beginRender();
                        bool more = updateListMenu(_screen.current);
                        updateStatusBar();
                        Debug::endRender();
                        if (more)
                            _flags.needRedraw = 1;
                        return;
                    }

                    Debug::beginRender();
                    _flags.renderToSprite = 1;
                    clear(_render.bgColor);
                    _screen.callbacks[_screen.current](*this);
                    _flags.renderToSprite = 0;

                    renderStatusBar(true);
                    if (_disp.display)
                    {
                        if (_flags.toastActive)
                            renderToastOverlay();
                        else
                            _render.sprite.writeToDisplay(*_disp.display, 0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
                    }
                    Debug::endRender();
                    _dirty.count = 0;
                }
                else
                {
                    _screen.callbacks[_screen.current](*this);
                    renderStatusBar(false);
                }
            }
            else
            {
                _screen.callbacks[_screen.current](*this);
                renderStatusBar(false);
            }
        }

        if (_flags.notifActive && !_flags.spriteEnabled)
            renderNotificationOverlay();
        if (_flags.toastActive)
            _flags.needRedraw = 1;
    }

    void GUI::loopWithInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();
        loop();
    }
}


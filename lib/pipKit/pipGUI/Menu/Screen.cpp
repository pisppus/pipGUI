#include <pipGUI/Core/API/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {

        float cubicBezierPoint(float u, float p1, float p2)
        {
            const float inv = 1.0f - u;
            return 3.0f * inv * inv * u * p1 + 3.0f * inv * u * u * p2 + u * u * u;
        }

        float navigationEase(float t)
        {
            if (t <= 0.0f)
                return 0.0f;
            if (t >= 1.0f)
                return 1.0f;

            float lo = 0.0f;
            float hi = 1.0f;
            float u = t;
            for (uint8_t i = 0; i < 10; ++i)
            {
                const float x = cubicBezierPoint(u, 0.45f, 0.55f);
                if (x < t)
                    lo = u;
                else
                    hi = u;
                u = (lo + hi) * 0.5f;
            }

            return cubicBezierPoint(u, 0.45f, 0.92f);
        }

        uint16_t resolveBgColor565(uint32_t bgColor, uint16_t bgColor565)
        {
            return bgColor565 ? bgColor565 : static_cast<uint16_t>(bgColor);
        }

        bool screenAnimationEnabled(ScreenAnim anim)
        {
            return anim != ScreenAnimNone;
        }

    }

    void GUI::syncRegisteredScreens()
    {
        if (_screen.registrySynced)
            return;

        const detail::ScreenRegistration *reg = detail::ScreenRegistration::head();
        if (!reg)
        {
            _screen.registrySynced = true;
            return;
        }

        uint8_t maxOrder = 0;
        uint8_t orderCounts[256] = {};
        ScreenCallback orderedCallbacks[256] = {};

        for (; reg; reg = reg->next)
        {
            if (!reg->callback)
                continue;
            if (reg->order > maxOrder)
                maxOrder = reg->order;
            if (orderCounts[reg->order] < 255)
                ++orderCounts[reg->order];
            orderedCallbacks[reg->order] = reg->callback;
        }

        ensureScreenState(maxOrder);
        if (!_screen.callbacks)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
            _screen.callbacks[i] = nullptr;

        for (uint16_t i = 0; i <= maxOrder; ++i)
        {
            if (orderCounts[i] == 1)
                _screen.callbacks[i] = orderedCallbacks[i];
        }
        _screen.registrySynced = true;
    }

    void GUI::setScreenId(uint8_t id)
    {
        if (_screen.current != id)
            freeBlurBuffers(platform());

        _flags.screenTransition = 0;

        if (id == INVALID_SCREEN_ID)
        {
            _screen.current = INVALID_SCREEN_ID;
            _flags.needRedraw = 1;
            return;
        }

        ensureScreenState(id);
        if (id < _screen.capacity)
            _screen.current = id;
        else
            _screen.current = INVALID_SCREEN_ID;
        _flags.needRedraw = 1;
    }

    void GUI::setScreen(uint8_t id)
    {
        syncRegisteredScreens();
        if (id == INVALID_SCREEN_ID || id == _screen.current)
        {
            setScreenId(id);
            return;
        }

        const bool canUseOrderedDirection = (_screen.current < _screen.capacity && id < _screen.capacity);
        const int8_t transDir = (canUseOrderedDirection && id < _screen.current) ? -1 : 1;
        activateScreenId(id, transDir);
    }

    uint8_t GUI::currentScreen() const { return _screen.current; }

    bool GUI::screenTransitionActive() const { return _flags.screenTransition; }

    void GUI::requestRedraw() { _flags.needRedraw = 1; }

    void GUI::activateScreenId(uint8_t id, int8_t transDir)
    {
        if (screenAnimationEnabled(_screen.anim) &&
            _flags.spriteEnabled &&
            _disp.display &&
            !_flags.notifActive &&
            _screen.current < _screen.capacity &&
            _screen.callbacks &&
            _screen.callbacks[_screen.current] &&
            id < _screen.capacity &&
            _screen.callbacks[id])
        {
            if (_screen.current != id)
                freeBlurBuffers(platform());
            _screen.to = id;
            _screen.transDir = transDir;
            _screen.animStartMs = nowMs();
            _dirty.count = 0;
            Debug::clearRects();
            _flags.screenTransition = 1;
            return;
        }

        setScreenId(id);
    }

    void GUI::renderScreenToMainSprite(ScreenCallback cb, uint8_t screenId)
    {
        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        const uint8_t prevCurrent = _screen.current;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        if (screenId != INVALID_SCREEN_ID)
            _screen.current = screenId;
        clear(resolveBgColor565(_render.bgColor, _render.bgColor565));
        if (cb)
            cb(*this);

        _screen.current = prevCurrent;
        _render.activeSprite = prevActive;
        _flags.inSpritePass = prevRender;
    }

    bool GUI::presentSpriteRegion(int16_t dstX, int16_t dstY,
                                  int16_t srcX, int16_t srcY,
                                  int16_t w, int16_t h,
                                  const char *stage)
    {
        if (!_disp.display || !_flags.spriteEnabled || w <= 0 || h <= 0)
            return false;

        const auto *buf = static_cast<const uint16_t *>(_render.sprite.getBuffer());
        const int16_t srcW = _render.sprite.width();
        const int16_t srcH = _render.sprite.height();
        if (!buf || srcW <= 0 || srcH <= 0)
            return false;

        if (srcX < 0)
        {
            dstX -= srcX;
            w += srcX;
            srcX = 0;
        }
        if (srcY < 0)
        {
            dstY -= srcY;
            h += srcY;
            srcY = 0;
        }
        if (dstX < 0)
        {
            srcX -= dstX;
            w += dstX;
            dstX = 0;
        }
        if (dstY < 0)
        {
            srcY -= dstY;
            h += dstY;
            dstY = 0;
        }

        if (srcX + w > srcW)
            w = srcW - srcX;
        if (srcY + h > srcH)
            h = srcH - srcY;

        if (w <= 0 || h <= 0)
            return false;

        _disp.display->writeRect565(dstX, dstY, w, h, buf + (size_t)srcY * srcW + srcX, srcW);
        reportPlatformErrorOnce(stage);

        pipcore::Platform *plat = platform();
        return !plat || plat->lastError() == pipcore::PlatformError::None;
    }

    void GUI::nextScreen()
    {
        syncRegisteredScreens();
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
                activateScreenId(static_cast<uint8_t>(idx), 1);
                return;
            }
        }
    }

    void GUI::prevScreen()
    {
        syncRegisteredScreens();
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
                activateScreenId(static_cast<uint8_t>(idx), -1);
                return;
            }
        }
    }

    void GUI::renderScreenTransition(uint32_t now)
    {
        if (!_flags.screenTransition)
            return;
        if (_screen.to >= _screen.capacity || !_flags.spriteEnabled || !_disp.display || !_screen.callbacks || !_screen.callbacks[_screen.to])
        {
            _screen.current = _screen.to;
            _flags.needRedraw = 1;
            _flags.screenTransition = 0;
            return;
        }

        uint32_t el = now - _screen.animStartMs;
        uint32_t dur = _screen.animDurationMs ? _screen.animDurationMs : 1;
        if (el > dur)
            el = dur;
        const float p = navigationEase((float)el / dur);
        const bool keepStatusBarStatic = _flags.statusBarEnabled && _status.height > 0;
        int16_t contentX = 0;
        int16_t contentY = 0;
        int16_t contentW = (int16_t)_render.screenWidth;
        int16_t contentH = (int16_t)_render.screenHeight;

        if (keepStatusBarStatic)
        {
            const int16_t sbh = (int16_t)std::min<uint16_t>(_status.height, (uint16_t)contentH);
            contentH -= sbh;
            if (_status.pos == Bottom)
                ;
            else
                contentY = sbh;
        }

        const bool horizontal = (_screen.anim == SlideX);
        const int16_t revealPrimary = (int16_t)lroundf((horizontal ? contentW : contentH) * p);

        renderScreenToMainSprite(_screen.callbacks[_screen.to], _screen.to);
        if (!keepStatusBarStatic)
            renderStatusBar();
        _dirty.count = 0;
        Debug::clearRects();

        if (revealPrimary > 0)
        {
            const bool forward = (_screen.transDir >= 0);
            if (horizontal)
            {
                const int16_t dstX = forward ? (int16_t)(contentX + contentW - revealPrimary) : contentX;
                const int16_t srcX = forward ? contentX : (int16_t)(contentX + contentW - revealPrimary);

                presentSpriteRegion(dstX,
                                    contentY,
                                    srcX,
                                    contentY,
                                    revealPrimary,
                                    contentH,
                                    "present");
            }
            else
            {
                const int16_t dstY = forward ? (int16_t)(contentY + contentH - revealPrimary) : contentY;
                const int16_t srcY = forward ? contentY : (int16_t)(contentY + contentH - revealPrimary);

                presentSpriteRegion(contentX,
                                    dstY,
                                    contentX,
                                    srcY,
                                    contentW,
                                    revealPrimary,
                                    "present");
            }
        }

        if (el >= dur)
        {
            _flags.screenTransition = 0;
            _screen.current = _screen.to;
            if (keepStatusBarStatic)
            {
                renderStatusBar();
                const int16_t sbh = (int16_t)std::min<uint16_t>(_status.height, (uint16_t)_render.screenHeight);
                const int16_t sbY = (_status.pos == Bottom) ? (int16_t)(_render.screenHeight - sbh) : 0;
                presentSpriteRegion(0,
                                    sbY,
                                    0,
                                    sbY,
                                    (int16_t)_render.screenWidth,
                                    sbh,
                                    "present");
            }
            _flags.needRedraw = 0;
            _dirty.count = 0;
            Debug::clearRects();
        }
    }

    void GUI::loop()
    {
        uint32_t now = nowMs();

        Debug::update();

        const auto renderOverlays = [&]()
        {
            bool wroteOverlay = false;
            if (_flags.notifActive)
            {
                renderNotificationOverlay();
                wroteOverlay = true;
            }
            if (_flags.toastActive)
            {
                renderToastOverlay();
                wroteOverlay = true;
            }
            if (wroteOverlay && _flags.spriteEnabled && _disp.display)
                presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
        };

        if (_flags.bootActive)
        {
            renderBootFrame(now);
            renderOverlays();
            return;
        }
        if (_flags.errorActive)
        {
            renderErrorFrame(now);
            renderOverlays();
            return;
        }
        if (_flags.screenTransition)
        {
            renderScreenTransition(now);
            renderOverlays();
            return;
        }

        ScreenCallback currentCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                       ? _screen.callbacks[_screen.current]
                                       : nullptr;

        if (_flags.notifActive && _flags.spriteEnabled)
        {
            if (currentCb)
            {
                renderScreenToMainSprite(currentCb, _screen.current);
                renderStatusBar();
            }
            renderOverlays();
            return;
        }

        if (_flags.needRedraw && currentCb)
        {
            _flags.needRedraw = 0;
            if (_flags.spriteEnabled)
            {
                if (_disp.display)
                {
                    ListState *lm = getList(_screen.current);
                    if (lm && lm->configured && lm->itemCount > 0)
                    {
                        updateList(_screen.current);
                        updateStatusBar();
                        if (_flags.toastActive || _flags.notifActive)
                            renderOverlays();
                        return;
                    }

                    renderScreenToMainSprite(currentCb, _screen.current);
                    renderStatusBar();
                    if (_flags.toastActive || _flags.notifActive)
                        renderOverlays();
                    else
                        presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
                    _dirty.count = 0;
                }
                else
                {
                    currentCb(*this);
                    renderStatusBar();
                }
            }
            else
            {
                currentCb(*this);
                renderStatusBar();
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


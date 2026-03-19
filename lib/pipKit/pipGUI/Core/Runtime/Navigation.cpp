#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <pipGUI/Core/Debug.hpp>

namespace pipgui
{
    namespace
    {
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

    uint8_t GUI::currentScreen() const noexcept { return _screen.current; }

    bool GUI::screenTransitionActive() const noexcept { return _flags.screenTransition; }

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
            }
        }
    }
}


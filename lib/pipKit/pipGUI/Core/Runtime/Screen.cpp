#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <cstring>
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

        const bool notifActive = _flags.notifActive;
        const bool toastActive = _flags.toastActive;
        if (!notifActive && !toastActive)
        {
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
            return;
        }

        if (!notifActive && toastActive)
        {
            renderScreenToMainSprite(_screen.callbacks[_screen.to], _screen.to);
            if (!keepStatusBarStatic)
                renderStatusBar();
            _dirty.count = 0;
            Debug::clearRects();

            const auto isEmpty = [](const DirtyRect &r) { return r.w <= 0 || r.h <= 0; };
            const auto rectUnion = [&](DirtyRect a, DirtyRect b) -> DirtyRect
            {
                if (isEmpty(a))
                    return b;
                if (isEmpty(b))
                    return a;
                const int16_t x1 = min(a.x, b.x);
                const int16_t y1 = min(a.y, b.y);
                const int16_t x2 = max<int16_t>(a.x + a.w, b.x + b.w);
                const int16_t y2 = max<int16_t>(a.y + a.h, b.y + b.h);
                return {x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
            };
            const auto rectIntersect = [&](DirtyRect a, DirtyRect b) -> DirtyRect
            {
                const int16_t x1 = max(a.x, b.x);
                const int16_t y1 = max(a.y, b.y);
                const int16_t x2 = min<int16_t>(a.x + a.w, b.x + b.w);
                const int16_t y2 = min<int16_t>(a.y + a.h, b.y + b.h);
                DirtyRect r{x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
                if (isEmpty(r))
                    return {0, 0, 0, 0};
                return r;
            };
            const auto rectClipToScreen = [&](DirtyRect r) -> DirtyRect
            {
                if (isEmpty(r))
                    return {0, 0, 0, 0};
                int16_t x1 = r.x;
                int16_t y1 = r.y;
                int16_t x2 = (int16_t)(r.x + r.w);
                int16_t y2 = (int16_t)(r.y + r.h);
                if (x1 < 0)
                    x1 = 0;
                if (y1 < 0)
                    y1 = 0;
                if (x2 > (int16_t)_render.screenWidth)
                    x2 = (int16_t)_render.screenWidth;
                if (y2 > (int16_t)_render.screenHeight)
                    y2 = (int16_t)_render.screenHeight;
                DirtyRect out{x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
                if (isEmpty(out))
                    return {0, 0, 0, 0};
                return out;
            };

            DirtyRect curToast = {};
            const bool curVisible = computeToastBounds(now, curToast);
            const bool hadPrev = _toast.lastRectValid;
            const DirtyRect prevToast = _toast.lastRect;

            DirtyRect paint = {};
            if (hadPrev && !curVisible)
                paint = prevToast;
            else if (!hadPrev && curVisible)
                paint = curToast;
            else if (hadPrev && curVisible)
                paint = rectUnion(prevToast, curToast);
            paint = rectClipToScreen(paint);

            const auto presentStrip = [&](DirtyRect dstStrip, int16_t src0X, int16_t src0Y, DirtyRect cut)
            {
                if (isEmpty(dstStrip))
                    return;

                cut = rectIntersect(dstStrip, cut);
                if (isEmpty(cut))
                {
                    presentSpriteRegion(dstStrip.x, dstStrip.y, src0X, src0Y, dstStrip.w, dstStrip.h, "present");
                    return;
                }

                auto presentPart = [&](DirtyRect r)
                {
                    if (isEmpty(r))
                        return;
                    const int16_t srcX = (int16_t)(src0X + (r.x - dstStrip.x));
                    const int16_t srcY = (int16_t)(src0Y + (r.y - dstStrip.y));
                    presentSpriteRegion(r.x, r.y, srcX, srcY, r.w, r.h, "present");
                };

                const int16_t stripX2 = (int16_t)(dstStrip.x + dstStrip.w);
                const int16_t stripY2 = (int16_t)(dstStrip.y + dstStrip.h);
                const int16_t cutX2 = (int16_t)(cut.x + cut.w);
                const int16_t cutY2 = (int16_t)(cut.y + cut.h);

                // Top / bottom
                presentPart({dstStrip.x, dstStrip.y, dstStrip.w, (int16_t)(cut.y - dstStrip.y)});
                presentPart({dstStrip.x, cutY2, dstStrip.w, (int16_t)(stripY2 - cutY2)});

                // Middle band: left / right
                presentPart({dstStrip.x, cut.y, (int16_t)(cut.x - dstStrip.x), cut.h});
                presentPart({cutX2, cut.y, (int16_t)(stripX2 - cutX2), cut.h});
            };

            int16_t srcX = 0, srcY = 0, dstX = 0, dstY = 0;
            int16_t revealW = 0, revealH = 0;
            if (revealPrimary > 0)
            {
                const bool forward = (_screen.transDir >= 0);
                if (horizontal)
                {
                    revealW = revealPrimary;
                    revealH = contentH;
                    dstX = forward ? (int16_t)(contentX + contentW - revealW) : contentX;
                    srcX = forward ? contentX : (int16_t)(contentX + contentW - revealW);
                    dstY = contentY;
                    srcY = contentY;

                    const DirtyRect dstStrip{dstX, dstY, revealW, revealH};
                    presentStrip(dstStrip, srcX, srcY, paint);
                }
                else
                {
                    revealW = contentW;
                    revealH = revealPrimary;
                    dstY = forward ? (int16_t)(contentY + contentH - revealH) : contentY;
                    srcY = forward ? contentY : (int16_t)(contentY + contentH - revealH);
                    dstX = contentX;
                    srcX = contentX;

                    const DirtyRect dstStrip{dstX, dstY, revealW, revealH};
                    presentStrip(dstStrip, srcX, srcY, paint);
                }
            }

            if (!isEmpty(paint))
            {
                uint16_t *buf = static_cast<uint16_t *>(_render.sprite.getBuffer());
                const int16_t stride = _render.sprite.width();
                const int16_t sh = _render.sprite.height();

                const DirtyRect dstStrip = (revealPrimary > 0)
                                               ? DirtyRect{dstX, dstY, revealW, revealH}
                                               : DirtyRect{0, 0, 0, 0};
                DirtyRect toDst = rectIntersect(paint, dstStrip);
                const int16_t dx = (int16_t)(dstX - srcX);
                const int16_t dy = (int16_t)(dstY - srcY);

                ClipState prevClip = _clip;

                uint16_t *toPixels = nullptr;
                DirtyRect toSrc = {};
                if (!isEmpty(toDst) && buf && stride > 0 && sh > 0)
                {
                    toSrc = { (int16_t)(toDst.x - dx), (int16_t)(toDst.y - dy), toDst.w, toDst.h };

                    if (toSrc.x < 0)
                    {
                        const int16_t d = (int16_t)-toSrc.x;
                        toSrc.x = 0;
                        toSrc.w = (int16_t)(toSrc.w - d);
                        toDst.x = (int16_t)(toDst.x + d);
                        toDst.w = (int16_t)(toDst.w - d);
                    }
                    if (toSrc.y < 0)
                    {
                        const int16_t d = (int16_t)-toSrc.y;
                        toSrc.y = 0;
                        toSrc.h = (int16_t)(toSrc.h - d);
                        toDst.y = (int16_t)(toDst.y + d);
                        toDst.h = (int16_t)(toDst.h - d);
                    }
                    if (toSrc.x + toSrc.w > stride)
                    {
                        const int16_t over = (int16_t)(toSrc.x + toSrc.w - stride);
                        toSrc.w = (int16_t)(toSrc.w - over);
                        toDst.w = (int16_t)(toDst.w - over);
                    }
                    if (toSrc.y + toSrc.h > sh)
                    {
                        const int16_t over = (int16_t)(toSrc.y + toSrc.h - sh);
                        toSrc.h = (int16_t)(toSrc.h - over);
                        toDst.h = (int16_t)(toDst.h - over);
                    }

                    if (!isEmpty(toDst) && !isEmpty(toSrc))
                    {
                        const uint32_t needPixels = (uint32_t)toDst.w * (uint32_t)toDst.h;
                        if ((!_toast.scratch || _toast.scratchPixels < needPixels) && platform())
                        {
                            pipcore::Platform *plat = platform();
                            if (_toast.scratch)
                                plat->free(_toast.scratch);
                            _toast.scratch = (uint16_t *)plat->alloc(sizeof(uint16_t) * needPixels, pipcore::AllocCaps::Default);
                            _toast.scratchPixels = _toast.scratch ? needPixels : 0;
                        }

                        toPixels = _toast.scratch;
                        if (toPixels && _toast.scratchPixels >= needPixels)
                        {
                            for (int16_t yy = 0; yy < toDst.h; yy++)
                            {
                                memcpy(toPixels + (size_t)yy * toDst.w,
                                       buf + (size_t)(toSrc.y + yy) * stride + toSrc.x,
                                (size_t)toDst.w * sizeof(uint16_t));
                            }
                        }
                        else
                        {
                            toPixels = nullptr;
                        }
                    }
                }

                setClip(paint.x, paint.y, paint.w, paint.h);
                const ScreenCallback fromCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                                  ? _screen.callbacks[_screen.current]
                                                  : nullptr;
                renderScreenToMainSprite(fromCb, _screen.current);
                renderStatusBar();

                if (toPixels && !isEmpty(toDst))
                {
                    for (int16_t yy = 0; yy < toDst.h; yy++)
                    {
                        memcpy(buf + (size_t)(toDst.y + yy) * stride + toDst.x,
                               toPixels + (size_t)yy * toDst.w,
                               (size_t)toDst.w * sizeof(uint16_t));
                    }
                }

                setClip(paint.x, paint.y, paint.w, paint.h);
                if (curVisible)
                    renderToastOverlay(now);

                _clip = prevClip;
                presentSpriteRegion(paint.x, paint.y, paint.x, paint.y, paint.w, paint.h, "present");
            }

            _toast.lastRect = curToast;
            _toast.lastRectValid = curVisible;
            if (!_flags.toastActive && !_toast.lastRectValid && _toast.scratch && platform())
            {
                platform()->free(_toast.scratch);
                _toast.scratch = nullptr;
                _toast.scratchPixels = 0;
            }

            if (el >= dur)
            {
                _flags.screenTransition = 0;
                _screen.current = _screen.to;
                if (_toast.scratch && platform())
                {
                    platform()->free(_toast.scratch);
                    _toast.scratch = nullptr;
                    _toast.scratchPixels = 0;
                }
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
            return;
        }

        auto blitInPlace = [&](int16_t dstX, int16_t dstY,
                               int16_t srcX, int16_t srcY,
                               int16_t w, int16_t h)
        {
            if (w <= 0 || h <= 0)
                return;
            uint16_t *buf = static_cast<uint16_t *>(_render.sprite.getBuffer());
            const int16_t stride = _render.sprite.width();
            const int16_t sh = _render.sprite.height();
            if (!buf || stride <= 0 || sh <= 0)
                return;

            if (srcX < 0 || srcY < 0 || dstX < 0 || dstY < 0)
                return;
            if (srcX + w > stride || dstX + w > stride)
                return;
            if (srcY + h > sh || dstY + h > sh)
                return;

            const bool bottomUp = (dstY > srcY);
            const int yStart = bottomUp ? (h - 1) : 0;
            const int yEnd = bottomUp ? -1 : h;
            const int yStep = bottomUp ? -1 : 1;

            for (int yy = yStart; yy != yEnd; yy += yStep)
            {
                uint16_t *dst = buf + (size_t)(dstY + yy) * stride + dstX;
                const uint16_t *src = buf + (size_t)(srcY + yy) * stride + srcX;
                memmove(dst, src, (size_t)w * sizeof(uint16_t));
            }
        };

        auto renderScreenClipped = [&](ScreenCallback cb, uint8_t screenId,
                                       int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH)
        {
            if (!cb)
                return;

            const bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;
            const uint8_t prevCurrent = _screen.current;
            const ClipState prevClip = _clip;

            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;
            _screen.current = screenId;

            setClip(clipX, clipY, clipW, clipH);
            clear(resolveBgColor565(_render.bgColor, _render.bgColor565));
            cb(*this);

            _clip = prevClip;
            _screen.current = prevCurrent;
            _render.activeSprite = prevActive;
            _flags.inSpritePass = prevRender;
        };

        const ScreenCallback fromCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                          ? _screen.callbacks[_screen.current]
                                          : nullptr;
        const ScreenCallback toCb = _screen.callbacks[_screen.to];

        renderScreenToMainSprite(fromCb, _screen.current);

        if (revealPrimary > 0)
        {
            const bool forward = (_screen.transDir >= 0);
            if (horizontal)
            {
                const int16_t revealW = revealPrimary;
                const int16_t offset = (int16_t)(contentW - revealW);

                const int16_t srcX = forward ? contentX : (int16_t)(contentX + offset);
                const int16_t dstX = forward ? (int16_t)(contentX + offset) : contentX;

                renderScreenClipped(toCb, _screen.to, srcX, contentY, revealW, contentH);
                blitInPlace(dstX, contentY, srcX, contentY, revealW, contentH);

                if (offset > 0)
                {
                    const int16_t restoreX = forward ? contentX : (int16_t)(contentX + revealW);
                    renderScreenClipped(fromCb, _screen.current, restoreX, contentY, offset, contentH);
                }
            }
            else
            {
                const int16_t revealH = revealPrimary;
                const int16_t offset = (int16_t)(contentH - revealH);

                const int16_t srcY = forward ? contentY : (int16_t)(contentY + offset);
                const int16_t dstY = forward ? (int16_t)(contentY + offset) : contentY;

                renderScreenClipped(toCb, _screen.to, contentX, srcY, contentW, revealH);
                blitInPlace(contentX, dstY, contentX, srcY, contentW, revealH);

                if (offset > 0)
                {
                    const int16_t restoreY = forward ? contentY : (int16_t)(contentY + revealH);
                    renderScreenClipped(fromCb, _screen.current, contentX, restoreY, contentW, offset);
                }
            }
        }

        renderStatusBar();
        if (_flags.notifActive)
            renderNotificationOverlay();
        if (_flags.toastActive)
            renderToastOverlay(now);
        presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");

        _dirty.count = 0;
        Debug::clearRects();

        if (el >= dur)
        {
            _flags.screenTransition = 0;
            _screen.current = _screen.to;
            _flags.needRedraw = 0;
            _dirty.count = 0;
            Debug::clearRects();
        }
    }

    void GUI::loop()
    {
        uint32_t now = nowMs();

        Debug::update();

#if PIPGUI_WIFI
        net::wifiService();
#endif

#if PIPGUI_OTA
        otaService();
#endif

#if PIPGUI_SCREENSHOTS
        if (_diag.screenshotNext && _diag.screenshotPrev)
            handleScreenshotShortcut(_diag.screenshotNext->isDown(), _diag.screenshotPrev->isDown());

        serviceScreenshotStream();
#endif

        const auto presentOverlaysFull = [&]()
        {
            bool wroteOverlay = false;
            if (_flags.notifActive)
            {
                renderNotificationOverlay();
                wroteOverlay = true;
            }
            if (_flags.toastActive)
            {
                renderToastOverlay(now);
                wroteOverlay = true;
                DirtyRect r = {};
                const bool vis = computeToastBounds(now, r);
                _toast.lastRect = r;
                _toast.lastRectValid = vis;
            }
            if (wroteOverlay && _flags.spriteEnabled && _disp.display)
                presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
        };

        if (_flags.bootActive)
        {
            renderBootFrame(now);
            presentOverlaysFull();
            return;
        }
        if (_flags.errorActive)
        {
            renderErrorFrame(now);
            presentOverlaysFull();
            return;
        }
        if (_flags.screenTransition)
        {
            renderScreenTransition(now);
            return;
        }

        ScreenCallback currentCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                       ? _screen.callbacks[_screen.current]
                                       : nullptr;

        const auto serviceToast = [&]()
        {
            if (_flags.notifActive)
                return;
            if (!_flags.spriteEnabled || !_disp.display)
                return;

            const auto isEmpty = [](const DirtyRect &r) { return r.w <= 0 || r.h <= 0; };
            const auto rectUnion = [&](DirtyRect a, DirtyRect b) -> DirtyRect
            {
                if (isEmpty(a))
                    return b;
                if (isEmpty(b))
                    return a;
                const int16_t x1 = min(a.x, b.x);
                const int16_t y1 = min(a.y, b.y);
                const int16_t x2 = max<int16_t>(a.x + a.w, b.x + b.w);
                const int16_t y2 = max<int16_t>(a.y + a.h, b.y + b.h);
                return {x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
            };
            const auto rectClipToScreen = [&](DirtyRect r) -> DirtyRect
            {
                if (isEmpty(r))
                    return {0, 0, 0, 0};
                int16_t x1 = r.x;
                int16_t y1 = r.y;
                int16_t x2 = (int16_t)(r.x + r.w);
                int16_t y2 = (int16_t)(r.y + r.h);
                if (x1 < 0)
                    x1 = 0;
                if (y1 < 0)
                    y1 = 0;
                if (x2 > (int16_t)_render.screenWidth)
                    x2 = (int16_t)_render.screenWidth;
                if (y2 > (int16_t)_render.screenHeight)
                    y2 = (int16_t)_render.screenHeight;
                DirtyRect out{x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
                if (isEmpty(out))
                    return {0, 0, 0, 0};
                return out;
            };

            DirtyRect curToast = {};
            const bool curVisible = computeToastBounds(now, curToast);
            const bool hadPrev = _toast.lastRectValid;
            const DirtyRect prevToast = _toast.lastRect;

            DirtyRect paint = {};
            if (hadPrev && !curVisible)
                paint = prevToast;
            else if (!hadPrev && curVisible)
                paint = curToast;
            else if (hadPrev && curVisible)
                paint = rectUnion(prevToast, curToast);

            paint = rectClipToScreen(paint);
            if (isEmpty(paint))
            {
                _toast.lastRect = curToast;
                _toast.lastRectValid = curVisible;
                if (!_flags.toastActive && !_toast.lastRectValid && _toast.scratch && platform())
                {
                    platform()->free(_toast.scratch);
                    _toast.scratch = nullptr;
                    _toast.scratchPixels = 0;
                }
                return;
            }

            const ClipState prevClip = _clip;
            setClip(paint.x, paint.y, paint.w, paint.h);
            renderScreenToMainSprite(currentCb, _screen.current);
            renderStatusBar();
            setClip(paint.x, paint.y, paint.w, paint.h);
            if (curVisible)
                renderToastOverlay(now);
            _clip = prevClip;

            presentSpriteRegion(paint.x, paint.y, paint.x, paint.y, paint.w, paint.h, "present");

            _toast.lastRect = curToast;
            _toast.lastRectValid = curVisible;
            if (!_flags.toastActive && !_toast.lastRectValid && _toast.scratch && platform())
            {
                platform()->free(_toast.scratch);
                _toast.scratch = nullptr;
                _toast.scratchPixels = 0;
            }
        };

        if (_flags.notifActive && _flags.spriteEnabled)
        {
            if (currentCb)
            {
                renderScreenToMainSprite(currentCb, _screen.current);
                renderStatusBar();
            }
            presentOverlaysFull();
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
                        if (_flags.notifActive)
                            presentOverlaysFull();
                        else if (_flags.toastActive || _toast.lastRectValid)
                            serviceToast();
                        return;
                    }

                    renderScreenToMainSprite(currentCb, _screen.current);
                    renderStatusBar();
                    if (_flags.notifActive)
                    {
                        renderNotificationOverlay();
                    }
                    if (_flags.toastActive)
                    {
                        renderToastOverlay(now);
                        DirtyRect r = {};
                        const bool vis = computeToastBounds(now, r);
                        _toast.lastRect = r;
                        _toast.lastRectValid = vis;
                    }
                    presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
                    _dirty.count = 0;
                    return;
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
        if (!_flags.needRedraw && _dirty.count > 0 && _flags.spriteEnabled && _disp.display)
            flushDirty();
        if (_flags.toastActive || _toast.lastRectValid)
            serviceToast();

#if PIPGUI_SCREENSHOTS && (PIPGUI_SCREENSHOT_MODE == 2)
        serviceScreenshotGalleryFlash();
#endif
    }

    void GUI::loopWithInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();
#if PIPGUI_SCREENSHOTS
        handleScreenshotShortcut(next.isDown(), prev.isDown());
#endif
        loop();
    }
}

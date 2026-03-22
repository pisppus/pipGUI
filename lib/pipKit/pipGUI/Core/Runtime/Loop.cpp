#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kIdleBlurCacheMs = 250;
        constexpr uint32_t kIdleShotGalleryCacheMs = 250;
    }

    void GUI::loop()
    {
        uint32_t now = nowMs();

        Debug::update();

#if PIPGUI_OTA
        otaService();
#elif PIPGUI_WIFI
        net::wifiService();
#endif

#if PIPGUI_SCREENSHOTS
        serviceScreenshotStream();
#endif

#if PIPGUI_OTA
        {
            constexpr uint8_t kOtaConfirmOkFrames = 30;
            const OtaStatus &st = otaStatus();
            if (!st.pendingVerify)
            {
                _diag.otaOkFrames = 0;
                _diag.otaAutoConfirmed = false;
            }
            else if (!_diag.otaAutoConfirmed)
            {
                pipcore::Platform *plat = pipcore::GetPlatform();
                const bool ok = (!_flags.bootActive &&
                                 !_flags.errorActive &&
                                 !_flags.screenTransition &&
                                 _disp.display &&
                                 (!plat || plat->lastError() == pipcore::PlatformError::None));

                if (ok)
                {
                    if (_diag.otaOkFrames < 255)
                        ++_diag.otaOkFrames;
                    if (_diag.otaOkFrames >= kOtaConfirmOkFrames)
                    {
                        otaMarkAppValid();
                        _diag.otaOkFrames = 0;
                        _diag.otaAutoConfirmed = !otaStatus().pendingVerify;
                    }
                }
                else
                {
                    _diag.otaOkFrames = 0;
                }
            }
        }
#endif

        const auto presentOverlaysFull = [&]()
        {
            bool wroteOverlay = false;
            if (_flags.notifActive)
            {
                renderNotificationOverlay();
                wroteOverlay = true;
            }
            if (_flags.popupActive)
            {
                renderPopupMenuOverlay(now);
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
            {
                invalidateRect(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
                flushDirty();
            }
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

            const auto isEmpty = [](const DirtyRect &r)
            { return r.w <= 0 || r.h <= 0; };
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
            applyClip(paint.x, paint.y, paint.w, paint.h);
            renderScreenToMainSprite(currentCb, _screen.current);
            renderStatusBar();
            applyClip(paint.x, paint.y, paint.w, paint.h);
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

        if ((_flags.notifActive || _flags.popupActive) && _flags.spriteEnabled)
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
                        if (_flags.notifActive || _flags.popupActive)
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
                    if (_flags.popupActive)
                    {
                        renderPopupMenuOverlay(now);
                    }
                    if (_flags.toastActive)
                    {
                        renderToastOverlay(now);
                        DirtyRect r = {};
                        const bool vis = computeToastBounds(now, r);
                        _toast.lastRect = r;
                        _toast.lastRectValid = vis;
                    }
                    invalidateRect(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
                    flushDirty();
                    return;
                }
                else
                {
                    beginGraphFrame(_screen.current);
                    currentCb(*this);
                    endGraphFrame(_screen.current);
                    renderStatusBar();
                }
            }
            else
            {
                beginGraphFrame(_screen.current);
                currentCb(*this);
                endGraphFrame(_screen.current);
                renderStatusBar();
            }
        }

        if (_flags.notifActive && !_flags.spriteEnabled)
            renderNotificationOverlay();
        if (_flags.popupActive && !_flags.spriteEnabled)
            renderPopupMenuOverlay(now);
        if (!_flags.needRedraw && _dirty.count > 0 && _flags.spriteEnabled && _disp.display)
            flushDirty();
        if (_flags.toastActive || _toast.lastRectValid)
            serviceToast();

#if PIPGUI_SCREENSHOTS && (PIPGUI_SCREENSHOT_MODE == 2)
        const bool galleryHot = (_shots.lastUseMs != 0) && ((now - _shots.lastUseMs) <= kIdleShotGalleryCacheMs);
        if (galleryHot)
        {
            serviceScreenshotGalleryFlash();
        }
        else if (_shots.lastUseMs != 0)
        {
            releaseScreenshotGalleryCache(platform());
        }
#endif

        if (_blur.lastUseMs != 0 && (now - _blur.lastUseMs) > kIdleBlurCacheMs)
        {
            freeBlurBuffers(platform());
            _blur.lastUseMs = 0;
        }
    }

    void GUI::loopWithInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();
#if PIPGUI_SCREENSHOTS
        handleScreenshotShortcut(next.isDown() && prev.isDown() && !_flags.errorActive);
#endif
        loop();
    }
}

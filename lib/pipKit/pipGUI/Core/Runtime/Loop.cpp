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
                presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
                _dirty.count = 0;
                Debug::clearRects();
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

        if (!_flags.needRedraw && _screen.current < _screen.capacity)
            flushPendingGraphRender(_screen.current);
        if (!_flags.needRedraw && statusBarAnimationActive())
            updateStatusBar();

        ScreenCallback currentCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                       ? _screen.callbacks[_screen.current]
                                       : nullptr;
        const auto renderCurrentScreenDirty = [&](ScreenCallback cb, uint8_t screenId)
        {
            if (_dirty.count == 0)
                return;

            const bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;
            const uint8_t prevCurrent = _screen.current;
            const ClipState prevClip = _clip;
            int32_t prevClipX = 0;
            int32_t prevClipY = 0;
            int32_t prevClipW = 0;
            int32_t prevClipH = 0;
            _render.sprite.getClipRect(&prevClipX, &prevClipY, &prevClipW, &prevClipH);

            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;
            _screen.current = screenId;

            beginGraphFrame(screenId);
            for (uint8_t i = 0; i < _dirty.count; ++i)
            {
                const DirtyRect &dirty = _dirty.rects[i];
                if (dirty.w <= 0 || dirty.h <= 0)
                    continue;

                _clip = prevClip;
                applyClip(dirty.x, dirty.y, dirty.w, dirty.h);
                clear(_render.bgColor565 ? _render.bgColor565 : (uint16_t)_render.bgColor);
                if (cb)
                    cb(*this);
            }
            endGraphFrame(screenId);

            _clip = prevClip;
            _render.sprite.setClipRect((int16_t)prevClipX, (int16_t)prevClipY, (int16_t)prevClipW, (int16_t)prevClipH);
            _screen.current = prevCurrent;
            _render.activeSprite = prevActive;
            _flags.inSpritePass = prevRender;
        };

        const auto serviceOverlays = [&](bool forceFullPresent)
        {
            if (_flags.notifActive)
                return false;
            if (!_flags.spriteEnabled || !_disp.display)
                return false;

            const uint32_t popupDur = _popup.animDurationMs ? _popup.animDurationMs : 1;
            const bool popupCloseFinished = _flags.popupClosing && (now - _popup.startMs) >= popupDur;
            if (popupCloseFinished)
            {
                _flags.popupActive = 0;
                _flags.popupClosing = 0;
            }

            DirtyRect curPopup = {};
            const bool curVisible = computePopupBounds(now, curPopup);
            DirtyRect curToast = {};
            const bool curToastVisible = computeToastBounds(now, curToast);
            if (!curVisible && !_popup.lastRectValid && !curToastVisible && !_toast.lastRectValid && !forceFullPresent && _dirty.count == 0)
                return false;

            bool paintSet = forceFullPresent;
            DirtyRect paint = {0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight};
            const auto expandPaint = [&](const DirtyRect &rect)
            {
                if (rect.w <= 0 || rect.h <= 0)
                    return;
                if (!paintSet)
                {
                    paint = rect;
                    paintSet = true;
                    return;
                }
                const int16_t x1 = (paint.x < rect.x) ? paint.x : rect.x;
                const int16_t y1 = (paint.y < rect.y) ? paint.y : rect.y;
                const int16_t x2a = (int16_t)(paint.x + paint.w);
                const int16_t x2b = (int16_t)(rect.x + rect.w);
                const int16_t y2a = (int16_t)(paint.y + paint.h);
                const int16_t y2b = (int16_t)(rect.y + rect.h);
                paint.x = x1;
                paint.y = y1;
                paint.w = ((x2a > x2b) ? x2a : x2b) - x1;
                paint.h = ((y2a > y2b) ? y2a : y2b) - y1;
            };

            for (uint8_t i = 0; i < _dirty.count; ++i)
                expandPaint(_dirty.rects[i]);
            if (curVisible)
                expandPaint(curPopup);
            if (_popup.lastRectValid)
                expandPaint(_popup.lastRect);
            if (curToastVisible)
                expandPaint(curToast);
            if (_toast.lastRectValid)
                expandPaint(_toast.lastRect);

            if (!paintSet)
                return false;

            if (currentCb)
                renderScreenToMainSprite(currentCb, _screen.current);
            else
                clear(_render.bgColor565 ? _render.bgColor565 : (uint16_t)_render.bgColor);
            renderStatusBar();
            if (curVisible)
                renderPopupMenuOverlay(now);
            if (curToastVisible)
                renderToastOverlay(now);

            presentSprite(paint.x, paint.y, paint.w, paint.h, "present");
            _dirty.count = 0;
            Debug::clearRects();

            _popup.lastRect = curPopup;
            _popup.lastRectValid = curVisible;
            _toast.lastRect = curToast;
            _toast.lastRectValid = curToastVisible;
            return true;
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
                    ListState *list = getList(_screen.current);
                    TileState *tile = getTile(_screen.current);
                    const bool currentIsList = list && list->configured && list->itemCount > 0;
                    const bool currentIsTile = tile && tile->configured && tile->itemCount > 0;
                    const bool overlaysActive = _flags.toastActive || _toast.lastRectValid || _flags.notifActive || _flags.popupActive;

                    if (!overlaysActive && currentIsList)
                    {
                        updateListScreen(_screen.current);
                        renderStatusBar();
                        _flags.dirtyRedrawPending = 0;
                        if (_dirty.count > 0)
                            flushDirty();
                        return;
                    }

                    if (!overlaysActive && currentIsTile)
                    {
                        updateTile(_screen.current, tile->selectedIndex);
                        renderStatusBar();
                        _flags.dirtyRedrawPending = 0;
                        if (_dirty.count > 0)
                            flushDirty();
                        return;
                    }

                    if (!overlaysActive && _flags.dirtyRedrawPending && _dirty.count > 0)
                    {
                        renderCurrentScreenDirty(currentCb, _screen.current);
                        renderStatusBar();
                        _flags.dirtyRedrawPending = 0;
                        if (_dirty.count > 0)
                            flushDirty();
                        return;
                    }

                    _flags.dirtyRedrawPending = 0;
                    renderScreenToMainSprite(currentCb, _screen.current);
                    renderStatusBar();

                    if (_flags.popupActive || _popup.lastRectValid || _flags.toastActive || _toast.lastRectValid)
                    {
                        serviceOverlays(true);
                        return;
                    }

                    presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
                    if (!_flags.dirtyRedrawPending || _dirty.count == 0)
                    {
                        _dirty.count = 0;
                        Debug::clearRects();
                    }
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
        if (_flags.popupActive || _popup.lastRectValid || _flags.toastActive || _toast.lastRectValid)
            serviceOverlays(false);
        if (!_flags.needRedraw && _dirty.count > 0 && _flags.spriteEnabled && _disp.display &&
            !_flags.toastActive && !_toast.lastRectValid && !_flags.popupActive && !_popup.lastRectValid)
            flushDirty();

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

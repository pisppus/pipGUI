#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Debug.hpp>
#if PIPGUI_STATUS_BAR
#include <pipGUI/Graphics/Text/Icons/Metrics.hpp>
#include <algorithm>
#endif

namespace pipgui
{

#if PIPGUI_STATUS_BAR

    namespace
    {
        constexpr uint32_t kStatusBarIconAnimMs = 180;
        constexpr int16_t kStatusBarInsetPx = 2;
        constexpr int16_t kStatusBarItemGapPx = 4;
        constexpr IconId kInvalidStatusBarIcon = static_cast<IconId>(0xFFFF);

        struct StatusBarLayout
        {
            detail::DirtyRect leftRect = {};
            detail::DirtyRect centerRect = {};
            detail::DirtyRect rightRect = {};
            detail::DirtyRect batteryRect = {};

            int16_t leftTextX = 0;
            int16_t centerTextX = 0;
            int16_t rightTextX = 0;

            int16_t leftIconX = 0;
            int16_t centerIconX = 0;
            int16_t rightIconX = 0;
            int16_t batteryX = 0;

            int16_t leftIconY = 0;
            int16_t centerIconY = 0;
            int16_t rightIconY = 0;
            int16_t batteryY = 0;

            uint16_t leftIconSize = 0;
            uint16_t centerIconSize = 0;
            uint16_t rightIconSize = 0;
            uint16_t batterySize = 0;
        };

        [[nodiscard]] bool rectEmpty(const detail::DirtyRect &rect) noexcept
        {
            return rect.w <= 0 || rect.h <= 0;
        }

        [[nodiscard]] detail::DirtyRect rectUnion(const detail::DirtyRect &a, const detail::DirtyRect &b) noexcept
        {
            if (rectEmpty(a))
                return b;
            if (rectEmpty(b))
                return a;

            const int16_t x1 = (a.x < b.x) ? a.x : b.x;
            const int16_t y1 = (a.y < b.y) ? a.y : b.y;
            const int16_t x2 = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
            const int16_t y2 = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);
            return {x1, y1, static_cast<int16_t>(x2 - x1), static_cast<int16_t>(y2 - y1)};
        }

        [[nodiscard]] uint16_t resolveStatusBarSlotIconSize(int16_t h, uint16_t requested) noexcept
        {
            uint16_t size = requested ? requested : static_cast<uint16_t>((h > 2) ? (h - 2) : h);
            if (size < 8)
                size = 8;
            return size;
        }

        [[nodiscard]] uint16_t resolveStatusBarBatterySize(int16_t h) noexcept
        {
            uint16_t size = static_cast<uint16_t>(h + ((h >= 18) ? 4 : 2));
            if (size < 10)
                size = 10;
            return size;
        }

        [[nodiscard]] uint8_t smoothAlpha(uint8_t from, uint8_t to, uint32_t elapsedMs) noexcept
        {
            if (elapsedMs >= kStatusBarIconAnimMs)
                return to;
            const float t = static_cast<float>(elapsedMs) / static_cast<float>(kStatusBarIconAnimMs);
            const float tt = t * t * (3.0f - 2.0f * t);
            return static_cast<uint8_t>(from + (to - from) * tt + 0.5f);
        }

        uint8_t stepStatusBarIcon(detail::StatusBarIconState &icon, uint32_t now) noexcept
        {
            if (icon.alpha != icon.alphaTo)
            {
                const uint32_t elapsed = (now >= icon.animStartMs) ? (now - icon.animStartMs) : 0;
                icon.alpha = smoothAlpha(icon.alphaFrom, icon.alphaTo, elapsed);
                if (icon.alpha == 0 && !icon.visible)
                    icon.iconId = kInvalidStatusBarIcon;
            }
            return icon.alpha;
        }

        bool configureStatusBarIcon(detail::StatusBarIconState &icon,
                                    IconId iconId,
                                    uint16_t color565,
                                    uint16_t sizePx,
                                    bool visible,
                                    uint32_t now) noexcept
        {
            stepStatusBarIcon(icon, now);
            if (!visible)
            {
                if (!icon.visible && icon.alpha == 0 && icon.iconId == kInvalidStatusBarIcon)
                    return false;
                icon.visible = false;
                icon.alphaFrom = icon.alpha;
                icon.alphaTo = 0;
                icon.animStartMs = now;
                if (icon.alpha == 0)
                    icon.iconId = kInvalidStatusBarIcon;
                return true;
            }

            if (icon.visible &&
                icon.iconId == iconId &&
                icon.color565 == color565 &&
                icon.sizePx == sizePx &&
                icon.alpha == 255 &&
                icon.alphaTo == 255)
                return false;

            icon.iconId = iconId;
            icon.color565 = color565;
            icon.sizePx = sizePx;
            icon.visible = true;
            icon.alphaFrom = icon.alpha;
            icon.alphaTo = 255;
            icon.animStartMs = now;
            return true;
        }

        [[nodiscard]] detail::DirtyRect makeTextRect(int16_t x, int16_t y, int16_t w, int16_t h) noexcept
        {
            return (w > 0) ? detail::DirtyRect{static_cast<int16_t>(x - 2), static_cast<int16_t>(y - 2), static_cast<int16_t>(w + 4), static_cast<int16_t>(h + 4)} : detail::DirtyRect{};
        }

        [[nodiscard]] detail::DirtyRect makeIconRect(int16_t x, int16_t y, uint16_t size) noexcept
        {
            return (size > 0) ? detail::DirtyRect{static_cast<int16_t>(x - 1), static_cast<int16_t>(y - 1), static_cast<int16_t>(size + 2), static_cast<int16_t>(size + 2)} : detail::DirtyRect{};
        }

        [[nodiscard]] StatusBarLayout resolveStatusBarLayout(const detail::StatusBarState &status,
                                                             int16_t x,
                                                             int16_t y,
                                                             int16_t w,
                                                             int16_t h,
                                                             int16_t leftTextW,
                                                             int16_t centerTextW,
                                                             int16_t rightTextW) noexcept
        {
            StatusBarLayout layout;

            if (status.leftIcon.iconId != kInvalidStatusBarIcon && status.leftIcon.alpha > 0)
                layout.leftIconSize = resolveStatusBarSlotIconSize(h, status.leftIcon.sizePx);
            if (status.centerIcon.iconId != kInvalidStatusBarIcon && status.centerIcon.alpha > 0)
                layout.centerIconSize = resolveStatusBarSlotIconSize(h, status.centerIcon.sizePx);
            if (status.rightIcon.iconId != kInvalidStatusBarIcon && status.rightIcon.alpha > 0)
                layout.rightIconSize = resolveStatusBarSlotIconSize(h, status.rightIcon.sizePx);
            if (status.batteryStyle != Hidden && status.batteryLevel >= 0)
                layout.batterySize = resolveStatusBarBatterySize(h);

            int16_t leftCursor = static_cast<int16_t>(x + kStatusBarInsetPx);
            if (layout.leftIconSize > 0)
            {
                layout.leftIconX = leftCursor;
                layout.leftIconY = static_cast<int16_t>(y + (h - (int16_t)layout.leftIconSize) / 2);
                leftCursor = static_cast<int16_t>(leftCursor + layout.leftIconSize + kStatusBarItemGapPx);
            }
            layout.leftTextX = leftCursor;
            layout.leftRect = rectUnion(makeIconRect(layout.leftIconX, layout.leftIconY, layout.leftIconSize),
                                        makeTextRect(layout.leftTextX, y, leftTextW, h));

            const int16_t centerGroupW =
                static_cast<int16_t>(centerTextW +
                                     ((layout.centerIconSize > 0) ? (layout.centerIconSize + ((centerTextW > 0) ? kStatusBarItemGapPx : 0)) : 0));
            int16_t centerCursor = static_cast<int16_t>(x + (w - centerGroupW) / 2);
            if (layout.centerIconSize > 0)
            {
                layout.centerIconX = centerCursor;
                layout.centerIconY = static_cast<int16_t>(y + (h - (int16_t)layout.centerIconSize) / 2);
                centerCursor = static_cast<int16_t>(centerCursor + layout.centerIconSize + ((centerTextW > 0) ? kStatusBarItemGapPx : 0));
            }
            layout.centerTextX = centerCursor;
            layout.centerRect = rectUnion(makeIconRect(layout.centerIconX, layout.centerIconY, layout.centerIconSize),
                                          makeTextRect(layout.centerTextX, y, centerTextW, h));

            int16_t rightCursor = static_cast<int16_t>(x + w - kStatusBarInsetPx);
            if (layout.batterySize > 0)
            {
                layout.batteryX = static_cast<int16_t>(rightCursor - layout.batterySize);
                if (layout.batteryX < x + 1)
                    layout.batteryX = static_cast<int16_t>(x + 1);
                layout.batteryY = static_cast<int16_t>(y + (h - (int16_t)layout.batterySize) / 2);
                rightCursor = static_cast<int16_t>(layout.batteryX - kStatusBarItemGapPx);
                layout.batteryRect = makeIconRect(layout.batteryX, layout.batteryY, layout.batterySize);
            }
            if (layout.rightIconSize > 0)
            {
                layout.rightIconX = static_cast<int16_t>(rightCursor - layout.rightIconSize);
                if (layout.rightIconX < x + 1)
                    layout.rightIconX = static_cast<int16_t>(x + 1);
                layout.rightIconY = static_cast<int16_t>(y + (h - (int16_t)layout.rightIconSize) / 2);
                rightCursor = static_cast<int16_t>(layout.rightIconX - kStatusBarItemGapPx);
            }
            layout.rightTextX = static_cast<int16_t>(rightCursor - rightTextW);
            if (layout.rightTextX < x + kStatusBarInsetPx)
                layout.rightTextX = static_cast<int16_t>(x + kStatusBarInsetPx);
            layout.rightRect = rectUnion(makeIconRect(layout.rightIconX, layout.rightIconY, layout.rightIconSize),
                                         makeTextRect(layout.rightTextX, y, rightTextW, h));

            return layout;
        }
    }

    void GUI::configureStatusBar(bool enabled, uint32_t bgColor, uint8_t height, StatusBarPosition pos)
    {
        _flags.statusBarConfigured = 1;
        _flags.statusBarEnabled = enabled;

        _status.dirtyMask = detail::StatusBarDirtyAll;
        _status.lastLeft = {};
        _status.lastCenter = {};
        _status.lastRight = {};
        _status.lastBattery = {};

        if (!_flags.statusBarEnabled)
        {
            _status.height = 0;
            return;
        }

        _status.bg = bgColor;
        _status.pos = (pos == Bottom) ? Bottom : Top;

        uint8_t hLocal = height ? height : 18;

        if (_render.screenWidth == 0 || _render.screenHeight == 0)
            _status.height = hLocal;
        else
        {
            hLocal = static_cast<uint8_t>(std::min<uint16_t>(hLocal, _render.screenHeight));
            _status.height = hLocal;
        }

        _status.fg = detail::autoTextColor(detail::color888To565(bgColor), 128);
    }

    void GUI::setStatusBarText(const String &left, const String &center, const String &right)
    {
        uint8_t dirty = 0;
        if (_status.textLeft != left)
            dirty |= detail::StatusBarDirtyLeft;
        if (_status.textCenter != center)
            dirty |= detail::StatusBarDirtyCenter;
        if (_status.textRight != right)
            dirty |= detail::StatusBarDirtyRight;
        if (dirty == 0)
            return;
        _status.dirtyMask |= dirty;
        _status.textLeft = left;
        _status.textCenter = center;
        _status.textRight = right;
    }

    void GUI::setStatusBarBattery(int8_t levelPercent, BatteryStyle style)
    {
        if (levelPercent < 0)
        {
            if (_status.batteryLevel == -1 && _status.batteryStyle == Hidden)
                return;
            _status.batteryLevel = -1;
            _status.batteryStyle = Hidden;
            _status.dirtyMask |= detail::StatusBarDirtyBattery;
            return;
        }

        const int8_t lvl = static_cast<int8_t>(std::clamp<int>(levelPercent, 0, 100));
        if (_status.batteryLevel == lvl && _status.batteryStyle == style)
            return;
        _status.batteryLevel = lvl;
        _status.batteryStyle = style;
        _status.dirtyMask |= detail::StatusBarDirtyBattery;
    }

    void GUI::setStatusBarIcon(TextAlign side, IconId iconId, int32_t color, uint16_t sizePx)
    {
        detail::StatusBarIconState *slot = nullptr;
        uint8_t dirtyBit = 0;

        if (side == TextAlign::Left)
        {
            slot = &_status.leftIcon;
            dirtyBit = detail::StatusBarDirtyLeft;
        }
        else if (side == TextAlign::Center)
        {
            slot = &_status.centerIcon;
            dirtyBit = detail::StatusBarDirtyCenter;
        }
        else if (side == TextAlign::Right)
        {
            slot = &_status.rightIcon;
            dirtyBit = detail::StatusBarDirtyRight;
        }

        if (!slot)
            return;

        const uint16_t color565 = (color < 0) ? static_cast<uint16_t>(_status.fg) : static_cast<uint16_t>(color);
        if (configureStatusBarIcon(*slot, iconId, color565, sizePx, iconId != kInvalidStatusBarIcon, nowMs()))
            _status.dirtyMask |= dirtyBit;
    }

    void GUI::clearStatusBarIcon(TextAlign side)
    {
        setStatusBarIcon(side, kInvalidStatusBarIcon);
    }

    bool GUI::statusBarAnimationActive() const noexcept
    {
        const auto active = [](const detail::StatusBarIconState &icon) noexcept
        {
            return icon.alpha != icon.alphaTo;
        };

        return active(_status.leftIcon) ||
               active(_status.centerIcon) ||
               active(_status.rightIcon);
    }

    void GUI::updateStatusBar()
    {
        if (!_flags.statusBarEnabled || _status.height == 0 || !_disp.display)
            return;

        const uint32_t now = nowMs();
        auto animateIcon = [&](detail::StatusBarIconState &icon, uint8_t dirtyBit)
        {
            const uint8_t prevAlpha = icon.alpha;
            if (stepStatusBarIcon(icon, now) != prevAlpha)
                _status.dirtyMask |= dirtyBit;
        };

        animateIcon(_status.leftIcon, detail::StatusBarDirtyLeft);
        animateIcon(_status.centerIcon, detail::StatusBarDirtyCenter);
        animateIcon(_status.rightIcon, detail::StatusBarDirtyRight);

        if (_status.dirtyMask == 0)
            return;

        if (_flags.statusBarDebugMetrics)
        {
            Debug::update();
            _status.dirtyMask = detail::StatusBarDirtyAll;
        }

        DirtyRect bar = {0, 0, (int16_t)_render.screenWidth, (int16_t)_status.height};
        if (_status.pos == Bottom)
            bar.y = (int16_t)(_render.screenHeight - _status.height);

        if (bar.w <= 0 || bar.h <= 0)
        {
            _status.dirtyMask = 0;
            return;
        }

        auto redrawBlurBackdrop = [&]()
        {
            if (_status.style != StatusBarStyleBlur)
                return;

            const ClipState prevClip = _clip;
            applyClip(bar.x, bar.y, bar.w, bar.h);

            const ScreenCallback currentCb = (_screen.current < _screen.capacity && _screen.callbacks)
                                                 ? _screen.callbacks[_screen.current]
                                                 : nullptr;
            if (currentCb)
                renderScreenToMainSprite(currentCb, _screen.current);
            else
                clear(_render.bgColor565 ? _render.bgColor565 : (uint16_t)_render.bgColor);

            _clip = prevClip;
        };

        if (_flags.statusBarDebugMetrics || _status.custom)
        {
            redrawBlurBackdrop();
            invalidateRect(bar.x, bar.y, bar.w, bar.h);
            renderStatusBar();
            _status.dirtyMask = 0;
            return;
        }

        uint8_t mask = _status.dirtyMask;
        if (mask == detail::StatusBarDirtyAll)
            mask = (uint8_t)(detail::StatusBarDirtyLeft | detail::StatusBarDirtyCenter | detail::StatusBarDirtyRight | detail::StatusBarDirtyBattery);
        if ((mask & detail::StatusBarDirtyBattery) != 0)
            mask |= detail::StatusBarDirtyRight;

        {
            uint16_t px = (bar.h > 6) ? (uint16_t)(bar.h - 4) : (uint16_t)bar.h;
            if (px < 8)
                px = 8;
            setFontSize(px);
            setFontWeight(Medium);
        }

        auto textWidth = [&](const String &s) -> int16_t
        {
            if (s.length() == 0)
                return 0;
            int16_t tw = 0, th = 0;
            measureText(s, tw, th);
            return tw;
        };

        const StatusBarLayout layout = resolveStatusBarLayout(_status,
                                                              bar.x,
                                                              bar.y,
                                                              bar.w,
                                                              bar.h,
                                                              textWidth(_status.textLeft),
                                                              textWidth(_status.textCenter),
                                                              textWidth(_status.textRight));

        DirtyRect dirtyRects[4] = {};
        uint8_t dirtyCount = 0;

        auto trackDirty = [&](const DirtyRect &current, DirtyRect &last) noexcept
        {
            const DirtyRect dirty = rectUnion(current, last);
            last = current;
            if (!rectEmpty(dirty) && dirtyCount < 4)
                dirtyRects[dirtyCount++] = dirty;
        };

        if ((mask & detail::StatusBarDirtyLeft) != 0)
            trackDirty(layout.leftRect, _status.lastLeft);

        if ((mask & detail::StatusBarDirtyCenter) != 0)
            trackDirty(layout.centerRect, _status.lastCenter);

        if ((mask & detail::StatusBarDirtyRight) != 0)
            trackDirty(layout.rightRect, _status.lastRight);

        if ((mask & detail::StatusBarDirtyBattery) != 0)
            trackDirty(layout.batteryRect, _status.lastBattery);

        if (_status.style == StatusBarStyleBlur)
        {
            redrawBlurBackdrop();
            renderStatusBar();
            for (uint8_t i = 0; i < dirtyCount; ++i)
            {
                const DirtyRect &dirty = dirtyRects[i];
                invalidateRect(dirty.x, dirty.y, dirty.w, dirty.h);
            }
        }
        else
        {
            const ClipState prevClip = _clip;
            for (uint8_t i = 0; i < dirtyCount; ++i)
            {
                const DirtyRect &dirty = dirtyRects[i];
                invalidateRect(dirty.x, dirty.y, dirty.w, dirty.h);
                applyClip(dirty.x, dirty.y, dirty.w, dirty.h);
                renderStatusBar();
            }
            _clip = prevClip;
        }

        _status.dirtyMask &= (uint8_t)~(mask);
    }

    void GUI::setStatusBarCustom(StatusBarCustomCallback cb)
    {
        _status.custom = cb;
        _status.dirtyMask = detail::StatusBarDirtyAll;
    }

    int16_t GUI::statusBarHeight() const noexcept
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return 0;
        if (_status.style != StatusBarStyleSolid)
            return 0;
        return (int16_t)_status.height;
    }

    void GUI::renderStatusBar()
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        auto t = getDrawTarget();
        if (!t)
        {
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;
            return;
        }

        uint16_t bg565 = _status.bg;
        uint16_t fg565 = _status.fg;

        int16_t x = 0;
        int16_t y = 0;
        int16_t w = (int16_t)_render.screenWidth;
        int16_t h = (int16_t)_status.height;

        if (_status.pos == Bottom)
            y = (int16_t)(_render.screenHeight - h);

        if (w <= 0 || h <= 0)
        {
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;
            return;
        }

        if (_status.style == StatusBarStyleSolid)
        {
            drawRect()
                .pos(x, y)
                .size(w, h)
                .fill(bg565)
                .draw();
        }
        else if (_status.style == StatusBarStyleBlur)
        {
            BlurDirection dir = (_status.pos == Bottom) ? BottomUp : TopDown;
            int16_t dim = (h > w) ? h : w;
            uint8_t blurRadius = (uint8_t)(dim / 8);
            if (blurRadius < 1)
                blurRadius = 1;
            drawBlurRegion(x, y, w, h, blurRadius, dir, true, 0, -1);
        }

        auto prepareTextMetrics = [&]() -> int16_t
        {
            uint16_t px = (h > 6) ? (uint16_t)(h - 4) : (uint16_t)h;
            if (px < 8)
                px = 8;
            setFontSize(px);
            setFontWeight(Medium);

            int16_t textH = 0;
            int16_t tmpW = 0;
            measureText(String("Ag"), tmpW, textH);
            if (textH <= 0 || textH > h)
                textH = h;
            return textH;
        };

        const int16_t textH = prepareTextMetrics();
        const int16_t baseY = std::max<int16_t>(y, (int16_t)(y + (h - textH) / 2));
        const auto measureTextWidth = [&](const String &s) -> int16_t
        {
            if (!s.length())
                return 0;
            int16_t tw = 0;
            int16_t th = 0;
            measureText(s, tw, th);
            return tw;
        };
        const StatusBarLayout layout = resolveStatusBarLayout(_status,
                                                              x,
                                                              y,
                                                              w,
                                                              h,
                                                              measureTextWidth(_status.textLeft),
                                                              measureTextWidth(_status.textCenter),
                                                              measureTextWidth(_status.textRight));

        const auto drawTextAt = [&](const String &s, int16_t tx)
        {
            if (!s.length())
                return;
            drawTextAligned(s, tx, baseY, fg565, bg565, TextAlign::Left);
        };
        const auto drawStatusIcon = [&](const detail::StatusBarIconState &icon, int16_t ix, int16_t iy, uint16_t sizePx)
        {
            if (icon.iconId == kInvalidStatusBarIcon || icon.alpha == 0 || sizePx == 0)
                return;
            const uint16_t color565 = (icon.alpha >= 255) ? icon.color565 : static_cast<uint16_t>(detail::blend565(bg565, icon.color565, icon.alpha));
            drawIcon()
                .pos(ix, iy)
                .size(sizePx)
                .icon(icon.iconId)
                .color(color565)
                .bgColor(bg565)
                .draw();
        };

        if (_flags.statusBarDebugMetrics)
        {
            char metricsText[64];
            Debug::formatStatusBar(metricsText, sizeof(metricsText));
            int16_t tw = measureTextWidth(String(metricsText));
            int16_t mx = x + (w - tw) / 2;
            if (mx < x + 2)
                mx = x + 2;
            drawTextAt(String(metricsText), mx);

            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;
            return;
        }

        drawStatusIcon(_status.leftIcon, layout.leftIconX, layout.leftIconY, layout.leftIconSize);
        if (_status.textLeft.length() > 0)
            drawTextAt(_status.textLeft, layout.leftTextX);

        if (_status.batteryStyle != Hidden && _status.batteryLevel >= 0 && layout.batterySize > 0)
        {
            uint16_t fillCol = (_status.batteryLevel <= 20) ? 0xF800 : 0x07E0;

            drawIcon()
                .pos(layout.batteryX, layout.batteryY)
                .size(layout.batterySize)
                .icon(battery_l2)
                .color(fillCol)
                .bgColor(bg565)
                .draw();
            int16_t cutX = static_cast<int16_t>(layout.batteryX + (int32_t)layout.batterySize * _status.batteryLevel / 100);
            drawRect()
                .pos(cutX, layout.batteryY)
                .size(static_cast<int16_t>(layout.batteryX + layout.batterySize - cutX), (int16_t)layout.batterySize)
                .fill(bg565)
                .draw();

            drawIcon()
                .pos(layout.batteryX, layout.batteryY)
                .size(layout.batterySize)
                .icon(battery_l0)
                .color(fg565)
                .bgColor(bg565)
                .draw();
            drawIcon()
                .pos(layout.batteryX, layout.batteryY)
                .size(layout.batterySize)
                .icon(battery_l1)
                .color(fg565)
                .bgColor(bg565)
                .draw();
        }

        drawStatusIcon(_status.rightIcon, layout.rightIconX, layout.rightIconY, layout.rightIconSize);
        if (_status.textRight.length() > 0)
            drawTextAt(_status.textRight, layout.rightTextX);

        drawStatusIcon(_status.centerIcon, layout.centerIconX, layout.centerIconY, layout.centerIconSize);
        if (_status.textCenter.length() > 0)
            drawTextAt(_status.textCenter, layout.centerTextX);

        if (_status.custom)
            _status.custom(*this, x, y, w, h);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
    }

#else

    void GUI::configureStatusBar(bool enabled, uint32_t, uint8_t, StatusBarPosition)
    {
        _flags.statusBarConfigured = 1;
        _flags.statusBarEnabled = enabled ? 1 : 0;
        _status.height = 0;
    }

    void GUI::setStatusBarText(const String &, const String &, const String &) {}
    void GUI::setStatusBarBattery(int8_t, BatteryStyle) {}
    void GUI::setStatusBarIcon(TextAlign, IconId, int32_t, uint16_t) {}
    void GUI::clearStatusBarIcon(TextAlign) {}
    bool GUI::statusBarAnimationActive() const noexcept { return false; }
    void GUI::updateStatusBar() {}
    void GUI::setStatusBarCustom(StatusBarCustomCallback) {}

    int16_t GUI::statusBarHeight() const noexcept { return 0; }
    void GUI::renderStatusBar() {}

#endif
}

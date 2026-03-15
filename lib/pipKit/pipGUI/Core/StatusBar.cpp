#include <pipGUI/Core/API/pipGUI.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Icons/metrics.hpp>
#include <algorithm>

namespace pipgui
{

    void GUI::configureStatusBar(bool enabled, uint32_t bgColor, uint8_t height, StatusBarPosition pos)
    {
        _flags.statusBarEnabled = enabled;

        _status.dirtyMask = StatusBarDirtyAll;
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
        if (_status.textLeft != left)
            _status.dirtyMask |= StatusBarDirtyLeft;
        if (_status.textCenter != center)
            _status.dirtyMask |= StatusBarDirtyCenter;
        if (_status.textRight != right)
            _status.dirtyMask |= StatusBarDirtyRight;
        if (_status.dirtyMask == 0)
            return;
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
            _status.dirtyMask |= StatusBarDirtyBattery;
            return;
        }

        const int8_t lvl = static_cast<int8_t>(std::clamp<int>(levelPercent, 0, 100));
        if (_status.batteryLevel == lvl && _status.batteryStyle == style)
            return;
        _status.batteryLevel = lvl;
        _status.batteryStyle = style;
        _status.dirtyMask |= StatusBarDirtyBattery;
    }

    void GUI::updateStatusBar()
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return;
        if (!_disp.display)
            return;

        if (_status.dirtyMask == 0)
            return;

        if (_flags.statusBarDebugMetrics)
        {
            Debug::update();
            _status.dirtyMask = StatusBarDirtyAll;
        }

        DirtyRect bar = {0, 0, (int16_t)_render.screenWidth, (int16_t)_status.height};
        if (_status.pos == Bottom)
            bar.y = (int16_t)(_render.screenHeight - _status.height);

        if (bar.w <= 0 || bar.h <= 0)
        {
            _status.dirtyMask = 0;
            return;
        }

        if (_flags.statusBarDebugMetrics)
        {
            invalidateRect(bar.x, bar.y, bar.w, bar.h);
            renderStatusBar();
            _status.dirtyMask = 0;
            return;
        }

        if (_status.custom)
            _status.dirtyMask = StatusBarDirtyAll;

        uint8_t mask = _status.dirtyMask;
        if (mask == StatusBarDirtyAll)
            mask = (uint8_t)(StatusBarDirtyLeft | StatusBarDirtyCenter | StatusBarDirtyRight | StatusBarDirtyBattery);

        DirtyRect totalDirty = {};

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

        auto unionRect = [](const DirtyRect &a, const DirtyRect &b) noexcept -> DirtyRect
        {
            if (a.w <= 0 || a.h <= 0)
                return b;
            if (b.w <= 0 || b.h <= 0)
                return a;

            const int16_t x1 = (a.x < b.x) ? a.x : b.x;
            const int16_t y1 = (a.y < b.y) ? a.y : b.y;
            const int16_t x2 = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
            const int16_t y2 = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);
            return {x1, y1, static_cast<int16_t>(x2 - x1), static_cast<int16_t>(y2 - y1)};
        };

        auto trackDirty = [&](const DirtyRect &current, DirtyRect &last) noexcept
        {
            totalDirty = unionRect(totalDirty, unionRect(current, last));
            last = current;
        };

        if ((mask & StatusBarDirtyLeft) != 0)
        {
            int16_t tw = textWidth(_status.textLeft);
            DirtyRect newLeft = {0, 0, 0, 0};
            if (tw > 0)
                newLeft = {(int16_t)(bar.x), (int16_t)(bar.y - 2), (int16_t)(tw + 4), (int16_t)(bar.h + 4)};
            trackDirty(newLeft, _status.lastLeft);
        }

        DirtyRect newBat = {};
        if ((mask & StatusBarDirtyBattery) != 0 && _status.batteryStyle != Hidden && _status.batteryLevel >= 0)
        {
            int16_t iconSize = (bar.h > 2) ? (int16_t)(bar.h - 2) : bar.h;
            if (iconSize < 8)
                iconSize = 8;

            int16_t bx = (int16_t)(bar.x + bar.w - 1 - iconSize);
            if (bx < bar.x + 1)
                bx = (int16_t)(bar.x + 1);
            int16_t by = (int16_t)(bar.y + (bar.h - iconSize) / 2);

            newBat = {(int16_t)(bx - 1), (int16_t)(by - 1), (int16_t)(iconSize + 2), (int16_t)(iconSize + 2)};
            trackDirty(newBat, _status.lastBattery);
        }
        else if (_status.batteryStyle != Hidden && _status.batteryLevel >= 0 && _status.lastBattery.w > 0)
        {
            newBat = _status.lastBattery;
        }

        if ((mask & StatusBarDirtyCenter) != 0)
        {
            int16_t tw = textWidth(_status.textCenter);
            DirtyRect newCenter = {0, 0, 0, 0};
            if (tw > 0)
            {
                int16_t startX = (int16_t)(bar.x + (bar.w - tw) / 2);
                newCenter = {(int16_t)(startX - 2), (int16_t)(bar.y - 2), (int16_t)(tw + 4), (int16_t)(bar.h + 4)};
            }
            trackDirty(newCenter, _status.lastCenter);
        }

        if ((mask & StatusBarDirtyRight) != 0)
        {
            int16_t rightCursor = (int16_t)(bar.x + bar.w - 2);
            if (newBat.w > 0)
                rightCursor = (int16_t)(newBat.x - 2);
            else if (_status.lastBattery.w > 0 && !(mask & StatusBarDirtyBattery))
                rightCursor = (int16_t)(_status.lastBattery.x - 2);

            int16_t tw = textWidth(_status.textRight);
            DirtyRect newRight = {0, 0, 0, 0};
            if (tw > 0)
            {
                int16_t startX = (int16_t)(rightCursor - tw);
                if (startX < bar.x)
                    startX = bar.x;
                newRight = {(int16_t)(startX - 2), (int16_t)(bar.y - 2), (int16_t)(tw + 4), (int16_t)(bar.h + 4)};
            }
            trackDirty(newRight, _status.lastRight);
        }

        if (totalDirty.w > 0 && totalDirty.h > 0)
        {
            invalidateRect(totalDirty.x, totalDirty.y, totalDirty.w, totalDirty.h);
            renderStatusBar();
        }

        _status.dirtyMask &= (uint8_t)~(mask);
    }

    void GUI::setStatusBarCustom(StatusBarCustomCallback cb)
    {
        _status.custom = cb;
        _status.dirtyMask = StatusBarDirtyAll;
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
            fillRect()
                .pos(x, y)
                .size(w, h)
                .color(bg565)
                .draw();
        }
        else if (_status.style == StatusBarStyleBlurGradient)
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

        auto measureTextWidth = [&](const String &s) -> int16_t
        {
            if (!s.length())
                return 0;
            int16_t tw = 0;
            int16_t th = 0;
            measureText(s, tw, th);
            return tw;
        };

        auto drawTextAt = [&](const String &s, int16_t tx)
        {
            if (!s.length())
                return;
            drawTextAligned(s, tx, baseY, fg565, bg565, AlignLeft);
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

        int16_t leftX = x + 2;
        if (_status.textLeft.length() > 0)
        {
            int16_t lx = leftX;
            if (lx > x + w - 4)
                lx = x + w - 4;
            drawTextAt(_status.textLeft, lx);
        }

        int16_t rightCursor = x + w - 2;

        if (_status.batteryStyle != Hidden && _status.batteryLevel >= 0)
        {
            int16_t iconSize = (h > 2) ? (int16_t)(h - 2) : h;
            if (iconSize < 8)
                iconSize = 8;

            int16_t bx = rightCursor - iconSize;
            if (bx < x + 1)
                bx = x + 1;
            int16_t by = y + (h - iconSize) / 2;

            uint16_t frameColor = _status.fg;
            uint16_t fillCol = (_status.batteryLevel <= 20) ? 0xF800 : 0x07E0;

            drawIcon()
                .pos(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer2)
                .color(fillCol)
                .bgColor(bg565)
                .draw();
            int16_t cutX = (int16_t)(bx + (int32_t)iconSize * _status.batteryLevel / 100);
            fillRect()
                .pos(cutX, by)
                .size((int16_t)(bx + iconSize - cutX), (int16_t)iconSize)
                .color(bg565)
                .draw();

            drawIcon()
                .pos(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer0)
                .color(frameColor)
                .bgColor(bg565)
                .draw();
            drawIcon()
                .pos(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer1)
                .color(frameColor)
                .bgColor(bg565)
                .draw();

            rightCursor = bx - 4;
        }

        if (_status.textRight.length() > 0)
        {
            int16_t tw = measureTextWidth(_status.textRight);
            int16_t rx = rightCursor - tw;
            if (rx < x + 2)
                rx = x + 2;
            drawTextAt(_status.textRight, rx);
            rightCursor = rx - 4;
        }

        if (_status.textCenter.length() > 0)
        {
            int16_t textW = measureTextWidth(_status.textCenter);
            if (textW > 0)
            {
                int16_t tx = x + (w - textW) / 2;
                if (tx < x + 2)
                    tx = x + 2;
                if (tx + textW > x + w - 2)
                    tx = x + w - 2 - textW;
                drawTextAt(_status.textCenter, tx);
            }
        }

        if (_status.custom)
            _status.custom(*this, x, y, w, h);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
    }
}

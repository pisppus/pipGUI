#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/Debug.hpp>
#include <pipGUI/icons/metrics.hpp>

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
        _status.pos = pos;

        uint8_t hLocal = height;
        if (hLocal == 0)
            hLocal = 18;

        if (_render.screenWidth == 0 || _render.screenHeight == 0)
            _status.height = hLocal;
        else
        {
            if (pos == Left || pos == Right)
            {
                if (hLocal > _render.screenWidth)
                    hLocal = (uint8_t)_render.screenWidth;
            }
            else
            {
                if (hLocal > _render.screenHeight)
                    hLocal = (uint8_t)_render.screenHeight;
            }

            _status.height = hLocal;
        }

        // Auto-pick text color (black/white) based on background brightness.
        // Use mid-luminance threshold so light bars get dark text and vice versa.
        _status.fg = detail::autoTextColor888(bgColor, 128);
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

        int8_t lvl = (levelPercent > 100) ? 100 : levelPercent;
        if (_status.batteryLevel == lvl && _status.batteryStyle == style)
            return;
        _status.batteryLevel = lvl;
        _status.batteryStyle = style;
        _status.dirtyMask |= StatusBarDirtyBattery;
    }

    void GUI::updateStatusBarBattery()
    {
        updateStatusBar();
    }

    void GUI::updateStatusBarCenter()
    {
        updateStatusBar();
    }

    void GUI::updateStatusBar()
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return;
        if (!_disp.display)
            return;

        auto unionRect = [](DirtyRect a, DirtyRect b) -> DirtyRect
        {
            if (a.w <= 0 || a.h <= 0)
                return b;
            if (b.w <= 0 || b.h <= 0)
                return a;
            int16_t x1 = (a.x < b.x) ? a.x : b.x;
            int16_t y1 = (a.y < b.y) ? a.y : b.y;
            int16_t x2a = (int16_t)(a.x + a.w);
            int16_t y2a = (int16_t)(a.y + a.h);
            int16_t x2b = (int16_t)(b.x + b.w);
            int16_t y2b = (int16_t)(b.y + b.h);
            int16_t x2 = (x2a > x2b) ? x2a : x2b;
            int16_t y2 = (y2a > y2b) ? y2a : y2b;
            return {x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
        };

        auto calcBarRect = [&]() -> DirtyRect
        {
            int16_t x = 0;
            int16_t y = 0;
            int16_t w = (int16_t)_render.screenWidth;
            int16_t h = (int16_t)_status.height;

            if (_status.pos == Bottom)
            {
                y = (int16_t)(_render.screenHeight - h);
            }
            else if (_status.pos == Left)
            {
                w = (int16_t)_status.height;
                h = (int16_t)_render.screenHeight;
            }
            else if (_status.pos == Right)
            {
                w = (int16_t)_status.height;
                h = (int16_t)_render.screenHeight;
                x = (int16_t)(_render.screenWidth - w);
            }

            return {x, y, w, h};
        };

        if (!_flags.spriteEnabled)
        {
            if (_status.dirtyMask == 0)
                return;

            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _render.activeSprite;

            _flags.renderToSprite = 0;
            renderStatusBar(false);
            _flags.renderToSprite = prevRender;
            _render.activeSprite = prevActive;

            _status.dirtyMask = 0;
            return;
        }

        if (_status.dirtyMask == 0)
            return;

        // Update debug metrics if enabled
        if (_flags.statusBarDebugMetrics)
        {
            Debug::update();
            // Always mark as dirty when debug metrics are enabled to update values
            // And invalidate the entire status bar to avoid flickering
            _status.dirtyMask = StatusBarDirtyAll;
        }

        DirtyRect bar = calcBarRect();
        if (bar.w <= 0 || bar.h <= 0)
        {
            _status.dirtyMask = 0;
            return;
        }

        // In debug metrics mode, always redraw the entire status bar
        if (_flags.statusBarDebugMetrics)
        {
            invalidateRect(bar.x, bar.y, bar.w, bar.h);
            renderStatusBar(true);
            // Don't flush here - let the main loop handle it
            _status.dirtyMask = 0;
            return;
        }

        if (_status.pos == Left || _status.pos == Right)
            _status.dirtyMask = StatusBarDirtyAll;

        if (_status.custom)
            _status.dirtyMask = StatusBarDirtyAll;

        uint8_t mask = _status.dirtyMask;
        if (mask == StatusBarDirtyAll)
            mask = (uint8_t)(StatusBarDirtyLeft | StatusBarDirtyCenter | StatusBarDirtyRight | StatusBarDirtyBattery);

        int16_t pad = 2;
        DirtyRect dirtyLeft = {};
        DirtyRect dirtyCenter = {};
        DirtyRect dirtyRight = {};
        DirtyRect dirtyBattery = {};

        {
            uint16_t px = (bar.h > 6) ? (uint16_t)(bar.h - 4) : (uint16_t)bar.h;
            if (px < 8)
                px = 8;
            setPSDFFontSize(px);
        }

        auto textWidth = [&](const String &s) -> int16_t
        {
            if (s.length() == 0)
                return 0;
            int16_t tw = 0;
            int16_t th = 0;
            psdfMeasureText(s, tw, th);
            return tw;
        };

        if ((mask & StatusBarDirtyLeft) != 0)
        {
            DirtyRect newLeft = {};

            int16_t startX = bar.x + 2;
            int16_t tw = textWidth(_status.textLeft);
            if (tw > 0)
                newLeft = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(tw + pad * 2), (int16_t)(bar.h + pad * 2)};

            DirtyRect merged = unionRect(newLeft, _status.lastLeft);
            dirtyLeft = merged;
            _status.lastLeft = newLeft;
        }

        DirtyRect newBat = {};
        if (_status.batteryStyle != Hidden && _status.batteryLevel >= 0)
        {
            int16_t rightCursor = bar.x + bar.w - 2;

            int16_t bwTotal = 30;
            if (bwTotal + 2 > bar.w)
                bwTotal = (bar.w > 4) ? (bar.w - 2) : bar.w;

            int16_t bh = 15;
            if (bh + 4 > bar.h)
            {
                bh = bar.h - 4;
                if (bh < 6)
                    bh = bar.h - 2;
                if (bh < 4)
                    bh = bar.h;
            }

            int16_t bx = rightCursor - bwTotal;
            if (bx < bar.x + 2)
                bx = bar.x + 2;
            int16_t by = bar.y + (bar.h - bh) / 2;
            if (by < bar.y)
                by = bar.y;

            newBat = {(int16_t)(bx - pad), (int16_t)(by - pad), (int16_t)(bwTotal + pad * 2), (int16_t)(bh + pad * 2)};
        }

        if ((mask & StatusBarDirtyBattery) != 0)
        {
            DirtyRect merged = unionRect(newBat, _status.lastBattery);
            dirtyBattery = merged;
            _status.lastBattery = newBat;
        }

        if ((mask & StatusBarDirtyCenter) != 0)
        {
            DirtyRect newCenter = {};

            int16_t textW = textWidth(_status.textCenter);
            if (textW > 0)
            {
                int16_t startX = bar.x + (bar.w - textW) / 2;
                newCenter = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(textW + pad * 2), (int16_t)(bar.h + pad * 2)};
            }

            DirtyRect merged = unionRect(newCenter, _status.lastCenter);
            dirtyCenter = merged;
            _status.lastCenter = newCenter;
        }

        if ((mask & StatusBarDirtyRight) != 0)
        {
            DirtyRect newRight = {};

            int16_t rightCursor = bar.x + bar.w - 2;
            if (newBat.w > 0)
            {
                int16_t bx = (int16_t)(newBat.x + pad);
                rightCursor = (int16_t)(bx - 4);
            }

            int16_t rightBound = rightCursor;
            int16_t tw = textWidth(_status.textRight);
            if (tw > 0)
            {
                int16_t startX = (int16_t)(rightBound - tw);
                if (startX < bar.x)
                    startX = bar.x;
                newRight = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(tw + pad * 2), (int16_t)(bar.h + pad * 2)};
            }

            DirtyRect merged = unionRect(newRight, _status.lastRight);
            dirtyRight = merged;
            _status.lastRight = newRight;
        }

        bool anyDirty = false;

        if (dirtyLeft.w > 0 && dirtyLeft.h > 0)
        {
            invalidateRect(dirtyLeft.x, dirtyLeft.y, dirtyLeft.w, dirtyLeft.h);
            anyDirty = true;
        }
        if (dirtyCenter.w > 0 && dirtyCenter.h > 0)
        {
            invalidateRect(dirtyCenter.x, dirtyCenter.y, dirtyCenter.w, dirtyCenter.h);
            anyDirty = true;
        }
        if (dirtyRight.w > 0 && dirtyRight.h > 0)
        {
            invalidateRect(dirtyRight.x, dirtyRight.y, dirtyRight.w, dirtyRight.h);
            anyDirty = true;
        }
        if (dirtyBattery.w > 0 && dirtyBattery.h > 0)
        {
            invalidateRect(dirtyBattery.x, dirtyBattery.y, dirtyBattery.w, dirtyBattery.h);
            anyDirty = true;
        }

        if (!anyDirty)
        {
            _status.dirtyMask = 0;
            return;
        }

        renderStatusBar(true);
        flushDirty();
        _status.dirtyMask &= (uint8_t)~(mask);
    }

    void GUI::setStatusBarCustom(StatusBarCustomCallback cb)
    {
        _status.custom = cb;
        _status.dirtyMask = StatusBarDirtyAll;
    }

    int16_t GUI::statusBarHeight() const
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return 0;
        // Only Solid style reserves layout space. Blur overlays content.
        if (_status.style != StatusBarStyleSolid)
            return 0;
        return (int16_t)_status.height;
    }

    void GUI::renderStatusBar(bool useSprite)
    {
        if (!_flags.statusBarEnabled || _status.height == 0)
            return;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _render.activeSprite;

        if (useSprite && _flags.spriteEnabled)
        {
            _flags.renderToSprite = 1;
            _render.activeSprite = &_render.sprite;
        }
        else
        {
            _flags.renderToSprite = 0;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        uint16_t bg565 = _status.bg;
        uint16_t fg565 = _status.fg;

        int16_t x = 0;
        int16_t y = 0;
        int16_t w = (int16_t)_render.screenWidth;
        int16_t h = (int16_t)_status.height;

        if (_status.pos == Bottom)
        {
            y = (int16_t)(_render.screenHeight - h);
        }
        else if (_status.pos == Left)
        {
            w = (int16_t)_status.height;
            h = (int16_t)_render.screenHeight;
        }
        else if (_status.pos == Right)
        {
            w = (int16_t)_status.height;
            h = (int16_t)_render.screenHeight;
            x = (int16_t)(_render.screenWidth - w);
        }

        if (w <= 0 || h <= 0)
        {
            _flags.renderToSprite = prevRender;
            _render.activeSprite = prevActive;
            return;
        }

        if (_status.style == StatusBarStyleSolid)
        {
            fillRect()
                .at(x, y)
                .size(w, h)
                .color(bg565)
                .draw();
        }
        else if (_status.style == StatusBarStyleBlurGradient)
        {
            BlurDirection dir = TopDown;
            if (_status.pos == Bottom)
                dir = BottomUp;
            else if (_status.pos == Left)
                dir = RightLeft;
            else if (_status.pos == Right)
                dir = LeftRight;
            int16_t dim = (h > w) ? h : w;
            uint8_t blurRadius = (uint8_t)(dim / 8);
            if (blurRadius < 1)
                blurRadius = 1;
            drawBlurRegion(x, y, w, h, blurRadius, dir, true, 0, 0, -1);
        }

        // If debug metrics mode is enabled, only render metrics and skip normal content
        if (_flags.statusBarDebugMetrics)
        {
            int16_t textH = 0;
            {
                uint16_t px = (h > 6) ? (uint16_t)(h - 4) : (uint16_t)h;
                if (px < 8)
                    px = 8;
                setPSDFFontSize(px);
                int16_t tmpW = 0;
                psdfMeasureText(String("Ag"), tmpW, textH);
                if (textH <= 0 || textH > h)
                    textH = h;
            }

            int16_t baseY = y + (h - textH) / 2;
            if (baseY < y)
                baseY = y;

            auto measureText = [&](const String &s) -> int16_t
            {
                if (!s.length())
                    return 0;
                int16_t tw = 0;
                int16_t th = 0;
                psdfMeasureText(s, tw, th);
                return tw;
            };

            auto drawTextAt = [&](const String &s, int16_t tx)
            {
                if (!s.length())
                    return;
                psdfDrawTextInternal(s, tx, baseY, fg565, bg565, AlignLeft);
            };

            char metricsText[64];
            Debug::formatStatusBar(metricsText, sizeof(metricsText));
            int16_t tw = measureText(String(metricsText));
            int16_t mx = x + (w - tw) / 2;
            if (mx < x + 2)
                mx = x + 2;
            drawTextAt(String(metricsText), mx);

            _flags.renderToSprite = prevRender;
            _render.activeSprite = prevActive;
            return;
        }

        int16_t textH = 0;

        {
            uint16_t px = (h > 6) ? (uint16_t)(h - 4) : (uint16_t)h;
            if (px < 8)
                px = 8;
            setPSDFFontSize(px);
            int16_t tmpW = 0;
            psdfMeasureText(String("Ag"), tmpW, textH);
            if (textH <= 0 || textH > h)
                textH = h;
        }

        int16_t baseY = y + (h - textH) / 2;
        if (baseY < y)
            baseY = y;

        auto measureText = [&](const String &s) -> int16_t
        {
            if (!s.length())
                return 0;
            int16_t tw = 0;
            int16_t th = 0;
            psdfMeasureText(s, tw, th);
            return tw;
        };

        auto drawTextAt = [&](const String &s, int16_t tx)
        {
            if (!s.length())
                return;
            psdfDrawTextInternal(s, tx, baseY, fg565, bg565, AlignLeft);
        };

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
            int16_t bwTotal = 27;
            if (bwTotal + 2 > w)
                bwTotal = (w > 4) ? (w - 2) : w;

            int16_t bh = 15;
            if (bh + 4 > h)
            {
                bh = h - 4;
                if (bh < 6)
                    bh = h - 2;
                if (bh < 4)
                    bh = h;
            }

            int16_t iconSize = bwTotal;
            if (iconSize < 8)
                iconSize = 8;

            int16_t bx = rightCursor - iconSize;
            if (bx < x + 2)
                bx = x + 2;
            int16_t by = y + (h - iconSize) / 2;
            if (by < y)
                by = y;

            uint16_t frameColor = _status.fg;
            uint16_t fillCol = (_status.batteryLevel <= 20) ? 0xF800 : 0x07E0;

            drawIcon()
                .at(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer2)
                .color(fillCol)
                .bgColor(bg565)
                .draw();
            int16_t cutX = (int16_t)(bx + (int32_t)iconSize * _status.batteryLevel / 100);
            fillRect()
                .at(cutX, by)
                .size((int16_t)(bx + iconSize - cutX), (int16_t)iconSize)
                .color(bg565)
                .draw();

            drawIcon()
                .at(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer0)
                .color(frameColor)
                .bgColor(bg565)
                .draw();
            drawIcon()
                .at(bx, by)
                .size((uint16_t)iconSize)
                .icon(battery_layer1)
                .color(frameColor)
                .bgColor(bg565)
                .draw();

            rightCursor = bx - 4;
        }

        if (_status.textRight.length() > 0)
        {
            int16_t tw = measureText(_status.textRight);
            int16_t rx = rightCursor - tw;
            if (rx < x + 2)
                rx = x + 2;
            drawTextAt(_status.textRight, rx);
            rightCursor = rx - 4;
        }

        if (_status.textCenter.length() > 0)
        {
            int16_t textW = measureText(_status.textCenter);
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

        _flags.renderToSprite = prevRender;
        _render.activeSprite = prevActive;
    }
}

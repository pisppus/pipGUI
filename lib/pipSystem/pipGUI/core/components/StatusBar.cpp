#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{

    void GUI::configureStatusBar(bool enabled, uint16_t bgColor, uint8_t height, StatusBarPosition pos)
    {
        _flags.statusBarEnabled = enabled;

        _statusBarDirtyMask = StatusBarDirtyAll;
        _statusBarLastLeft = {};
        _statusBarLastCenter = {};
        _statusBarLastRight = {};
        _statusBarLastBattery = {};

        if (!_flags.statusBarEnabled)
        {
            _statusBarHeight = 0;
            return;
        }

        _statusBarBg = bgColor;
        _statusBarPos = pos;

        uint8_t hLocal = height;
        if (hLocal == 0)
            hLocal = 18;

        if (_screenWidth == 0 || _screenHeight == 0)
        {
            _statusBarHeight = hLocal;
        }
        else
        {
            if (pos == Left || pos == Right)
            {
                if (hLocal > _screenWidth)
                    hLocal = (uint8_t)_screenWidth;
            }
            else
            {
                if (hLocal > _screenHeight)
                    hLocal = (uint8_t)_screenHeight;
            }

            _statusBarHeight = hLocal;
        }

        uint8_t r = (bgColor >> 11) & 0x1F;
        uint8_t g = (bgColor >> 5) & 0x3F;
        uint8_t b = bgColor & 0x1F;
        uint16_t y = (uint16_t)r * 30U + (uint16_t)g * 59U + (uint16_t)b * 11U;
        uint16_t maxY = (30U * 31U + 59U * 63U + 11U * 31U);
        _statusBarFg = (y > (maxY / 2U)) ? 0x0000 : 0xFFFF;
    }

    void GUI::setStatusBarText(const String &left, const String &center, const String &right)
    {
        if (_statusTextLeft != left)
            _statusBarDirtyMask |= StatusBarDirtyLeft;
        if (_statusTextCenter != center)
            _statusBarDirtyMask |= StatusBarDirtyCenter;
        if (_statusTextRight != right)
            _statusBarDirtyMask |= StatusBarDirtyRight;
        if (_statusBarDirtyMask == 0)
            return;
        _statusTextLeft = left;
        _statusTextCenter = center;
        _statusTextRight = right;
    }

    void GUI::setStatusBarBattery(int8_t levelPercent, BatteryStyle style)
    {
        if (levelPercent < 0)
        {
            if (_batteryLevel == -1 && _batteryStyle == Hidden)
                return;
            _batteryLevel = -1;
            _batteryStyle = Hidden;
            _statusBarDirtyMask |= StatusBarDirtyBattery;
            return;
        }

        int8_t lvl = (levelPercent > 100) ? 100 : levelPercent;
        if (_batteryLevel == lvl && _batteryStyle == style)
            return;
        _batteryLevel = lvl;
        _batteryStyle = style;
        _statusBarDirtyMask |= StatusBarDirtyBattery;
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
        if (!_flags.statusBarEnabled || _statusBarHeight == 0)
            return;
        if (!_display)
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
            int16_t w = (int16_t)_screenWidth;
            int16_t h = (int16_t)_statusBarHeight;

            if (_statusBarPos == Bottom)
            {
                y = (int16_t)(_screenHeight - h);
            }
            else if (_statusBarPos == Left)
            {
                w = (int16_t)_statusBarHeight;
                h = (int16_t)_screenHeight;
            }
            else if (_statusBarPos == Right)
            {
                w = (int16_t)_statusBarHeight;
                h = (int16_t)_screenHeight;
                x = (int16_t)(_screenWidth - w);
            }

            return {x, y, w, h};
        };

        if (!_flags.spriteEnabled)
        {
            if (_statusBarDirtyMask == 0)
                return;

            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            renderStatusBar(false);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;

            _statusBarDirtyMask = 0;
            return;
        }

        if (_statusBarDirtyMask == 0)
            return;

        DirtyRect bar = calcBarRect();
        if (bar.w <= 0 || bar.h <= 0)
        {
            _statusBarDirtyMask = 0;
            return;
        }

        if (_statusBarPos == Left || _statusBarPos == Right)
            _statusBarDirtyMask = StatusBarDirtyAll;

        if (_statusBarCustom)
            _statusBarDirtyMask = StatusBarDirtyAll;

        uint8_t mask = _statusBarDirtyMask;
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
            int16_t cursor = startX;
            for (uint8_t i = 0; i < _iconLeftCount; ++i)
            {
                const StatusIcon &ic = _iconsLeft[i];
                if (!ic.bitmap || ic.w == 0 || ic.h == 0)
                    continue;
                if (cursor + (int16_t)ic.w > bar.x + bar.w)
                    break;
                cursor = (int16_t)(cursor + ic.w + 2);
            }

            int16_t iconsSpanW = cursor - startX;
            int16_t tw = textWidth(_statusTextLeft);
            int16_t totalW = iconsSpanW + tw;
            if (totalW > 0)
                newLeft = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(totalW + pad * 2), (int16_t)(bar.h + pad * 2)};

            DirtyRect merged = unionRect(newLeft, _statusBarLastLeft);
            dirtyLeft = merged;
            _statusBarLastLeft = newLeft;
        }

        DirtyRect newBat = {};
        if (_batteryStyle != Hidden && _batteryLevel >= 0)
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
            DirtyRect merged = unionRect(newBat, _statusBarLastBattery);
            dirtyBattery = merged;
            _statusBarLastBattery = newBat;
        }

        if ((mask & StatusBarDirtyCenter) != 0)
        {
            DirtyRect newCenter = {};

            int16_t iconsW = 0;
            for (uint8_t i = 0; i < _iconCenterCount; ++i)
            {
                const StatusIcon &ic = _iconsCenter[i];
                if (!ic.bitmap || ic.w == 0)
                    continue;
                if (iconsW > 0)
                    iconsW += 2;
                iconsW += ic.w;
            }

            int16_t textW = textWidth(_statusTextCenter);

            int16_t gap = (iconsW > 0 && textW > 0) ? 4 : 0;
            int16_t totalW = iconsW + gap + textW;

            if (totalW > 0)
            {
                int16_t startX = bar.x + (bar.w - totalW) / 2;
                newCenter = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(totalW + pad * 2), (int16_t)(bar.h + pad * 2)};
            }

            DirtyRect merged = unionRect(newCenter, _statusBarLastCenter);
            dirtyCenter = merged;
            _statusBarLastCenter = newCenter;
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
            int16_t cursor = rightCursor;
            for (int i = (int)_iconRightCount - 1; i >= 0; --i)
            {
                const StatusIcon &ic = _iconsRight[i];
                if (!ic.bitmap || ic.w == 0 || ic.h == 0)
                    continue;
                if (cursor - (int16_t)ic.w < bar.x + 2)
                    break;
                cursor = (int16_t)(cursor - ic.w - 2);
            }

            int16_t iconsSpanW = rightBound - cursor;
            int16_t tw = textWidth(_statusTextRight);
            int16_t totalW = iconsSpanW + tw;
            if (totalW > 0)
            {
                int16_t startX = (int16_t)(rightBound - totalW);
                if (startX < bar.x)
                    startX = bar.x;
                newRight = {(int16_t)(startX - pad), (int16_t)(bar.y - pad), (int16_t)(totalW + pad * 2), (int16_t)(bar.h + pad * 2)};
            }

            DirtyRect merged = unionRect(newRight, _statusBarLastRight);
            dirtyRight = merged;
            _statusBarLastRight = newRight;
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
            _statusBarDirtyMask = 0;
            return;
        }

        renderStatusBar(true);
        flushDirty();
        _statusBarDirtyMask &= (uint8_t)~(mask);
    }

    static void addStatusIcon(pipgui::StatusIcon *&arr, uint8_t &count, uint8_t &capacity,
                              const uint16_t *bitmap, uint8_t w, uint8_t h)
    {
        if (!bitmap || w == 0 || h == 0)
            return;
        if (!pipgui::detail::ensureCapacity(GUI::sharedPlatform(), arr, capacity, (uint8_t)(count + 1)))
            return;

        arr[count].bitmap = bitmap;
        arr[count].w = w;
        arr[count].h = h;
        ++count;
    }

    void GUI::addStatusBarIcon(StatusBarIconPos pos, const uint16_t *bitmap, uint8_t w, uint8_t h)
    {
        switch (pos)
        {
        case StatusBarIconLeft:
            addStatusIcon(_iconsLeft, _iconLeftCount, _iconLeftCapacity, bitmap, w, h);
            break;
        case StatusBarIconCenter:
            addStatusIcon(_iconsCenter, _iconCenterCount, _iconCenterCapacity, bitmap, w, h);
            break;
        case StatusBarIconRight:
            addStatusIcon(_iconsRight, _iconRightCount, _iconRightCapacity, bitmap, w, h);
            break;
        default:
            break;
        }

        if (pos == StatusBarIconLeft)
            _statusBarDirtyMask |= StatusBarDirtyLeft;
        else if (pos == StatusBarIconCenter)
            _statusBarDirtyMask |= StatusBarDirtyCenter;
        else if (pos == StatusBarIconRight)
            _statusBarDirtyMask |= StatusBarDirtyRight;
    }

    void GUI::setStatusBarCustom(StatusBarCustomCallback cb)
    {
        _statusBarCustom = cb;
        _statusBarDirtyMask = StatusBarDirtyAll;
    }

    int16_t GUI::statusBarHeight() const
    {
        return (_flags.statusBarEnabled && _statusBarHeight > 0) ? (int16_t)_statusBarHeight : 0;
    }

    void GUI::renderStatusBar(bool useSprite)
    {
        if (!_flags.statusBarEnabled || _statusBarHeight == 0)
            return;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;

        if (useSprite && _flags.spriteEnabled)
        {
            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;
        }
        else
        {
            _flags.renderToSprite = 0;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        int16_t x = 0;
        int16_t y = 0;
        int16_t w = (int16_t)_screenWidth;
        int16_t h = (int16_t)_statusBarHeight;

        if (_statusBarPos == Bottom)
        {
            y = (int16_t)(_screenHeight - h);
        }
        else if (_statusBarPos == Left)
        {
            w = (int16_t)_statusBarHeight;
            h = (int16_t)_screenHeight;
        }
        else if (_statusBarPos == Right)
        {
            w = (int16_t)_statusBarHeight;
            h = (int16_t)_screenHeight;
            x = (int16_t)(_screenWidth - w);
        }

        if (w <= 0 || h <= 0)
        {
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        t->fillRect(x, y, w, h, _statusBarBg);
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
            psdfDrawTextInternal(s, tx, baseY, _statusBarFg, _statusBarBg, AlignLeft);
        };

        int16_t leftX = x + 2;
        for (uint8_t i = 0; i < _iconLeftCount; ++i)
        {
            const StatusIcon &ic = _iconsLeft[i];
            if (!ic.bitmap || ic.w == 0 || ic.h == 0)
                continue;
            if (leftX + ic.w > x + w)
                break;
            int16_t iy = y + (h - ic.h) / 2;
            if (iy < y)
                iy = y;
            t->pushImage(leftX, iy, ic.w, ic.h, ic.bitmap);
            leftX += ic.w + 2;
        }

        if (_statusTextLeft.length() > 0)
        {
            int16_t lx = leftX;
            if (lx > x + w - 4)
                lx = x + w - 4;
            drawTextAt(_statusTextLeft, lx);
        }

        int16_t rightCursor = x + w - 2;

        if (_batteryStyle != Hidden && _batteryLevel >= 0)
        {
            int16_t bwTotal = 30;
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

            int16_t noseW = 2;
            int16_t bwCase = bwTotal - noseW;
            if (bwCase < 10)
            {
                bwCase = bwTotal;
                noseW = 0;
            }

            int16_t bx = rightCursor - bwTotal;
            if (bx < x + 2)
                bx = x + 2;
            int16_t by = y + (h - bh) / 2;
            if (by < y)
                by = y;

            uint16_t frameColor = _statusBarFg;
            uint16_t innerBg = _statusBarBg;

            t->fillRoundRect(bx, by, bwCase, bh, 4, frameColor);
            if (bwCase > 2 && bh > 2)
                t->fillRoundRect(bx + 1, by + 1, bwCase - 2, bh - 2, 3, innerBg);

            if (noseW > 0)
            {
                int16_t tipH = bh - 4;
                if (tipH < 4)
                    tipH = bh / 2;
                int16_t tipY = by + (bh - tipH) / 2;
                int16_t tipX = bx + bwCase + 1;
                t->fillRoundRect(tipX, tipY, noseW, tipH, 1, frameColor);
            }

            if (_batteryStyle == Numeric)
            {
                String txt = String(_batteryLevel);
                int16_t tw = measureText(txt);
                int16_t tx = bx + (bwCase - tw) / 2;
                if (tx < bx + 1)
                    tx = bx + 1;
                drawTextAt(txt, tx);
            }
            else
            {
                int16_t innerW = bwCase - 4;
                int16_t innerH = bh - 4;
                if (innerW > 0 && innerH > 0)
                {
                    int16_t ix = bx + 2;
                    int16_t iy = by + 2;

                    t->fillRoundRect(ix, iy, innerW, innerH, 2, innerBg);

                    int16_t fillW = (int16_t)((innerW * _batteryLevel) / 100);
                    if (fillW > 0)
                    {
                        uint16_t col = (_batteryLevel <= 20) ? 0xF800 : t->color565(0, 214, 4);
                        t->fillRoundRect(ix, iy, fillW, innerH, 2, col);
                    }
                }
            }

            rightCursor = bx - 4;
        }

        for (int i = (int)_iconRightCount - 1; i >= 0; --i)
        {
            const StatusIcon &ic = _iconsRight[i];
            if (!ic.bitmap || ic.w == 0 || ic.h == 0)
                continue;
            if (rightCursor - (int16_t)ic.w < x + 2)
                break;
            int16_t ixr = rightCursor - ic.w;
            int16_t iy = y + (h - ic.h) / 2;
            if (iy < y)
                iy = y;
            t->pushImage(ixr, iy, ic.w, ic.h, ic.bitmap);
            rightCursor = ixr - 2;
        }

        if (_statusTextRight.length() > 0)
        {
            int16_t tw = measureText(_statusTextRight);
            int16_t rx = rightCursor - tw;
            if (rx < x + 2)
                rx = x + 2;
            drawTextAt(_statusTextRight, rx);
            rightCursor = rx - 4;
        }

        if (_statusTextCenter.length() > 0 || _iconCenterCount > 0)
        {
            int16_t iconsW = 0;
            for (uint8_t i = 0; i < _iconCenterCount; ++i)
            {
                const StatusIcon &ic = _iconsCenter[i];
                if (!ic.bitmap || ic.w == 0)
                    continue;
                if (iconsW > 0)
                    iconsW += 2;
                iconsW += ic.w;
            }

            int16_t textW = measureText(_statusTextCenter);
            int16_t gap = (iconsW > 0 && textW > 0) ? 4 : 0;
            int16_t totalW = iconsW + gap + textW;

            if (totalW > 0)
            {
                int16_t startX = x + (w - totalW) / 2;
                int16_t curX = startX;

                for (uint8_t i = 0; i < _iconCenterCount; ++i)
                {
                    const StatusIcon &ic = _iconsCenter[i];
                    if (!ic.bitmap || ic.w == 0 || ic.h == 0)
                        continue;
                    int16_t iy = y + (h - ic.h) / 2;
                    if (iy < y)
                        iy = y;
                    t->pushImage(curX, iy, ic.w, ic.h, ic.bitmap);
                    curX += ic.w + 2;
                }

                if (textW > 0)
                {
                    if (curX < startX)
                        curX = startX;
                    if (curX + textW > x + w - 2)
                        curX = x + w - 2 - textW;
                    drawTextAt(_statusTextCenter, curX);
                }
            }
        }

        if (_statusBarCustom)
            _statusBarCustom(*this, x, y, w, h);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;
    }

}

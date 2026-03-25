#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

#include <math.h>

namespace pipgui
{
    namespace
    {
        int16_t clampI16(int16_t v, int16_t lo, int16_t hi)
        {
            if (v < lo)
                return lo;
            if (v > hi)
                return hi;
            return v;
        }
    }

    void GUI::showPopupMenuInternal(PopupMenuItemFn itemFn,
                                   void *itemUser,
                                   uint8_t count,
                                   uint8_t selectedIndex,
                                   int16_t x,
                                   int16_t y,
                                   int16_t w,
                                   uint8_t maxVisible)
    {
        if (!itemFn || count == 0)
            return;

        const uint16_t screenW = _render.screenWidth;
        const uint16_t screenH = _render.screenHeight;
        if (screenW == 0 || screenH == 0)
            return;

        _popup.itemFn = itemFn;
        _popup.itemUser = itemUser;
        _popup.count = count;
        _popup.itemHeight = 28;
        _popup.radius = 12;
        _popup.maxVisible = maxVisible ? maxVisible : 1;
        if (_popup.maxVisible > 12)
            _popup.maxVisible = 12;
        {
            // Clamp to what actually fits on screen.
            const int16_t pad = 8;
            const int16_t maxH = (int16_t)screenH - 16; // top/bottom margin
            const int16_t usable = (int16_t)(maxH - pad * 2);
            if (usable > 0)
            {
                const uint8_t fit = (uint8_t)max<int16_t>(1, usable / (int16_t)_popup.itemHeight);
                if (_popup.maxVisible > fit)
                    _popup.maxVisible = fit;
            }
            else
            {
                _popup.maxVisible = 1;
            }
        }

        if (selectedIndex >= count)
            selectedIndex = 0;
        _popup.selectedIndex = selectedIndex;
        _popup.resultIndex = -1;
        _popup.resultReady = false;
        _popup.nextHoldStartMs = 0;
        _popup.prevHoldStartMs = 0;
        _popup.nextLongFired = false;
        _popup.prevLongFired = false;
        _popup.lastNextDown = false;
        _popup.lastPrevDown = false;

        const uint8_t visibleCount = (count < _popup.maxVisible) ? count : _popup.maxVisible;
        const int16_t pad = 8;
        const int16_t fullH = (int16_t)(pad * 2 + visibleCount * _popup.itemHeight);

        if (w <= 0)
            w = (int16_t)((screenW - 16u) < 220u ? (screenW - 16u) : 220u);
        if (w < 120)
            w = 120;
        if (w > (int16_t)screenW - 16)
            w = (int16_t)screenW - 16;

        const int16_t minX = 8;
        const int16_t minY = 8;
        const int16_t maxX = (int16_t)screenW - 8 - w;
        const int16_t maxY = (int16_t)screenH - 8 - fullH;
        x = clampI16(x, minX, maxX);
        y = clampI16(y, minY, maxY);

        _popup.x = x;
        _popup.y = y;
        _popup.w = w;
        _popup.scrollIndex = 0;
        if (selectedIndex >= visibleCount)
            _popup.scrollIndex = (uint8_t)(selectedIndex - visibleCount + 1);

        _popup.bg565 = rgb(18, 18, 18);
        _popup.border565 = (uint16_t)detail::blend565(_popup.bg565, (uint16_t)0xFFFF, 18);
        _popup.selBg565 = rgb(40, 150, 255);
        _popup.fg565 = 0xFFFF;

        _popup.startMs = nowMs();
        _popup.animDurationMs = 220;

        _flags.popupActive = 1;
        _flags.popupClosing = 0;
        requestRedraw();
    }

    void GUI::handlePopupMenuInput(bool nextDown, bool prevDown)
    {
        if (!_flags.popupActive || _flags.popupClosing || _popup.count == 0)
            return;

        const uint32_t now = nowMs();
        bool changed = false;
        const uint32_t holdMs = 400;

        const auto adjustScroll = [&]()
        {
            const uint8_t visibleCount = (_popup.count < _popup.maxVisible) ? _popup.count : _popup.maxVisible;
            if (visibleCount == 0)
                return;
            if (_popup.selectedIndex < _popup.scrollIndex)
                _popup.scrollIndex = _popup.selectedIndex;
            else if (_popup.selectedIndex >= (uint8_t)(_popup.scrollIndex + visibleCount))
                _popup.scrollIndex = (uint8_t)(_popup.selectedIndex - visibleCount + 1);
        };

        if (nextDown)
        {
            if (!_popup.lastNextDown)
            {
                _popup.nextHoldStartMs = now;
                _popup.nextLongFired = false;
            }
            else if (!_popup.nextLongFired && _popup.nextHoldStartMs && (now - _popup.nextHoldStartMs) >= holdMs)
            {
                _popup.resultIndex = (int16_t)_popup.selectedIndex;
                _popup.resultReady = true;
                _flags.popupClosing = 1;
                _popup.startMs = now;
                _popup.nextLongFired = true;
                requestRedraw();
                return;
            }
        }
        else
        {
            if (_popup.lastNextDown && !_popup.nextLongFired)
            {
                if (_popup.selectedIndex + 1 < _popup.count)
                    _popup.selectedIndex++;
                else
                    _popup.selectedIndex = 0;
                changed = true;
            }
            _popup.nextHoldStartMs = 0;
            _popup.nextLongFired = false;
        }

        if (prevDown)
        {
            if (!_popup.lastPrevDown)
            {
                _popup.prevHoldStartMs = now;
                _popup.prevLongFired = false;
            }
            else if (!_popup.prevLongFired && _popup.prevHoldStartMs && (now - _popup.prevHoldStartMs) >= holdMs)
            {
                _flags.popupClosing = 1;
                _popup.startMs = now;
                _popup.prevLongFired = true;
                requestRedraw();
                return;
            }
        }
        else
        {
            if (_popup.lastPrevDown && !_popup.prevLongFired)
            {
                if (_popup.selectedIndex > 0)
                    _popup.selectedIndex--;
                else
                    _popup.selectedIndex = (uint8_t)(_popup.count - 1);
                changed = true;
            }
            _popup.prevHoldStartMs = 0;
            _popup.prevLongFired = false;
        }

        _popup.lastNextDown = nextDown;
        _popup.lastPrevDown = prevDown;

        if (changed)
        {
            adjustScroll();
            requestRedraw();
        }
    }

    void GUI::renderPopupMenuOverlay(uint32_t now)
    {
        if (!_flags.popupActive || _popup.count == 0 || !_popup.itemFn)
            return;

        uint32_t dur = _popup.animDurationMs ? _popup.animDurationMs : 1;
        uint32_t elapsed = now - _popup.startMs;
        if (elapsed > dur)
            elapsed = dur;

        const float t = (float)elapsed / (float)dur;
        const float eased = detail::motion::cubicBezierEase(t, 0.16f, 1.0f, 0.30f, 1.0f);
        const float visualP = _flags.popupClosing ? (1.0f - eased) : eased;

        if (_flags.popupClosing && elapsed >= dur)
        {
            _flags.popupActive = 0;
            _flags.popupClosing = 0;
            requestRedraw();
            return;
        }
        if (!_flags.popupClosing && elapsed < dur)
        {
            requestRedraw();
        }

        const uint8_t visibleCount = (_popup.count < _popup.maxVisible) ? _popup.count : _popup.maxVisible;
        if (visibleCount == 0)
            return;

        const int16_t pad = 8;
        const int16_t fullH = (int16_t)(pad * 2 + visibleCount * _popup.itemHeight);

        int16_t x = _popup.x;
        int16_t y = _popup.y;
        int16_t w = _popup.w;
        int16_t h = fullH;

        const int16_t revealH = (int16_t)lroundf((float)h * visualP);
        if (revealH <= 0)
            return;

        // Slight slide for a nicer "drop" feel.
        const int16_t slideY = (int16_t)lroundf((1.0f - visualP) * 6.0f);
        y = (int16_t)(y - slideY);

        auto *target = getDrawTarget();
        if (!target)
            return;

        const ClipState prevGuiClip = _clip;
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        target->getClipRect(&clipX, &clipY, &clipW, &clipH);
        applyClip(x, y, w, revealH);
        target->setClipRect(x, y, w, revealH);

        const uint16_t shadow565 = (uint16_t)detail::blend565((uint16_t)0x0000, _popup.bg565, 80);
        fillRoundRect((int16_t)(x + 2), (int16_t)(y + 2), w, h, _popup.radius, shadow565);
        fillRoundRect(x, y, w, h, _popup.radius, _popup.bg565);
        drawRoundRect(x, y, w, h, _popup.radius, _popup.border565);

        setTextStyle(Body);
        for (uint8_t i = 0; i < visibleCount; ++i)
        {
            const uint8_t idx = (uint8_t)(_popup.scrollIndex + i);
            if (idx >= _popup.count)
                break;

            const int16_t iy = (int16_t)(y + pad + i * _popup.itemHeight);
            const bool sel = (idx == _popup.selectedIndex);
            if (sel)
            {
                fillRoundRect((int16_t)(x + 4), (int16_t)(iy + 2), (int16_t)(w - 8), (int16_t)(_popup.itemHeight - 4), 8, _popup.selBg565);
            }

            const char *label = _popup.itemFn(_popup.itemUser, idx);
            if (!label)
                label = "";

            const uint16_t fg = sel ? 0xFFFF : _popup.fg565;
            const uint16_t bg = sel ? _popup.selBg565 : _popup.bg565;
            (void)drawTextEllipsized(String(label), (int16_t)(x + pad), (int16_t)(iy + (_popup.itemHeight / 2) - 7),
                                     (int16_t)(w - pad * 2), fg, Left);
        }

        _clip = prevGuiClip;
        target->setClipRect(clipX, clipY, clipW, clipH);
    }
}

#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/API/Builders/Base.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
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

        int16_t resolveAxis(int16_t v, int16_t extent, int16_t total)
        {
            return v == center ? static_cast<int16_t>((total - extent) / 2) : v;
        }

        uint8_t resolvePopupVisibleCount(uint8_t count, uint8_t maxVisible, uint16_t screenH, uint8_t itemHeight)
        {
            uint8_t resolved = maxVisible ? maxVisible : 4;
            if (resolved > 4)
                resolved = 4;

            const int16_t pad = 8;
            const int16_t usable = static_cast<int16_t>(screenH) - 16 - pad * 2;
            if (usable <= 0)
                return 1;

            const uint8_t fit = static_cast<uint8_t>(max<int16_t>(1, usable / itemHeight));
            if (resolved > fit)
                resolved = fit;
            return (count < resolved) ? count : resolved;
        }

        int16_t resolvePopupWidth(uint16_t screenW, int16_t requestedW)
        {
            int16_t w = requestedW;
            if (w <= 0)
                w = static_cast<int16_t>(((screenW - 16u) < 220u) ? (screenW - 16u) : 220u);
            if (w < 120)
                w = 120;
            const int16_t maxW = static_cast<int16_t>(screenW) - 16;
            return (w > maxW) ? maxW : w;
        }

        void resolvePopupPlacement(uint16_t screenW, uint16_t screenH,
                                   int16_t anchorX, int16_t anchorY,
                                   int16_t anchorW, int16_t anchorH,
                                   int16_t popupW, int16_t popupH,
                                   int16_t &outX, int16_t &outY)
        {
            constexpr int16_t margin = 8;
            constexpr int16_t gap = 8;

            int16_t x = center;
            int16_t y = center;
            if (anchorW > 0 && anchorH > 0)
            {
                const int16_t resolvedAnchorX = resolveAxis(anchorX, anchorW, static_cast<int16_t>(screenW));
                const int16_t resolvedAnchorY = resolveAxis(anchorY, anchorH, static_cast<int16_t>(screenH));
                const int16_t belowY = static_cast<int16_t>(resolvedAnchorY + anchorH + gap);
                const int16_t aboveY = static_cast<int16_t>(resolvedAnchorY - popupH - gap);
                const int16_t freeAbove = static_cast<int16_t>(resolvedAnchorY - margin - gap);
                const int16_t freeBelow = static_cast<int16_t>(static_cast<int16_t>(screenH) - margin - gap - (resolvedAnchorY + anchorH));

                x = static_cast<int16_t>(resolvedAnchorX + anchorW / 2 - popupW / 2);
                if (freeBelow >= popupH)
                    y = belowY;
                else if (freeAbove >= popupH)
                    y = aboveY;
                else
                    y = (freeBelow >= freeAbove) ? belowY : aboveY;
            }

            const int16_t minX = margin;
            const int16_t minY = margin;
            const int16_t maxX = static_cast<int16_t>(screenW) - margin - popupW;
            const int16_t maxY = static_cast<int16_t>(screenH) - margin - popupH;
            outX = clampI16(resolveAxis(x, popupW, static_cast<int16_t>(screenW)), minX, maxX);
            outY = clampI16(resolveAxis(y, popupH, static_cast<int16_t>(screenH)), minY, maxY);
        }

        struct PopupOverlayFrame
        {
            int16_t x = 0;
            int16_t y = 0;
            int16_t w = 0;
            int16_t h = 0;
            int16_t revealH = 0;
            uint8_t visibleCount = 0;
        };

        bool computePopupOverlayFrame(const detail::PopupMenuState &popup, bool popupActive, bool popupClosing, uint32_t now, PopupOverlayFrame &out)
        {
            out = {};
            if (!popupActive || popup.list.itemCount == 0 || !popup.itemFn)
                return false;

            const uint32_t dur = popup.animDurationMs ? popup.animDurationMs : 1;
            uint32_t elapsed = now - popup.startMs;
            if (elapsed > dur)
                elapsed = dur;

            const float t = static_cast<float>(elapsed) / static_cast<float>(dur);
            const float eased = detail::motion::cubicBezierEase(t, 0.16f, 1.0f, 0.30f, 1.0f);
            const float visualP = popupClosing ? (1.0f - eased) : eased;

            const uint8_t visibleCount = (popup.list.itemCount < popup.maxVisible) ? popup.list.itemCount : popup.maxVisible;
            if (visibleCount == 0)
                return false;

            const int16_t pad = 8;
            const int16_t h = static_cast<int16_t>(pad * 2 + visibleCount * popup.itemHeight);
            const int16_t revealH = static_cast<int16_t>(lroundf(static_cast<float>(h) * visualP));
            if (revealH <= 0)
                return false;

            int16_t y = popup.y;
            const int16_t slideY = static_cast<int16_t>(lroundf((1.0f - visualP) * 6.0f));
            y = static_cast<int16_t>(y - slideY);

            out.x = popup.x;
            out.y = y;
            out.w = popup.w;
            out.h = h;
            out.revealH = revealH;
            out.visibleCount = visibleCount;
            return true;
        }

        void resetPopupListRuntime(ListState &list)
        {
            list.selectedIndex = 0;
            list.scrollPos = 0.0f;
            list.targetScroll = 0.0f;
            list.scrollVel = 0.0f;
            list.nextHoldStartMs = 0;
            list.prevHoldStartMs = 0;
            list.nextLongFired = false;
            list.prevLongFired = false;
            list.lastNextDown = false;
            list.lastPrevDown = false;
            list.scrollbarAlpha = 0;
            list.lastScrollActivityMs = 0;
            list.marqueeStartMs = 0;
            list.lastUpdateMs = 0;
        }

        void freePopupItems(ListState &list, pipcore::Platform *plat)
        {
            if (!list.items)
                return;
            for (uint8_t i = 0; i < list.capacity; ++i)
                list.items[i].~Item();
            detail::free(plat, list.items);
            list.items = nullptr;
            list.capacity = 0;
            list.itemCount = 0;
            list.configured = false;
        }

        bool ensurePopupItems(ListState &list, uint8_t count, pipcore::Platform *plat)
        {
            if (count == 0)
            {
                freePopupItems(list, plat);
                return true;
            }
            return detail::ensureCapacity(plat, list.items, list.capacity, count);
        }

        bool assignPopupItem(ListState::Item &item, const char *label)
        {
            if (!detail::assignString(item.title, label))
                return false;
            item.subtitle = String();
            item.targetScreen = INVALID_SCREEN_ID;
            item.iconId = 0xFFFF;
            item.titleW = item.titleH = item.subW = item.subH = 0;
            item.cachedTitlePx = item.cachedSubPx = 0;
            item.cachedTitleWeight = item.cachedSubWeight = 0;
            return true;
        }
    }

    void GUI::freePopupMenu(pipcore::Platform *plat) noexcept
    {
        freePopupItems(_popup.list, plat);
        _popup = {};
    }

    void GUI::showPopupMenuInternal(PopupMenuItemFn itemFn,
                                   void *itemUser,
                                   uint8_t count,
                                   uint8_t selectedIndex,
                                   int16_t w,
                                   uint8_t maxVisible,
                                   int16_t anchorX,
                                   int16_t anchorY,
                                   int16_t anchorW,
                                   int16_t anchorH)
    {
        if (!itemFn || count == 0)
            return;

        const uint16_t screenW = _render.screenWidth;
        const uint16_t screenH = _render.screenHeight;
        if (screenW == 0 || screenH == 0)
            return;

        _popup.itemHeight = 30;
        _popup.radius = 18;
        _popup.bg565 = rgb(18, 18, 18);
        _popup.border565 = (uint16_t)detail::blend565(_popup.bg565, (uint16_t)0xFFFF, 18);
        _popup.selBg565 = rgb(40, 150, 255);
        _popup.fg565 = 0xFFFF;
        const uint8_t resolvedMaxVisible = resolvePopupVisibleCount(count, maxVisible, screenH, _popup.itemHeight);
        _popup.maxVisible = resolvedMaxVisible;

        const int16_t pad = 8;
        const int16_t popupW = resolvePopupWidth(screenW, w);
        const int16_t popupH = static_cast<int16_t>(pad * 2 + resolvedMaxVisible * _popup.itemHeight);
        int16_t popupX = 0;
        int16_t popupY = 0;
        resolvePopupPlacement(screenW, screenH, anchorX, anchorY, anchorW, anchorH, popupW, popupH, popupX, popupY);

        const bool sameMenu = _flags.popupActive &&
                              !_flags.popupClosing &&
                              _popup.itemFn == itemFn &&
                              _popup.itemUser == itemUser &&
                              _popup.list.itemCount == count;

        ListState &list = _popup.list;
        if (!ensurePopupItems(list, count, platform()))
            return;
        for (uint8_t i = 0; i < count; ++i)
        {
            if (!assignPopupItem(list.items[i], itemFn(itemUser, i)))
                return;
        }

        _popup.itemFn = itemFn;
        _popup.itemUser = itemUser;
        list.itemCount = count;
        list.style = {_popup.bg565, _popup.selBg565, 14, 0, 0, _popup.itemHeight, 14, 0, 0, Plain};

        if (sameMenu)
        {
            const bool moved = _popup.x != popupX || _popup.y != popupY || _popup.w != popupW || _popup.maxVisible != resolvedMaxVisible;
            _popup.maxVisible = resolvedMaxVisible;
            _popup.x = popupX;
            _popup.y = popupY;
            _popup.w = popupW;
            if (moved)
                requestRedraw();
            return;
        }

        resetPopupListRuntime(list);
        list.configured = true;
        list.checkedIndex = 0xFF;
        list.checkedIconId = IconCheckmark;

        if (_popup.rememberedItemFn == itemFn && _popup.rememberedItemUser == itemUser && _popup.rememberedCount == count && _popup.rememberedIndex < count)
            list.checkedIndex = _popup.rememberedIndex;

        if (selectedIndex == 0xFF)
            selectedIndex = (list.checkedIndex < count) ? list.checkedIndex : 0;
        else if (selectedIndex >= count)
            selectedIndex = 0;
        list.selectedIndex = selectedIndex;

        _popup.resultIndex = -1;
        _popup.resultReady = false;
        _popup.inputArmed = false;
        _popup.x = popupX;
        _popup.y = popupY;
        _popup.w = popupW;

        _popup.startMs = nowMs();
        _popup.animDurationMs = 220;
        _popup.lastRectValid = false;

        _flags.popupActive = 1;
        _flags.popupClosing = 0;
        requestRedraw();
    }

    void GUI::handlePopupMenuInput(bool nextDown, bool prevDown)
    {
        if (!_flags.popupActive || _flags.popupClosing || _popup.list.itemCount == 0)
            return;

        const uint32_t now = nowMs();
        ListState &list = _popup.list;
        if (!_popup.inputArmed)
        {
            const uint32_t readyAfter = _popup.animDurationMs ? _popup.animDurationMs : 1;
            list.lastNextDown = nextDown;
            list.lastPrevDown = prevDown;
            if ((now - _popup.startMs) < readyAfter || nextDown || prevDown)
                return;
            _popup.inputArmed = true;
            list.lastNextDown = false;
            list.lastPrevDown = false;
            return;
        }

        bool changed = false;
        const uint32_t holdMs = 400;

        if (nextDown)
        {
            if (!list.lastNextDown)
            {
                list.nextHoldStartMs = now;
                list.nextLongFired = false;
            }
            else if (!list.nextLongFired && list.nextHoldStartMs && (now - list.nextHoldStartMs) >= holdMs)
            {
                _popup.rememberedItemFn = _popup.itemFn;
                _popup.rememberedItemUser = _popup.itemUser;
                _popup.rememberedCount = list.itemCount;
                _popup.rememberedIndex = list.selectedIndex;
                list.checkedIndex = list.selectedIndex;
                _popup.resultIndex = (int16_t)list.selectedIndex;
                _popup.resultReady = true;
                _flags.popupClosing = 1;
                _popup.startMs = now;
                list.nextLongFired = true;
                requestRedraw();
                return;
            }
        }
        else
        {
            if (list.lastNextDown && !list.nextLongFired)
            {
                if (list.selectedIndex + 1 < list.itemCount)
                    list.selectedIndex++;
                else
                    list.selectedIndex = 0;
                changed = true;
            }
            list.nextHoldStartMs = 0;
            list.nextLongFired = false;
        }

        if (prevDown)
        {
            if (!list.lastPrevDown)
            {
                list.prevHoldStartMs = now;
                list.prevLongFired = false;
            }
            else if (!list.prevLongFired && list.prevHoldStartMs && (now - list.prevHoldStartMs) >= holdMs)
            {
                _flags.popupClosing = 1;
                _popup.startMs = now;
                list.prevLongFired = true;
                requestRedraw();
                return;
            }
        }
        else
        {
            if (list.lastPrevDown && !list.prevLongFired)
            {
                if (list.selectedIndex > 0)
                    list.selectedIndex--;
                else
                    list.selectedIndex = (uint8_t)(list.itemCount - 1);
                changed = true;
            }
            list.prevHoldStartMs = 0;
            list.prevLongFired = false;
        }

        list.lastNextDown = nextDown;
        list.lastPrevDown = prevDown;

        if (changed)
        {
            list.scrollbarAlpha = 255;
            list.lastScrollActivityMs = now;
            list.marqueeStartMs = now;
            requestRedraw();
        }
    }

    void GUI::renderPopupMenuOverlay(uint32_t now)
    {
        if (!_flags.popupActive || _popup.list.itemCount == 0 || !_popup.itemFn)
            return;

        const uint32_t dur = _popup.animDurationMs ? _popup.animDurationMs : 1;
        uint32_t elapsed = now - _popup.startMs;
        if (elapsed > dur)
            elapsed = dur;

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

        PopupOverlayFrame frame = {};
        if (!computePopupOverlayFrame(_popup, _flags.popupActive, _flags.popupClosing, now, frame))
            return;

        auto *target = getDrawTarget();
        if (!target)
            return;

        const ClipState prevGuiClip = _clip;
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        target->getClipRect(&clipX, &clipY, &clipW, &clipH);
        applyClip(frame.x, frame.y, frame.w, frame.revealH);
        target->setClipRect(frame.x, frame.y, frame.w, frame.revealH);

        const uint16_t shadow565 = (uint16_t)detail::blend565((uint16_t)0x0000, _popup.bg565, 80);
        drawSquircleRect()
            .pos((int16_t)(frame.x + 2), (int16_t)(frame.y + 2))
            .size(frame.w, frame.h)
            .radius({_popup.radius})
            .fill(shadow565);
        drawSquircleRect()
            .pos(frame.x, frame.y)
            .size(frame.w, frame.h)
            .radius({_popup.radius})
            .fill(_popup.bg565)
            .border(1, _popup.border565);
        renderListState(_popup.list, frame.x + 8, frame.y + 8, frame.w - 16, frame.visibleCount * _popup.itemHeight, _popup.bg565, false);

        _clip = prevGuiClip;
        target->setClipRect(clipX, clipY, clipW, clipH);
    }

    bool GUI::computePopupBounds(uint32_t now, DirtyRect &outRect)
    {
        outRect = {0, 0, 0, 0};
        PopupOverlayFrame frame = {};
        if (!computePopupOverlayFrame(_popup, _flags.popupActive, _flags.popupClosing, now, frame))
            return false;

        int16_t x1 = frame.x;
        int16_t y1 = frame.y;
        int16_t x2 = static_cast<int16_t>(frame.x + frame.w);
        int16_t y2 = static_cast<int16_t>(frame.y + frame.revealH);

        if (x1 < 0)
            x1 = 0;
        if (y1 < 0)
            y1 = 0;
        if (x2 > (int16_t)_render.screenWidth)
            x2 = (int16_t)_render.screenWidth;
        if (y2 > (int16_t)_render.screenHeight)
            y2 = (int16_t)_render.screenHeight;

        outRect.x = x1;
        outRect.y = y1;
        outRect.w = x2 - x1;
        outRect.h = y2 - y1;
        return outRect.w > 0 && outRect.h > 0;
    }
}

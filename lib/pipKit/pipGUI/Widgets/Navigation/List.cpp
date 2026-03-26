#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <math.h>
namespace pipgui
{
    namespace
    {
        constexpr uint8_t kDefaultListRadius = 17;
    }

    static void resetListItemCache(ListState::Item &item)
    {
        item.titleW = item.titleH = item.subW = item.subH = 0;
        item.cachedTitlePx = item.cachedSubPx = 0;
        item.cachedTitleWeight = item.cachedSubWeight = 0;
    }

    static void resetListRuntime(ListState &menu)
    {
        menu.selectedIndex = 0;
        menu.scrollPos = menu.targetScroll = menu.scrollVel = 0.0f;
        menu.nextHoldStartMs = menu.prevHoldStartMs = 0;
        menu.nextLongFired = menu.prevLongFired = false;
        menu.lastNextDown = menu.lastPrevDown = false;
        menu.scrollbarAlpha = 0;
        menu.marqueeStartMs = 0;
        menu.lastScrollActivityMs = menu.lastUpdateMs = 0;
    }

    static bool initListItem(ListState::Item &item, const ListItemDef &def)
    {
        if (!detail::assignString(item.title, def.title) || !detail::assignString(item.subtitle, def.subtitle))
            return false;

        item.targetScreen = def.targetScreen;
        item.iconId = def.iconId;
        resetListItemCache(item);
        return true;
    }

    static void destroyList(ListState &menu, pipcore::Platform *plat)
    {
        if (menu.items)
        {
            for (uint8_t i = 0; i < menu.capacity; ++i)
                menu.items[i].~Item();
            detail::free(plat, menu.items);
        }
        menu = {};
    }

    static bool ensureListCapacity(ListState &menu, uint8_t newCapacity, pipcore::Platform *plat)
    {
        if (newCapacity == 0)
        {
            destroyList(menu, plat);
            return true;
        }

        return detail::ensureCapacity(plat, menu.items, menu.capacity, newCapacity);
    }

    void ConfigureListFluent::apply()
    {
        if (_consumed || !_gui || !_items || _itemCount == 0)
            return;
        _consumed = true;

        if (_screenId == INVALID_SCREEN_ID)
            return;
        ListState *menu = detail::GuiAccess::ensureList(*_gui, _screenId);
        if (!menu)
            return;
        if (!ensureListCapacity(*menu, _itemCount, detail::GuiAccess::platform(*_gui)))
        {
            menu->configured = false;
            return;
        }

        menu->configured = true;
        menu->itemCount = _itemCount;
        resetListRuntime(*menu);

        for (uint8_t i = 0; i < _itemCount; ++i)
        {
            if (!initListItem(menu->items[i], _items[i]))
            {
                menu->configured = false;
                return;
            }
        }

        menu->style = {_cardColor, _cardActiveColor, _radius, 6,
                       _cardWidth, _cardHeight, 0, 0, 0, _mode};
        menu->checkedIndex = _checkedIndex;
        menu->checkedIconId = (_checkedIndex == 0xFF) ? static_cast<uint16_t>(0xFFFF) : static_cast<uint16_t>(IconCheckmark);

        if (menu->style.cardColor == 0 || menu->style.cardActiveColor == 0)
        {
            menu->style.cardColor = (uint16_t)detail::blend565(0x0000, 0xFFFF, 18);
            menu->style.cardActiveColor = (uint16_t)(((130 >> 2) << 5) | (220 >> 3));
        }
    }

    void GUI::handleListInput(uint8_t screenId, bool nextDown, bool prevDown)
    {
        ListState *menuPtr = getList(screenId);
        if (!menuPtr)
            return;
        ListState &menu = *menuPtr;
        if (!menu.configured || menu.itemCount == 0)
            return;

#if PIPGUI_SCREENSHOTS
        if (nextDown && prevDown)
        {
            menu.nextHoldStartMs = 0;
            menu.prevHoldStartMs = 0;
            menu.nextLongFired = false;
            menu.prevLongFired = false;
            menu.lastNextDown = false;
            menu.lastPrevDown = false;
            return;
        }
#endif

        const uint32_t now = nowMs();
        bool changed = false;
        const uint32_t holdMs = 400;

        if (nextDown)
        {
            if (!menu.lastNextDown)
            {
                menu.nextHoldStartMs = now;
                menu.nextLongFired = false;
            }
            else if (!menu.nextLongFired && menu.nextHoldStartMs && (now - menu.nextHoldStartMs) >= holdMs)
            {
                if (menu.selectedIndex < menu.itemCount)
                {
                    uint8_t target = menu.items[menu.selectedIndex].targetScreen;
                    if (target != INVALID_SCREEN_ID)
                        activateScreenId(target, 1);
                }
                menu.nextLongFired = true;
            }
        }
        else
        {
            if (menu.lastNextDown && !menu.nextLongFired)
            {
                if (menu.selectedIndex + 1 < menu.itemCount)
                    menu.selectedIndex++;
                else
                    menu.selectedIndex = 0;
                changed = true;
            }

            menu.nextHoldStartMs = 0;
            menu.nextLongFired = false;
        }

        if (prevDown)
        {
            if (!menu.lastPrevDown)
            {
                menu.prevHoldStartMs = now;
                menu.prevLongFired = false;
            }
            else if (!menu.prevLongFired && menu.prevHoldStartMs && (now - menu.prevHoldStartMs) >= holdMs)
            {
                prevScreen();
                menu.prevLongFired = true;
            }
        }
        else
        {
            if (menu.lastPrevDown && !menu.prevLongFired)
            {
                if (menu.selectedIndex > 0)
                    menu.selectedIndex--;
                else
                    menu.selectedIndex = menu.itemCount - 1;
                changed = true;
            }

            menu.prevHoldStartMs = 0;
            menu.prevLongFired = false;
        }

        menu.lastNextDown = nextDown;
        menu.lastPrevDown = prevDown;

        if (changed)
        {
            menu.scrollbarAlpha = 255;
            menu.lastScrollActivityMs = now;
            menu.marqueeStartMs = now;
            if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass && _screen.current == screenId)
                updateList(screenId);
            else
                requestRedraw();
        }
    }

    bool GUI::updateList(uint8_t screenId)
    {
        if (screenId == INVALID_SCREEN_ID)
            return false;

        int16_t top = 0;
        int16_t height = (int16_t)_render.screenHeight;
        const int16_t sb = statusBarHeight();
        if (sb > 0)
        {
            if (_status.pos == Top)
                top += sb;
            else if (_status.pos == Bottom)
                height -= sb;
        }

        const int16_t contentHeight = (int16_t)(height - top);
        if (_render.screenWidth <= 0 || contentHeight <= 0)
            return false;

        return updateList(screenId, 0, top, (int16_t)_render.screenWidth, contentHeight, _render.bgColor565);
    }

    bool GUI::updateList(uint8_t screenId, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bgColor565)
    {
        ListState *menuPtr = getList(screenId);
        if (!menuPtr || !menuPtr->configured || menuPtr->itemCount == 0 || w <= 0 || h <= 0)
            return false;
        return renderListState(*menuPtr, x, y, w, h, bgColor565, true);
    }

    bool GUI::renderListState(ListState &menu, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bgColor565, bool invalidate)
    {
        if (!menu.configured || menu.itemCount == 0 || w <= 0 || h <= 0)
            return false;

        auto drawList = [&]()
        {
            auto *target = getDrawTarget();
            const uint32_t now = nowMs();
            setTextStyle(Body);
            if (menu.marqueeStartMs == 0)
                menu.marqueeStartMs = now;

            uint32_t dtMs = 0;
            if (menu.lastUpdateMs != 0)
                dtMs = (now >= menu.lastUpdateMs) ? (now - menu.lastUpdateMs) : 0;
            menu.lastUpdateMs = now;
            if (dtMs > 100)
                dtMs = 100;

            const int16_t left = x;
            const int16_t top = y;
            const int16_t right = left + w;
            const int16_t bottom = top + h;
            const int16_t padY = (int16_t)menu.style.spacing;
            const int16_t contentTop = top + padY;
            const int16_t contentBottom = bottom - padY;
            if (contentBottom <= contentTop)
                return;

            const bool cardMode = (menu.style.mode == Cards);
            const int16_t marginX = 1;
            int16_t contentRight = right;
            int16_t cardW = (menu.style.cardWidth > 0 && menu.style.cardWidth < w)
                                ? menu.style.cardWidth
                                : (int16_t)(w - marginX * 2);
            if (cardW < 20)
                cardW = 20;

            int16_t cardH = (menu.style.cardHeight > 0) ? menu.style.cardHeight : (cardMode ? 50 : 34);
            if (menu.style.cardHeight <= 0 && cardH * 2 + menu.style.spacing > h)
                cardH = h / 3;
            int16_t cardX = left + (w - cardW) / 2;

            const ClipState prevGuiClip = _clip;
            int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
            target->getClipRect(&clipX, &clipY, &clipW, &clipH);
            applyClip(left, top, w, h);
            target->setClipRect(left, top, w, h);

            int32_t bgL = left;
            int32_t bgT = top;
            int32_t bgR = right;
            int32_t bgB = bottom;

            if (clipW > 0 && clipH > 0)
            {
                int32_t cR = clipX + clipW;
                int32_t cB = clipY + clipH;

                if (bgL < clipX)
                    bgL = clipX;
                if (bgT < clipY)
                    bgT = clipY;
                if (bgR > cR)
                    bgR = cR;
                if (bgB > cB)
                    bgB = cB;
            }

            const int32_t bgW = bgR - bgL;
            const int32_t bgH = bgB - bgT;
            if (bgW > 0 && bgH > 0)
                target->fillRect((int16_t)bgL, (int16_t)bgT, (int16_t)bgW, (int16_t)bgH, bgColor565);

            applyClip(left, contentTop, w, contentBottom - contentTop);
            target->setClipRect(left, contentTop, w, contentBottom - contentTop);

            int16_t visibleHeight = contentBottom - contentTop;
            if (visibleHeight < cardH)
                visibleHeight = cardH;

            const float itemSpan = (float)(cardH + menu.style.spacing);
            uint8_t visibleCount = 1;
            if (itemSpan > 1.0f)
            {
                if (visibleHeight > cardH)
                {
                    const int16_t extra = (int16_t)(visibleHeight - cardH);
                    visibleCount = (uint8_t)(1 + (uint8_t)(extra / (int16_t)itemSpan));
                }
                if (visibleCount > menu.itemCount)
                    visibleCount = menu.itemCount;
            }

            float maxScroll = 0.0f;
            if (itemSpan > 0.5f)
            {
                const float contentHeightPx = (float)menu.itemCount * itemSpan - (float)menu.style.spacing;
                const float maxScrollPx = contentHeightPx - (float)visibleHeight;
                if (maxScrollPx > 0.0f)
                    maxScroll = maxScrollPx / itemSpan;
            }

            const bool hasScrollbar = menu.itemCount > visibleCount;
            if (hasScrollbar)
            {
                constexpr int16_t scrollbarWidth = 3;
                constexpr int16_t scrollbarGap = 2;
                contentRight = right - (scrollbarWidth + scrollbarGap);
                if (menu.style.cardWidth <= 0)
                {
                    cardX = left + marginX;
                    cardW = (int16_t)(contentRight - cardX);
                    if (cardW < 20)
                        cardW = 20;
                }
                else
                {
                    cardX = (int16_t)(contentRight - cardW);
                    const int16_t minCardX = left + marginX;
                    if (cardX < minCardX)
                        cardX = minCardX;
                }
            }

            float targetTop = menu.targetScroll;
            const float viewTop = menu.scrollPos;
            const float viewBottom = viewTop + (float)visibleCount;
            const float idxTop = (float)menu.selectedIndex;
            const float idxBottom = idxTop + 1.0f;

            if (idxBottom > viewBottom)
                targetTop = idxBottom - (float)visibleCount;
            else if (idxTop < viewTop)
                targetTop = idxTop;
            else
                targetTop = viewTop;

            if (targetTop < 0.0f)
                targetTop = 0.0f;
            if (targetTop > maxScroll)
                targetTop = maxScroll;

            if (maxScroll > 0.1f)
            {
                const float bounceOvershoot = 0.7f;
                const bool bounceDown = (targetTop >= maxScroll - 0.01f) && (menu.scrollVel > 7.0f);
                const bool bounceUp = (targetTop <= 0.01f) && (menu.scrollVel < -7.0f);

                if (bounceDown)
                    targetTop = maxScroll + bounceOvershoot;
                else if (bounceUp)
                    targetTop = -bounceOvershoot;
            }

            menu.targetScroll = targetTop;
            const float diff = menu.targetScroll - menu.scrollPos;

            if (fabsf(diff) > 0.0005f)
            {
                const float dtSec = (float)dtMs * 0.001f;

                const float omega = 12.0f;
                const float xVal = omega * dtSec;
                const float expVal = 1.0f / (1.0f + xVal + 0.48f * xVal * xVal + 0.235f * xVal * xVal * xVal);

                const float change = menu.scrollPos - menu.targetScroll;
                const float temp = (menu.scrollVel + omega * change) * dtSec;
                menu.scrollVel = (menu.scrollVel - omega * temp) * expVal;
                menu.scrollPos = menu.targetScroll + (change + temp) * expVal;

                if (fabsf(menu.scrollPos - menu.targetScroll) < 0.005f && fabsf(menu.scrollVel) < 0.2f)
                {
                    menu.scrollPos = menu.targetScroll;
                    menu.scrollVel = 0.0f;
                }

                const float maxAllowed = maxScroll + 2.0f;
                if (menu.scrollPos < -2.0f)
                    menu.scrollPos = -2.0f;
                if (menu.scrollPos > maxAllowed)
                    menu.scrollPos = maxAllowed;

                menu.scrollbarAlpha = 255;
                menu.lastScrollActivityMs = now;
                requestRedraw();
            }
            else
            {
                menu.scrollVel = 0.0f;
                menu.scrollPos = menu.targetScroll;
            }

            float renderScrollPos = menu.scrollPos;
            if (renderScrollPos < 0.0f)
                renderScrollPos = 0.0f;
            if (renderScrollPos > maxScroll)
                renderScrollPos = maxScroll;

            const float overscroll = menu.scrollPos - renderScrollPos;
            float overscrollPx = 0.0f;
            if (fabsf(overscroll) > 0.0001f)
            {
                const float mag = fabsf(overscroll);
                const float rubber = mag / (1.0f + mag);
                const float sign = (overscroll < 0.0f) ? -1.0f : 1.0f;
                const float maxVisualPx = 0.85f;
                overscrollPx = -sign * (rubber * (maxVisualPx * itemSpan));
            }

            uint8_t radius = menu.style.radius;
            if (radius == 0)
                radius = kDefaultListRadius;
            else if (radius < 2)
                radius = 2;

            const int16_t lastIdx = (int16_t)menu.itemCount - 1;
            int16_t startIndex = (int16_t)floorf(renderScrollPos) - 3;
            if (startIndex < 0)
                startIndex = 0;
            if (startIndex > lastIdx)
                startIndex = lastIdx;
            int16_t endIndex = startIndex + (int16_t)visibleCount + 6;
            if (endIndex > lastIdx)
                endIndex = lastIdx;

            auto setTextFont = [&](uint16_t weight, uint16_t px)
            {
                setFontWeight(weight);
                setFontSize(px);
            };

            auto ensureTextMetrics = [&](ListState::Item &item,
                                         uint16_t titlePx, uint16_t titleWeight,
                                         bool hasSub, uint16_t subPx, uint16_t subWeight)
            {
                if (item.cachedTitlePx != titlePx || item.cachedTitleWeight != titleWeight)
                {
                    setTextFont(titleWeight, titlePx);
                    measureText(item.title, item.titleW, item.titleH);
                    item.cachedTitlePx = titlePx;
                    item.cachedTitleWeight = titleWeight;
                }

                if (hasSub && subPx > 0)
                {
                    if (item.cachedSubPx != subPx || item.cachedSubWeight != subWeight)
                    {
                        setTextFont(subWeight, subPx);
                        measureText(item.subtitle, item.subW, item.subH);
                        item.cachedSubPx = subPx;
                        item.cachedSubWeight = subWeight;
                    }
                }
                else
                {
                    item.subW = item.subH = 0;
                    item.cachedSubPx = item.cachedSubWeight = 0;
                }
            };

            auto drawItemIcon = [&](uint16_t iconId, int16_t iconX, int16_t itemY,
                                    int16_t iconSize, int16_t gap,
                                    uint16_t fg, uint16_t bg, bool roundY) -> int16_t
            {
                if (iconId == 0xFFFF || iconId >= psdf_icons::IconCount)
                    return 0;

                const int16_t iconY = roundY
                                          ? itemY + (int16_t)lroundf((float)(cardH - iconSize) * 0.5f)
                                          : itemY + (cardH - iconSize) / 2;

                drawIcon()
                    .pos(iconX, iconY)
                    .size((uint16_t)iconSize)
                    .icon(iconId)
                    .color(fg)
                    .bgColor(bg)
                    .draw();
                return (int16_t)(iconSize + gap);
            };

            constexpr uint16_t TITLE_WEIGHT = 600;
            constexpr uint16_t SUBTITLE_WEIGHT = 500;
            const MarqueeTextOptions marqueeOpts(28, 700, menu.marqueeStartMs);
            const auto drawTextLine = [&](const String &text,
                                          int16_t textX,
                                          float textY,
                                          int16_t textMaxWidth,
                                          uint16_t fg,
                                          uint16_t bg,
                                          bool active)
            {
                const float prevSubX = _typo.subpixelOffsetX;
                const float prevSubY = _typo.subpixelOffsetY;
                const int16_t textYInt = (int16_t)floorf(textY);
                _typo.subpixelOffsetX = 0.0f;
                _typo.subpixelOffsetY = textY - (float)textYInt;
                if (active)
                {
                    if (!drawTextMarquee(text, textX, textYInt, textMaxWidth, fg, TextAlign::Left, marqueeOpts))
                        drawTextAligned(text, textX, textYInt, fg, bg, TextAlign::Left);
                    _typo.subpixelOffsetX = prevSubX;
                    _typo.subpixelOffsetY = prevSubY;
                    return;
                }

                if (!drawTextEllipsized(text, textX, textYInt, textMaxWidth, fg, TextAlign::Left))
                    drawTextAligned(text, textX, textYInt, fg, bg, TextAlign::Left);
                _typo.subpixelOffsetX = prevSubX;
                _typo.subpixelOffsetY = prevSubY;
            };

            const uint16_t inactiveTextColor = detail::autoTextColor(bgColor565);
            const int16_t textClipX = cardX + 6;
            const int16_t textClipW = cardW - 12;
            const bool hasCheckedMarker = (menu.checkedIndex < menu.itemCount) && menu.checkedIconId != 0xFFFF;
            const int16_t trailingIconSize = (cardH > 40) ? 18 : 16;
            const int16_t trailingPad = hasCheckedMarker ? (trailingIconSize + 12) : 0;

            for (int16_t itemIndex = startIndex; itemIndex <= endIndex; ++itemIndex)
            {
                const uint8_t i = (uint8_t)itemIndex;
                const float relIndex = (float)itemIndex - renderScrollPos;
                const float posY = (float)contentTop + relIndex * itemSpan + overscrollPx;
                const int16_t itemY = (int16_t)floorf(posY);

                if (itemY >= contentBottom || (itemY + cardH) <= contentTop)
                    continue;

                ListState::Item &item = menu.items[i];
                const bool active = (i == menu.selectedIndex);
                const bool checked = (i == menu.checkedIndex) && menu.checkedIconId != 0xFFFF;
                const uint16_t bg = active ? menu.style.cardActiveColor : (cardMode ? menu.style.cardColor : bgColor565);
                const uint16_t textColor = detail::autoTextColor(bg);

                if (!cardMode)
                {
                    if (active)
                        drawSquircleRect().pos(cardX, itemY).size(cardW, cardH).radius({radius}).fill(bg);

                    const int16_t textOffsetX = drawItemIcon(item.iconId, cardX + 8, itemY, (cardH > 30) ? 22 : 18, 6,
                                                             active ? textColor : inactiveTextColor,
                                                             active ? bg : bgColor565, false);
                    const int16_t textX = cardX + 12 + textOffsetX;
                    const int16_t textMaxWidth = (cardX + cardW - textX - 10 - trailingPad > 4) ? (cardX + cardW - textX - 10 - trailingPad) : 4;
                    uint16_t titlePx = menu.style.titleFontPx;
                    if (titlePx == 0)
                        titlePx = 18;

                    ensureTextMetrics(item, titlePx, TITLE_WEIGHT, false, 0, 0);

                    const int16_t itemClipY = itemY + 2;
                    const int16_t itemClipH = cardH - 4;
                    if (itemClipH <= 0 || item.titleH > itemClipH)
                        continue;

                    float baseY = posY + 2.0f + (float)(itemClipH - item.titleH) * 0.5f;
                    if (baseY < (float)itemClipY)
                        baseY = (float)itemClipY;

                    if (baseY + item.titleH <= (float)contentTop || baseY >= (float)contentBottom)
                        continue;

                    int32_t prevItemClipX = 0, prevItemClipY = 0, prevItemClipW = 0, prevItemClipH = 0;
                    target->getClipRect(&prevItemClipX, &prevItemClipY, &prevItemClipW, &prevItemClipH);
                    const ClipState prevItemGuiClip = _clip;
                    applyClip(textClipX, itemClipY, textClipW, itemClipH);
                    target->setClipRect(textClipX, itemClipY, textClipW, itemClipH);

                    setTextFont(TITLE_WEIGHT, titlePx);
                    drawTextLine(item.title, textX, baseY, textMaxWidth, textColor, bg, active);

                    if (checked)
                    {
                        drawIcon()
                            .pos((int16_t)(cardX + cardW - trailingIconSize - 8), (int16_t)(itemY + (cardH - trailingIconSize) / 2))
                            .size((uint16_t)trailingIconSize)
                            .icon(menu.checkedIconId)
                            .color(active ? textColor : inactiveTextColor)
                            .bgColor(bg)
                            .draw();
                    }

                    _clip = prevItemGuiClip;
                    target->setClipRect(prevItemClipX, prevItemClipY, prevItemClipW, prevItemClipH);

                    continue;
                }

                drawSquircleRect().pos(cardX, itemY).size(cardW, cardH).radius({radius}).fill(bg);

                const String &subtitle = item.subtitle;
                const bool hasSub = subtitle.length() > 0;
                const uint16_t subColor = (textColor == 0xFFFF) ? (uint16_t)0xC618 : (uint16_t)0x8410;
                const int16_t textOffsetX = drawItemIcon(item.iconId, cardX + 10, itemY, (cardH > 40) ? 20 : 18, 8,
                                                         textColor, bg, true);
                const int16_t textX = cardX + 10 + textOffsetX;
                const int16_t textMaxWidth = (cardX + cardW - textX - 10 - trailingPad > 4) ? (cardX + cardW - textX - 10 - trailingPad) : 4;
                const bool autoTitlePx = (menu.style.titleFontPx == 0);
                const bool autoSubPx = (menu.style.subtitleFontPx == 0);
                const bool autoGapPx = (menu.style.lineGapPx == 0);
                uint16_t titlePx = menu.style.titleFontPx;
                uint16_t subPx = menu.style.subtitleFontPx;
                uint16_t gapPx = menu.style.lineGapPx;

                if (titlePx == 0)
                    titlePx = hasSub ? 18 : 20;
                if (hasSub)
                {
                    if (subPx == 0)
                        subPx = (uint16_t)((titlePx * 7U) / 10U);
                    if (gapPx == 0)
                        gapPx = (uint16_t)(titlePx / 5U);
                }
                else
                {
                    subPx = 0;
                    gapPx = 0;
                }

                if (hasSub && cardH <= 52)
                {
                    if (autoTitlePx)
                        titlePx = 16;
                    if (autoSubPx)
                        subPx = 11;
                    if (autoGapPx)
                        gapPx = 1;
                }
                else if (hasSub && cardH <= 58)
                {
                    if (autoTitlePx)
                        titlePx = 17;
                    if (autoSubPx)
                        subPx = 12;
                    if (autoGapPx)
                        gapPx = 2;
                }

                ensureTextMetrics(item, titlePx, TITLE_WEIGHT, hasSub, subPx, SUBTITLE_WEIGHT);

                const int16_t itemClipY = itemY + 4;
                const int16_t itemClipH = cardH - 8;
                if (itemClipH <= 0 || item.titleH > itemClipH)
                    continue;

                bool showSub = hasSub && subPx > 0 && item.subH > 0;
                int16_t totalH = item.titleH;
                if (showSub)
                    totalH += gapPx + item.subH;
                if (showSub && totalH > itemClipH)
                {
                    showSub = false;
                    totalH = item.titleH;
                }

                float baseY = posY + 4.0f + (float)(itemClipH - totalH) * 0.5f;
                if (baseY < (float)itemClipY)
                    baseY = (float)itemClipY;

                const float titleY = baseY;
                const float subY = baseY + item.titleH + (showSub ? gapPx : 0);
                int32_t prevItemClipX = 0, prevItemClipY = 0, prevItemClipW = 0, prevItemClipH = 0;
                target->getClipRect(&prevItemClipX, &prevItemClipY, &prevItemClipW, &prevItemClipH);
                const ClipState prevItemGuiClip = _clip;
                applyClip(textClipX, itemClipY, textClipW, itemClipH);
                target->setClipRect(textClipX, itemClipY, textClipW, itemClipH);

                setTextFont(TITLE_WEIGHT, titlePx);
                drawTextLine(item.title, textX, titleY, textMaxWidth, textColor, bg, active);

                if (showSub)
                {
                    setTextFont(SUBTITLE_WEIGHT, subPx);
                    drawTextLine(subtitle, textX, subY, textMaxWidth, subColor, bg, active);
                }

                if (checked)
                {
                    drawIcon()
                        .pos((int16_t)(cardX + cardW - trailingIconSize - 8), (int16_t)(itemY + (cardH - trailingIconSize) / 2))
                        .size((uint16_t)trailingIconSize)
                        .icon(menu.checkedIconId)
                        .color(textColor)
                        .bgColor(bg)
                        .draw();
                }

                _clip = prevItemGuiClip;
                target->setClipRect(prevItemClipX, prevItemClipY, prevItemClipW, prevItemClipH);
            }

            constexpr uint32_t SHOW_MS = 700;
            constexpr uint32_t FADE_MS = 450;
            constexpr uint32_t SLIDE_MS = 350;

            float targetAlpha = 0.0f;
            float slideT = 0.0f;
            bool animActive = false;
            uint32_t since = 0;

            if (menu.lastScrollActivityMs == 0)
            {
                slideT = 1.0f;
            }
            else
            {
                since = (now >= menu.lastScrollActivityMs) ? (now - menu.lastScrollActivityMs) : 0;
                animActive = (since <= (SHOW_MS + FADE_MS + SLIDE_MS));

                if (since <= SHOW_MS)
                {
                    targetAlpha = 1.0f;
                }
                else if (since <= SHOW_MS + FADE_MS)
                {
                    const float tFade = (float)(since - SHOW_MS) / (float)FADE_MS;
                    targetAlpha = 1.0f - tFade * tFade * (3.0f - 2.0f * tFade);
                }
                else if (since <= SHOW_MS + FADE_MS + SLIDE_MS)
                {
                    slideT = (float)(since - (SHOW_MS + FADE_MS)) / (float)SLIDE_MS;
                    if (slideT > 1.0f)
                        slideT = 1.0f;
                }
                else
                {
                    slideT = 1.0f;
                }
            }

            const uint8_t newAlpha = (uint8_t)(targetAlpha * 255.0f + 0.5f);
            if (newAlpha != menu.scrollbarAlpha)
            {
                menu.scrollbarAlpha = newAlpha;
                requestRedraw();
            }
            else if (animActive && (newAlpha > 0 || slideT < 1.0f))
            {
                requestRedraw();
            }

            applyClip(left, top, w, h);
            target->setClipRect(left, top, w, h);

            if (hasScrollbar && menu.scrollbarAlpha > 5)
            {
                constexpr int16_t scrollbarWidth = 3;
                constexpr int16_t scrollbarSlidePx = 4;
                const int16_t sbInset = 2;
                int16_t trackTop = top + sbInset;
                int16_t trackH = h - (int16_t)(2 * sbInset);
                if (trackH < 1)
                    trackH = 1;
                const int16_t trackX = (int16_t)(right - scrollbarWidth + (int16_t)lroundf((float)scrollbarSlidePx * slideT));

                const float ratio = (float)visibleCount / (float)menu.itemCount;
                int16_t thumbH = (int16_t)(trackH * ratio);
                if (thumbH < 10)
                    thumbH = 10;

                float pct = (maxScroll <= 0.0f) ? 0.0f : (menu.scrollPos / maxScroll);
                if (pct < 0.0f)
                    pct = 0.0f;
                if (pct > 1.0f)
                    pct = 1.0f;

                int16_t thumbY = trackTop + (int16_t)((trackH - thumbH) * pct);

                const uint16_t thumb565 = target->color565(168, 168, 168);
                const uint16_t col = detail::blend565(bgColor565, thumb565, menu.scrollbarAlpha);

                uint8_t thumbRadius = (thumbH < 6) ? 1 : 2;
                fillRoundRect(trackX, thumbY, scrollbarWidth, thumbH, thumbRadius, col);
            }

            _clip = prevGuiClip;
            target->setClipRect(clipX, clipY, clipW, clipH);
        };

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawList();
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (invalidate && !prevRender)
            invalidateRect(x, y, w, h);
        return true;
    }
}

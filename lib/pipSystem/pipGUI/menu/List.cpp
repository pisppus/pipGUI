#include <pipGUI/core/api/pipGUI.h>
#include <pipGUI/core/utils/Colors.h>
#include <math.h>
#include <new>
#include <utility>
#include <cassert>
#include <limits>
#include <cstring>

namespace pipgui
{
    static void clearListMenuCache(ListMenuState &m, pipcore::GuiPlatform *plat)
    {
        if (m.cacheNormal)
        {
            for (uint8_t i = 0; i < m.capacity; ++i)
            {
                if (m.cacheNormal[i])
                {
                    detail::guiFree(plat, m.cacheNormal[i]);
                    m.cacheNormal[i] = nullptr;
                }
            }
        }

        if (m.cacheActive)
        {
            for (uint8_t i = 0; i < m.capacity; ++i)
            {
                if (m.cacheActive[i])
                {
                    detail::guiFree(plat, m.cacheActive[i]);
                    m.cacheActive[i] = nullptr;
                }
            }
        }

        m.cacheW = 0;
        m.cacheH = 0;
        m.cacheValid = false;

        m.viewportValid = false;
    }

    static bool ensureListMenuCapacity(ListMenuState &m, uint8_t newCapacity, pipcore::GuiPlatform *plat)
    {
        if (newCapacity == 0)
        {
            clearListMenuCache(m, plat);

            if (m.items)
            {
                using ListItem = pipgui::ListMenuState::Item;
                for (uint8_t i = 0; i < m.capacity; ++i)
                    m.items[i].~ListItem();
                detail::guiFree(plat, m.items);
                m.items = nullptr;
            }
            if (m.cacheNormal)
            {
                detail::guiFree(plat, m.cacheNormal);
                m.cacheNormal = nullptr;
            }
            if (m.cacheActive)
            {
                detail::guiFree(plat, m.cacheActive);
                m.cacheActive = nullptr;
            }

            if (m.viewportSprite)
            {
                m.viewportSprite->deleteSprite();
                m.viewportSprite->~LGFX_Sprite();
                detail::guiFree(plat, m.viewportSprite);
                m.viewportSprite = nullptr;
            }
            m.viewportValid = false;
            m.capacity = 0;
            return true;
        }

        if (m.capacity >= newCapacity && m.items && m.cacheNormal && m.cacheActive)
            return true;

        clearListMenuCache(m, plat);

        ListMenuState::Item *newItems = (ListMenuState::Item *)detail::guiAlloc(plat, sizeof(ListMenuState::Item) * newCapacity, pipcore::GuiAllocCaps::Default);
        uint16_t **newCacheNormal = (uint16_t **)detail::guiAlloc(plat, sizeof(uint16_t *) * newCapacity, pipcore::GuiAllocCaps::Default);
        uint16_t **newCacheActive = (uint16_t **)detail::guiAlloc(plat, sizeof(uint16_t *) * newCapacity, pipcore::GuiAllocCaps::Default);

        if (!newItems || !newCacheNormal || !newCacheActive)
        {
            if (newItems)
                detail::guiFree(plat, newItems);
            if (newCacheNormal)
                detail::guiFree(plat, newCacheNormal);
            if (newCacheActive)
                detail::guiFree(plat, newCacheActive);
            return false;
        }

        for (uint8_t i = 0; i < newCapacity; ++i)
            new (&newItems[i]) ListMenuState::Item();

        for (uint8_t i = 0; i < newCapacity; ++i)
        {
            newCacheNormal[i] = nullptr;
            newCacheActive[i] = nullptr;
        }

        uint8_t toCopy = (m.items && m.itemCount < newCapacity) ? m.itemCount : (m.items ? newCapacity : 0);
        for (uint8_t i = 0; i < toCopy; ++i)
        {
            newItems[i] = std::move(m.items[i]);
        }

        if (m.items)
        {
            using ListItem = pipgui::ListMenuState::Item;
            for (uint8_t i = 0; i < m.capacity; ++i)
                m.items[i].~ListItem();
            detail::guiFree(plat, m.items);
        }
        if (m.cacheNormal)
            detail::guiFree(plat, m.cacheNormal);
        if (m.cacheActive)
            detail::guiFree(plat, m.cacheActive);

        m.items = newItems;
        m.cacheNormal = newCacheNormal;
        m.cacheActive = newCacheActive;
        m.capacity = newCapacity;
        return true;
    }

    void GUI::configureListMenu(uint8_t screenId, uint8_t parentScreen, const ListMenuItemDef *items, uint8_t itemCount)
    {
        if (screenId == 255 || !items || itemCount == 0)
            return;
        ListMenuState &m = ensureListMenu(screenId);
        if (!ensureListMenuCapacity(m, itemCount, platform()))
            return;

        clearListMenuCache(m, platform());

        m.configured = true;
        m.parentScreen = parentScreen;
        m.itemCount = itemCount;
        m.selectedIndex = 0;
        m.scrollPos = 0.0f;
        m.targetScroll = 0.0f;
        m.nextHoldStartMs = 0;
        m.prevHoldStartMs = 0;
        m.nextLongFired = false;
        m.prevLongFired = false;
        m.lastNextDown = false;
        m.lastPrevDown = false;
        m.scrollbarAlpha = 0;
        m.lastScrollActivityMs = 0;

        for (uint8_t i = 0; i < itemCount; ++i)
        {
            m.items[i].title = items[i].title ? String(items[i].title) : String("");
            m.items[i].subtitle = items[i].subtitle ? String(items[i].subtitle) : String("");
            m.items[i].targetScreen = items[i].targetScreen;
        }

        if (m.style.cardColor == 0 && m.style.cardActiveColor == 0)
        {
            uint16_t base = _bgColor ? _bgColor : 0x0000;
            m.style.cardColor = pipgui::detail::lighten565(base, 4);
            m.style.cardActiveColor = rgb(0, 130, 220);
            m.style.radius = 10;
            m.style.spacing = 10;
        }
    }

    void GUI::configureListMenu(uint8_t screenId, uint8_t parentScreen, std::initializer_list<ListMenuItemDef> items)
    {
        if (items.size() == 0)
            return;

        configureListMenu(screenId,
                          parentScreen,
                          items.begin(),
                          static_cast<uint8_t>(items.size()));
    }

    void GUI::setListMenuStyle(uint8_t screenId,
                               uint16_t cardColor,
                               uint16_t cardActiveColor,
                               uint8_t spacing,
                               uint8_t radius,
                               int16_t cardWidth,
                               int16_t cardHeight,
                               uint16_t titleFontPx,
                               uint16_t subtitleFontPx,
                               uint16_t lineGapPx,
                               ListMenuMode mode)
    {
        if (screenId == 255)
            return;
        ListMenuState &m = ensureListMenu(screenId);

        clearListMenuCache(m, platform());

        m.style.cardColor = cardColor;
        m.style.cardActiveColor = cardActiveColor;
        m.style.spacing = spacing;
        m.style.radius = radius;
        m.style.cardWidth = cardWidth;
        m.style.cardHeight = cardHeight;
        m.style.titleFontPx = titleFontPx;
        m.style.subtitleFontPx = subtitleFontPx;
        m.style.lineGapPx = lineGapPx;
        m.style.mode = mode;
    }

    void GUI::handleListMenuInput(uint8_t screenId, bool nextPressed, bool prevPressed, bool nextDown, bool prevDown)
    {
        (void)nextPressed;
        (void)prevPressed;

        ListMenuState *pm = getListMenu(screenId);
        if (!pm)
            return;
        ListMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        uint32_t now = nowMs();
        bool changed = false;

        const uint32_t ENTER_HOLD_MS = 400;
        const uint32_t BACK_HOLD_MS = 400;

        if (nextDown)
        {
            if (!m.lastNextDown)
            {
                m.nextHoldStartMs = now;
                m.nextLongFired = false;
            }
            else if (!m.nextLongFired && m.nextHoldStartMs && (now - m.nextHoldStartMs) >= ENTER_HOLD_MS)
            {
                if (m.selectedIndex < m.itemCount)
                {
                    uint8_t target = m.items[m.selectedIndex].targetScreen;
                    if (target != 255)
                        setScreen(target);
                }
                m.nextLongFired = true;
            }
        }
        else
        {
            if (m.lastNextDown && !m.nextLongFired)
            {
                if (m.itemCount > 0)
                {
                    if (m.selectedIndex + 1 < m.itemCount)
                        m.selectedIndex++;
                    else
                        m.selectedIndex = 0;
                    changed = true;
                }
            }

            m.nextHoldStartMs = 0;
            m.nextLongFired = false;
        }

        if (prevDown)
        {
            if (!m.lastPrevDown)
            {
                m.prevHoldStartMs = now;
                m.prevLongFired = false;
            }
            else if (!m.prevLongFired && m.prevHoldStartMs && (now - m.prevHoldStartMs) >= BACK_HOLD_MS)
            {
                if (m.parentScreen != 255)
                    setScreen(m.parentScreen);
                m.prevLongFired = true;
            }
        }
        else
        {
            if (m.lastPrevDown && !m.prevLongFired)
            {
                if (m.itemCount > 0)
                {
                    if (m.selectedIndex > 0)
                        m.selectedIndex--;
                    else
                        m.selectedIndex = m.itemCount - 1;
                    changed = true;
                }
            }

            m.prevHoldStartMs = 0;
            m.prevLongFired = false;
        }

        m.lastNextDown = nextDown;
        m.lastPrevDown = prevDown;

        if (changed)
        {
            m.scrollbarAlpha = 255;
            m.lastScrollActivityMs = now;
            requestRedraw();
        }
    }

    void GUI::renderListMenu(uint8_t screenId)
    {
        int16_t left = 0, right = _screenWidth, top = 0, bottom = _screenHeight;
        if (_flags.statusBarEnabled && _statusBarHeight > 0)
        {
            if (_statusBarPos == Top)
                top += _statusBarHeight;
            else if (_statusBarPos == Bottom)
                bottom -= _statusBarHeight;
            else if (_statusBarPos == Left)
                left += _statusBarHeight;
            else if (_statusBarPos == Right)
                right -= _statusBarHeight;
        }
        int16_t w = right - left;
        int16_t h = bottom - top;
        if (w <= 0 || h <= 0)
            return;

        renderListMenu(screenId, left, top, w, h, _bgColor);
    }

    void GUI::renderListMenu(uint8_t screenId, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bgColor)
    {
        ListMenuState *pm = getListMenu(screenId);
        if (!pm)
            return;
        ListMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        if (w <= 0 || h <= 0)
            return;

        auto t = getDrawTarget();
        uint32_t now = nowMs();

        int16_t left = x;
        int16_t right = x + w;
        int16_t top = y;
        int16_t bottom = y + h;

        int16_t usableW = w;
        int16_t usableH = h;
        int16_t contentTop = top + 4;

        int16_t marginX = 5;

        const bool cardMode = (m.style.mode == Cards);
        const bool plainMode = (m.style.mode == Plain);

        int16_t cardW;
        if (m.style.cardWidth > 0 && m.style.cardWidth < usableW)
            cardW = m.style.cardWidth;
        else
            cardW = usableW - marginX * 2;
        if (cardW > usableW)
            cardW = usableW;
        if (cardW < 20)
            cardW = 20;

        int16_t cardH;
        if (m.style.cardHeight > 0)
            cardH = m.style.cardHeight;
        else
            cardH = cardMode ? 50 : 34;

        if (m.style.cardHeight <= 0 && cardH * 2 + m.style.spacing > usableH)
            cardH = usableH / 3;

        int16_t cx = left + (usableW - cardW) / 2;

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        t->getClipRect(&clipX, &clipY, &clipW, &clipH);

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

        int32_t bgW = bgR - bgL;
        int32_t bgH = bgB - bgT;
        if (bgW > 0 && bgH > 0)
            t->fillRect((int16_t)bgL, (int16_t)bgT, (int16_t)bgW, (int16_t)bgH, bgColor);

        int16_t visibleHeight = bottom - contentTop;
        if (visibleHeight < cardH)
            visibleHeight = cardH;

        float itemSpan = (float)(cardH + m.style.spacing);
        uint8_t visibleCount = 1;
        if (itemSpan > 1.0f)
        {
            if (visibleHeight > cardH)
            {
                int16_t extra = (int16_t)(visibleHeight - cardH);
                uint8_t additional = (uint8_t)(extra / (int16_t)itemSpan);
                visibleCount = (uint8_t)(1 + additional);
            }
            if (visibleCount < 1)
                visibleCount = 1;
            if (visibleCount > m.itemCount)
                visibleCount = m.itemCount;
        }

        float maxScroll = 0.0f;
        if (m.itemCount > visibleCount)
            maxScroll = (float)(m.itemCount - visibleCount);

        float targetTop = m.targetScroll;
        if (m.itemCount > 0)
        {
            float viewTop = m.scrollPos;
            float viewBottom = viewTop + (float)visibleCount;
            float idxTop = (float)m.selectedIndex;
            float idxBottom = idxTop + 1.0f;

            if (idxTop < viewTop)
                targetTop = idxTop;
            else if (idxBottom > viewBottom)
                targetTop = idxBottom - (float)visibleCount;
        }

        if (targetTop < 0.0f)
            targetTop = 0.0f;
        if (targetTop > maxScroll)
            targetTop = maxScroll;

        m.targetScroll = targetTop;

        float diff = m.targetScroll - m.scrollPos;
        const float speed = 0.25f;

        if (fabsf(diff) > 0.005f)
        {
            m.scrollPos += diff * speed;
            m.scrollbarAlpha = 255;
            m.lastScrollActivityMs = now;
            requestRedraw();
        }

        uint8_t r = m.style.radius;
        if (r < 2)
            r = 2;

        pipcore::GuiPlatform *plat = platform();

        bool useCache = (cardMode || plainMode) && _flags.ttfLoaded && (bgColor == _bgColor);
        if (!useCache && m.cacheValid)
        {
            clearListMenuCache(m, plat);
        }

        if (useCache)
        {
            if (!m.cacheValid || m.cacheW != cardW || m.cacheH != cardH)
            {
                clearListMenuCache(m, plat);
                m.cacheW = cardW;
                m.cacheH = cardH;
                m.cacheValid = true;
            }
        }

        if (plainMode && useCache && m.cacheValid)
        {
            int32_t base = (int32_t)floorf(m.scrollPos);
            int32_t keepMin = base - 2;
            int32_t keepMax = base + (int32_t)visibleCount + 2;
            if (keepMin < 0)
                keepMin = 0;
            if (keepMax < 0)
                keepMax = 0;
            if (keepMax >= (int32_t)m.itemCount)
                keepMax = (int32_t)m.itemCount - 1;

            for (uint8_t j = 0; j < m.itemCount; ++j)
            {
                if ((int32_t)j >= keepMin && (int32_t)j <= keepMax)
                    continue;
                if (m.cacheNormal && m.cacheNormal[j])
                {
                    detail::guiFree(plat, m.cacheNormal[j]);
                    m.cacheNormal[j] = nullptr;
                }
                if (m.cacheActive && m.cacheActive[j])
                {
                    detail::guiFree(plat, m.cacheActive[j]);
                    m.cacheActive[j] = nullptr;
                }
            }
        }

        auto renderItemToCache = [&](uint8_t index, bool activeState, uint16_t bg, uint16_t border, uint16_t txtCol, uint16_t subCol) -> uint16_t *
        {
            if (!useCache || !m.cacheValid)
                return nullptr;
            if (index >= m.itemCount)
                return nullptr;

            uint16_t *&slot = activeState ? m.cacheActive[index] : m.cacheNormal[index];
            if (slot)
                return slot;

            if (m.cacheW <= 0 || m.cacheH <= 0)
                return nullptr;
            size_t pixels = (size_t)m.cacheW * (size_t)m.cacheH;
            assert(pixels <= (std::numeric_limits<size_t>::max() / sizeof(uint16_t)));
            if (pixels == 0 || pixels > (std::numeric_limits<size_t>::max() / sizeof(uint16_t)))
                return nullptr;
            size_t bytes = pixels * sizeof(uint16_t);
            slot = (uint16_t *)detail::guiAlloc(plat, bytes, pipcore::GuiAllocCaps::PreferExternal);
            if (!slot)
                return nullptr;

            if (!_tft)
            {
                detail::guiFree(plat, slot);
                slot = nullptr;
                return nullptr;
            }

            lgfx::LGFX_Sprite spr(_tft);
            spr.setColorDepth(16);
            if (!spr.createSprite(m.cacheW, m.cacheH))
            {
                detail::guiFree(plat, slot);
                slot = nullptr;
                return nullptr;
            }

            const String &title = m.items[index].title;
            const String &sub = m.items[index].subtitle;
            bool hasSub = sub.length() > 0;

            spr.fillScreen(_bgColor);
            spr.fillSmoothRoundRect(0, 0, m.cacheW, m.cacheH, r, bg);
            spr.drawRoundRect(0, 0, m.cacheW, m.cacheH, r, border);

            int16_t txLocal = 10;

            if (_flags.ttfLoaded)
            {
                _ttf.setDrawer(spr);

                uint16_t titlePx = m.style.titleFontPx;
                uint16_t subPx = m.style.subtitleFontPx;
                uint16_t gapPx = m.style.lineGapPx;

                if (titlePx == 0)
                    titlePx = hasSub ? 18 : 20;
                if (hasSub)
                {
                    if (subPx == 0)
                        subPx = (uint16_t)((titlePx * 7U) / 10U);
                    if (gapPx == 0)
                        gapPx = (uint16_t)(titlePx / 3U);
                    if (gapPx > 0)
                        gapPx = (uint16_t)(gapPx + 4);
                }
                else
                {
                    subPx = 0;
                    gapPx = 0;
                }

                int16_t titleH = 0;
                int16_t subH = 0;

                _ttf.setFontSize(titlePx);
                titleH = (int16_t)_ttf.getTextHeight("%s", title.c_str());

                if (hasSub && subPx > 0)
                {
                    _ttf.setFontSize(subPx);
                    subH = (int16_t)_ttf.getTextHeight("%s", sub.c_str());
                }

                int16_t totalH = titleH;
                if (hasSub && subH > 0)
                    totalH += gapPx + subH;

                int16_t baseY = (int16_t)((m.cacheH - totalH) / 2);
                baseY -= 4;
                if (baseY < 2)
                    baseY = 2;

                int16_t tyTitleLocal = baseY;
                int16_t tySubLocal = baseY + titleH + (hasSub ? gapPx : 0);

                _ttf.setFontSize(titlePx);
                _ttf.setFontColor(txtCol, bg);
                _ttf.drawString(title.c_str(), txLocal, tyTitleLocal, txtCol, bg, Layout::Horizontal);

                if (hasSub && subPx > 0)
                {
                    _ttf.setFontSize(subPx);
                    _ttf.setFontColor(subCol, bg);
                    _ttf.drawString(sub.c_str(), txLocal, tySubLocal, subCol, bg, Layout::Horizontal);
                }
            }

            memcpy(slot, (uint16_t *)spr.getBuffer(), pixels * sizeof(uint16_t));
            spr.deleteSprite();

            return slot;
        };

        auto renderPlainItemToCache = [&](uint8_t index, bool activeState, uint16_t bg, uint16_t border, uint16_t txtCol) -> uint16_t *
        {
            if (!useCache || !m.cacheValid)
                return nullptr;
            if (index >= m.itemCount)
                return nullptr;

            uint16_t *&slot = activeState ? m.cacheActive[index] : m.cacheNormal[index];
            if (slot)
                return slot;

            if (m.cacheW <= 0 || m.cacheH <= 0)
                return nullptr;
            size_t pixels = (size_t)m.cacheW * (size_t)m.cacheH;
            if (pixels == 0 || pixels > (std::numeric_limits<size_t>::max() / sizeof(uint16_t)))
                return nullptr;
            size_t bytes = pixels * sizeof(uint16_t);

            slot = (uint16_t *)detail::guiAlloc(plat, bytes, pipcore::GuiAllocCaps::PreferExternal);
            if (!slot)
                return nullptr;

            if (!_tft)
            {
                detail::guiFree(plat, slot);
                slot = nullptr;
                return nullptr;
            }

            lgfx::LGFX_Sprite spr(_tft);
            spr.setColorDepth(16);
            if (!spr.createSprite(m.cacheW, m.cacheH))
            {
                detail::guiFree(plat, slot);
                slot = nullptr;
                return nullptr;
            }

            const String &title = m.items[index].title;

            spr.fillScreen(_bgColor);
            if (activeState)
            {
                spr.fillSmoothRoundRect(0, 0, m.cacheW, m.cacheH, r, bg);
                spr.drawRoundRect(0, 0, m.cacheW, m.cacheH, r, border);
            }
            else
            {
                spr.fillRect(0, 0, m.cacheW, m.cacheH, bg);
            }

            int16_t txLocal = 12;

            uint16_t titlePx = m.style.titleFontPx;
            if (titlePx == 0)
                titlePx = 18;

            _ttf.setDrawer(spr);
            _ttf.setFontSize(titlePx);

            int16_t titleH = (int16_t)_ttf.getTextHeight("%s", title.c_str());
            int16_t baseY = (int16_t)((m.cacheH - titleH) / 2);
            baseY -= 4;
            if (baseY < 2)
                baseY = 2;

            _ttf.setFontSize(titlePx);
            _ttf.setFontColor(txtCol, bg);
            _ttf.drawString(title.c_str(), txLocal, baseY, txtCol, bg, Layout::Horizontal);

            memcpy(slot, (uint16_t *)spr.getBuffer(), pixels * sizeof(uint16_t));
            spr.deleteSprite();

            return slot;
        };

        int16_t startIndex = (int16_t)floorf(m.scrollPos) - 2;
        int16_t endIndex = startIndex + (int16_t)visibleCount + 4;
        if (startIndex < 0)
            startIndex = 0;
        if (endIndex < 0)
            endIndex = 0;
        if (endIndex >= (int16_t)m.itemCount)
            endIndex = (int16_t)m.itemCount - 1;

        for (int16_t ii = startIndex; ii <= endIndex; ++ii)
        {
            uint8_t i = (uint8_t)ii;
            float relIndex = (float)ii - m.scrollPos;
            float posY = (float)contentTop + relIndex * itemSpan;
            int16_t yy = (int16_t)posY;

            if (yy >= bottom || (yy + cardH) <= top)
                continue;

            bool active = (i == m.selectedIndex);

            uint16_t bg = active ? m.style.cardActiveColor : (cardMode ? m.style.cardColor : bgColor);
            uint16_t border = pipgui::detail::lighten565(bg, 4);
            uint16_t txtCol = pipgui::detail::autoTextColorForBg(bg);
            uint16_t subCol = pipgui::detail::lighten565((txtCol == 0xFFFF ? 0xC618 : 0x8410), 0);

            if (!cardMode)
            {
                const String &title = m.items[i].title;

                if (plainMode && useCache)
                {
                    uint16_t bg2 = active ? m.style.cardActiveColor : bgColor;
                    uint16_t border2 = pipgui::detail::lighten565(bg2, 4);
                    uint16_t txt2 = pipgui::detail::autoTextColorForBg(bg2);
                    uint16_t *buf = renderPlainItemToCache(i, active, bg2, border2, txt2);
                    if (buf)
                    {
                        t->pushImage(cx, yy, cardW, cardH, buf);
                        continue;
                    }
                }

                if (active)
                {
                    t->fillSmoothRoundRect(cx, yy, cardW, cardH, r, bg);
                    t->drawRoundRect(cx, yy, cardW, cardH, r, border);
                }

                int16_t tx = cx + 12;

                if (_flags.ttfLoaded)
                {
                    _ttf.setDrawer(*t);

                    uint16_t titlePx = m.style.titleFontPx;
                    if (titlePx == 0)
                        titlePx = 18;

                    _ttf.setFontSize(titlePx);
                    int16_t titleH = (int16_t)_ttf.getTextHeight("%s", title.c_str());
                    int16_t baseY = yy + (int16_t)((cardH - titleH) / 2);
                    baseY -= 4;
                    if (baseY < yy + 2)
                        baseY = (int16_t)(yy + 2);

                    _ttf.setFontSize(titlePx);
                    _ttf.setFontColor(txtCol, bg);
                    _ttf.drawString(title.c_str(), tx, baseY, txtCol, bg, Layout::Horizontal);
                }
                else
                {
                    t->setTextColor(txtCol, bg);

                    t->setTextFont(2);
                    int16_t titleH = t->fontHeight();
                    int16_t baseY = yy + (int16_t)((cardH - titleH) / 2);
                    baseY -= 4;
                    if (baseY < yy + 2)
                        baseY = (int16_t)(yy + 2);

                    t->drawString(title, tx, baseY);
                }

                continue;
            }

            if (useCache)
            {
                uint16_t *buf = renderItemToCache(i, active, bg, border, txtCol, subCol);
                if (buf)
                {
                    t->pushImage(cx, yy, cardW, cardH, buf);
                    continue;
                }
            }

            t->fillSmoothRoundRect(cx, yy, cardW, cardH, r, bg);
            t->drawRoundRect(cx, yy, cardW, cardH, r, border);

            int16_t tx = cx + 10;
            const String &title = m.items[i].title;
            const String &sub = m.items[i].subtitle;
            bool hasSub = sub.length() > 0;

            if (_flags.ttfLoaded)
            {
                _ttf.setDrawer(*t);

                uint16_t titlePx = m.style.titleFontPx;
                uint16_t subPx = m.style.subtitleFontPx;
                uint16_t gapPx = m.style.lineGapPx;

                if (titlePx == 0)
                    titlePx = hasSub ? 18 : 20;
                if (hasSub)
                {
                    if (subPx == 0)
                        subPx = (uint16_t)((titlePx * 7U) / 10U);
                    if (gapPx == 0)
                        gapPx = (uint16_t)(titlePx / 3U);
                    if (gapPx > 0)
                        gapPx = (uint16_t)(gapPx + 3);
                }
                else
                {
                    subPx = 0;
                    gapPx = 0;
                }

                int16_t titleH = 0;
                int16_t subH = 0;

                _ttf.setFontSize(titlePx);
                titleH = (int16_t)_ttf.getTextHeight("%s", title.c_str());

                if (hasSub && subPx > 0)
                {
                    _ttf.setFontSize(subPx);
                    subH = (int16_t)_ttf.getTextHeight("%s", sub.c_str());
                }

                int16_t totalH = titleH;
                if (hasSub && subH > 0)
                    totalH += gapPx + subH;

                int16_t baseY = yy + (int16_t)((cardH - totalH) / 2);
                baseY -= 4;
                if (baseY < yy + 2)
                    baseY = (int16_t)(yy + 2);

                int16_t tyTitle = baseY;
                int16_t tySub = baseY + titleH + (hasSub ? gapPx : 0);

                _ttf.setFontSize(titlePx);
                _ttf.setFontColor(txtCol, bg);
                _ttf.drawString(title.c_str(), tx, tyTitle, txtCol, bg, Layout::Horizontal);

                if (hasSub && subPx > 0)
                {
                    _ttf.setFontSize(subPx);
                    _ttf.setFontColor(subCol, bg);
                    _ttf.drawString(sub.c_str(), tx, tySub, subCol, bg, Layout::Horizontal);
                }
            }
            else
            {
                t->setTextColor(txtCol, bg);

                t->setTextFont(2);
                int16_t titleH = t->fontHeight();
                int16_t subH = 0;

                if (hasSub)
                {
                    t->setTextFont(1);
                    subH = t->fontHeight();
                    t->setTextFont(2);
                }

                int16_t gap = hasSub ? 5 : 0;

                int16_t totalH = titleH + (hasSub ? (gap + subH) : 0);

                int16_t baseY = yy + (int16_t)((cardH - totalH) / 2);
                baseY -= 4;
                if (baseY < yy + 2)
                    baseY = (int16_t)(yy + 2);

                int16_t tyTitle = baseY;
                int16_t tySub = baseY + titleH + (hasSub ? gap : 0);

                t->setTextFont(2);
                t->drawString(title, tx, tyTitle);

                if (hasSub)
                {
                    t->setTextFont(1);
                    t->setTextColor(subCol, bg);
                    t->drawString(sub, tx, tySub);
                }
            }
        }

        const uint32_t SHOW_MS = 400;
        const uint32_t FADE_MS = 300;
        const uint32_t SLIDE_MS = 250;

        float targetAlpha = 0.0f;
        float slideT = 0.0f;

        uint32_t since = (m.lastScrollActivityMs == 0) ? 0 : (now - m.lastScrollActivityMs);

        if (since <= SHOW_MS)
        {
            targetAlpha = 1.0f;
        }
        else if (since <= SHOW_MS + FADE_MS)
        {
            float tFade = (float)(since - SHOW_MS) / (float)FADE_MS;
            float tt = tFade * tFade * (3.0f - 2.0f * tFade);
            targetAlpha = 1.0f - tt;
        }
        else if (since <= SHOW_MS + FADE_MS + SLIDE_MS)
        {
            targetAlpha = 0.0f;
            slideT = (float)(since - (SHOW_MS + FADE_MS)) / (float)SLIDE_MS;
            if (slideT > 1.0f)
                slideT = 1.0f;
        }
        else
        {
            targetAlpha = 0.0f;
            slideT = 1.0f;
        }

        uint8_t newAlpha = (uint8_t)(targetAlpha * 255.0f + 0.5f);
        if (newAlpha != m.scrollbarAlpha)
        {
            m.scrollbarAlpha = newAlpha;
            if (newAlpha > 0)
            {
                requestRedraw();
            }
        }

        if (m.itemCount > visibleCount && m.scrollbarAlpha > 5)
        {
            int16_t trH = bottom - contentTop;
            int16_t baseX = right - 4;
            int16_t trX = baseX + (int16_t)(6.0f * slideT);

            float ratio = (float)visibleCount / (float)m.itemCount;
            if (ratio > 1.0f)
                ratio = 1.0f;
            int16_t thH = (int16_t)(trH * ratio);
            if (thH < 10)
                thH = 10;

            float pct = (maxScroll <= 0.0f) ? 0.0f : (m.scrollPos / maxScroll);
            if (pct < 0.0f)
                pct = 0.0f;
            if (pct > 1.0f)
                pct = 1.0f;

            int16_t thY = contentTop + (int16_t)((trH - thH) * pct);

            uint8_t minV = 40;
            uint8_t maxV = 140;
            uint8_t v = minV;
            if (m.scrollbarAlpha > 0)
            {
                uint32_t range = (uint32_t)(maxV - minV);
                v = (uint8_t)(minV + (range * (uint32_t)m.scrollbarAlpha) / 255U);
            }

            uint16_t col = t->color565(v, v, v);

            t->fillSmoothRoundRect(trX, thY, 3, thH, 1, col);
        }
    }

    bool GUI::updateListMenu(uint8_t screenId)
    {
        int16_t left = 0, right = _screenWidth, top = 0, bottom = _screenHeight;
        if (_flags.statusBarEnabled && _statusBarHeight > 0)
        {
            if (_statusBarPos == Top)
                top += _statusBarHeight;
            else if (_statusBarPos == Bottom)
                bottom -= _statusBarHeight;
            else if (_statusBarPos == Left)
                left += _statusBarHeight;
            else if (_statusBarPos == Right)
                right -= _statusBarHeight;
        }

        int16_t w = right - left;
        int16_t h = bottom - top;
        if (w <= 0 || h <= 0)
            return false;

        return updateListMenu(screenId, left, top, w, h, _bgColor);
    }

    bool GUI::updateListMenu(uint8_t screenId, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bgColor)
    {
        ListMenuState *pm = getListMenu(screenId);
        if (!pm)
            return false;
        ListMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return false;
        if (w <= 0 || h <= 0)
            return false;

        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            renderListMenu(screenId, x, y, w, h, bgColor);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return true;
        }

        pipcore::GuiPlatform *plat = platform();

        if (!m.viewportSprite)
        {
            void *mem = detail::guiAlloc(plat, sizeof(lgfx::LGFX_Sprite), pipcore::GuiAllocCaps::Default);
            if (mem)
            {
                m.viewportSprite = new (mem) lgfx::LGFX_Sprite(_tft);
                m.viewportSprite->setColorDepth(16);
            }
        }

        lgfx::LGFX_Sprite *vp = m.viewportSprite;
        if (vp)
        {
            if (!vp->getBuffer() || vp->width() != w || vp->height() != h)
            {
                vp->deleteSprite();
                vp->setColorDepth(16);
                if (!vp->createSprite(w, h))
                    vp->deleteSprite();
                m.viewportValid = false;
            }
        }

        bool usedViewport = (vp && vp->getBuffer());

        uint8_t prevNeedRedraw = _flags.needRedraw;
        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        bool incrementalUsed = false;
        int16_t dirtyX = 0, dirtyY = 0, dirtyW = w, dirtyH = h;

        if (usedViewport)
        {
            const bool cardMode = (m.style.mode == Cards);
            int16_t marginX = 5;
            int16_t usableW = w;
            int16_t usableH = h;

            int16_t cardW;
            if (m.style.cardWidth > 0 && m.style.cardWidth < usableW)
                cardW = m.style.cardWidth;
            else
                cardW = usableW - marginX * 2;
            if (cardW > usableW)
                cardW = usableW;
            if (cardW < 20)
                cardW = 20;

            int16_t cardH;
            if (m.style.cardHeight > 0)
                cardH = m.style.cardHeight;
            else
                cardH = cardMode ? 50 : 34;
            if (m.style.cardHeight <= 0 && cardH * 2 + m.style.spacing > usableH)
                cardH = usableH / 3;

            int16_t itemSpanPx = (int16_t)(cardH + m.style.spacing);
            if (itemSpanPx <= 0)
                itemSpanPx = 1;

            int32_t newScrollPx = (int32_t)lroundf(m.scrollPos * (float)itemSpanPx);
            int32_t oldScrollPx = m.viewportScrollPx;
            int32_t dy = newScrollPx - oldScrollPx;
            uint8_t oldSel = m.viewportSelectedIndex;

            bool canIncremental = m.viewportValid &&
                                  m.viewportW == w && m.viewportH == h &&
                                  m.viewportBgColor == bgColor &&
                                  m.viewportCardW == cardW && m.viewportCardH == cardH &&
                                  m.viewportItemSpanPx == itemSpanPx;

            _flags.renderToSprite = 1;
            _activeSprite = vp;
            _flags.needRedraw = 0;

            int32_t vpClipX = 0, vpClipY = 0, vpClipW = 0, vpClipH = 0;
            vp->getClipRect(&vpClipX, &vpClipY, &vpClipW, &vpClipH);

            auto clipRender = [&](int16_t cx, int16_t cy, int16_t cw, int16_t ch)
            {
                if (cw <= 0 || ch <= 0)
                    return;
                if (cx < 0)
                {
                    cw = (int16_t)(cw + cx);
                    cx = 0;
                }
                if (cy < 0)
                {
                    ch = (int16_t)(ch + cy);
                    cy = 0;
                }
                if (cx >= w || cy >= h)
                    return;
                if (cx + cw > w)
                    cw = (int16_t)(w - cx);
                if (cy + ch > h)
                    ch = (int16_t)(h - cy);
                if (cw <= 0 || ch <= 0)
                    return;
                vp->setClipRect(cx, cy, cw, ch);
                renderListMenu(screenId, 0, 0, w, h, bgColor);
            };

            auto addDirty = [&](int16_t rx, int16_t ry, int16_t rw, int16_t rh)
            {
                if (rw <= 0 || rh <= 0)
                    return;
                if (rx < 0)
                {
                    rw = (int16_t)(rw + rx);
                    rx = 0;
                }
                if (ry < 0)
                {
                    rh = (int16_t)(rh + ry);
                    ry = 0;
                }
                if (rx >= w || ry >= h)
                    return;
                if (rx + rw > w)
                    rw = (int16_t)(w - rx);
                if (ry + rh > h)
                    rh = (int16_t)(h - ry);
                if (rw <= 0 || rh <= 0)
                    return;

                if (dirtyW == 0 || dirtyH == 0)
                {
                    dirtyX = rx;
                    dirtyY = ry;
                    dirtyW = rw;
                    dirtyH = rh;
                    return;
                }

                int16_t x0 = (dirtyX < rx) ? dirtyX : rx;
                int16_t y0 = (dirtyY < ry) ? dirtyY : ry;
                int16_t x1a = (int16_t)(dirtyX + dirtyW);
                int16_t x1b = (int16_t)(rx + rw);
                int16_t y1a = (int16_t)(dirtyY + dirtyH);
                int16_t y1b = (int16_t)(ry + rh);
                int16_t x1 = (x1a > x1b) ? x1a : x1b;
                int16_t y1 = (y1a > y1b) ? y1a : y1b;

                dirtyX = x0;
                dirtyY = y0;
                dirtyW = (int16_t)(x1 - x0);
                dirtyH = (int16_t)(y1 - y0);
            };

            auto shiftViewportY = [&](int32_t shift)
            {
                if (shift == 0)
                    return;

                uint16_t *buf = (uint16_t *)vp->getBuffer();
                if (!buf)
                    return;
                int32_t ww = vp->width();
                int32_t hh = vp->height();
                if (ww <= 0 || hh <= 0)
                    return;

                if (shift > 0)
                {
                    if (shift >= hh)
                        shift = hh;
                    size_t rowsToCopy = (size_t)(hh - shift);
                    memmove(buf, buf + (size_t)shift * (size_t)ww, rowsToCopy * (size_t)ww * sizeof(uint16_t));
                    vp->fillRect(0, (int16_t)(hh - shift), (int16_t)ww, (int16_t)shift, bgColor);
                }
                else
                {
                    int32_t d = -shift;
                    if (d >= hh)
                        d = hh;
                    size_t rowsToCopy = (size_t)(hh - d);
                    memmove(buf + (size_t)d * (size_t)ww, buf, rowsToCopy * (size_t)ww * sizeof(uint16_t));
                    vp->fillRect(0, 0, (int16_t)ww, (int16_t)d, bgColor);
                }
            };

            auto computeCardY = [&](uint8_t idx) -> int16_t
            {
                const int16_t contentTop = 4;
                float relIndex = (float)idx - m.scrollPos;
                float posY = (float)contentTop + relIndex * (float)itemSpanPx;
                return (int16_t)posY;
            };

            dirtyW = 0;
            dirtyH = 0;

            int32_t absDy = (dy < 0) ? -dy : dy;
            int32_t maxInc = (int32_t)itemSpanPx * 2;

            if (canIncremental && dy != 0 && absDy < h && absDy <= maxInc)
            {
                shiftViewportY(dy);

                if (dy > 0)
                {
                    clipRender(0, (int16_t)(h - dy), w, (int16_t)dy);
                    addDirty(0, (int16_t)(h - dy), w, (int16_t)dy);
                }
                else
                {
                    int32_t d = -dy;
                    clipRender(0, 0, w, (int16_t)d);
                    addDirty(0, 0, w, (int16_t)d);
                }
                incrementalUsed = true;
            }
            else if (canIncremental && dy == 0)
            {
                incrementalUsed = true;
            }

            if (incrementalUsed)
            {
                int16_t barW = 12;
                if (barW > w)
                    barW = w;
                if (barW > 0)
                {
                    clipRender((int16_t)(w - barW), 0, barW, h);
                    addDirty((int16_t)(w - barW), 0, barW, h);
                }

                if (oldSel != m.selectedIndex)
                {
                    int16_t cx = (int16_t)((usableW - cardW) / 2);

                    int16_t yyOld = computeCardY(oldSel);
                    if (!(yyOld >= h || (yyOld + cardH) <= 0))
                    {
                        clipRender(cx, yyOld, cardW, cardH);
                        addDirty(cx, yyOld, cardW, cardH);
                    }

                    int16_t yyNew = computeCardY(m.selectedIndex);
                    if (!(yyNew >= h || (yyNew + cardH) <= 0))
                    {
                        clipRender(cx, yyNew, cardW, cardH);
                        addDirty(cx, yyNew, cardW, cardH);
                    }
                }

                if (dirtyW <= 0 || dirtyH <= 0)
                {
                    dirtyX = 0;
                    dirtyY = 0;
                    dirtyW = w;
                    dirtyH = h;
                }
            }
            else
            {
                vp->setClipRect(0, 0, w, h);
                renderListMenu(screenId, 0, 0, w, h, bgColor);
                dirtyX = 0;
                dirtyY = 0;
                dirtyW = w;
                dirtyH = h;
            }

            vp->setClipRect(vpClipX, vpClipY, vpClipW, vpClipH);

            m.viewportX = x;
            m.viewportY = y;
            m.viewportW = w;
            m.viewportH = h;
            m.viewportCardW = cardW;
            m.viewportCardH = cardH;
            m.viewportItemSpanPx = itemSpanPx;
            m.viewportBgColor = bgColor;
            m.viewportScrollPos = m.scrollPos;
            m.viewportScrollPx = newScrollPx;
            m.viewportSelectedIndex = m.selectedIndex;
            m.viewportValid = true;
        }
        else
        {
            int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
            _sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);

            _sprite.setClipRect(x, y, w, h);

            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;
            _flags.needRedraw = 0;
            renderListMenu(screenId, x, y, w, h, bgColor);

            _sprite.setClipRect(clipX, clipY, clipW, clipH);
        }

        bool requestedMore = (_flags.needRedraw != 0);

        _flags.needRedraw = prevNeedRedraw;
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        if (usedViewport)
        {
            int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
            _sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);

            int32_t vpClipX = 0, vpClipY = 0, vpClipW = 0, vpClipH = 0;
            vp->getClipRect(&vpClipX, &vpClipY, &vpClipW, &vpClipH);

            if (incrementalUsed)
                vp->setClipRect(dirtyX, dirtyY, dirtyW, dirtyH);

            if (incrementalUsed)
                _sprite.setClipRect((int16_t)(x + dirtyX), (int16_t)(y + dirtyY), dirtyW, dirtyH);
            else
                _sprite.setClipRect(x, y, w, h);

            vp->pushSprite(&_sprite, x, y);

            if (incrementalUsed)
                vp->setClipRect(vpClipX, vpClipY, vpClipW, vpClipH);
            _sprite.setClipRect(clipX, clipY, clipW, clipH);
        }

        if (usedViewport && incrementalUsed)
        {
            invalidateRect((int16_t)(x + dirtyX), (int16_t)(y + dirtyY), dirtyW, dirtyH);
        }
        else
        {
            invalidateRect(x, y, w, h);
        }
        flushDirty();
        return requestedMore;
    }
}
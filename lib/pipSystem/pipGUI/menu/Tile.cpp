#include <pipGUI/core/api/pipGUI.h>
#include <pipGUI/core/utils/Colors.h>
#include <math.h>

namespace pipgui
{
    static bool ensureTileCapacityInternal(GUI *self, TileMenuState &m, uint8_t need)
    {
        if (!pipgui::detail::ensureCapacity(self->platform(), m.layout, m.itemCapacity, need))
            return false;

        if (!pipgui::detail::ensureCapacity(self->platform(), m.items, m.itemCapacity, need))
            return false;

        for (uint8_t i = 0; i < m.itemCapacity; ++i)
        {
            m.layout[i].col = 0;
            m.layout[i].row = 0;
            m.layout[i].colSpan = m.layout[i].colSpan == 0 ? 1 : m.layout[i].colSpan;
            m.layout[i].rowSpan = m.layout[i].rowSpan == 0 ? 1 : m.layout[i].rowSpan;
        }

        return true;
    }

    void GUI::configureTileMenu(uint8_t screenId, uint8_t parentScreen, const TileMenuItemDef *items, uint8_t itemCount)
    {
        if (screenId == 255 || !items || itemCount == 0)
            return;

        TileMenuState &m = ensureTileMenu(screenId);
        if (!ensureTileCapacityInternal(this, m, itemCount))
        {
            if (m.itemCapacity == 0)
                return;
            if (itemCount > m.itemCapacity)
                itemCount = m.itemCapacity;
        }

        m.configured = true;
        m.parentScreen = parentScreen;
        m.itemCount = itemCount;
        m.selectedIndex = 0;

        m.nextHoldStartMs = 0;
        m.prevHoldStartMs = 0;
        m.nextLongFired = false;
        m.prevLongFired = false;
        m.lastNextDown = false;
        m.lastPrevDown = false;

        m.customLayout = false;
        m.layoutCols = 0;
        m.layoutRows = 0;


        for (uint8_t i = 0; i < itemCount; ++i)
        {
            m.items[i].title = items[i].title ? String(items[i].title) : String("");
            m.items[i].subtitle = items[i].subtitle ? String(items[i].subtitle) : String("");
            m.items[i].targetScreen = items[i].targetScreen;
            m.items[i].iconBitmap = items[i].iconBitmap;
            m.items[i].iconW = items[i].iconW;
            m.items[i].iconH = items[i].iconH;
        }
        if (m.style.cardColor == 0 && m.style.cardActiveColor == 0)
        {
            uint32_t base = _bgColor ? _bgColor : 0x000000;
            m.style.cardColor = pipgui::detail::lighten888(base, 4);
            m.style.cardActiveColor = rgb(0, 130, 220);
            m.style.radius = 10;
            m.style.spacing = 8;
            m.style.columns = 2;
            m.style.tileWidth = 0;
            m.style.tileHeight = 0;
            m.style.titleFontPx = 0;
            m.style.subtitleFontPx = 0;
            m.style.lineGapPx = 0;
            m.style.contentMode = TextSubtitle;
        }
    }

    void GUI::configureTileMenu(uint8_t screenId, uint8_t parentScreen, std::initializer_list<TileMenuItemDef> items)
    {
        if (items.size() == 0)
            return;

        configureTileMenu(screenId,
                          parentScreen,
                          items.begin(),
                          static_cast<uint8_t>(items.size()));
    }

    void GUI::setTileMenuStyle(uint8_t screenId,
                               uint16_t cardColor,
                               uint16_t cardActiveColor,
                               uint8_t radius,
                               uint8_t spacing,
                               uint8_t columns,
                               int16_t tileWidth,
                               int16_t tileHeight,
                               uint16_t titleFontPx,
                               uint16_t subtitleFontPx,
                               uint16_t lineGapPx,
                               TileContentMode contentMode)
    {
        if (screenId == 255)
            return;

        TileMenuState &m = ensureTileMenu(screenId);
        m.style.cardColor = cardColor;
        m.style.cardActiveColor = cardActiveColor;
        m.style.radius = radius;
        m.style.spacing = spacing;
        m.style.columns = columns;
        m.style.tileWidth = tileWidth;
        m.style.tileHeight = tileHeight;
        m.style.titleFontPx = titleFontPx;
        m.style.subtitleFontPx = subtitleFontPx;
        m.style.lineGapPx = lineGapPx;
        m.style.contentMode = contentMode;
    }

    void GUI::setTileMenuLayout(uint8_t screenId,
                                uint8_t layoutCols,
                                uint8_t layoutRows,
                                const TileLayoutCell *tiles,
                                uint8_t count)
    {
        if (screenId == 255)
            return;

        TileMenuState &m = ensureTileMenu(screenId);
        if (!m.configured || m.itemCount == 0)
            return;

        if (!ensureTileCapacityInternal(this, m, m.itemCount))
        {
            return;
        }

        if (layoutCols == 0 || layoutRows == 0)
        {
            m.customLayout = false;
            m.layoutCols = 0;
            m.layoutRows = 0;
            return;
        }

        m.customLayout = true;
        m.layoutCols = layoutCols;
        m.layoutRows = layoutRows;

        if (count > m.itemCount)
            count = m.itemCount;

        for (uint8_t i = 0; i < count; ++i)
        {
            TileLayoutCell c = tiles ? tiles[i] : TileLayoutCell{0, 0, 1, 1};
            if (c.colSpan == 0)
                c.colSpan = 1;
            if (c.rowSpan == 0)
                c.rowSpan = 1;
            if (c.col >= layoutCols)
                c.col = (uint8_t)(layoutCols - 1);
            if (c.row >= layoutRows)
                c.row = (uint8_t)(layoutRows - 1);
            if (c.col + c.colSpan > layoutCols)
                c.colSpan = (uint8_t)(layoutCols - c.col);
            if (c.row + c.rowSpan > layoutRows)
                c.rowSpan = (uint8_t)(layoutRows - c.row);

            m.layout[i] = c;
        }

        for (uint8_t i = count; i < m.itemCount; ++i)
        {
            uint8_t col = (uint8_t)(i % layoutCols);
            uint8_t row = (uint8_t)((i / layoutCols) % layoutRows);
            m.layout[i].col = col;
            m.layout[i].row = row;
            m.layout[i].colSpan = 1;
            m.layout[i].rowSpan = 1;
        }
    }

    void GUI::setTileMenuLayout(uint8_t screenId,
                                uint8_t layoutCols,
                                uint8_t layoutRows,
                                std::initializer_list<TileLayoutCell> tiles)
    {
        if (tiles.size() == 0)
        {
            setTileMenuLayout(screenId, 0, 0, (const TileLayoutCell *)nullptr, 0);
            return;
        }
        setTileMenuLayout(screenId,
                          layoutCols,
                          layoutRows,
                          tiles.begin(),
                          static_cast<uint8_t>(tiles.size()));
    }

    bool GUI::ensureTileCapacity(TileMenuState &m, uint8_t need)
    {
        return ensureTileCapacityInternal(this, m, need);
    }

    void GUI::handleTileMenuInput(uint8_t screenId,
                                  bool nextPressed,
                                  bool prevPressed,
                                  bool nextDown,
                                  bool prevDown)
    {
        (void)nextPressed;
        (void)prevPressed;

        TileMenuState *pm = getTileMenu(screenId);
        if (!pm)
            return;

        TileMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        uint8_t prevSelected = m.selectedIndex;

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
            if (_flags.spriteEnabled && _tft && !_flags.renderToSprite && _currentScreen == screenId)
                updateTileMenu(screenId, prevSelected);
            else
                requestRedraw();
        }
    }

    void GUI::updateTileMenu(uint8_t screenId, uint8_t prevSelectedIndex)
    {
        TileMenuState *pm = getTileMenu(screenId);
        if (!pm)
            return;

        TileMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;
            _flags.renderToSprite = 0;
            renderTileMenu(screenId);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        if (prevSelectedIndex >= m.itemCount)
            prevSelectedIndex = m.selectedIndex;
        if (m.selectedIndex >= m.itemCount)
            m.selectedIndex = m.itemCount - 1;

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
        if (right - left <= 0 || bottom - top <= 0)
            return;

        int16_t usableW = right - left;
        int16_t usableH = bottom - top;

        uint8_t cols = 1;
        uint8_t rows = 1;
        if (m.customLayout && m.layoutCols > 0 && m.layoutRows > 0)
        {
            cols = m.layoutCols;
            rows = m.layoutRows;
        }
        else
        {
            cols = m.style.columns ? m.style.columns : 2;
            if (cols < 1)
                cols = 1;
            rows = (uint8_t)((m.itemCount + cols - 1) / cols);
            if (rows < 1)
                rows = 1;
        }

        uint8_t spacing = m.style.spacing;
        if (spacing == 0)
            spacing = 8;

        int16_t unitW = 0;
        int16_t unitH = 0;

        if (m.style.tileWidth > 0 && m.style.tileWidth < usableW)
            unitW = m.style.tileWidth;
        if (m.style.tileHeight > 0 && m.style.tileHeight < usableH)
            unitH = m.style.tileHeight;

        if (unitW <= 0)
        {
            if (cols > 1)
                unitW = (usableW - (int16_t)spacing * (cols - 1)) / cols;
            else
                unitW = usableW - 16;
        }
        if (unitH <= 0)
        {
            if (rows > 1)
                unitH = (usableH - (int16_t)spacing * (rows - 1)) / rows;
            else
                unitH = usableH - 16;
        }

        if (unitW < 20)
            unitW = 20;
        if (unitH < 20)
            unitH = 20;

        int16_t gridW = (int16_t)cols * unitW + (int16_t)spacing * (cols - 1);
        int16_t gridH = (int16_t)rows * unitH + (int16_t)spacing * (rows - 1);

        if (gridW > usableW || gridH > usableH)
        {
            if (cols > 1)
                unitW = (usableW - (int16_t)spacing * (cols - 1)) / cols;
            else
                unitW = usableW - 16;
            if (rows > 1)
                unitH = (usableH - (int16_t)spacing * (rows - 1)) / rows;
            else
                unitH = usableH - 16;

            if (unitW < 20)
                unitW = 20;
            if (unitH < 20)
                unitH = 20;

            gridW = (int16_t)cols * unitW + (int16_t)spacing * (cols - 1);
            gridH = (int16_t)rows * unitH + (int16_t)spacing * (rows - 1);
        }

        int16_t gridX = left + (usableW - gridW) / 2;
        int16_t gridY = top + (usableH - gridH) / 2;

        auto tileRect = [&](uint8_t idx, int16_t &x, int16_t &y, int16_t &w, int16_t &h)
        {
            uint8_t row = 0;
            uint8_t col = 0;
            uint8_t colSpan = 1;
            uint8_t rowSpan = 1;

            if (m.customLayout && cols > 0 && rows > 0)
            {
                const TileLayoutCell &c = m.layout[idx];
                col = c.col;
                row = c.row;
                colSpan = c.colSpan ? c.colSpan : 1;
                rowSpan = c.rowSpan ? c.rowSpan : 1;
                if (col >= cols)
                    col = (uint8_t)(cols - 1);
                if (row >= rows)
                    row = (uint8_t)(rows - 1);
                if (col + colSpan > cols)
                    colSpan = (uint8_t)(cols - col);
                if (row + rowSpan > rows)
                    rowSpan = (uint8_t)(rows - row);
            }
            else
            {
                row = (uint8_t)(idx / cols);
                col = (uint8_t)(idx % cols);
            }

            x = gridX + (int16_t)col * (unitW + spacing);
            y = gridY + (int16_t)row * (unitH + spacing);

            w = (int16_t)colSpan * unitW + (int16_t)(colSpan - 1) * spacing;
            h = (int16_t)rowSpan * unitH + (int16_t)(rowSpan - 1) * spacing;
            if (w < 20)
                w = 20;
            if (h < 20)
                h = 20;
        };

        int16_t xOld = 0, yOld = 0, wOld = 0, hOld = 0;
        int16_t xNew = 0, yNew = 0, wNew = 0, hNew = 0;
        tileRect(prevSelectedIndex, xOld, yOld, wOld, hOld);
        tileRect(m.selectedIndex, xNew, yNew, wNew, hNew);

        int16_t pad = 2;
        int16_t bxOld = (int16_t)(xOld - pad);
        int16_t byOld = (int16_t)(yOld - pad);
        int16_t bwOld = (int16_t)(wOld + pad * 2);
        int16_t bhOld = (int16_t)(hOld + pad * 2);

        int16_t bxNew = (int16_t)(xNew - pad);
        int16_t byNew = (int16_t)(yNew - pad);
        int16_t bwNew = (int16_t)(wNew + pad * 2);
        int16_t bhNew = (int16_t)(hNew + pad * 2);

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        _sprite.setClipRect(bxOld, byOld, bwOld, bhOld);
        renderTileMenu(screenId);

        if (prevSelectedIndex != m.selectedIndex)
        {
            _sprite.setClipRect(bxNew, byNew, bwNew, bhNew);
            renderTileMenu(screenId);
        }

        _sprite.setClipRect(clipX, clipY, clipW, clipH);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect(bxOld, byOld, bwOld, bhOld);
        if (prevSelectedIndex != m.selectedIndex)
            invalidateRect(bxNew, byNew, bwNew, bhNew);
        flushDirty();
    }

    void GUI::renderTileMenu(uint8_t screenId)
    {
        TileMenuState *pm = getTileMenu(screenId);
        if (!pm)
            return;

        TileMenuState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        LovyanGFX *t = getDrawTarget();

        auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };

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
        if (right - left <= 0 || bottom - top <= 0)
            return;

        int16_t usableW = right - left;
        int16_t usableH = bottom - top;

        t->fillRect(left, top, usableW, bottom - top, to565(_bgColor));

        uint8_t cols = 1;
        uint8_t rows = 1;
        if (m.customLayout && m.layoutCols > 0 && m.layoutRows > 0)
        {
            cols = m.layoutCols;
            rows = m.layoutRows;
        }
        else
        {
            cols = m.style.columns ? m.style.columns : 2;
            if (cols < 1)
                cols = 1;
            rows = (uint8_t)((m.itemCount + cols - 1) / cols);
            if (rows < 1)
                rows = 1;
        }

        uint8_t spacing = m.style.spacing;
        if (spacing == 0)
            spacing = 8;


        int16_t unitW = 0;
        int16_t unitH = 0;

        if (m.style.tileWidth > 0 && m.style.tileWidth < usableW)
            unitW = m.style.tileWidth;
        if (m.style.tileHeight > 0 && m.style.tileHeight < usableH)
            unitH = m.style.tileHeight;

        if (unitW <= 0)
        {
            if (cols > 1)
                unitW = (usableW - (int16_t)spacing * (cols - 1)) / cols;
            else
                unitW = usableW - 16;
        }
        if (unitH <= 0)
        {
            if (rows > 1)
                unitH = (usableH - (int16_t)spacing * (rows - 1)) / rows;
            else
                unitH = usableH - 16;
        }

        if (unitW < 20)
            unitW = 20;
        if (unitH < 20)
            unitH = 20;

        int16_t gridW = (int16_t)cols * unitW + (int16_t)spacing * (cols - 1);
        int16_t gridH = (int16_t)rows * unitH + (int16_t)spacing * (rows - 1);

        if (gridW > usableW || gridH > usableH)
        {
            if (cols > 1)
                unitW = (usableW - (int16_t)spacing * (cols - 1)) / cols;
            else
                unitW = usableW - 16;
            if (rows > 1)
                unitH = (usableH - (int16_t)spacing * (rows - 1)) / rows;
            else
                unitH = usableH - 16;

            if (unitW < 20)
                unitW = 20;
            if (unitH < 20)
                unitH = 20;

            gridW = (int16_t)cols * unitW + (int16_t)spacing * (cols - 1);
            gridH = (int16_t)rows * unitH + (int16_t)spacing * (rows - 1);
        }

        int16_t gridX = left + (usableW - gridW) / 2;
        int16_t gridY = top + (usableH - gridH) / 2;

        uint8_t r = m.style.radius;
        if (r < 2)
            r = 2;

        for (uint8_t i = 0; i < m.itemCount; ++i)
        {
            uint8_t row = 0;
            uint8_t col = 0;
            uint8_t colSpan = 1;
            uint8_t rowSpan = 1;

            if (m.customLayout && cols > 0 && rows > 0)
            {
                const TileLayoutCell &c = m.layout[i];
                col = c.col;
                row = c.row;
                colSpan = c.colSpan ? c.colSpan : 1;
                rowSpan = c.rowSpan ? c.rowSpan : 1;
                if (col >= cols)
                    col = (uint8_t)(cols - 1);
                if (row >= rows)
                    row = (uint8_t)(rows - 1);
                if (col + colSpan > cols)
                    colSpan = (uint8_t)(cols - col);
                if (row + rowSpan > rows)
                    rowSpan = (uint8_t)(rows - row);
            }
            else
            {
                row = (uint8_t)(i / cols);
                col = (uint8_t)(i % cols);
            }

            int16_t x = gridX + (int16_t)col * (unitW + spacing);
            int16_t y = gridY + (int16_t)row * (unitH + spacing);

            int16_t tileW = (int16_t)colSpan * unitW + (int16_t)(colSpan - 1) * spacing;
            int16_t tileH = (int16_t)rowSpan * unitH + (int16_t)(rowSpan - 1) * spacing;
            if (tileW < 20)
                tileW = 20;
            if (tileH < 20)
                tileH = 20;

            uint32_t bg = (i == m.selectedIndex) ? m.style.cardActiveColor : m.style.cardColor;
            if (bg == 0)
                bg = _bgColor;

            auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };

            if (_frcProfile == FrcProfile::Off)
            {
                t->fillSmoothRoundRect(x, y, tileW, tileH, r, to565(bg));
                uint32_t border = pipgui::detail::lighten888(bg, 4);
                t->drawRoundRect(x, y, tileW, tileH, r, to565(border));
            }
            else
            {
                // tile-based FRC fill for rounded rect
                Color888 cc{(uint8_t)((bg >> 16) & 0xFF), (uint8_t)((bg >> 8) & 0xFF), (uint8_t)(bg & 0xFF)};
                uint16_t tileBuf[16 * 16];
                for (int16_t ty = 0; ty < 16; ++ty)
                    for (int16_t tx = 0; tx < 16; ++tx)
                        tileBuf[(uint16_t)ty * 16U + (uint16_t)tx] = detail::quantize565(cc, tx, ty, _frcSeed, _frcProfile);

                int16_t xx0 = x;
                int16_t yy0 = y;
                int16_t ww = tileW;
                int16_t hh = tileH;
                int16_t rr = (int16_t)(r >= 1 ? r - 1 : 0);
                int32_t rSq = (int32_t)rr * (int32_t)rr;

                auto pointInRounded = [&](int16_t px, int16_t py) -> bool {
                    int16_t lx = (int16_t)(px - xx0);
                    int16_t ly = (int16_t)(py - yy0);
                    if (lx < 0 || ly < 0 || lx >= ww || ly >= hh)
                        return false;
                    if (lx >= r && lx < ww - r)
                        return true;
                    if (ly >= r && ly < hh - r)
                        return true;
                    // corners
                    if (lx < r && ly < r)
                    {
                        int32_t dx = (int32_t)(rr - lx);
                        int32_t dy = (int32_t)(rr - ly);
                        return dx * dx + dy * dy <= rSq;
                    }
                    else if (lx >= ww - r && ly < r)
                    {
                        int32_t dx = (int32_t)(lx - (ww - r));
                        int32_t dy = (int32_t)(rr - ly);
                        return dx * dx + dy * dy <= rSq;
                    }
                    else if (lx < r && ly >= hh - r)
                    {
                        int32_t dx = (int32_t)(rr - lx);
                        int32_t dy = (int32_t)(ly - (hh - r));
                        return dx * dx + dy * dy <= rSq;
                    }
                    else if (lx >= ww - r && ly >= hh - r)
                    {
                        int32_t dx = (int32_t)(lx - (ww - r));
                        int32_t dy = (int32_t)(ly - (hh - r));
                        return dx * dx + dy * dy <= rSq;
                    }
                    return true;
                };

                // fill
                for (int16_t py = y; py < y + tileH; ++py)
                {
                    if (py < 0 || py >= _screenHeight) continue;
                    for (int16_t px = x; px < x + tileW; ++px)
                    {
                        if (px < 0 || px >= _screenWidth) continue;
                        if (!pointInRounded(px, py))
                            continue;
                        uint16_t out = tileBuf[((uint16_t)py & 15U) * 16U + ((uint16_t)px & 15U)];
                        t->drawPixel(px, py, out);
                    }
                }

                // border (1px) using quantized border color
                uint32_t borderColor = pipgui::detail::lighten888(bg, 4);
                Color888 bc{(uint8_t)((borderColor >> 16) & 0xFF), (uint8_t)((borderColor >> 8) & 0xFF), (uint8_t)(borderColor & 0xFF)};
                for (int16_t py = y; py < y + tileH; ++py)
                {
                    if (py < 0 || py >= _screenHeight) continue;
                    for (int16_t px = x; px < x + tileW; ++px)
                    {
                        if (px < 0 || px >= _screenWidth) continue;
                        if (!pointInRounded(px, py))
                            continue;
                        // check if any neighbor is outside -> border pixel
                        bool borderPixel = false;
                        const int dxs[4] = {1, -1, 0, 0};
                        const int dys[4] = {0, 0, 1, -1};
                        for (int di = 0; di < 4; ++di)
                        {
                            int nx = px + dxs[di];
                            int ny = py + dys[di];
                            if (nx < x || ny < y || nx >= x + tileW || ny >= y + tileH || !pointInRounded(nx, ny))
                            {
                                borderPixel = true;
                                break;
                            }
                        }
                        if (borderPixel)
                        {
                            uint16_t outb = detail::quantize565(bc, px, py, _frcSeed, _frcProfile);
                            t->drawPixel(px, py, outb);
                        }
                    }
                }
            }

            uint32_t txtCol = pipgui::detail::autoTextColorForBg(bg);
            auto from565 = [](uint16_t c) -> uint32_t { uint8_t r5 = (c >> 11) & 0x1F; uint8_t g6 = (c >> 5) & 0x3F; uint8_t b5 = c & 0x1F; uint8_t r8 = (uint8_t)((r5 * 255U) / 31U); uint8_t g8 = (uint8_t)((g6 * 255U) / 63U); uint8_t b8 = (uint8_t)((b5 * 255U) / 31U); return (uint32_t)((r8 << 16) | (g8 << 8) | b8); };
            uint32_t subCol = pipgui::detail::lighten888((txtCol == 0xFFFFFF ? from565(0xC618) : from565(0x8410)), 0);

            const TileMenuState::Item &it = m.items[i];
            const String &title = it.title;
            const String &sub = it.subtitle;

            bool hasSub = (m.style.contentMode == TextSubtitle && sub.length() > 0);
            bool hasIcon = (it.iconBitmap && it.iconW > 0 && it.iconH > 0);

            int16_t centerX = x + tileW / 2;

            int16_t titleH = 0, subH = 0;
            int16_t titleW = 0, subW = 0;
            int16_t totalTextH = 0;
            int16_t maxTextW = 0;

            uint16_t titlePx = m.style.titleFontPx;
            uint16_t subPx = m.style.subtitleFontPx;
            uint16_t gapPx = m.style.lineGapPx;

            {
                if (titlePx == 0)
                    titlePx = hasSub ? 18 : 20;
                if (hasSub)
                {
                    if (subPx == 0)
                        subPx = (uint16_t)((titlePx * 7U) / 10U);
                    if (gapPx == 0)
                        gapPx = (uint16_t)(titlePx / 3U);
                }
                else
                {
                    subPx = 0;
                    gapPx = 0;
                }

                setPSDFFontSize(titlePx);
                psdfMeasureText(title, titleW, titleH);

                if (hasSub && subPx > 0)
                {
                    setPSDFFontSize(subPx);
                    psdfMeasureText(sub, subW, subH);
                }

                totalTextH = titleH;
                if (hasSub && subH > 0)
                    totalTextH += gapPx + subH;

                maxTextW = titleW;
                if (hasSub && subW > 0 && subW > maxTextW)
                    maxTextW = subW;
            }

            int16_t iconW = hasIcon ? it.iconW : 0;
            int16_t iconH = hasIcon ? it.iconH : 0;
            int16_t gapIconText = (hasIcon && totalTextH > 0) ? 6 : 0;

            int16_t blockH = totalTextH;
            if (hasIcon)
                blockH += iconH + gapIconText;
            if (blockH <= 0)
                blockH = hasIcon ? iconH : 0;

            int16_t baseY = y + (int16_t)((tileH - blockH) / 2);
            if (baseY < y + 2)
                baseY = (int16_t)(y + 2);

            int16_t iconY = baseY;
            int16_t textTopY = baseY + (hasIcon ? (iconH + gapIconText) : 0);

            if (hasIcon)
            {
                int16_t iconX = centerX - iconW / 2;
                t->pushImage(iconX, iconY, iconW, iconH, it.iconBitmap);
            }

            {
                int16_t tyTitle = textTopY;
                int16_t tySub = textTopY + titleH + (hasSub ? gapPx : 0);

                int16_t titleX = centerX - titleW / 2;
                int16_t subX = centerX - subW / 2;

                setPSDFFontSize(titlePx);
                psdfDrawTextInternal(title, titleX, tyTitle, to565(txtCol), to565(bg), AlignLeft);

                if (hasSub && subPx > 0)
                {
                    setPSDFFontSize(subPx);
                    psdfDrawTextInternal(sub, subX, tySub, to565(subCol), to565(bg), AlignLeft);
                }
            }
        }
    }
}

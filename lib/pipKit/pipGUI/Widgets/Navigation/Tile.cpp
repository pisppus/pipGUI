#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <algorithm>

namespace pipgui
{
    namespace
    {
        constexpr uint8_t kDefaultTileRadius = 17;

        struct TileLayoutCell
        {
            uint8_t col;
            uint8_t row;
            uint8_t colSpan;
            uint8_t rowSpan;

            constexpr TileLayoutCell(uint8_t c = 0, uint8_t r = 0, uint8_t cs = 1, uint8_t rs = 1)
                : col(c), row(r), colSpan(cs), rowSpan(rs) {}
        };

        struct TileViewport
        {
            int16_t left = 0;
            int16_t right = 0;
            int16_t top = 0;
            int16_t bottom = 0;
            int16_t usableW = 0;
            int16_t usableH = 0;
        };

        struct TileGridMetrics
        {
            uint8_t cols = 1;
            uint8_t rows = 1;
            uint8_t spacing = 8;
            uint8_t radius = 10;
            int16_t unitW = 20;
            int16_t unitH = 20;
            int16_t gridX = 0;
            int16_t gridY = 0;
        };

        static void resetTileRuntime(TileState &menu)
        {
            menu.selectedIndex = 0;
            menu.nextHoldStartMs = menu.prevHoldStartMs = 0;
            menu.nextLongFired = menu.prevLongFired = false;
            menu.lastNextDown = menu.lastPrevDown = false;
            menu.marqueeStartMs = 0;
            menu.customLayout = false;
            menu.layoutCols = 0;
            menu.layoutRows = 0;
        }

        static TileStyle normalizedTileStyle(const TileStyle &style, uint32_t bg888)
        {
            TileStyle out = style;
            const uint32_t base = bg888 ? bg888 : 0x000000;

            if (out.cardColor == 0)
                out.cardColor = detail::lighten888(base, 4);
            if (out.cardActiveColor == 0)
                out.cardActiveColor = 0x0082DC;
            if (out.radius == 0)
                out.radius = kDefaultTileRadius;
            if (out.spacing == 0)
                out.spacing = 8;
            if (out.columns == 0)
                out.columns = 2;

            return out;
        }

        static bool resolveTileViewport(int16_t screenWidth,
                                        int16_t screenHeight,
                                        bool reserveStatusBar,
                                        int16_t statusBarHeight,
                                        StatusBarPosition statusBarPos,
                                        TileViewport &out)
        {
            out.left = 0;
            out.right = screenWidth;
            out.top = 0;
            out.bottom = screenHeight;
            if (reserveStatusBar && statusBarHeight > 0)
            {
                if (statusBarPos == Top)
                    out.top += statusBarHeight;
                else if (statusBarPos == Bottom)
                    out.bottom -= statusBarHeight;
            }

            out.usableW = out.right - out.left;
            out.usableH = out.bottom - out.top;
            return out.usableW > 0 && out.usableH > 0;
        }

        static TileLayoutCell normalizedTileCell(TileLayoutCell cell, uint8_t cols, uint8_t rows)
        {
            if (cell.colSpan == 0)
                cell.colSpan = 1;
            if (cell.rowSpan == 0)
                cell.rowSpan = 1;
            if (cell.col >= cols)
                cell.col = (uint8_t)(cols - 1);
            if (cell.row >= rows)
                cell.row = (uint8_t)(rows - 1);
            if (cell.col + cell.colSpan > cols)
                cell.colSpan = (uint8_t)(cols - cell.col);
            if (cell.row + cell.rowSpan > rows)
                cell.rowSpan = (uint8_t)(rows - cell.row);
            return cell;
        }

        static TileLayoutCell tileCellAt(const TileState &menu, uint8_t idx, uint8_t cols, uint8_t rows)
        {
            if (menu.customLayout && menu.layoutCols > 0 && menu.layoutRows > 0)
                return normalizedTileCell(TileLayoutCell(menu.items[idx].layoutCol,
                                                         menu.items[idx].layoutRow,
                                                         menu.items[idx].layoutColSpan,
                                                         menu.items[idx].layoutRowSpan),
                                          cols,
                                          rows);

            return TileLayoutCell(static_cast<uint8_t>(idx % cols),
                                  static_cast<uint8_t>(idx / cols),
                                  1,
                                  1);
        }

        static void resolveTileGrid(const TileState &menu, const TileViewport &viewport, TileGridMetrics &out)
        {
            const auto resolveUnit = [](int16_t requested, int16_t usable, uint8_t count, uint8_t spacing) noexcept
            {
                const int16_t unit = (requested > 0 && requested < usable)
                                         ? requested
                                     : (count > 1)
                                         ? (usable - (int16_t)spacing * (count - 1)) / count
                                         : (usable - 16);
                return std::max<int16_t>(20, unit);
            };

            out.cols = 1;
            out.rows = 1;
            if (menu.customLayout && menu.layoutCols > 0 && menu.layoutRows > 0)
            {
                out.cols = menu.layoutCols;
                out.rows = menu.layoutRows;
            }
            else
            {
                out.cols = menu.style.columns ? menu.style.columns : 2;
                out.rows = (uint8_t)((menu.itemCount + out.cols - 1) / out.cols);
            }

            out.spacing = menu.style.spacing ? menu.style.spacing : 8;
            out.radius = std::max<uint8_t>(2, menu.style.radius ? menu.style.radius : kDefaultTileRadius);

            out.unitW = resolveUnit(menu.style.tileWidth, viewport.usableW, out.cols, out.spacing);
            out.unitH = resolveUnit(menu.style.tileHeight, viewport.usableH, out.rows, out.spacing);

            int16_t gridW = (int16_t)out.cols * out.unitW + (int16_t)out.spacing * (out.cols - 1);
            int16_t gridH = (int16_t)out.rows * out.unitH + (int16_t)out.spacing * (out.rows - 1);

            if (gridW > viewport.usableW || gridH > viewport.usableH)
            {
                out.unitW = resolveUnit(0, viewport.usableW, out.cols, out.spacing);
                out.unitH = resolveUnit(0, viewport.usableH, out.rows, out.spacing);

                gridW = (int16_t)out.cols * out.unitW + (int16_t)out.spacing * (out.cols - 1);
                gridH = (int16_t)out.rows * out.unitH + (int16_t)out.spacing * (out.rows - 1);
            }

            out.gridX = viewport.left + (viewport.usableW - gridW) / 2;
            out.gridY = viewport.top + (viewport.usableH - gridH) / 2;
        }

        static void tileRectAt(const TileGridMetrics &grid,
                               const TileLayoutCell &cell,
                               int16_t &x,
                               int16_t &y,
                               int16_t &w,
                               int16_t &h)
        {
            x = grid.gridX + (int16_t)cell.col * (grid.unitW + grid.spacing);
            y = grid.gridY + (int16_t)cell.row * (grid.unitH + grid.spacing);
            w = (int16_t)cell.colSpan * grid.unitW + (int16_t)(cell.colSpan - 1) * grid.spacing;
            h = (int16_t)cell.rowSpan * grid.unitH + (int16_t)(cell.rowSpan - 1) * grid.spacing;
            w = std::max<int16_t>(20, w);
            h = std::max<int16_t>(20, h);
        }

        static void tileRectAtIndex(const TileState &menu,
                                    uint8_t idx,
                                    const TileGridMetrics &grid,
                                    int16_t &x,
                                    int16_t &y,
                                    int16_t &w,
                                    int16_t &h)
        {
            tileRectAt(grid, tileCellAt(menu, idx, grid.cols, grid.rows), x, y, w, h);
        }

        static void applyTileLayout(TileState &menu,
                                    uint8_t layoutCols,
                                    uint8_t layoutRows,
                                    const TileLayoutCell *tiles,
                                    uint8_t count)
        {
            if (layoutCols == 0 || layoutRows == 0)
            {
                menu.customLayout = false;
                menu.layoutCols = 0;
                menu.layoutRows = 0;
                return;
            }

            menu.customLayout = true;
            menu.layoutCols = layoutCols;
            menu.layoutRows = layoutRows;

            if (count > menu.itemCount)
                count = menu.itemCount;

            for (uint8_t i = 0; i < count; ++i)
            {
                const TileLayoutCell cell = normalizedTileCell(tiles ? tiles[i] : TileLayoutCell{}, layoutCols, layoutRows);
                menu.items[i].layoutCol = cell.col;
                menu.items[i].layoutRow = cell.row;
                menu.items[i].layoutColSpan = cell.colSpan;
                menu.items[i].layoutRowSpan = cell.rowSpan;
            }

            for (uint8_t i = count; i < menu.itemCount; ++i)
            {
                menu.items[i].layoutCol = static_cast<uint8_t>(i % layoutCols);
                menu.items[i].layoutRow = static_cast<uint8_t>((i / layoutCols) % layoutRows);
                menu.items[i].layoutColSpan = 1;
                menu.items[i].layoutRowSpan = 1;
            }
        }
    }

    static void resetTileItemCache(TileState::Item &item)
    {
        item.titleW = item.titleH = item.subW = item.subH = 0;
        item.cachedTitlePx = item.cachedSubPx = 0;
        item.cachedTitleWeight = item.cachedSubWeight = 0;
    }

    static bool initTileItem(TileState::Item &item, const TileItemDef &def)
    {
        if (!detail::assignString(item.title, def.title) || !detail::assignString(item.subtitle, def.subtitle))
            return false;

        item.targetScreen = def.targetScreen;
        item.iconId = def.iconId;
        item.layoutCol = 0;
        item.layoutRow = 0;
        item.layoutColSpan = 1;
        item.layoutRowSpan = 1;
        resetTileItemCache(item);
        return true;
    }

    static bool ensureTileCapacityInternal(GUI *self, TileState &m, uint8_t need)
    {
        return pipgui::detail::ensureCapacity(self->platform(), m.items, m.itemCapacity, need);
    }

    static bool parseTileLayoutSpec(const char *const *rows,
                                    uint8_t rowCount,
                                    uint8_t itemCount,
                                    TileLayoutCell *outCells,
                                    uint8_t &outCellCount,
                                    uint8_t &outCols,
                                    uint8_t &outRows)
    {
        outCellCount = 0;
        outCols = 0;
        outRows = rowCount;

        if (!rows || rowCount == 0)
            return true;

        const size_t cols = rows[0] ? strlen(rows[0]) : 0;
        if (cols == 0 || cols > 255)
            return false;

        struct Bounds
        {
            uint8_t minCol = 255;
            uint8_t minRow = 255;
            uint8_t maxCol = 0;
            uint8_t maxRow = 0;
            uint16_t count = 0;
            bool used = false;
        };

        Bounds bounds[256] = {};
        uint8_t symbolToIndex[256];
        std::fill_n(symbolToIndex, 256, 0xFF);
        uint8_t order[256] = {};

        for (uint8_t row = 0; row < rowCount; ++row)
        {
            if (!rows[row] || strlen(rows[row]) != cols)
                return false;

            for (uint8_t col = 0; col < cols; ++col)
            {
                const uint8_t key = static_cast<uint8_t>(rows[row][col]);
                if (key == ' ' || key == '.')
                    continue;

                uint8_t index = symbolToIndex[key];
                if (index == 0xFF)
                {
                    if (outCellCount >= itemCount)
                        return false;
                    index = outCellCount;
                    symbolToIndex[key] = index;
                    order[outCellCount++] = key;
                }

                Bounds &b = bounds[index];
                if (!b.used)
                {
                    b.minCol = b.maxCol = col;
                    b.minRow = b.maxRow = row;
                    b.used = true;
                }
                else
                {
                    if (col < b.minCol)
                        b.minCol = col;
                    if (col > b.maxCol)
                        b.maxCol = col;
                    if (row < b.minRow)
                        b.minRow = row;
                    if (row > b.maxRow)
                        b.maxRow = row;
                }
                ++b.count;
            }
        }

        for (uint8_t i = 0; i < outCellCount; ++i)
        {
            const uint8_t key = order[i];
            const Bounds &b = bounds[i];
            if (!b.used)
                return false;

            const uint8_t spanCols = (uint8_t)(b.maxCol - b.minCol + 1);
            const uint8_t spanRows = (uint8_t)(b.maxRow - b.minRow + 1);
            const uint16_t area = static_cast<uint16_t>(spanCols) * static_cast<uint16_t>(spanRows);
            if (area != b.count)
                return false;

            for (uint8_t row = b.minRow; row <= b.maxRow; ++row)
            {
                for (uint8_t col = b.minCol; col <= b.maxCol; ++col)
                {
                    if (static_cast<uint8_t>(rows[row][col]) != key)
                        return false;
                }
            }

            outCells[i] = TileLayoutCell(b.minCol, b.minRow, spanCols, spanRows);
        }

        outCols = static_cast<uint8_t>(cols);
        return true;
    }

    void TileInputFluent::apply()
    {
        if (_consumed || !_gui)
            return;
        _consumed = true;
        detail::GuiAccess::handleTileInput(*_gui, _screenId, _nextDown, _prevDown);
    }

    void ConfigureTileFluent::apply()
    {
        if (_consumed || !_gui || !_items || _itemCount == 0)
            return;
        _consumed = true;
        if (_screenId == INVALID_SCREEN_ID)
            return;

        const TileStyle style = {
            _cardColor,
            _cardActiveColor,
            _radius,
            _spacing,
            _columns,
            _tileWidth,
            _tileHeight,
            _lineGapPx,
            _contentMode};
        detail::GuiAccess::configureTile(*_gui, _screenId, _items, _itemCount, style);
        if (_layoutConfigured)
        {
            TileLayoutCell layoutCells[255] = {};
            uint8_t layoutCount = 0;
            uint8_t layoutCols = 0;
            uint8_t layoutRows = 0;
            if (parseTileLayoutSpec(_layoutRowsSpec, _layoutRowCount, _itemCount, layoutCells, layoutCount, layoutCols, layoutRows))
            {
                TileState *menu = detail::GuiAccess::getTile(*_gui, _screenId);
                if (menu)
                    applyTileLayout(*menu, layoutCols, layoutRows, layoutCells, layoutCount);
            }
        }
    }

    void GUI::configureTile(uint8_t screenId, const TileItemDef *items, uint8_t itemCount, const TileStyle &style)
    {
        if (screenId == INVALID_SCREEN_ID || !items || itemCount == 0)
            return;

        TileState *m = ensureTile(screenId);
        if (!m)
            return;
        if (!ensureTileCapacityInternal(this, *m, itemCount))
        {
            if (m->itemCapacity == 0)
                return;
            if (itemCount > m->itemCapacity)
                itemCount = m->itemCapacity;
        }

        m->configured = true;
        m->itemCount = itemCount;
        resetTileRuntime(*m);
        m->style = normalizedTileStyle(style, _render.bgColor);

        for (uint8_t i = 0; i < itemCount; ++i)
        {
            if (!initTileItem(m->items[i], items[i]))
            {
                m->configured = false;
                return;
            }
        }
    }

    void GUI::handleTileInput(uint8_t screenId,
                              bool nextDown,
                              bool prevDown)
    {
        TileState *pm = getTile(screenId);
        if (!pm)
            return;

        TileState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

#if PIPGUI_SCREENSHOTS
        if (nextDown && prevDown)
        {
            m.nextHoldStartMs = 0;
            m.prevHoldStartMs = 0;
            m.nextLongFired = false;
            m.prevLongFired = false;
            m.lastNextDown = false;
            m.lastPrevDown = false;
            return;
        }
#endif

        uint8_t prevSelected = m.selectedIndex;

        uint32_t now = nowMs();
        bool changed = false;

        const uint32_t holdMs = 400;

        if (nextDown)
        {
            if (!m.lastNextDown)
            {
                m.nextHoldStartMs = now;
                m.nextLongFired = false;
            }
            else if (!m.nextLongFired && m.nextHoldStartMs && (now - m.nextHoldStartMs) >= holdMs)
            {
                if (m.selectedIndex < m.itemCount)
                {
                    uint8_t target = m.items[m.selectedIndex].targetScreen;
                    if (target != INVALID_SCREEN_ID)
                        activateScreenId(target, 1);
                }
                m.nextLongFired = true;
            }
        }
        else
        {
            if (m.lastNextDown && !m.nextLongFired)
            {
                if (m.selectedIndex + 1 < m.itemCount)
                    m.selectedIndex++;
                else
                    m.selectedIndex = 0;
                changed = true;
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
            else if (!m.prevLongFired && m.prevHoldStartMs && (now - m.prevHoldStartMs) >= holdMs)
            {
                prevScreen();
                m.prevLongFired = true;
            }
        }
        else
        {
            if (m.lastPrevDown && !m.prevLongFired)
            {
                if (m.selectedIndex > 0)
                    m.selectedIndex--;
                else
                    m.selectedIndex = m.itemCount - 1;
                changed = true;
            }

            m.prevHoldStartMs = 0;
            m.prevLongFired = false;
        }

        m.lastNextDown = nextDown;
        m.lastPrevDown = prevDown;

        if (changed)
        {
            m.marqueeStartMs = now;
            if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass && _screen.current == screenId)
                updateTile(screenId, prevSelected);
            else
                requestRedraw();
        }
    }

    void GUI::updateTile(uint8_t screenId, uint8_t prevSelectedIndex)
    {
        TileState *pm = getTile(screenId);
        if (!pm)
            return;

        TileState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        if (!_flags.spriteEnabled || !_disp.display)
        {
            renderTile(screenId);
            return;
        }

        if (prevSelectedIndex >= m.itemCount)
            prevSelectedIndex = m.selectedIndex;
        if (m.selectedIndex >= m.itemCount)
            m.selectedIndex = m.itemCount - 1;
        if (m.marqueeStartMs == 0)
            m.marqueeStartMs = nowMs();

        TileViewport viewport;
        const int16_t sb = statusBarHeight();
        if (!resolveTileViewport(_render.screenWidth,
                                 _render.screenHeight,
                                 sb > 0,
                                 sb,
                                 _status.pos,
                                 viewport))
            return;

        TileGridMetrics grid;
        resolveTileGrid(m, viewport, grid);

        int16_t xOld = 0, yOld = 0, wOld = 0, hOld = 0;
        int16_t xNew = 0, yNew = 0, wNew = 0, hNew = 0;
        tileRectAtIndex(m, prevSelectedIndex, grid, xOld, yOld, wOld, hOld);
        tileRectAtIndex(m, m.selectedIndex, grid, xNew, yNew, wNew, hNew);

        const int16_t pad = 2;
        DirtyRect dirtyRects[2] = {
            {(int16_t)(xOld - pad), (int16_t)(yOld - pad), (int16_t)(wOld + pad * 2), (int16_t)(hOld + pad * 2)},
            {(int16_t)(xNew - pad), (int16_t)(yNew - pad), (int16_t)(wNew + pad * 2), (int16_t)(hNew + pad * 2)}};
        uint8_t dirtyCount = (prevSelectedIndex != m.selectedIndex) ? 2 : 1;

        if (dirtyCount == 2)
        {
            const DirtyRect &a = dirtyRects[0];
            const DirtyRect &b = dirtyRects[1];
            const bool touches = (a.x <= b.x + b.w) && (b.x <= a.x + a.w) &&
                                 (a.y <= b.y + b.h) && (b.y <= a.y + a.h);
            if (touches)
            {
                const int16_t x1 = std::min<int16_t>(a.x, b.x);
                const int16_t y1 = std::min<int16_t>(a.y, b.y);
                const int16_t x2 = std::max<int16_t>(a.x + a.w, b.x + b.w);
                const int16_t y2 = std::max<int16_t>(a.y + a.h, b.y + b.h);
                dirtyRects[0] = {x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1)};
                dirtyCount = 1;
            }
        }

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        _render.sprite.getClipRect(&clipX, &clipY, &clipW, &clipH);

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        for (uint8_t i = 0; i < dirtyCount; ++i)
        {
            const DirtyRect &dirty = dirtyRects[i];
            _render.sprite.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
            renderTile(screenId);
        }

        _render.sprite.setClipRect(clipX, clipY, clipW, clipH);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            for (uint8_t i = 0; i < dirtyCount; ++i)
            {
                const DirtyRect &dirty = dirtyRects[i];
                invalidateRect(dirty.x, dirty.y, dirty.w, dirty.h);
            }
        }
    }

    void GUI::renderTile(uint8_t screenId)
    {
        if (screenId == INVALID_SCREEN_ID)
            return;

        TileState *pm = getTile(screenId);
        if (!pm)
            return;

        TileState &m = *pm;
        if (!m.configured || m.itemCount == 0)
            return;

        auto t = getDrawTarget();
        if (!t)
            return;

        int32_t baseClipX = 0, baseClipY = 0, baseClipW = 0, baseClipH = 0;
        t->getClipRect(&baseClipX, &baseClipY, &baseClipW, &baseClipH);
        if (baseClipW <= 0 || baseClipH <= 0)
            return;
        const ClipState prevRootGuiClip = _clip;
        _clip.enabled = true;
        _clip.x = (int16_t)baseClipX;
        _clip.y = (int16_t)baseClipY;
        _clip.w = (int16_t)baseClipW;
        _clip.h = (int16_t)baseClipH;

        TileViewport viewport;
        const int16_t sb = statusBarHeight();
        if (!resolveTileViewport(_render.screenWidth,
                                 _render.screenHeight,
                                 sb > 0,
                                 sb,
                                 _status.pos,
                                 viewport))
            return;

        drawRect()
            .pos(viewport.left, viewport.top)
            .size(viewport.usableW, viewport.usableH)
            .fill(_render.bgColor)
            .draw();

        TileGridMetrics grid;
        resolveTileGrid(m, viewport, grid);

        auto setTextFont = [&](uint16_t weight, uint16_t px)
        {
            setFontWeight(weight);
            setFontSize(px);
        };

        auto ensureTextMetrics = [&](TileState::Item &item,
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

        auto drawTileIcon = [&](uint16_t iconId, int16_t cx, int16_t y, int16_t sizePx, uint16_t fg, uint16_t bg)
        {
            if (iconId == 0xFFFF || iconId >= psdf_icons::IconCount || sizePx <= 0)
                return;

            drawIcon()
                .pos((int16_t)(cx - sizePx / 2), y)
                .size((uint16_t)sizePx)
                .icon(iconId)
                .color(fg)
                .bgColor(bg)
                .draw();
        };

        const MarqueeTextOptions marqueeOpts(28, 700, m.marqueeStartMs);
        auto drawTextLine = [&](const String &text,
                                int16_t centerX,
                                int16_t y,
                                int16_t maxWidth,
                                uint16_t fg,
                                uint16_t bg,
                                bool active)
        {
            if (active)
            {
                if (!drawTextMarquee(text, centerX, y, maxWidth, fg, TextAlign::Center, marqueeOpts))
                    drawTextAligned(text, centerX, y, fg, bg, TextAlign::Center);
            }
            else if (!drawTextEllipsized(text, centerX, y, maxWidth, fg, TextAlign::Center))
            {
                drawTextAligned(text, centerX, y, fg, bg, TextAlign::Center);
            }
        };

        for (uint8_t i = 0; i < m.itemCount; ++i)
        {
            int16_t x = 0, y = 0, tileW = 0, tileH = 0;
            tileRectAtIndex(m, i, grid, x, y, tileW, tileH);

            const uint16_t bg = detail::color888To565((i == m.selectedIndex) ? m.style.cardActiveColor : m.style.cardColor);
            drawSquircleRect().pos(x, y).size(tileW, tileH).radius({grid.radius}).fill(bg);

            uint16_t txtCol = detail::autoTextColor(bg, 140);
            uint16_t subCol = (txtCol == 0xFFFF) ? (uint16_t)0xC618 : (uint16_t)0x8410;
            const bool active = (i == m.selectedIndex);

            TileState::Item &it = m.items[i];
            const String &title = it.title;
            const String &sub = it.subtitle;
            const bool hasIcon = (it.iconId != 0xFFFF && it.iconId < psdf_icons::IconCount);
            const bool hasSub = (m.style.contentMode == TextSubtitle && sub.length() > 0);
            const int16_t centerX = x + tileW / 2;
            const int16_t innerPadX = (tileW >= 90) ? 12 : 8;
            const int16_t innerPadY = (tileH >= 72) ? 10 : 8;
            const int16_t textMaxWidth = (tileW - innerPadX * 2 > 4) ? (tileW - innerPadX * 2) : 4;
            const int16_t contentClipX = x + innerPadX;
            const int16_t contentClipY = y + innerPadY;
            const int16_t contentClipW = tileW - innerPadX * 2;
            const int16_t contentClipH = tileH - innerPadY * 2;
            if (contentClipW <= 0 || contentClipH <= 0)
                continue;

            const uint16_t titlePx = hasSub ? 18 : 20;
            uint16_t subPx = hasSub ? static_cast<uint16_t>((titlePx * 7U) / 10U) : 0;
            uint16_t gapPx = m.style.lineGapPx;
            if (hasSub)
            {
                if (gapPx == 0)
                    gapPx = (uint16_t)(titlePx / 5U);
            }
            else
            {
                subPx = 0;
                gapPx = 0;
            }

            ensureTextMetrics(it, titlePx, 600, hasSub, subPx, 500);

            bool showSub = hasSub && subPx > 0 && it.subH > 0;
            int16_t iconSize = hasIcon ? (int16_t)std::min<int16_t>((int16_t)(tileH / (hasSub ? 3 : 2)), (int16_t)(tileW / 3)) : 0;
            int16_t iconGap = hasIcon ? (int16_t)((tileH >= 84) ? 8 : 6) : 0;
            int16_t textBlockH = it.titleH;
            if (showSub)
                textBlockH += gapPx + it.subH;
            if (showSub && textBlockH + (iconSize > 0 ? iconSize + iconGap : 0) > contentClipH)
            {
                showSub = false;
                textBlockH = it.titleH;
            }
            if (iconSize > 0)
            {
                const int16_t maxIconArea = contentClipH - textBlockH;
                if (maxIconArea <= 0)
                {
                    iconSize = 0;
                    iconGap = 0;
                }
                else
                {
                    if (iconSize > maxIconArea)
                        iconSize = maxIconArea;
                    if (iconSize + iconGap > maxIconArea)
                        iconGap = (int16_t)std::max<int16_t>(0, maxIconArea - iconSize);
                    if (iconSize < 10)
                    {
                        iconSize = 0;
                        iconGap = 0;
                    }
                }
            }
            int16_t blockH = it.titleH;
            if (showSub)
                blockH += gapPx + it.subH;
            if (iconSize > 0)
                blockH += iconSize + iconGap;
            if (blockH > contentClipH)
                blockH = contentClipH;

            int16_t baseY = contentClipY + (contentClipH - blockH) / 2;
            if (baseY < contentClipY)
                baseY = contentClipY;

            int16_t contentY = baseY;
            const ClipState prevGuiClip = _clip;
            int32_t prevClipX = 0, prevClipY = 0, prevClipW = 0, prevClipH = 0;
            t->getClipRect(&prevClipX, &prevClipY, &prevClipW, &prevClipH);
            applyClip(contentClipX, contentClipY, contentClipW, contentClipH);
            t->setClipRect(_clip.x, _clip.y, _clip.w, _clip.h);

            if (iconSize > 0)
            {
                drawTileIcon(it.iconId, centerX, contentY, iconSize, txtCol, bg);
                contentY += iconSize + iconGap;
            }

            const int16_t titleY = contentY;
            const int16_t subY = contentY + it.titleH + (showSub ? gapPx : 0);

            setTextFont(600, titlePx);
            drawTextLine(title, centerX, titleY, textMaxWidth, txtCol, bg, active);

            if (showSub)
            {
                setTextFont(500, subPx);
                drawTextLine(sub, centerX, subY, textMaxWidth, subCol, bg, active);
            }

            _clip = prevGuiClip;
            t->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);
        }

        _clip = prevRootGuiClip;
        t->setClipRect(baseClipX, baseClipY, baseClipW, baseClipH);
    }
}

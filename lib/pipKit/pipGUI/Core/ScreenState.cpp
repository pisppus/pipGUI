#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <algorithm>
#include <new>

namespace pipgui
{

    template <typename T, typename InitFunc, typename PtrArr>
    static T *ensureLazy(pipcore::Platform *plat, uint8_t screenId, PtrArr &arr, uint16_t cap, InitFunc initFunc)
    {
        if (!plat || screenId >= cap || !arr)
            return nullptr;
        if (!arr[screenId])
        {
            void *mem = detail::alloc(plat, sizeof(T), pipcore::AllocCaps::Default);
            if (!mem)
                return nullptr;
            T *obj = new (mem) T();
            initFunc(*obj);
            arr[screenId] = obj;
        }
        return arr[screenId];
    }
    static void initGraphAreaDefaults(pipgui::GraphArea &area) noexcept
    {
        area = {};
        area.autoMax = 1;
        area.speed = 1.0f;
        area.direction = pipgui::LeftToRight;
    }
    static void initListDefaults(pipgui::ListState &menu) noexcept
    {
        menu = {};
        menu.parentScreen = INVALID_SCREEN_ID;
        menu.style.radius = 8;
        menu.style.spacing = 6;
        menu.style.mode = pipgui::Cards;
    }
    static void initTileDefaults(pipgui::TileState &tile) noexcept
    {
        tile = {};
        tile.parentScreen = INVALID_SCREEN_ID;
        tile.style.radius = 8;
        tile.style.spacing = 6;
        tile.style.columns = 2;
        tile.style.contentMode = pipgui::TextSubtitle;
    }

    void GUI::ensureScreenState(uint8_t id)
    {
        if (id < _screen.capacity)
            return;
        pipcore::Platform *plat = platform();
        uint16_t newCap = _screen.capacity ? _screen.capacity : 8;
        while (newCap <= id)
        {
            uint16_t next = (uint16_t)(newCap * 2);
            if (next <= newCap)
            {
                newCap = (uint16_t)(id + 1);
                break;
            }
            newCap = next;
        }
        if (newCap < (uint16_t)(id + 1))
            newCap = (uint16_t)(id + 1);
        ScreenCallback *newScreens = (ScreenCallback *)detail::alloc(plat, sizeof(ScreenCallback) * newCap, pipcore::AllocCaps::Default);
        GraphArea **newGraphs = (GraphArea **)detail::alloc(plat, sizeof(GraphArea *) * newCap, pipcore::AllocCaps::Default);
        ListState **newLists = (ListState **)detail::alloc(plat, sizeof(ListState *) * newCap, pipcore::AllocCaps::Default);
        TileState **newTiles = (TileState **)detail::alloc(plat, sizeof(TileState *) * newCap, pipcore::AllocCaps::Default);
        if (!newScreens || !newGraphs || !newLists || !newTiles)
        {
            if (newScreens)
                detail::free(plat, newScreens);
            if (newGraphs)
                detail::free(plat, newGraphs);
            if (newLists)
                detail::free(plat, newLists);
            if (newTiles)
                detail::free(plat, newTiles);
            return;
        }
        std::fill_n(newScreens, newCap, nullptr);
        std::fill_n(newGraphs, newCap, nullptr);
        std::fill_n(newLists, newCap, nullptr);
        std::fill_n(newTiles, newCap, nullptr);
        if (_screen.capacity > 0)
        {
            for (uint16_t i = 0; i < _screen.capacity; ++i)
            {
                if (_screen.callbacks)
                    newScreens[i] = _screen.callbacks[i];
                if (_screen.graphAreas)
                    newGraphs[i] = _screen.graphAreas[i];
                if (_screen.lists)
                    newLists[i] = _screen.lists[i];
                if (_screen.tiles)
                    newTiles[i] = _screen.tiles[i];
            }
        }
        if (_screen.callbacks)
            detail::free(plat, _screen.callbacks);
        if (_screen.graphAreas)
            detail::free(plat, _screen.graphAreas);
        if (_screen.lists)
            detail::free(plat, _screen.lists);
        if (_screen.tiles)
            detail::free(plat, _screen.tiles);
        _screen.callbacks = newScreens;
        _screen.graphAreas = newGraphs;
        _screen.lists = newLists;
        _screen.tiles = newTiles;
        _screen.capacity = newCap;
    }

    GraphArea *GUI::ensureGraphArea(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<GraphArea>(platform(), screenId, _screen.graphAreas, _screen.capacity, initGraphAreaDefaults);
    }
    ListState *GUI::ensureList(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<ListState>(platform(), screenId, _screen.lists, _screen.capacity, initListDefaults);
    }
    TileState *GUI::ensureTile(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<TileState>(platform(), screenId, _screen.tiles, _screen.capacity, initTileDefaults);
    }

    ListState *GUI::getList(uint8_t screenId)
    {
        if (screenId >= _screen.capacity || !_screen.lists)
            return nullptr;
        return _screen.lists[screenId];
    }
    TileState *GUI::getTile(uint8_t screenId)
    {
        if (screenId >= _screen.capacity || !_screen.tiles)
            return nullptr;
        return _screen.tiles[screenId];
    }

}




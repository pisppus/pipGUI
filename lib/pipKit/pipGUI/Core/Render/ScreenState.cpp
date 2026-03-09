#include <pipGUI/Core/API/pipGUI.hpp>
#include <new>

namespace pipgui
{

    template <typename T, typename InitFunc, typename PtrArr>
    static T *ensureLazy(uint8_t screenId, PtrArr &arr, uint16_t cap, InitFunc initFunc)
    {
        if (screenId >= cap || !arr)
            return nullptr;
        if (!arr[screenId])
        {
            pipcore::GuiPlatform *plat = GUI::sharedPlatform();
            void *mem = detail::guiAlloc(plat, sizeof(T), pipcore::GuiAllocCaps::Default);
            if (!mem)
                return nullptr;
            T *obj = new (mem) T();
            initFunc(*obj);
            arr[screenId] = obj;
        }
        return arr[screenId];
    }
    static void initGraphAreaDefaults(pipgui::GraphArea &area)
    {
        area = {};
        area.autoMax = 1;
        area.speed = 1.0f;
        area.direction = pipgui::LeftToRight;
    }
    static void initListDefaults(pipgui::ListState &menu)
    {
        menu = {};
        menu.parentScreen = INVALID_SCREEN_ID;
        menu.style.radius = 8;
        menu.style.spacing = 6;
        menu.style.mode = pipgui::Cards;
    }
    static void initTileDefaults(pipgui::TileState &tile)
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
        pipcore::GuiPlatform *plat = platform();
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
        ScreenCallback *newScreens = (ScreenCallback *)detail::guiAlloc(plat, sizeof(ScreenCallback) * newCap, pipcore::GuiAllocCaps::Default);
        GraphArea **newGraphs = (GraphArea **)detail::guiAlloc(plat, sizeof(GraphArea *) * newCap, pipcore::GuiAllocCaps::Default);
        ListState **newLists = (ListState **)detail::guiAlloc(plat, sizeof(ListState *) * newCap, pipcore::GuiAllocCaps::Default);
        TileState **newTiles = (TileState **)detail::guiAlloc(plat, sizeof(TileState *) * newCap, pipcore::GuiAllocCaps::Default);
        if (!newScreens || !newGraphs || !newLists || !newTiles)
        {
            if (newScreens)
                detail::guiFree(plat, newScreens);
            if (newGraphs)
                detail::guiFree(plat, newGraphs);
            if (newLists)
                detail::guiFree(plat, newLists);
            if (newTiles)
                detail::guiFree(plat, newTiles);
            return;
        }
        for (uint16_t i = 0; i < newCap; ++i)
        {
            newScreens[i] = nullptr;
            newGraphs[i] = nullptr;
            newLists[i] = nullptr;
            newTiles[i] = nullptr;
        }
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
            detail::guiFree(plat, _screen.callbacks);
        if (_screen.graphAreas)
            detail::guiFree(plat, _screen.graphAreas);
        if (_screen.lists)
            detail::guiFree(plat, _screen.lists);
        if (_screen.tiles)
            detail::guiFree(plat, _screen.tiles);
        _screen.callbacks = newScreens;
        _screen.graphAreas = newGraphs;
        _screen.lists = newLists;
        _screen.tiles = newTiles;
        _screen.capacity = newCap;
    }

    GraphArea *GUI::ensureGraphArea(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<GraphArea>(screenId, _screen.graphAreas, _screen.capacity, initGraphAreaDefaults);
    }
    ListState *GUI::ensureList(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<ListState>(screenId, _screen.lists, _screen.capacity, initListDefaults);
    }
    TileState *GUI::ensureTile(uint8_t screenId)
    {
        ensureScreenState(screenId);
        return ensureLazy<TileState>(screenId, _screen.tiles, _screen.capacity, initTileDefaults);
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

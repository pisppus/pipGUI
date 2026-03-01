#include <pipGUI/core/api/pipGUI.hpp>
#include <new>

namespace pipgui
{

    template <typename T, typename InitFunc, typename PtrArr>
    static T &ensureLazy(uint8_t screenId, PtrArr &arr, uint16_t cap, InitFunc initFunc)
    {
        static T dummy;
        static bool dummyInited = false;
        if (!dummyInited)
        {
            initFunc(dummy);
            dummyInited = true;
        }

        if (screenId >= cap || !arr)
            return dummy;
        if (!arr[screenId])
        {
            pipcore::GuiPlatform *plat = GUI::sharedPlatform();
            void *mem = detail::guiAlloc(plat, sizeof(T), pipcore::GuiAllocCaps::Default);
            if (!mem)
                return dummy;
            T *obj = new (mem) T();
            initFunc(*obj);
            arr[screenId] = obj;
        }
        return *(arr[screenId]);
    }
    static void initGraphAreaDefaults(pipgui::GraphArea &area)
    {
        area = {};
        area.autoMax = 1;
        area.speed = 1.0f;
        area.direction = pipgui::LeftToRight;
    }
    static void initListMenuDefaults(pipgui::ListMenuState &menu)
    {
        menu = {};
        menu.parentScreen = 255;
        menu.style.radius = 8;
        menu.style.spacing = 8;
        menu.style.mode = pipgui::Cards;
    }
    static void initTileMenuDefaults(pipgui::TileMenuState &tile)
    {
        tile = {};
        tile.parentScreen = 255;
        tile.style.radius = 8;
        tile.style.spacing = 8;
        tile.style.columns = 2;
        tile.style.contentMode = pipgui::TextSubtitle;
    }

    void GUI::ensureScreenStorage(uint8_t id)
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
        ListMenuState **newLists = (ListMenuState **)detail::guiAlloc(plat, sizeof(ListMenuState *) * newCap, pipcore::GuiAllocCaps::Default);
        TileMenuState **newTiles = (TileMenuState **)detail::guiAlloc(plat, sizeof(TileMenuState *) * newCap, pipcore::GuiAllocCaps::Default);
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
                if (_screen.listMenus)
                    newLists[i] = _screen.listMenus[i];
                if (_screen.tileMenus)
                    newTiles[i] = _screen.tileMenus[i];
            }
        }
        if (_screen.callbacks)
            detail::guiFree(plat, _screen.callbacks);
        if (_screen.graphAreas)
            detail::guiFree(plat, _screen.graphAreas);
        if (_screen.listMenus)
            detail::guiFree(plat, _screen.listMenus);
        if (_screen.tileMenus)
            detail::guiFree(plat, _screen.tileMenus);
        _screen.callbacks = newScreens;
        _screen.graphAreas = newGraphs;
        _screen.listMenus = newLists;
        _screen.tileMenus = newTiles;
        _screen.capacity = newCap;
    }

    GraphArea &GUI::ensureGraphArea(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<GraphArea>(screenId, _screen.graphAreas, _screen.capacity, initGraphAreaDefaults);
    }
    ListMenuState &GUI::ensureListMenu(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<ListMenuState>(screenId, _screen.listMenus, _screen.capacity, initListMenuDefaults);
    }
    TileMenuState &GUI::ensureTileMenu(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<TileMenuState>(screenId, _screen.tileMenus, _screen.capacity, initTileMenuDefaults);
    }

    ListMenuState *GUI::getListMenu(uint8_t screenId)
    {
        if (screenId >= _screen.capacity || !_screen.listMenus)
            return nullptr;
        return _screen.listMenus[screenId];
    }
    TileMenuState *GUI::getTileMenu(uint8_t screenId)
    {
        if (screenId >= _screen.capacity || !_screen.tileMenus)
            return nullptr;
        return _screen.tileMenus[screenId];
    }

}

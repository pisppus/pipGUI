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
        if (id < _screenCapacity)
            return;
        pipcore::GuiPlatform *plat = platform();
        uint16_t newCap = _screenCapacity ? _screenCapacity : 8;
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
        if (_screenCapacity > 0)
        {
            for (uint16_t i = 0; i < _screenCapacity; ++i)
            {
                if (_screens)
                    newScreens[i] = _screens[i];
                if (_graphAreas)
                    newGraphs[i] = _graphAreas[i];
                if (_listMenus)
                    newLists[i] = _listMenus[i];
                if (_tileMenus)
                    newTiles[i] = _tileMenus[i];
            }
        }
        if (_screens)
            detail::guiFree(plat, _screens);
        if (_graphAreas)
            detail::guiFree(plat, _graphAreas);
        if (_listMenus)
            detail::guiFree(plat, _listMenus);
        if (_tileMenus)
            detail::guiFree(plat, _tileMenus);
        _screens = newScreens;
        _graphAreas = newGraphs;
        _listMenus = newLists;
        _tileMenus = newTiles;
        _screenCapacity = newCap;
    }

    GraphArea &GUI::ensureGraphArea(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<GraphArea>(screenId, _graphAreas, _screenCapacity, initGraphAreaDefaults);
    }
    ListMenuState &GUI::ensureListMenu(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<ListMenuState>(screenId, _listMenus, _screenCapacity, initListMenuDefaults);
    }
    TileMenuState &GUI::ensureTileMenu(uint8_t screenId)
    {
        ensureScreenStorage(screenId);
        return ensureLazy<TileMenuState>(screenId, _tileMenus, _screenCapacity, initTileMenuDefaults);
    }

    ListMenuState *GUI::getListMenu(uint8_t screenId)
    {
        if (screenId >= _screenCapacity || !_listMenus)
            return nullptr;
        return _listMenus[screenId];
    }
    TileMenuState *GUI::getTileMenu(uint8_t screenId)
    {
        if (screenId >= _screenCapacity || !_tileMenus)
            return nullptr;
        return _tileMenus[screenId];
    }

}

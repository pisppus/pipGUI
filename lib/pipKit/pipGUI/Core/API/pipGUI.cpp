#include <pipGUI/Core/API/pipGUI.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipCore/Platforms/PlatformFactory.hpp>

namespace pipgui
{

    static void backlightPlatformCallback(uint16_t level)
    {
        pipcore::GuiPlatform *p = pipcore::GetPlatform();
        if (!p)
            return;
        if (level > 100)
            level = 100;
        p->setBacklightPercent(static_cast<uint8_t>(level));
    }

    GUI::GUI()
    {
    }

    GUI::~GUI() noexcept
    {
        pipcore::GuiPlatform *plat = pipcore::GetPlatform();

        freeBlurBuffers(plat);
        freeGraphAreas(plat);
        freeLists(plat);
        freeTiles(plat);
        freeErrors(plat);
        freeScreenState(plat);

        _render.sprite.deleteSprite();
        _flags.spriteEnabled = 0;
    }

    template <typename T>
    static void safeFree(pipcore::GuiPlatform *plat, T *&ptr) noexcept
    {
        if (ptr)
        {
            detail::guiFree(plat, ptr);
            ptr = nullptr;
        }
    }

    template <typename T>
    static void safeFreeArray(pipcore::GuiPlatform *plat, T *&arr, uint16_t count) noexcept
    {
        if (!arr)
            return;
        for (uint16_t i = 0; i < count; ++i)
            arr[i].~T();
        detail::guiFree(plat, arr);
        arr = nullptr;
    }

    template <typename T>
    static void safeFreeEntryArray(pipcore::GuiPlatform *plat, T *&arr,
                                   uint8_t &capacity, uint8_t &count) noexcept
    {
        if (!arr)
            return;
        for (uint8_t i = 0; i < count; ++i)
            arr[i].~T();
        detail::guiFree(plat, arr);
        arr = nullptr;
        capacity = 0;
        count = 0;
    }

    template <typename T>
    struct ObjectGuard
    {
        pipcore::GuiPlatform *plat;
        T *&ptr;
        bool released;

        ObjectGuard(pipcore::GuiPlatform *p, T *&obj)
            : plat(p), ptr(obj), released(false) {}

        ~ObjectGuard()
        {
            if (!released && ptr)
            {
                ptr->~T();
                detail::guiFree(plat, ptr);
                ptr = nullptr;
            }
        }
    };

    void GUI::freeBlurBuffers(pipcore::GuiPlatform *plat) noexcept
    {
        safeFree(plat, _blur.smallIn);
        safeFree(plat, _blur.smallTmp);
        safeFree(plat, _blur.rowR);
        safeFree(plat, _blur.rowG);
        safeFree(plat, _blur.rowB);
        safeFree(plat, _blur.colR);
        safeFree(plat, _blur.colG);
        safeFree(plat, _blur.colB);
        _blur.workLen = 0;
        _blur.rowCap = 0;
        _blur.colCap = 0;
    }

    void GUI::freeGraphAreas(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_screen.graphAreas)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
        {
            GraphArea *a = _screen.graphAreas[i];
            if (!a)
                continue;

            if (a->samples)
            {
                for (uint16_t line = 0; line < a->lineCount; ++line)
                    detail::guiFree(plat, a->samples[line]);
                detail::guiFree(plat, a->samples);
                a->samples = nullptr;
            }

            safeFree(plat, a->sampleCounts);
            safeFree(plat, a->sampleHead);

            _screen.graphAreas[i] = nullptr;
            ObjectGuard<GraphArea> guard(plat, a);
        }
    }

    void GUI::freeLists(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_screen.lists)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
        {
            ListState *m = _screen.lists[i];
            if (!m)
                continue;

            safeFreeArray(plat, m->items, m->capacity);

            ObjectGuard<ListState> guard(plat, m);
            _screen.lists[i] = nullptr;
        }
    }

    void GUI::freeTiles(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_screen.tiles)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
        {
            TileState *t = _screen.tiles[i];
            if (!t)
                continue;

            safeFreeArray(plat, t->items, t->itemCapacity);
            t->itemCapacity = 0;

            ObjectGuard<TileState> guard(plat, t);
            _screen.tiles[i] = nullptr;
        }
    }

    void GUI::freeScreenState(pipcore::GuiPlatform *plat) noexcept
    {
        freeGraphAreas(plat);
        freeLists(plat);
        freeTiles(plat);

        detail::guiFree(plat, _screen.callbacks);
        detail::guiFree(plat, _screen.graphAreas);
        detail::guiFree(plat, _screen.lists);
        detail::guiFree(plat, _screen.tiles);

        _screen.callbacks = nullptr;
        _screen.graphAreas = nullptr;
        _screen.lists = nullptr;
        _screen.tiles = nullptr;
        _screen.capacity = 0;
        _screen.current = INVALID_SCREEN_ID;
        _screen.registrySynced = false;
    }

    void GUI::freeErrors(pipcore::GuiPlatform *plat) noexcept
    {
        safeFreeEntryArray(plat, _error.entries, _error.capacity, _error.count);
    }

    ConfigureDisplayFluent GUI::configureDisplay()
    {
        return ConfigureDisplayFluent(this);
    }

    void GUI::configureDisplay(const pipcore::GuiDisplayConfig &cfg)
    {
        _disp.cfg = cfg;
        if (_disp.cfg.hz == 0)
            _disp.cfg.hz = 80000000;

        _disp.cfgConfigured = true;
        pipcore::GuiPlatform *plat = pipcore::GetPlatform();
        plat->guiConfigureDisplay(_disp.cfg);
    }

    void ConfigureDisplayFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        _gui->configureDisplay(_cfg);
    }

    void ListInputFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        _gui->handleListInput(_screenId, _nextDown, _prevDown);
    }

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::GuiPlatform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

#if defined(PIPGUI_DEBUG_DIRTY_RECTS) && (PIPGUI_DEBUG_DIRTY_RECTS)
        Debug::setDirtyRectEnabled(true);
#endif
#if defined(PIPGUI_DEBUG_METRICS) && (PIPGUI_DEBUG_METRICS)
        _flags.statusBarDebugMetrics = true;
        Debug::init();
        _status.dirtyMask = StatusBarDirtyAll;
#endif

        if (_disp.cfgConfigured)
        {
            if (!plat->guiConfigureDisplay(_disp.cfg))
                return;
        }

        if (!plat->guiBeginDisplay(rotation))
            return;

        _disp.display = plat->guiDisplay();
        if (!_disp.display)
            return;

        _render.screenWidth = _disp.display->width();
        _render.screenHeight = _disp.display->height();
        _render.bgColor = bgColor;
        _render.bgColor565 = bgColor;

        _render.sprite.deleteSprite();

        _flags.spriteEnabled = _render.sprite.createSprite((int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
        _render.activeSprite = _flags.spriteEnabled ? &_render.sprite : nullptr;

        if (_flags.spriteEnabled)
        {
            const bool prevRender = _flags.inSpritePass;
            _flags.inSpritePass = 1;
            clear(bgColor);
            _flags.inSpritePass = prevRender;
            _render.sprite.writeToDisplay(*_disp.display, 0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
            _dirty.count = 0;
        }

        syncRegisteredScreens();

        _disp.brightnessMax = plat->loadMaxBrightnessPercent();

        initFonts();
    }

    void GUI::initFonts()
    {
        if (_typo.psdfSizePx == 0)
            _typo.psdfSizePx = _typo.bodyPx;
    }

    uint32_t GUI::nowMs() const
    {
        return pipcore::GetPlatform()->nowMs();
    }

    pipcore::GuiPlatform *GUI::platform() const
    {
        return pipcore::GetPlatform();
    }

    pipcore::GuiPlatform *GUI::sharedPlatform()
    {
        return pipcore::GetPlatform();
    }

    void GUI::setBacklightPin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        pipcore::GuiPlatform *plat = pipcore::GetPlatform();
        plat->configureBacklightPin(pin, channel, freqHz, resolutionBits);
        setBacklightCallback(backlightPlatformCallback);
    }

    void GUI::setMaxBrightness(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;
        _disp.brightnessMax = percent;
        pipcore::GetPlatform()->storeMaxBrightnessPercent(_disp.brightnessMax);
    }

    void GUI::setScreenAnimation(ScreenAnim anim, uint32_t durationMs)
    {
        _screen.anim = anim;
        _screen.animDurationMs = durationMs;
    }
}

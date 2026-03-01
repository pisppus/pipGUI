#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/Debug.hpp>
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
        freeListMenus(plat);
        freeTileMenus(plat);
        freeErrors(plat);
        freeScreenStorage(plat);

        _render.sprite.deleteSprite();
        _render.screenFromSprite.deleteSprite();
        _render.screenToSprite.deleteSprite();
        _flags.spriteEnabled = 0;
        _flags.screenSpritesEnabled = 0;
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

        void release() { released = true; }
    };

    void GUI::freeBlurBuffers(pipcore::GuiPlatform *plat) noexcept
    {
        safeFree(plat, _blur.prevOrig);
        safeFree(plat, _blur.smallIn);
        safeFree(plat, _blur.smallTmp);
        safeFree(plat, _blur.rowR);
        safeFree(plat, _blur.rowG);
        safeFree(plat, _blur.rowB);
        safeFree(plat, _blur.colR);
        safeFree(plat, _blur.colG);
        safeFree(plat, _blur.colB);
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

    void GUI::freeListMenus(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_screen.listMenus)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
        {
            ListMenuState *m = _screen.listMenus[i];
            if (!m)
                continue;

            if (m->cacheNormal)
            {
                for (uint8_t j = 0; j < m->capacity; ++j)
                    detail::guiFree(plat, m->cacheNormal[j]);
                detail::guiFree(plat, m->cacheNormal);
                m->cacheNormal = nullptr;
            }

            if (m->cacheActive)
            {
                for (uint8_t j = 0; j < m->capacity; ++j)
                    detail::guiFree(plat, m->cacheActive[j]);
                detail::guiFree(plat, m->cacheActive);
                m->cacheActive = nullptr;
            }

            safeFreeArray(plat, m->items, m->capacity);

            if (m->viewportSprite)
            {
                m->viewportSprite->deleteSprite();
                m->viewportSprite->~Sprite();
                detail::guiFree(plat, m->viewportSprite);
                m->viewportSprite = nullptr;
            }

            ObjectGuard<ListMenuState> guard(plat, m);
            _screen.listMenus[i] = nullptr;
        }
    }

    void GUI::freeTileMenus(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_screen.tileMenus)
            return;

        for (uint16_t i = 0; i < _screen.capacity; ++i)
        {
            TileMenuState *t = _screen.tileMenus[i];
            if (!t)
                continue;

            const uint8_t cap = t->itemCapacity;

            safeFreeArray(plat, t->items, cap);
            safeFreeArray(plat, t->layout, cap);

            t->itemCapacity = 0;

            ObjectGuard<TileMenuState> guard(plat, t);
            _screen.tileMenus[i] = nullptr;
        }
    }

    void GUI::freeScreenStorage(pipcore::GuiPlatform *plat) noexcept
    {
        freeGraphAreas(plat);
        freeListMenus(plat);
        freeTileMenus(plat);

        detail::guiFree(plat, _screen.callbacks);
        detail::guiFree(plat, _screen.graphAreas);
        detail::guiFree(plat, _screen.listMenus);
        detail::guiFree(plat, _screen.tileMenus);

        _screen.callbacks = nullptr;
        _screen.graphAreas = nullptr;
        _screen.listMenus = nullptr;
        _screen.tileMenus = nullptr;
        _screen.capacity = 0;
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

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::GuiPlatform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

#if defined(PIPGUI_DEBUG_DIRTY_RECTS) && (PIPGUI_DEBUG_DIRTY_RECTS)
        Debug::setDirtyRectEnabled(true);
#endif
#if defined(PIPGUI_DEBUG_METRICS) && (PIPGUI_DEBUG_METRICS)
        setDebugMetrics(true);
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

        _disp.display->fillScreen565(_render.bgColor);

        _render.sprite.deleteSprite();
        _render.screenFromSprite.deleteSprite();
        _render.screenToSprite.deleteSprite();

        _flags.spriteEnabled = _render.sprite.createSprite((int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
        _render.activeSprite = _flags.spriteEnabled ? &_render.sprite : nullptr;

        bool fromOk = _render.screenFromSprite.createSprite((int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
        bool toOk = fromOk && _render.screenToSprite.createSprite((int16_t)_render.screenWidth, (int16_t)_render.screenHeight);

        if (!(fromOk && toOk))
        {
            _render.screenFromSprite.deleteSprite();
            _render.screenToSprite.deleteSprite();

            int16_t halfW = (int16_t)(_render.screenWidth / 2);
            int16_t halfH = (int16_t)(_render.screenHeight / 2);

            if (halfW > 0 && halfH > 0 && _flags.spriteEnabled)
            {
                fromOk = _render.screenFromSprite.createSprite(halfW, halfH);
                toOk = fromOk && _render.screenToSprite.createSprite(halfW, halfH);

                if (!(fromOk && toOk))
                {
                    _render.screenFromSprite.deleteSprite();
                    _render.screenToSprite.deleteSprite();
                }
            }
        }

        _flags.screenSpritesEnabled = fromOk && toOk;

        _disp.brightnessMax = plat->loadMaxBrightnessPercent();

        initFonts();
    }

    void GUI::initFonts()
    {
        loadBuiltinPSDF();
        enablePSDF(true);
        if (_typo.psdfSizePx == 0)
            _typo.psdfSizePx = _typo.bodyPx;
    }

    uint32_t GUI::nowMs() const
    {
        return pipcore::GetPlatform()->nowMs();
    }

    void GUI::setPlatform(pipcore::GuiPlatform *)
    {
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

    void GUI::setDebugMetrics(bool enabled)
    {
        _flags.statusBarDebugMetrics = enabled;
        Debug::setMetricsStatusBarEnabled(enabled);
        if (enabled)
            Debug::init();
        _status.dirtyMask = StatusBarDirtyAll;
        requestRedraw();
    }
}
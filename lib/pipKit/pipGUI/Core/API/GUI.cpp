#include <pipGUI/Core/API/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/BuilderAccess.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipCore/Platforms/Select.hpp>

#ifndef PIPGUI_SERIAL_ERRORS
#define PIPGUI_SERIAL_ERRORS 1
#endif

namespace pipgui
{
    namespace detail
    {
        pipcore::Platform *resolvePlatform(GUI *gui) noexcept
        {
            return gui ? gui->platform() : nullptr;
        }
    }

    namespace
    {
        class NullDisplay final : public pipcore::Display
        {
        public:
            bool begin(uint8_t) override { return false; }
            uint16_t width() const override { return 0; }
            uint16_t height() const override { return 0; }
            void fillScreen565(uint16_t) override {}
            void writeRect565(int16_t, int16_t, int16_t, int16_t, const uint16_t *, int32_t) override {}
        };

        void reportPlatformError(const char *stage, pipcore::Platform *plat)
        {
#if defined(PIPGUI_SERIAL_ERRORS) && (PIPGUI_SERIAL_ERRORS)
            if (!plat)
                return;

            const pipcore::PlatformError error = plat->lastError();
            const char *text = plat->lastErrorText();
            if (error == pipcore::PlatformError::None || !text || !text[0])
                return;

            Serial.print("[pipGUI] ");
            Serial.print(stage);
            Serial.print(": ");
            Serial.println(text);
#else
            (void)stage;
            (void)plat;
#endif
        }
    }

    void GUI::clearReportedPlatformError()
    {
        _diag.lastReportedError = pipcore::PlatformError::None;
    }

    void GUI::reportPlatformErrorOnce(const char *stage)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

        const pipcore::PlatformError error = plat->lastError();
        if (error == pipcore::PlatformError::None)
        {
            clearReportedPlatformError();
            return;
        }

        if (error == _diag.lastReportedError)
            return;

        reportPlatformError(stage, plat);
        _diag.lastReportedError = error;
    }

    bool GUI::presentSprite(int16_t x, int16_t y, int16_t w, int16_t h, const char *stage)
    {
        if (!_disp.display || !_flags.spriteEnabled || w <= 0 || h <= 0)
            return false;

        _render.sprite.writeToDisplay(*_disp.display, x, y, w, h);
        reportPlatformErrorOnce(stage);

        pipcore::Platform *plat = pipcore::GetPlatform();
        return !plat || plat->lastError() == pipcore::PlatformError::None;
    }

    static void backlightPlatformCallback(uint16_t level)
    {
        pipcore::Platform *p = pipcore::GetPlatform();
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
        pipcore::Platform *plat = pipcore::GetPlatform();

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
    static void safeFree(pipcore::Platform *plat, T *&ptr) noexcept
    {
        if (ptr)
        {
            detail::free(plat, ptr);
            ptr = nullptr;
        }
    }

    template <typename T>
    static void safeFreeArray(pipcore::Platform *plat, T *&arr, uint16_t count) noexcept
    {
        if (!arr)
            return;
        for (uint16_t i = 0; i < count; ++i)
            arr[i].~T();
        detail::free(plat, arr);
        arr = nullptr;
    }

    template <typename T>
    static void safeFreeEntryArray(pipcore::Platform *plat, T *&arr,
                                   uint8_t &capacity, uint8_t &count) noexcept
    {
        if (!arr)
            return;
        for (uint8_t i = 0; i < count; ++i)
            arr[i].~T();
        detail::free(plat, arr);
        arr = nullptr;
        capacity = 0;
        count = 0;
    }

    template <typename T>
    struct ObjectGuard
    {
        pipcore::Platform *plat;
        T *&ptr;
        bool released;

        ObjectGuard(pipcore::Platform *p, T *&obj)
            : plat(p), ptr(obj), released(false) {}

        ~ObjectGuard()
        {
            if (!released && ptr)
            {
                ptr->~T();
                detail::free(plat, ptr);
                ptr = nullptr;
            }
        }
    };

    void GUI::freeBlurBuffers(pipcore::Platform *plat) noexcept
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

    void GUI::freeGraphAreas(pipcore::Platform *plat) noexcept
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
                    detail::free(plat, a->samples[line]);
                detail::free(plat, a->samples);
                a->samples = nullptr;
            }

            safeFree(plat, a->sampleCounts);
            safeFree(plat, a->sampleHead);

            _screen.graphAreas[i] = nullptr;
            ObjectGuard<GraphArea> guard(plat, a);
        }
    }

    void GUI::freeLists(pipcore::Platform *plat) noexcept
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

    void GUI::freeTiles(pipcore::Platform *plat) noexcept
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

    void GUI::freeScreenState(pipcore::Platform *plat) noexcept
    {
        freeGraphAreas(plat);
        freeLists(plat);
        freeTiles(plat);

        detail::free(plat, _screen.callbacks);
        detail::free(plat, _screen.graphAreas);
        detail::free(plat, _screen.lists);
        detail::free(plat, _screen.tiles);

        _screen.callbacks = nullptr;
        _screen.graphAreas = nullptr;
        _screen.lists = nullptr;
        _screen.tiles = nullptr;
        _screen.capacity = 0;
        _screen.current = INVALID_SCREEN_ID;
        _screen.registrySynced = false;
    }

    void GUI::freeErrors(pipcore::Platform *plat) noexcept
    {
        safeFreeEntryArray(plat, _error.entries, _error.capacity, _error.count);
    }

    void GUI::resetDisplayRuntime() noexcept
    {
        _disp.display = nullptr;
        _render.screenWidth = 0;
        _render.screenHeight = 0;
        _render.bgColor = 0;
        _render.bgColor565 = 0;
        _render.sprite.deleteSprite();
        _render.activeSprite = nullptr;
        _flags.spriteEnabled = 0;
        _dirty.count = 0;
    }

    pipcore::Display &GUI::display()
    {
        if (_disp.display)
            return *_disp.display;

        reportPlatformErrorOnce("display");
        static NullDisplay nullDisplay;
        return nullDisplay;
    }

    ConfigureDisplayFluent GUI::configureDisplay()
    {
        return ConfigureDisplayFluent(this);
    }

    void GUI::configureDisplay(const pipcore::DisplayConfig &cfg)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

        pipcore::DisplayConfig normalized = cfg;
        if (normalized.hz == 0)
            normalized.hz = 80000000;

        if (!plat->configureDisplay(normalized))
        {
            reportPlatformError("configureDisplay", plat);
            return;
        }

        _disp.cfg = normalized;
        _disp.cfgConfigured = true;
        resetDisplayRuntime();
        clearReportedPlatformError();
    }

    void ConfigureDisplayFluent::apply()
    {
        if (_consumed || !_touched)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::BuilderAccess::configureDisplay(*_gui, _cfg);
    }

    void ListInputFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::BuilderAccess::handleListInput(*_gui, _screenId, _nextDown, _prevDown);
    }

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

        resetDisplayRuntime();

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
            if (!plat->configureDisplay(_disp.cfg))
            {
                reportPlatformError("configureDisplay", plat);
                return;
            }
            clearReportedPlatformError();
        }

        if (!plat->beginDisplay(rotation))
        {
            reportPlatformError("beginDisplay", plat);
            return;
        }
        clearReportedPlatformError();

        _disp.display = plat->display();
        if (!_disp.display)
        {
            reportPlatformError("display", plat);
            return;
        }

        _render.screenWidth = _disp.display->width();
        _render.screenHeight = _disp.display->height();
        _render.bgColor = bgColor;
        _render.bgColor565 = bgColor;

        _render.sprite.setPlatform(plat);

        _flags.spriteEnabled = _render.sprite.createSprite((int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
        _render.activeSprite = _flags.spriteEnabled ? &_render.sprite : nullptr;

        if (_flags.spriteEnabled)
        {
            const bool prevRender = _flags.inSpritePass;
            _flags.inSpritePass = 1;
            clear(bgColor);
            _flags.inSpritePass = prevRender;
            presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
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

    pipcore::Platform *GUI::platform() const
    {
        return pipcore::GetPlatform();
    }

    void GUI::setBacklightPin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
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

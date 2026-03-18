#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/BuilderAccess.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipCore/Platforms/Select.hpp>
#include <algorithm>

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
            uint16_t width() const noexcept override { return 0; }
            uint16_t height() const noexcept override { return 0; }
            void fillScreen565(uint16_t) override {}
            void writeRect565(int16_t, int16_t, int16_t, int16_t, const uint16_t *, int32_t) override {}
        };

        void reportPlatformError(const char *stage, pipcore::Platform *plat)
        {
            (void)stage;
            (void)plat;
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

    GUI::InputState GUI::pollInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();

        const bool nextDown = next.isDown();
        const bool prevDown = prev.isDown();
        const bool combo = nextDown && prevDown;

#if PIPGUI_SCREENSHOTS
        handleScreenshotShortcut(nextDown, prevDown);

        if (combo)
        {
            (void)next.wasPressed();
            (void)prev.wasPressed();
            return {};
        }
#endif

        InputState out;
        out.nextDown = nextDown;
        out.prevDown = prevDown;
        out.nextPressed = next.wasPressed();
        out.prevPressed = prev.wasPressed();
        out.comboDown = combo;
        return out;
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
#if PIPGUI_SCREENSHOTS
        freeScreenshotGallery(plat);
        freeScreenshotStream(plat);
#endif

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
#if PIPGUI_SCREENSHOTS
        freeScreenshotStream(platform());
#endif
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

    void PopupMenuInputFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::BuilderAccess::handlePopupMenuInput(*_gui, _nextDown, _prevDown);
    }

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

        resetDisplayRuntime();

#if PIPGUI_STATUS_BAR
        _flags.statusBarDebugMetrics = (PIPGUI_STATUS_BAR_DEBUG_METRICS_DEFAULT != 0);
#endif

#if PIPGUI_DEBUG_DIRTY_RECTS
        Debug::setDirtyRectEnabled(true);
#endif
#if PIPGUI_DEBUG_METRICS
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

    void GUI::setBackgroundColorCache(uint16_t color565) noexcept
    {
        _render.bgColor = detail::color565To888(color565);
        _render.bgColor565 = color565;
    }

    uint32_t GUI::nowMs() const
    {
        return pipcore::GetPlatform()->nowMs();
    }

    pipcore::Platform *GUI::platform() const noexcept
    {
        return pipcore::GetPlatform();
    }

    void GUI::requestWiFi(bool enabled) noexcept
    {
        net::wifiRequest(enabled);
    }

    bool GUI::wifiConnected() const noexcept
    {
        return net::wifiConnected();
    }

    uint32_t GUI::wifiLocalIpV4() const noexcept
    {
        return net::wifiLocalIpV4();
    }

    namespace
    {
        [[nodiscard]] pipcore::ota::Options buildOtaOptions() noexcept
        {
            pipcore::ota::Options opt;
            opt.currentVerMajor = static_cast<uint16_t>(PIPGUI_FIRMWARE_VER_MAJOR);
            opt.currentVerMinor = static_cast<uint16_t>(PIPGUI_FIRMWARE_VER_MINOR);
            opt.currentVerPatch = static_cast<uint16_t>(PIPGUI_FIRMWARE_VER_PATCH);
            opt.currentBuild =
                (static_cast<uint64_t>(PIPGUI_FIRMWARE_VER_MAJOR) * 1'000'000ull) +
                (static_cast<uint64_t>(PIPGUI_FIRMWARE_VER_MINOR) * 1'000ull) +
                static_cast<uint64_t>(PIPGUI_FIRMWARE_VER_PATCH);

#if PIPGUI_OTA
#if !defined(PIPGUI_OTA_ED25519_PUBKEY_HEX)
#error "Define PIPGUI_OTA_ED25519_PUBKEY_HEX (64 hex chars) in include/config.hpp"
#endif
            static_assert(sizeof(PIPGUI_OTA_ED25519_PUBKEY_HEX) == 65, "PIPGUI_OTA_ED25519_PUBKEY_HEX must be 64 hex chars");
            opt.ed25519PubkeyHex = PIPGUI_OTA_ED25519_PUBKEY_HEX;
#else
            opt.ed25519PubkeyHex = "";
#endif
            return opt;
        }
    }

    void GUI::otaConfigure(OtaStatusCallback cb, void *user) noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::configure(buildOtaOptions(), cb, user);
#else
        (void)cb;
        (void)user;
#endif
    }

    void GUI::otaRequestCheck() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::requestCheck();
#endif
    }

    void GUI::otaRequestCheck(OtaCheckMode mode) noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::requestCheck(mode);
#else
        (void)mode;
#endif
    }

    void GUI::otaRequestInstall() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::requestInstall();
#endif
    }

    void GUI::otaRequestStableList() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::requestStableList();
#endif
    }

    bool GUI::otaStableListReady() const noexcept
    {
#if PIPGUI_OTA
        return pipcore::ota::stableListReady();
#else
        return false;
#endif
    }

    uint8_t GUI::otaStableListCount() const noexcept
    {
#if PIPGUI_OTA
        return pipcore::ota::stableListCount();
#else
        return 0;
#endif
    }

    const char *GUI::otaStableListVersion(uint8_t idx) const noexcept
    {
#if PIPGUI_OTA
        return pipcore::ota::stableListVersion(idx);
#else
        (void)idx;
        return "";
#endif
    }

    void GUI::otaRequestInstallStableVersion(const char *version) noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::requestInstallStableVersion(version);
#else
        (void)version;
#endif
    }

    void GUI::otaCancel() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::cancel();
#endif
    }

    void GUI::otaService() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::service();
#endif
    }

    void GUI::otaMarkAppValid() noexcept
    {
#if PIPGUI_OTA
        pipcore::ota::markAppValid();
#endif
    }

    const OtaStatus &GUI::otaStatus() const noexcept
    {
        return pipcore::ota::status();
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

#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>
#include <pipCore/Platforms/Select.hpp>

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
        uint32_t hashButtonKey(const String &label, int16_t x, int16_t y, int16_t w, int16_t h,
                               uint16_t baseColor, uint8_t radius, IconId iconId) noexcept
        {
            (void)label;
            uint32_t hash = 2166136261u;
            auto mix = [&](uint32_t v)
            {
                hash ^= v;
                hash *= 16777619u;
            };

            mix(static_cast<uint16_t>(x));
            mix(static_cast<uint16_t>(y));
            mix(static_cast<uint16_t>(w));
            mix(static_cast<uint16_t>(h));
            mix(baseColor);
            mix(radius);
            mix(static_cast<uint16_t>(iconId));
            return hash ? hash : 1u;
        }

        uint32_t hashToggleKey(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint16_t activeColor, int32_t inactiveColor, int32_t knobColor) noexcept
        {
            uint32_t hash = 2166136261u;
            auto mix = [&](uint32_t v)
            {
                hash ^= v;
                hash *= 16777619u;
            };

            mix(static_cast<uint16_t>(x));
            mix(static_cast<uint16_t>(y));
            mix(static_cast<uint16_t>(w));
            mix(static_cast<uint16_t>(h));
            mix(activeColor);
            mix(static_cast<uint32_t>(inactiveColor));
            mix(static_cast<uint32_t>(knobColor));
            return hash ? hash : 1u;
        }

        uint32_t hashSliderKey(int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t minValue, int16_t maxValue, int16_t step,
                               uint16_t activeColor, int32_t inactiveColor, int32_t thumbColor) noexcept
        {
            uint32_t hash = 2166136261u;
            auto mix = [&](uint32_t v)
            {
                hash ^= v;
                hash *= 16777619u;
            };

            mix(static_cast<uint16_t>(x));
            mix(static_cast<uint16_t>(y));
            mix(static_cast<uint16_t>(w));
            mix(static_cast<uint16_t>(h));
            mix(static_cast<uint16_t>(minValue));
            mix(static_cast<uint16_t>(maxValue));
            mix(static_cast<uint16_t>(step));
            mix(activeColor);
            mix(static_cast<uint32_t>(inactiveColor));
            mix(static_cast<uint32_t>(thumbColor));
            return hash ? hash : 1u;
        }

        float drumRollCurrentIndex(const detail::DrumRollAnimState &state, uint32_t now) noexcept
        {
            if (state.startMs == 0 || state.durationMs == 0 || now <= state.startMs)
                return state.fromIndex;

            const uint32_t elapsed = now - state.startMs;
            if (elapsed >= state.durationMs)
                return static_cast<float>(state.toIndex);

            const float t = static_cast<float>(elapsed) / static_cast<float>(state.durationMs);
            const float eased = detail::motion::easeInOutCubic(t);
            return state.fromIndex + (static_cast<float>(state.toIndex) - state.fromIndex) * eased;
        }

        class NullDisplay final : public pipcore::Display
        {
        public:
            bool begin(uint8_t) override { return false; }
            uint16_t width() const noexcept override { return 0; }
            uint16_t height() const noexcept override { return 0; }
            void fillScreen565(uint16_t) override {}
            void writeRect565(int16_t, int16_t, int16_t, int16_t, const uint16_t *, int32_t) override {}
        };

    }

    void GUI::clearReportedPlatformError()
    {
        _diag.lastReportedError = pipcore::PlatformError::None;
    }

    void GUI::reportPlatformErrorOnce(const char *stage)
    {
        (void)stage;
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

    detail::ButtonState &GUI::resolveButtonState(const String &label, int16_t x, int16_t y,
                                                 int16_t w, int16_t h, uint16_t baseColor, uint8_t radius,
                                                 IconId iconId)
    {
        const uint32_t key = hashButtonKey(label, x, y, w, h, baseColor, radius, iconId);
        const uint32_t now = nowMs();
        detail::ButtonCacheEntry *best = &_buttonCache.entries[0];

        for (uint8_t i = 0; i < detail::BUTTON_CACHE_MAX; ++i)
        {
            detail::ButtonCacheEntry &entry = _buttonCache.entries[i];
            if (entry.used && entry.key == key)
            {
                entry.lastUseMs = now;
                return entry.state;
            }
            if (!entry.used)
                best = &entry;
            else if (best->used && entry.lastUseMs < best->lastUseMs)
                best = &entry;
        }

        best->used = true;
        best->key = key;
        best->lastUseMs = now;
        best->state = {};
        best->state.enabled = true;
        best->state.prevEnabled = true;
        best->state.fadeLevel = 255;
        return best->state;
    }

    detail::ToggleState &GUI::resolveToggleState(int16_t x, int16_t y, int16_t w, int16_t h,
                                                 uint16_t activeColor, int32_t inactiveColor, int32_t knobColor)
    {
        const uint32_t key = hashToggleKey(x, y, w, h, activeColor, inactiveColor, knobColor);
        const uint32_t now = nowMs();
        detail::ToggleCacheEntry *best = &_toggleCache.entries[0];

        for (uint8_t i = 0; i < detail::TOGGLE_CACHE_MAX; ++i)
        {
            detail::ToggleCacheEntry &entry = _toggleCache.entries[i];
            if (entry.used && entry.key == key)
            {
                entry.lastUseMs = now;
                return entry.state;
            }
            if (!entry.used)
                best = &entry;
            else if (best->used && entry.lastUseMs < best->lastUseMs)
                best = &entry;
        }

        best->used = true;
        best->key = key;
        best->lastUseMs = now;
        best->state = {};
        return best->state;
    }

    detail::SliderState &GUI::resolveSliderState(int16_t x, int16_t y, int16_t w, int16_t h,
                                                 int16_t minValue, int16_t maxValue, int16_t step,
                                                 uint16_t activeColor, int32_t inactiveColor, int32_t thumbColor)
    {
        const uint32_t key = hashSliderKey(x, y, w, h, minValue, maxValue, step, activeColor, inactiveColor, thumbColor);
        const uint32_t now = nowMs();
        detail::SliderCacheEntry *best = &_sliderCache.entries[0];

        for (uint8_t i = 0; i < detail::SLIDER_CACHE_MAX; ++i)
        {
            detail::SliderCacheEntry &entry = _sliderCache.entries[i];
            if (entry.used && entry.key == key)
            {
                entry.lastUseMs = now;
                return entry.state;
            }
            if (!entry.used)
                best = &entry;
            else if (best->used && entry.lastUseMs < best->lastUseMs)
                best = &entry;
        }

        best->used = true;
        best->key = key;
        best->lastUseMs = now;
        best->state = {};
        best->state.enabled = true;
        return best->state;
    }

    detail::DrumRollAnimState &GUI::resolveDrumRollState(uint32_t key, uint8_t selectedIndex, uint16_t durationMs)
    {
        const uint32_t now = nowMs();
        detail::DrumRollCacheEntry *best = &_drumRollCache.entries[0];

        for (uint8_t i = 0; i < detail::DRUM_ROLL_CACHE_MAX; ++i)
        {
            detail::DrumRollCacheEntry &entry = _drumRollCache.entries[i];
            if (entry.used && entry.key == key)
            {
                entry.lastUseMs = now;
                detail::DrumRollAnimState &state = entry.state;
                state.durationMs = durationMs > 0 ? durationMs : 1;
                if (state.toIndex != selectedIndex)
                {
                    state.fromIndex = drumRollCurrentIndex(state, now);
                    state.toIndex = selectedIndex;
                    state.startMs = now;
                }
                return state;
            }
            if (!entry.used)
                best = &entry;
            else if (best->used && entry.lastUseMs < best->lastUseMs)
                best = &entry;
        }

        best->used = true;
        best->key = key;
        best->lastUseMs = now;
        best->state = {};
        best->state.fromIndex = static_cast<float>(selectedIndex);
        best->state.toIndex = selectedIndex;
        best->state.durationMs = durationMs > 0 ? durationMs : 1;
        return best->state;
    }

    GUI::InputState GUI::pollInput(Button &next, Button &prev)
    {
        next.update();
        prev.update();

        const bool nextDown = next.isDown();
        const bool prevDown = prev.isDown();
        const bool combo = nextDown && prevDown;

#if PIPGUI_SCREENSHOTS
        handleScreenshotShortcut(combo && !_flags.errorActive);
        if (combo && !_flags.errorActive)
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
        freePopupMenu(plat);
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
        safeFree(plat, _blur.lookup);
        _blur.workLen = 0;
        _blur.rowCap = 0;
        _blur.colCap = 0;
        _blur.lookupSw = 0;
        _blur.lookupSh = 0;
        _blur.lookupW = 0;
        _blur.lookupH = 0;
        _blur.lastUseMs = 0;
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

            safeFree(plat, a->lineColors565);
            safeFree(plat, a->lineValueMins);
            safeFree(plat, a->lineValueMaxs);
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
            return;
        }

        _disp.cfg = normalized;
        _disp.cfgConfigured = true;
        resetDisplayRuntime();
        clearReportedPlatformError();
    }

    void GUI::configureBacklight(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;
        plat->configureBacklightPin(pin, channel, freqHz, resolutionBits);
        setBacklightCallback(backlightPlatformCallback);
    }

    void ConfigureDisplayFluent::apply()
    {
        if (_consumed || !_touched)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::configureDisplay(*_gui, _cfg);
    }

    void ConfigureBacklightFluent::apply()
    {
        if (_consumed || !_touched)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::configureBacklight(*_gui, _pin, _channel, _freqHz, _resolutionBits);
    }

    void SetClipFluent::apply()
    {
        if (_consumed || !_touched)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::setClip(*_gui, _x, _y, _w, _h);
    }

    void ShowLogoFluent::apply()
    {
        if (_consumed || !_touched)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::startLogo(*_gui, _title, _subtitle, _anim, _fg, _bg, _dur, _x, _y);
    }

    void ListInputFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::handleListInput(*_gui, _screenId, _nextDown, _prevDown);
    }

    void PopupMenuInputFluent::apply()
    {
        if (_consumed)
            return;
        _consumed = true;
        if (!_gui)
            return;
        detail::GuiAccess::handlePopupMenuInput(*_gui, _nextDown, _prevDown);
    }

    void ConfigureStatusBarFluent::apply()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::configureStatusBar(*_gui, _enabled, _bgColor, _height, _pos);
    }

    void SetStatusBarTextFluent::apply()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::setStatusBarText(*_gui, _left, _center, _right);
    }

    void SetStatusBarIconFluent::apply()
    {
        if (!beginCommit() || !_hasSide || !_hasIcon)
            return;
        detail::GuiAccess::setStatusBarIcon(*_gui, _side, _iconId, detail::optionalColor32(_color565), _sizePx);
    }

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::Platform *plat = pipcore::GetPlatform();
        if (!plat)
            return;

        resetDisplayRuntime();

#if PIPGUI_STATUS_BAR
        _flags.statusBarDebugMetrics = false;
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
                return;
            }
            clearReportedPlatformError();
        }

        if (!plat->beginDisplay(rotation))
        {
            return;
        }
        clearReportedPlatformError();

        _disp.display = plat->display();
        if (!_disp.display)
        {
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
            invalidateRect(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
            flushDirty();
        }

        syncRegisteredScreens();

        _disp.brightnessMax = plat->loadMaxBrightnessPercent();
        _disp.brightness = _disp.brightnessMax;

        initFonts();

    }

    void GUI::initFonts()
    {
        if (_typo.psdfSizePx == 0)
            _typo.psdfSizePx = 14;
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

    net::WifiState GUI::wifiState() const noexcept
    {
        return net::wifiState();
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

    void GUI::setBacklightCallback(BacklightCallback cb) noexcept
    {
        _disp.backlightCb = cb;
    }

    void GUI::setBrightness(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;
        if (percent > _disp.brightnessMax)
            percent = _disp.brightnessMax;
        _disp.brightness = percent;
        if (_disp.backlightCb)
            _disp.backlightCb(_disp.brightness);
    }

    void GUI::setMaxBrightness(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;
        _disp.brightnessMax = percent;
        if (_disp.brightness > _disp.brightnessMax)
            _disp.brightness = _disp.brightnessMax;
        pipcore::GetPlatform()->storeMaxBrightnessPercent(_disp.brightnessMax);
        if (_disp.backlightCb)
            _disp.backlightCb(_disp.brightness);
    }

    void GUI::setScreenAnim(ScreenAnim anim, uint32_t durationMs)
    {
        _screen.anim = anim;
        _screen.animDurationMs = durationMs;
    }
}

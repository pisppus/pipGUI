#include <pipGUI/core/api/pipGUI.h>
#include <math.h>

namespace pipgui
{

    static pipcore::GuiPlatform *g_sharedPlatform = nullptr;

    static void backlightPlatformCallback(uint16_t level)
    {
        pipcore::GuiPlatform *p = pipgui::GUI::sharedPlatform();
        if (!p)
            return;
        if (level > 100)
            level = 100;
        p->setBacklightPercent(static_cast<uint8_t>(level));
    }

    GUI::~GUI() noexcept
    {
        pipcore::GuiPlatform *plat = platform();

        freeBlurBuffers(plat);
        freeGraphAreas(plat);
        freeListMenus(plat);
        freeTileMenus(plat);
        freeRecovery(plat);
        freeErrors(plat);
        freeStatusBarIcons(plat);
        freeScreenStorage(plat);

        _sprite.deleteSprite();
        _screenFromSprite.deleteSprite();
        _screenToSprite.deleteSprite();
        _flags.spriteEnabled = 0;
        _flags.screenSpritesEnabled = 0;
    }

    void GUI::freeBlurBuffers(pipcore::GuiPlatform *plat) noexcept
    {
        detail::guiFree(plat, _blurPrevOrig);
        detail::guiFree(plat, _blurSmallIn);
        detail::guiFree(plat, _blurSmallTmp);
        detail::guiFree(plat, _blurRowR);
        detail::guiFree(plat, _blurRowG);
        detail::guiFree(plat, _blurRowB);
        detail::guiFree(plat, _blurColR);
        detail::guiFree(plat, _blurColG);
        detail::guiFree(plat, _blurColB);

        _blurPrevOrig = nullptr;
        _blurSmallIn = nullptr;
        _blurSmallTmp = nullptr;
        _blurRowR = nullptr;
        _blurRowG = nullptr;
        _blurRowB = nullptr;
        _blurColR = nullptr;
        _blurColG = nullptr;
        _blurColB = nullptr;
    }

    void GUI::freeGraphAreas(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_graphAreas)
            return;

        for (uint16_t i = 0; i < _screenCapacity; ++i)
        {
            GraphArea *a = _graphAreas[i];
            if (!a)
                continue;

            if (a->samples)
            {
                for (uint16_t line = 0; line < a->lineCount; ++line)
                    detail::guiFree(plat, a->samples[line]);
                detail::guiFree(plat, a->samples);
            }
            detail::guiFree(plat, a->sampleCounts);
            detail::guiFree(plat, a->sampleHead);

            a->~GraphArea();
            detail::guiFree(plat, a);
            _graphAreas[i] = nullptr;
        }
    }

    void GUI::freeListMenus(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_listMenus)
            return;

        for (uint16_t i = 0; i < _screenCapacity; ++i)
        {
            ListMenuState *m = _listMenus[i];
            if (!m)
                continue;

            if (m->cacheNormal)
            {
                for (uint8_t j = 0; j < m->capacity; ++j)
                    detail::guiFree(plat, m->cacheNormal[j]);
                detail::guiFree(plat, m->cacheNormal);
            }
            if (m->cacheActive)
            {
                for (uint8_t j = 0; j < m->capacity; ++j)
                    detail::guiFree(plat, m->cacheActive[j]);
                detail::guiFree(plat, m->cacheActive);
            }

            if (m->items)
            {
                using ListItem = pipgui::ListMenuState::Item;
                for (uint8_t j = 0; j < m->capacity; ++j)
                    m->items[j].~ListItem();
                detail::guiFree(plat, m->items);
                m->items = nullptr;
            }

            if (m->viewportSprite)
            {
                m->viewportSprite->deleteSprite();
                m->viewportSprite->~LGFX_Sprite();
                detail::guiFree(plat, m->viewportSprite);
                m->viewportSprite = nullptr;
            }

            m->~ListMenuState();
            detail::guiFree(plat, m);
            _listMenus[i] = nullptr;
        }
    }

    void GUI::freeTileMenus(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_tileMenus)
            return;

        for (uint16_t i = 0; i < _screenCapacity; ++i)
        {
            TileMenuState *t = _tileMenus[i];
            if (!t)
                continue;

            const uint8_t cap = t->itemCapacity;

            if (t->items)
            {
                using Item = TileMenuState::Item;
                for (uint8_t j = 0; j < cap; ++j)
                    t->items[j].~Item();
                detail::guiFree(plat, t->items);
                t->items = nullptr;
            }
            if (t->layout)
            {
                for (uint8_t j = 0; j < cap; ++j)
                    t->layout[j].~TileLayoutCell();
                detail::guiFree(plat, t->layout);
                t->layout = nullptr;
            }

            t->itemCapacity = 0;

            t->~TileMenuState();
            detail::guiFree(plat, t);
            _tileMenus[i] = nullptr;
        }
    }

    void GUI::freeScreenStorage(pipcore::GuiPlatform *plat) noexcept
    {
        detail::guiFree(plat, _screens);
        detail::guiFree(plat, _graphAreas);
        detail::guiFree(plat, _listMenus);
        detail::guiFree(plat, _tileMenus);

        _screens = nullptr;
        _graphAreas = nullptr;
        _listMenus = nullptr;
        _tileMenus = nullptr;
        _screenCapacity = 0;
    }

    void GUI::freeRecovery(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_recoveryItems)
            return;

        for (uint8_t i = 0; i < _recoveryItemCount; ++i)
            _recoveryItems[i].~RecoveryEntry();
        detail::guiFree(plat, _recoveryItems);
        _recoveryItems = nullptr;
        _recoveryItemCapacity = 0;
        _recoveryItemCount = 0;
    }

    void GUI::freeErrors(pipcore::GuiPlatform *plat) noexcept
    {
        if (!_errorEntries)
            return;

        for (uint8_t i = 0; i < _errorCapacity; ++i)
            _errorEntries[i].~ErrorEntry();
        detail::guiFree(plat, _errorEntries);
        _errorEntries = nullptr;
        _errorCapacity = 0;
        _errorCount = 0;
    }

    void GUI::freeStatusBarIcons(pipcore::GuiPlatform *plat) noexcept
    {
        if (_iconsLeft)
        {
            detail::guiFree(plat, _iconsLeft);
            _iconsLeft = nullptr;
            _iconLeftCapacity = 0;
            _iconLeftCount = 0;
        }
        if (_iconsCenter)
        {
            detail::guiFree(plat, _iconsCenter);
            _iconsCenter = nullptr;
            _iconCenterCapacity = 0;
            _iconCenterCount = 0;
        }
        if (_iconsRight)
        {
            detail::guiFree(plat, _iconsRight);
            _iconsRight = nullptr;
            _iconRightCapacity = 0;
            _iconRightCount = 0;
        }
    }

    void GUI::configureDisplay(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint16_t w, uint16_t h, uint32_t hz)
    {
        _displayCfg.mosi = mosi;
        _displayCfg.sclk = sclk;
        _displayCfg.cs = cs;
        _displayCfg.dc = dc;
        _displayCfg.rst = rst;
        _displayCfg.miso = -1;
        _displayCfg.width = w;
        _displayCfg.height = h;
        _displayCfg.hz = hz;
        _displayCfgConfigured = true;

        if (platform())
            platform()->guiConfigureDisplay(_displayCfg);
    }

    void GUI::begin(uint8_t rotation, uint16_t bgColor)
    {
        pipcore::GuiPlatform *plat = platform();
        if (!plat)
            return;

        if (_displayCfgConfigured)
        {
            if (!plat->guiConfigureDisplay(_displayCfg))
                return;
        }

        if (!plat->guiBeginDisplay(rotation))
            return;

        _tft = plat->guiDisplay();
        if (!_tft)
            return;

        _screenWidth = _tft->width();
        _screenHeight = _tft->height();
        _bgColor = bgColor;

        _tft->fillScreen(_bgColor);
        _tft->setTextWrap(false);
        _tft->setTextColor(0xFFFF, _bgColor);

        using SpriteT = lgfx::LGFX_Sprite;
        auto resetSprite = [&](SpriteT &s)
        {
            s.~SpriteT();
            new (&s) SpriteT(_tft);
            s.setColorDepth(16);
        };

        resetSprite(_sprite);
        resetSprite(_screenFromSprite);
        resetSprite(_screenToSprite);

        _flags.spriteEnabled = _sprite.createSprite(_screenWidth, _screenHeight) != nullptr;
        _activeSprite = _flags.spriteEnabled ? &_sprite : nullptr;

        bool fromOk = _screenFromSprite.createSprite(_screenWidth, _screenHeight);
        bool toOk = fromOk && _screenToSprite.createSprite(_screenWidth, _screenHeight);

        if (!(fromOk && toOk))
        {
            _screenFromSprite.deleteSprite();
            _screenToSprite.deleteSprite();

            int16_t halfW = _screenWidth / 2;
            int16_t halfH = _screenHeight / 2;

            if (halfW > 0 && halfH > 0 && _flags.spriteEnabled)
            {
                fromOk = _screenFromSprite.createSprite(halfW, halfH);
                toOk = fromOk && _screenToSprite.createSprite(halfW, halfH);

                if (!(fromOk && toOk))
                {
                    _screenFromSprite.deleteSprite();
                    _screenToSprite.deleteSprite();
                }
            }
        }

        _flags.screenSpritesEnabled = fromOk && toOk;

        if (plat)
            _brightnessMax = plat->loadMaxBrightnessPercent();

        loadBuiltinPSDF();
        enablePSDF(true);
        if (_psdfSizePx == 0)
            _psdfSizePx = _textStyleBodyPx;
    }

    void GUI::setPlatform(pipcore::GuiPlatform *platform)
    {
        _platform = platform;
        if (platform)
            g_sharedPlatform = platform;
    }

    pipcore::GuiPlatform *GUI::platform() const
    {
        return _platform ? _platform : g_sharedPlatform;
    }

    pipcore::GuiPlatform *GUI::sharedPlatform()
    {
        return g_sharedPlatform;
    }

    uint32_t GUI::nowMs() const
    {
        pipcore::GuiPlatform *plat = platform();
        return plat ? plat->nowMs() : 0U;
    }

    void GUI::setBacklightPin(uint8_t pin, uint8_t channel, uint32_t freqHz, uint8_t resolutionBits)
    {
        pipcore::GuiPlatform *plat = platform();
        if (!plat)
            return;
        plat->configureBacklightPin(pin, channel, freqHz, resolutionBits);
        setBacklightCallback(backlightPlatformCallback);
    }

    void GUI::setMaxBrightness(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;
        _brightnessMax = percent;
        if (pipcore::GuiPlatform *plat = platform())
            plat->storeMaxBrightnessPercent(_brightnessMax);
    }

    void GUI::setScreenAnimation(ScreenAnim anim, uint32_t durationMs)
    {
        _screenAnim = anim;
        _screenAnimDurationMs = durationMs;
    }

    void GUI::setFrcProfile(FrcProfile profile)
    {
        if (profile == FrcProfile::Off)
            profile = FrcProfile::BlueNoise;
        _frcProfile = profile;
    }

    void GUI::enableGamma22(bool enabled)
    {
        if (enabled)
            setFrcProfile(FrcProfile::BlueNoiseGamma22);
        else
            setFrcProfile(FrcProfile::BlueNoise);
    }

}
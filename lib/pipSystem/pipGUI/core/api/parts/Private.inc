#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.h>"
#endif

        void configureListMenu(uint8_t screenId, uint8_t parentScreen, const ListMenuItemDef *items, uint8_t itemCount);
        void configureTileMenu(uint8_t screenId, uint8_t parentScreen, const TileMenuItemDef *items, uint8_t itemCount);

        LovyanGFX *getDrawTarget();
        int16_t AutoX(int32_t contentWidth) const;
        int16_t AutoY(int32_t contentHeight) const;

        bool psdfMeasureText(const String &text, int16_t &outW, int16_t &outH) const;
        void psdfDrawTextInternal(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align);

        void drawCenteredTitle(const String &title,
                               const String &subtitle,
                               uint16_t fg,
                               uint16_t bg);
        void drawErrorCard(LovyanGFX &t,
                           int16_t x,
                           int16_t y,
                           int16_t w,
                           int16_t h,
                           const String &title,
                           const String &detail,
                           uint16_t accentColor,
                           uint8_t opacity,
                           int scrollX);

        void renderBootFrame(uint32_t now);
        void renderErrorFrame(uint32_t now);

        void renderScreenTransition(uint32_t now);
        void renderNotificationOverlay();
        void renderStatusBar(bool useSprite);

        void invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h);
        void flushDirty();
        void tickDebugDirtyOverlay(uint32_t now);

        uint16_t color888To565(int16_t x, int16_t y, uint32_t color888) const;
        void fillCircleFrc(int16_t cx, int16_t cy, int16_t r, uint32_t color888);
        void fillRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888);
        void drawRoundRectFrc(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint32_t color888);
        void blitImage565Frc(int16_t x, int16_t y, const uint16_t *bitmap565, int16_t w, int16_t h);

        uint32_t nowMs() const;
        void ensureScreenStorage(uint8_t id);
        void prepareScreenTransition(ScreenCallback fromCb, ScreenCallback toCb);
        bool ensureScreenTransitionSprites();
        void freeScreenTransitionSprites();
        bool ensureBlurWorkBuffers(uint32_t smallLen, int16_t sw, int16_t sh) noexcept;

        void freeBlurBuffers(pipcore::GuiPlatform *plat) noexcept;
        void freeGraphAreas(pipcore::GuiPlatform *plat) noexcept;
        void freeListMenus(pipcore::GuiPlatform *plat) noexcept;
        void freeTileMenus(pipcore::GuiPlatform *plat) noexcept;
        void freeScreenStorage(pipcore::GuiPlatform *plat) noexcept;
        void freeRecovery(pipcore::GuiPlatform *plat) noexcept;
        void freeErrors(pipcore::GuiPlatform *plat) noexcept;
        void freeStatusBarIcons(pipcore::GuiPlatform *plat) noexcept;

        bool ensureErrorCapacity(uint8_t need);
        bool ensureTileCapacity(TileMenuState &m, uint8_t need);

        void setTileMenuLayout(uint8_t screenId,
                               uint8_t layoutCols,
                               uint8_t layoutRows,
                               const TileLayoutCell *tiles,
                               uint8_t count);

        GraphArea &ensureGraphArea(uint8_t screenId);
        ListMenuState &ensureListMenu(uint8_t screenId);
        TileMenuState &ensureTileMenu(uint8_t screenId);
        ListMenuState *getListMenu(uint8_t screenId);
        TileMenuState *getTileMenu(uint8_t screenId);

        pipcore::GuiPlatform *_platform = nullptr;
        BacklightCallback _backlightCb = nullptr;
        uint8_t _brightnessMax = 100;

        PipLGFX *_tft = nullptr;
        lgfx::LGFX_Sprite _sprite;
        lgfx::LGFX_Sprite _screenFromSprite;
        lgfx::LGFX_Sprite _screenToSprite;
        lgfx::LGFX_Sprite *_activeSprite = nullptr;

        struct DirtyRect
        {
            int16_t x, y, w, h;
        };

        DirtyRect _dirty[8] = {};
        uint8_t _dirtyCount = 0;

        bool _debugShowDirtyRects = false;
        uint16_t _debugDirtyRectColor = 0x39E7;
        uint16_t _debugDirtyRectActiveColor = 0xF81F;
        DirtyRect _debugRects[8] = {};
        uint8_t _debugRectCount = 0;
        uint32_t _debugHighlightUntilMs = 0;
        uint8_t _debugOverlayDrawnState = 0;
        uint32_t _debugLastFlushMs = 0;

        uint16_t _screenWidth = 0, _screenHeight = 0;
        uint32_t _bgColor = 0;

        FrcProfile _frcProfile = FrcProfile::BlueNoiseGamma22;
        uint32_t _frcSeed = 0x6D2B79F5U;
        uint32_t _frcLastUpdateMs = 0;
        uint16_t _frcTemporalPeriodMs = 66;

        pipcore::GuiDisplayConfig _displayCfg;
        bool _displayCfgConfigured = false;

        uint16_t _screenCapacity = 0;
        ScreenCallback *_screens = nullptr;
        uint8_t _currentScreen = 255;

        ScreenAnim _screenAnim = None;
        uint8_t _screenFrom = 0, _screenTo = 0;
        int8_t _screenTransDir = 0;
        uint32_t _screenAnimStartMs = 0, _screenAnimDurationMs = 0;

        struct Flags
        {
            unsigned needRedraw : 1;
            unsigned spriteEnabled : 1;
            unsigned renderToSprite : 1;
            unsigned screenTransition : 1;
            unsigned screenSpritesEnabled : 1;
            unsigned bootActive : 1;
            unsigned psdfLoaded : 1;
            unsigned psdfEnabled : 1;
            unsigned errorTransition : 1;
            unsigned errorActive : 1;
            unsigned errorFlashState : 1;
            unsigned errorButtonDown : 1;
            unsigned errorAwaitRelease : 1;
            unsigned notifActive : 1;
            unsigned notifButtonDown : 1;
            unsigned notifClosing : 1;
            unsigned notifDelayed : 1;
            unsigned notifAwaitRelease : 1;
            unsigned statusBarEnabled : 1;
            unsigned recoveryEnabled : 1;
            unsigned recoveryArmed : 1;
            unsigned recoveryActive : 1;
            unsigned bootWasActive : 1;
            unsigned recoverySavedStatusBarEnabled : 1;
        } _flags = {};

        BootAnimation _bootAnim = None;
        String _bootTitle;
        String _bootSubtitle;
        uint32_t _bootFgColor = 0, _bootBgColor = 0;
        uint32_t _bootStartMs = 0, _bootDurationMs = 0;
        int16_t _bootX = 0, _bootY = 0;

        uint16_t _logoTitleSizePx = 0;
        uint16_t _logoSubtitleSizePx = 0;

        uint16_t _textStyleH1Px = 24;
        uint16_t _textStyleH2Px = 18;
        uint16_t _textStyleBodyPx = 14;
        uint16_t _textStyleCaptionPx = 12;

        uint16_t _psdfSizePx = 0;
        float _psdfWeightBias = 0.10f;

        struct ErrorEntry
        {
            String title, detail;
            ErrorType type;
            int16_t scrollPos, textWidth;
        };

        ErrorEntry *_errorEntries = nullptr;
        uint8_t _errorCount = 0;
        uint8_t _errorCapacity = 0;
        uint8_t _errorCurrentIndex = 0;
        uint8_t _errorNextIndex = 0;
        uint32_t _errorAnimStartMs = 0;
        uint32_t _errorLastScrollMs = 0;

        ErrorType _errorType = Warning;
        String _errorTitle;
        String _errorMessage;
        String _errorButtonText;

        uint32_t _errorLastToggleMs = 0, _errorFlashIntervalMs = 0, _errorEndMs = 0;
        ButtonVisualState _errorButtonState;

        String _notifTitle;
        String _notifMessage;
        String _notifButtonText;
        NotificationType _notifType = Normal;
        uint32_t _notifStartMs = 0, _notifAnimDurationMs = 0;
        uint32_t _notifUnlockMs = 0;
        ButtonVisualState _notifButtonState;

        StatusBarPosition _statusBarPos = Top;
        uint8_t _statusBarHeight = 0;
        uint16_t _statusBarBg = 0x0000;
        uint16_t _statusBarFg = 0xFFFF;
        String _statusTextLeft;
        String _statusTextCenter;
        String _statusTextRight;
        StatusBarCustomCallback _statusBarCustom = nullptr;

        enum StatusBarDirty : uint8_t
        {
            StatusBarDirtyLeft = 1 << 0,
            StatusBarDirtyCenter = 1 << 1,
            StatusBarDirtyRight = 1 << 2,
            StatusBarDirtyBattery = 1 << 3,
            StatusBarDirtyAll = 0xFF,
        };

        uint8_t _statusBarDirtyMask = 0;
        DirtyRect _statusBarLastLeft = {};
        DirtyRect _statusBarLastCenter = {};
        DirtyRect _statusBarLastRight = {};
        DirtyRect _statusBarLastBattery = {};

        int8_t _batteryLevel = -1;
        BatteryStyle _batteryStyle = Hidden;

        StatusIcon *_iconsLeft = nullptr;
        StatusIcon *_iconsCenter = nullptr;
        StatusIcon *_iconsRight = nullptr;
        uint8_t _iconLeftCount = 0, _iconCenterCount = 0, _iconRightCount = 0;
        uint8_t _iconLeftCapacity = 0, _iconCenterCapacity = 0, _iconRightCapacity = 0;

        GraphArea **_graphAreas = nullptr;
        ListMenuState **_listMenus = nullptr;
        TileMenuState **_tileMenus = nullptr;

        uint16_t *_blurPrevOrig = nullptr;
        uint32_t _blurPrevOrigLen = 0;
        int16_t _blurPrevX = 0, _blurPrevY = 0;
        int16_t _blurPrevW = 0, _blurPrevH = 0;
        uint8_t _blurPrevScale = 1;

        uint16_t *_blurSmallIn = nullptr;
        uint16_t *_blurSmallTmp = nullptr;
        uint32_t *_blurRowR = nullptr;
        uint32_t *_blurRowG = nullptr;
        uint32_t *_blurRowB = nullptr;
        uint32_t *_blurColR = nullptr;
        uint32_t *_blurColG = nullptr;
        uint32_t *_blurColB = nullptr;
        uint32_t _blurWorkLen = 0;
        uint16_t _blurRowCap = 0;
        uint16_t _blurColCap = 0;

        struct RecoveryEntry
        {
            String title;
            String subtitle;
            RecoveryActionCallback action;
            uint8_t actionScreen;
        };

        Button *_recoveryPrev = nullptr;
        Button *_recoveryNext = nullptr;
        Button *_recoveryHold = nullptr;

        uint32_t _recoveryHoldMs = 900;
        uint32_t _recoveryHoldStartMs = 0;
        uint8_t _recoveryScreenId = 250;
        uint8_t _recoveryActionScreenId = 251;
        uint8_t _recoveryExitScreenId = 252;
        uint8_t _recoveryReturnScreen = 255;
        uint8_t _recoveryItemCount = 0;
        uint8_t _recoveryItemCapacity = 0;
        RecoveryEntry *_recoveryItems = nullptr;

        uint16_t _recoverySavedBgColor = 0;
        StatusBarPosition _recoverySavedStatusBarPos = Top;
        uint8_t _recoverySavedStatusBarHeight = 0;
        uint16_t _recoverySavedStatusBarBg = 0x0000;
        uint16_t _recoverySavedStatusBarFg = 0xFFFF;
        String _recoverySavedStatusLeft;
        String _recoverySavedStatusCenter;
        String _recoverySavedStatusRight;
        int8_t _recoverySavedBatteryLevel = -1;
        BatteryStyle _recoverySavedBatteryStyle = Hidden;

        void tickRecovery(uint32_t now);
        void renderRecoveryScreen();
        static void recoveryScreenCallback(GUI &ui);
        static void recoveryExitAction(GUI &ui);

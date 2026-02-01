#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.h>"
#endif

        void drawGraphGrid(int16_t x,
                           int16_t y,
                           int16_t w,
                           int16_t h,
                           uint8_t radius,
                           GraphDirection dir,
                           uint16_t bgColor,
                           uint16_t gridColor,
                           float speed = 1.0f);
        void setGraphAutoScale(bool enabled);
        void drawGraphLine(uint8_t lineIndex, int16_t value, uint16_t color, int16_t valueMin, int16_t valueMax);

        void updateGraphGrid(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             uint8_t radius,
                             GraphDirection dir,
                             uint16_t bgColor,
                             uint16_t gridColor,
                             float speed = 1.0f);
        void updateGraphLine(uint8_t lineIndex, int16_t value, uint16_t color, int16_t valueMin, int16_t valueMax);

        void drawProgressBar(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             uint8_t value,
                             uint16_t baseColor,
                             uint16_t fillColor,
                             uint8_t radius = 6,
                             ProgressAnim anim = None);
        void drawProgressBar(int16_t x,
                             int16_t y,
                             int16_t w,
                             int16_t h,
                             uint8_t value,
                             uint16_t color,
                             uint8_t radius = 6,
                             ProgressAnim anim = None);
        void drawCircularProgressBar(int16_t x,
                                     int16_t y,
                                     int16_t r,
                                     uint8_t thickness,
                                     uint8_t value,
                                     uint16_t baseColor,
                                     uint16_t fillColor,
                                     ProgressAnim anim = None);
        void drawCircularProgressBar(int16_t x,
                                     int16_t y,
                                     int16_t r,
                                     uint8_t thickness,
                                     uint8_t value,
                                     uint16_t color,
                                     ProgressAnim anim = None);

        struct ProgressBarState
        {
            bool inited;
            uint8_t value;
            ProgressAnim anim;
            int16_t segL;
            int16_t segR;
        };

        void updateProgressBar(int16_t x,
                               int16_t y,
                               int16_t w,
                               int16_t h,
                               uint8_t value,
                               uint16_t baseColor,
                               uint16_t fillColor,
                               uint8_t radius = 6,
                               ProgressAnim anim = None);
        void updateProgressBar(int16_t x,
                               int16_t y,
                               int16_t w,
                               int16_t h,
                               uint8_t value,
                               uint16_t color,
                               uint8_t radius = 6,
                               ProgressAnim anim = None);
        void updateProgressBar(ProgressBarState &s,
                               int16_t x,
                               int16_t y,
                               int16_t w,
                               int16_t h,
                               uint8_t value,
                               uint16_t baseColor,
                               uint16_t fillColor,
                               uint8_t radius = 6,
                               ProgressAnim anim = None);
        void updateCircularProgressBar(int16_t x,
                                       int16_t y,
                                       int16_t r,
                                       uint8_t thickness,
                                       uint8_t value,
                                       uint16_t baseColor,
                                       uint16_t fillColor,
                                       ProgressAnim anim = None);
        void updateCircularProgressBar(int16_t x,
                                       int16_t y,
                                       int16_t r,
                                       uint8_t thickness,
                                       uint8_t value,
                                       uint16_t color,
                                       ProgressAnim anim = None);

        void updateStatusBar();

        void updateButtonPress(ButtonVisualState &s, bool isDown);

        void drawButton(const String &label,
                        const uint16_t *iconBitmap, uint8_t iconW, uint8_t iconH,
                        int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t baseColor,
                        uint8_t radius,
                        const ButtonVisualState &state);

        void drawButton(const String &label,
                        int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t baseColor,
                        uint8_t radius,
                        const ButtonVisualState &state);

        void updateButton(const String &label,
                          const uint16_t *iconBitmap, uint8_t iconW, uint8_t iconH,
                          int16_t x, int16_t y, int16_t w, int16_t h,
                          uint16_t baseColor,
                          uint8_t radius,
                          const ButtonVisualState &state);

        void updateButton(const String &label,
                          int16_t x, int16_t y, int16_t w, int16_t h,
                          uint16_t baseColor,
                          uint8_t radius,
                          const ButtonVisualState &state);

        bool updateToggleSwitch(ToggleSwitchState &s, bool pressed);

        void drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                              ToggleSwitchState &state,
                              uint16_t activeColor,
                              int32_t inactiveColor = -1,
                              int32_t knobColor = -1);

        void updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                ToggleSwitchState &state,
                                uint16_t activeColor,
                                int32_t inactiveColor = -1,
                                int32_t knobColor = -1);

        bool loadTTF(const unsigned char *data, size_t size, uint8_t faceIndex = 0);
        bool loadBuiltinTTF(BuiltinTTF font);
        bool ttfFontLoaded() const;

        void setTTFFontSize(uint16_t px);
        void setTTFFontColor(uint16_t fg, uint16_t bg);
        void configureTextStyles(uint16_t h1Px = 24, uint16_t h2Px = 18, uint16_t bodyPx = 14, uint16_t captionPx = 12);
        void setTextStyle(TextStyle style);
        void drawTTFText(const String &text,
                         int16_t x,
                         int16_t y,
                         uint16_t color,
                         uint16_t bgColor,
                         TextAlign align = Left);
        void drawTTFText(const String &text,
                         int16_t x,
                         int16_t y,
                         uint16_t color,
                         TextAlign align = Left);
        void updateTTFText(const String &text,
                           int16_t x,
                           int16_t y,
                           uint16_t color,
                           uint16_t bgColor,
                           TextAlign align = Left);
        void drawTTFTextStyle(const String &text,
                              TextStyle style,
                              int16_t x,
                              int16_t y,
                              uint16_t color,
                              TextAlign align = Left,
                              int32_t bgColor = -1);
        void drawEmojiIconTTF(const String &emoji,
                              int16_t x,
                              int16_t y,
                              uint16_t sizePx,
                              uint16_t color,
                              int32_t bgColor = -1);

        void setScreenAnimation(ScreenAnim anim, uint32_t durationMs);

        void logoTitleFont(const uint8_t *font) { _logoTitleFont = font; }
        void logoTitleFont(const uint8_t *font, uint16_t sizePx)
        {
            _logoTitleFont = font;
            _logoTitleTTFSize = sizePx;
        }
        void logoSubtitleFont(const uint8_t *font) { _logoSubtitleFont = font; }
        void logoSubtitleFont(const uint8_t *font, uint16_t sizePx)
        {
            _logoSubtitleFont = font;
            _logoSubtitleTTFSize = sizePx;
        }
        void logoFonts(const uint8_t *t, const uint8_t *s)
        {
            _logoTitleFont = t;
            _logoSubtitleFont = s;
        }
        void logoFonts(const uint8_t *t, const uint8_t *s, uint16_t titleSizePx, uint16_t subtitleSizePx)
        {
            _logoTitleFont = t;
            _logoSubtitleFont = s;
            _logoTitleTTFSize = titleSizePx;
            _logoSubtitleTTFSize = subtitleSizePx;
        }

        void showLogo(const String &t,
                      const String &s,
                      BootAnimation a = None,
                      uint16_t fg = 0xFFFF,
                      uint16_t bg = 0x0000,
                      uint32_t dur = 0,
                      int16_t x = -1,
                      int16_t y = -1);

        void regScreen(uint8_t id, ScreenCallback cb);
        void setScreen(uint8_t id);
        uint8_t currentScreen() const;

        void requestRedraw();
        void nextScreen();
        void prevScreen();

        void loop();
        void loopWithInput(Button &next, Button &prev);

        void showNotification(const String &t,
                              const String &m,
                              const String &btn = "OK",
                              uint16_t delay = 0,
                              NotificationType type = Normal);
        void setNotificationButtonDown(bool down);
        bool notificationActive() const;

        void showError(const String &title,
                       const String &message,
                       ErrorType type = Warning,
                       const String &buttonText = "OK");
        void nextError();
        bool errorActive() const;
        void setErrorButtonDown(bool down);

        void configureListMenu(uint8_t screenId, uint8_t parentScreen, std::initializer_list<ListMenuItemDef> items);
        template <size_t N>
        void configureListMenu(uint8_t screenId, uint8_t parentScreen, const ListMenuItemDef (&items)[N])
        {
            configureListMenu(screenId, parentScreen, items, static_cast<uint8_t>(N));
        }

        void setListMenuStyle(uint8_t screenId,
                              uint16_t cardColor,
                              uint16_t cardActiveColor,
                              uint8_t spacing = 8,
                              uint8_t radius = 8,
                              int16_t cardWidth = 0,
                              int16_t cardHeight = 0,
                              uint16_t titleFontPx = 0,
                              uint16_t subtitleFontPx = 0,
                              uint16_t lineGapPx = 0,
                              ListMenuMode mode = Cards);
        void handleListMenuInput(uint8_t screenId, bool nextPressed, bool prevPressed, bool nextDown, bool prevDown);
        void renderListMenu(uint8_t screenId);
        void renderListMenu(uint8_t screenId,
                            int16_t x,
                            int16_t y,
                            int16_t w,
                            int16_t h,
                            uint16_t bgColor);

        bool updateListMenu(uint8_t screenId);
        bool updateListMenu(uint8_t screenId,
                            int16_t x,
                            int16_t y,
                            int16_t w,
                            int16_t h,
                            uint16_t bgColor);

        void configureTileMenu(uint8_t screenId, uint8_t parentScreen, std::initializer_list<TileMenuItemDef> items);
        template <size_t N>
        void configureTileMenu(uint8_t screenId, uint8_t parentScreen, const TileMenuItemDef (&items)[N])
        {
            configureTileMenu(screenId, parentScreen, items, static_cast<uint8_t>(N));
        }

        void setTileMenuStyle(uint8_t screenId,
                              uint16_t cardColor,
                              uint16_t cardActiveColor,
                              uint8_t radius = 8,
                              uint8_t spacing = 8,
                              uint8_t columns = 2,
                              int16_t tileWidth = 0,
                              int16_t tileHeight = 0,
                              uint16_t titleFontPx = 0,
                              uint16_t subtitleFontPx = 0,
                              uint16_t lineGapPx = 0,
                              TileContentMode contentMode = TextSubtitle);

        void setTileMenuLayout(uint8_t screenId,
                               uint8_t layoutCols,
                               uint8_t layoutRows,
                               std::initializer_list<TileLayoutCell> tiles);
        template <size_t N>
        void setTileMenuLayout(uint8_t screenId,
                               uint8_t layoutCols,
                               uint8_t layoutRows,
                               const TileLayoutCell (&tiles)[N])
        {
            setTileMenuLayout(screenId, layoutCols, layoutRows, tiles, static_cast<uint8_t>(N));
        }

        void handleTileMenuInput(uint8_t screenId, bool nextPressed, bool prevPressed, bool nextDown, bool prevDown);
        void renderTileMenu(uint8_t screenId);

        void updateTileMenu(uint8_t screenId, uint8_t prevSelectedIndex);

        void configureStatusBar(bool enabled,
                                uint16_t bgColor = 0x0000,
                                uint8_t height = 0,
                                StatusBarPosition pos = Top);
        void setStatusBarText(const String &left, const String &center, const String &right);
        void setStatusBarBattery(int8_t levelPercent, BatteryStyle style);
        void clearStatusBarIcons()
        {
            _statusBarDirtyMask = StatusBarDirtyAll;
            _iconLeftCount = 0;
            _iconCenterCount = 0;
            _iconRightCount = 0;

            for (uint8_t i = 0; i < _iconLeftCapacity; ++i)
                _iconsLeft[i] = {};
            for (uint8_t i = 0; i < _iconCenterCapacity; ++i)
                _iconsCenter[i] = {};
            for (uint8_t i = 0; i < _iconRightCapacity; ++i)
                _iconsRight[i] = {};
        }
        void addStatusBarIcon(StatusBarIconPos pos,
                              const uint16_t *bitmap,
                              uint8_t w,
                              uint8_t h);
        void setStatusBarCustom(StatusBarCustomCallback cb);
        int16_t statusBarHeight() const;

        void updateStatusBarBattery();
        void updateStatusBarCenter();

        void setDebugShowDirtyRects(bool enabled) { _debugShowDirtyRects = enabled; }
        bool debugShowDirtyRects() const { return _debugShowDirtyRects; }
        void setDebugDirtyRectColor(uint16_t color) { _debugDirtyRectColor = color; }
        uint16_t debugDirtyRectColor() const { return _debugDirtyRectColor; }
        void setDebugDirtyRectActiveColor(uint16_t color) { _debugDirtyRectActiveColor = color; }
        uint16_t debugDirtyRectActiveColor() const { return _debugDirtyRectActiveColor; }

        void enableRecovery(Button *prevButton,
                            Button *nextButton,
                            uint32_t holdMs = 900);
        void enableRecovery(Button *prevButton,
                            Button *nextButton,
                            Button *holdButton,
                            uint32_t holdMs);
        void setRecoveryMenu(std::initializer_list<RecoveryItemDef> items);
        bool recoveryEnabled() const;
        bool recoveryActive() const;

        void setFrcProfile(FrcProfile profile) { _frcProfile = profile; }
        FrcProfile frcProfile() const { return _frcProfile; }
        void setFrcSeed(uint32_t seed) { _frcSeed = seed; }
        uint32_t frcSeed() const { return _frcSeed; }
        void setFrcTemporalPeriodMs(uint16_t ms) { _frcTemporalPeriodMs = ms ? ms : 1; }
        uint16_t frcTemporalPeriodMs() const { return _frcTemporalPeriodMs; }

        uint16_t rgb888To565Frc(uint8_t r,
                                uint8_t g,
                                uint8_t b,
                                int16_t x,
                                int16_t y,
                                FrcProfile profile) const;
        void clear888(uint8_t r,
                      uint8_t g,
                      uint8_t b,
                      FrcProfile profile);
        void fillRect888(int16_t x,
                         int16_t y,
                         int16_t w,
                         int16_t h,
                         uint8_t r,
                         uint8_t g,
                         uint8_t b,
                         FrcProfile profile);

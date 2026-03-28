#pragma once

#include <pipCore/Display.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <pipGUI/Core/Types.hpp>
#include <pipGUI/Core/Internal/GuiState.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <pipGUI/Systems/Update/Ota.hpp>

#if (PIPGUI_SCREENSHOT_MODE == 2)
#include <FS.h>
#include <LittleFS.h>
#endif

namespace pipgui
{
    struct ConfigDisplayFluent;
    struct ConfigureBacklightFluent;
    struct SetClipFluent;
    struct ShowLogoFluent;
    struct ShowErrorFluent;
    struct ConfigureStatusBarFluent;
    struct SetStatusBarTextFluent;
    struct SetStatusBarIconFluent;

    struct DrawRectFluent;
    struct GradientVerticalFluent;
    struct GradientHorizontalFluent;
    struct GradientCornersFluent;
    struct GradientDiagonalFluent;
    struct GradientRadialFluent;
    struct DrawLineFluent;
    struct DrawCircleFluent;
    struct DrawArcFluent;
    struct DrawEllipseFluent;
    struct DrawTriangleFluent;
    struct DrawSquircleRectFluent;

    template <bool IsUpdate>
    struct BlurRegionFluentT;
    using DrawBlurFluent = BlurRegionFluentT<false>;
    using UpdateBlurFluent = BlurRegionFluentT<true>;

    template <bool IsUpdate>
    struct GlowCircleFluentT;
    using DrawGlowCircleFluent = GlowCircleFluentT<false>;
    using UpdateGlowCircleFluent = GlowCircleFluentT<true>;

    template <bool IsUpdate>
    struct ToggleSwitchFluentT;
    using DrawToggleSwitchFluent = ToggleSwitchFluentT<false>;
    using UpdateToggleSwitchFluent = ToggleSwitchFluentT<true>;

    template <bool IsUpdate>
    struct ButtonFluentT;
    using DrawButtonFluent = ButtonFluentT<false>;
    using UpdateButtonFluent = ButtonFluentT<true>;

    template <bool IsUpdate>
    struct SliderFluentT;
    using DrawSliderFluent = SliderFluentT<false>;
    using UpdateSliderFluent = SliderFluentT<true>;

    template <bool IsUpdate>
    struct ProgressFluentT;
    using DrawProgressFluent = ProgressFluentT<false>;
    using UpdateProgressFluent = ProgressFluentT<true>;

    template <bool IsUpdate>
    struct CircleProgressFluentT;
    using DrawCircleProgressFluent = CircleProgressFluentT<false>;
    using UpdateCircleProgressFluent = CircleProgressFluentT<true>;

    template <bool IsUpdate>
    struct ScrollDotsFluentT;
    using DrawScrollDotsFluent = ScrollDotsFluentT<false>;
    using UpdateScrollDotsFluent = ScrollDotsFluentT<true>;

    template <bool IsUpdate>
    struct GraphGridFluentT;
    using DrawGraphGridFluent = GraphGridFluentT<false>;
    using UpdateGraphGridFluent = GraphGridFluentT<true>;

    template <bool IsUpdate>
    struct GraphLineFluentT;
    using DrawGraphLineFluent = GraphLineFluentT<false>;
    using UpdateGraphLineFluent = GraphLineFluentT<true>;

    template <bool IsUpdate>
    struct GraphSamplesFluentT;
    using DrawGraphSamplesFluent = GraphSamplesFluentT<false>;
    using UpdateGraphSamplesFluent = GraphSamplesFluentT<true>;

    struct ToastFluent;
    struct NotificationFluent;
    struct PopupMenuFluent;
    struct DrawIconFluent;
    template <bool IsUpdate>
    struct AnimIconFluentT;
    using DrawAnimIconFluent = AnimIconFluentT<false>;
    using UpdateAnimIconFluent = AnimIconFluentT<true>;
    struct DrawScreenshotFluent;
    struct ConfigGraphScopeFluent;

    template <bool IsUpdate>
    struct TextFluentT;
    using DrawTextFluent = TextFluentT<false>;
    using UpdateTextFluent = TextFluentT<true>;

    struct DrawTextMarqueeFluent;
    struct DrawTextEllipsizedFluent;

    struct ListInputFluent;
    struct ConfigureListFluent;
    struct TileInputFluent;
    struct ConfigureTileFluent;
    struct PopupMenuInputFluent;

    using PopupMenuItemFn = const char *(*)(void *user, uint8_t idx);

    struct GraphArea;
    struct ListState;
    struct TileState;
    struct TileStyle;

    namespace detail
    {
        struct GuiAccess;
        struct TextFontGuard;
    }

    class GUI
    {
    public:
        struct InputState
        {
            bool nextDown = false;
            bool prevDown = false;
            bool nextPressed = false;
            bool prevPressed = false;
            bool comboDown = false;
        };

        GUI();
        ~GUI() noexcept;

        GUI(const GUI &) = delete;
        GUI &operator=(const GUI &) = delete;

        [[nodiscard]] pipcore::Platform *platform() const noexcept;

        [[nodiscard]] ConfigDisplayFluent configDisplay();
        [[nodiscard]] ConfigureBacklightFluent setBacklight();
        [[nodiscard]] SetClipFluent setClip();
        [[nodiscard]] ShowLogoFluent showLogo();
        void begin(uint8_t rotation = 0);

        void setBacklightHandler(BacklightHandler handler) noexcept;
        void setBrightness(uint8_t percent);
        void setMaxBrightness(uint8_t percent);
        [[nodiscard]] uint8_t brightness() const noexcept { return _disp.brightness; }
        [[nodiscard]] uint8_t maxBrightness() const noexcept { return _disp.brightnessMax; }

        pipcore::Display &display();
        [[nodiscard]] pipcore::Display *displayPtr() const noexcept { return _disp.display; }
        [[nodiscard]] bool displayReady() const noexcept { return _disp.display != nullptr; }

        [[nodiscard]] bool startScreenshot();
        void configureScreenshotGallery(uint8_t maxShots = 12, uint16_t thumbW = 64, uint16_t thumbH = 40, uint16_t padding = 6);
        [[nodiscard]] uint8_t screenshotCount() const noexcept { return _shots.count; }
        [[nodiscard]] DrawScreenshotFluent drawScreenshot();
        InputState pollInput(Button &next, Button &prev);

        // WiFi
        void requestWiFi(bool enabled) noexcept;
        [[nodiscard]] net::WifiState wifiState() const noexcept;
        [[nodiscard]] bool wifiConnected() const noexcept;
        [[nodiscard]] uint32_t wifiLocalIpV4() const noexcept;

        // OTA (signed manifest, non-blocking state machine)
        void otaConfigure(OtaStatusCallback cb = nullptr,
                          void *user = nullptr) noexcept;

        void otaRequestCheck() noexcept;
        void otaRequestCheck(OtaCheckMode mode) noexcept;
        void otaRequestInstall() noexcept;
        void otaRequestStableList() noexcept;
        [[nodiscard]] bool otaStableListReady() const noexcept;
        [[nodiscard]] uint8_t otaStableListCount() const noexcept;
        [[nodiscard]] const char *otaStableListVersion(uint8_t idx) const noexcept;
        void otaRequestInstallStableVersion(const char *version) noexcept;
        void otaCancel() noexcept;
        void otaService() noexcept;
        void otaMarkAppValid() noexcept;
        [[nodiscard]] const OtaStatus &otaStatus() const noexcept;

        uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) const;
        void clear(uint16_t color = 0x0000);

        [[nodiscard]] DrawRectFluent drawRect();
        [[nodiscard]] GradientVerticalFluent gradientVertical();
        [[nodiscard]] GradientHorizontalFluent gradientHorizontal();
        [[nodiscard]] GradientCornersFluent gradientCorners();
        [[nodiscard]] GradientDiagonalFluent gradientDiagonal();
        [[nodiscard]] GradientRadialFluent gradientRadial();

        [[nodiscard]] DrawLineFluent drawLine();
        [[nodiscard]] DrawCircleFluent drawCircle();
        [[nodiscard]] DrawArcFluent drawArc();
        [[nodiscard]] DrawEllipseFluent drawEllipse();
        [[nodiscard]] DrawTriangleFluent drawTriangle();
        [[nodiscard]] DrawSquircleRectFluent drawSquircleRect();

        [[nodiscard]] DrawBlurFluent drawBlur();
        [[nodiscard]] UpdateBlurFluent updateBlur();

        [[nodiscard]] DrawGlowCircleFluent drawGlowCircle();
        [[nodiscard]] UpdateGlowCircleFluent updateGlowCircle();

        [[nodiscard]] DrawScrollDotsFluent drawScrollDots();
        [[nodiscard]] UpdateScrollDotsFluent updateScrollDots();
        [[nodiscard]] DrawGraphGridFluent drawGraphGrid();
        [[nodiscard]] UpdateGraphGridFluent updateGraphGrid();
        [[nodiscard]] DrawGraphLineFluent drawGraphLine();
        [[nodiscard]] UpdateGraphLineFluent updateGraphLine();
        [[nodiscard]] DrawGraphSamplesFluent drawGraphSamples();
        [[nodiscard]] UpdateGraphSamplesFluent updateGraphSamples();

        [[nodiscard]] DrawToggleSwitchFluent drawToggleSwitch();
        [[nodiscard]] UpdateToggleSwitchFluent updateToggleSwitch();

        [[nodiscard]] DrawButtonFluent drawButton();
        [[nodiscard]] UpdateButtonFluent updateButton();
        [[nodiscard]] DrawSliderFluent drawSlider();
        [[nodiscard]] UpdateSliderFluent updateSlider();

        [[nodiscard]] DrawProgressFluent drawProgress();
        [[nodiscard]] UpdateProgressFluent updateProgress();

        [[nodiscard]] DrawCircleProgressFluent drawCircleProgress();
        [[nodiscard]] UpdateCircleProgressFluent updateCircleProgress();

        [[nodiscard]] ToastFluent showToast();
        [[nodiscard]] NotificationFluent showNotification();
        [[nodiscard]] ShowErrorFluent showError();
        [[nodiscard]] PopupMenuFluent showPopupMenu();
        [[nodiscard]] PopupMenuInputFluent popupMenuInput();
        [[nodiscard]] ConfigGraphScopeFluent configGraphScope();
        [[nodiscard]] bool popupMenuActive() const noexcept { return _flags.popupActive; }
        [[nodiscard]] bool popupMenuHasResult() const noexcept { return _popup.resultReady; }
        [[nodiscard]] int16_t popupMenuTakeResult() noexcept
        {
            if (!_popup.resultReady)
                return -1;
            _popup.resultReady = false;
            return _popup.resultIndex;
        }

        void setGraphScale(bool enabled);
        [[nodiscard]] uint16_t graphRate(uint8_t screenId) const noexcept;
        [[nodiscard]] uint16_t graphTimebase(uint8_t screenId) const noexcept;
        [[nodiscard]] uint16_t graphSamplesVisible(uint8_t screenId) const noexcept;

        [[nodiscard]] DrawIconFluent drawIcon();
        [[nodiscard]] DrawAnimIconFluent drawAnimIcon();
        [[nodiscard]] UpdateAnimIconFluent updateAnimIcon();
        [[nodiscard]] DrawTextFluent drawText();
        [[nodiscard]] UpdateTextFluent updateText();
        [[nodiscard]] DrawTextMarqueeFluent drawTextMarquee();
        [[nodiscard]] DrawTextEllipsizedFluent drawTextEllipsized();

        void drawDrumRollHorizontal(int16_t x, int16_t y, int16_t w, int16_t h,
                                    const String *options, uint8_t count, uint8_t selectedIndex,
                                    uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0, uint16_t animDurationMs = 280);

        void drawDrumRollVertical(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const String *options, uint8_t count, uint8_t selectedIndex,
                                  uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0, uint16_t animDurationMs = 280);

        FontId registerFont(const uint8_t *atlasData,
                            uint16_t atlasWidth, uint16_t atlasHeight,
                            float distanceRange, float nominalSizePx,
                            float ascender, float descender, float lineHeight,
                            const void *glyphs, uint16_t glyphCount,
                            const void *kerningPairs = nullptr, uint16_t kerningPairCount = 0,
                            int8_t tracking128 = 0);

        [[nodiscard]] bool setFont(FontId fontId);
        [[nodiscard]] FontId fontId() const noexcept;
        void setFontSize(uint16_t px);
        [[nodiscard]] uint16_t fontSize() const noexcept;
        void setFontWeight(uint16_t weight);
        [[nodiscard]] uint16_t fontWeight() const noexcept;

        void setTextStyle(TextStyle style);

        [[nodiscard]] ConfigureListFluent configureList();
        bool updateList(uint8_t screenId);
        [[nodiscard]] ListInputFluent listInput(uint8_t screenId);

        [[nodiscard]] ConfigureTileFluent configureTile();
        void renderTile(uint8_t screenId);
        [[nodiscard]] TileInputFluent tileInput(uint8_t screenId);

        void setScreen(uint8_t id);
        [[nodiscard]] uint8_t currentScreen() const noexcept;
        [[nodiscard]] bool screenTransitionActive() const noexcept;
        void nextScreen();
        void prevScreen();
        void loop();
        void loopWithInput(Button &next, Button &prev);
        void requestRedraw();
        void setScreenAnim(ScreenAnim anim, uint32_t durationMs);
        void clearClip();

        void setNotificationButtonDown(bool down);
        [[nodiscard]] bool notificationActive() const noexcept;
        [[nodiscard]] bool toastActive() const;

        void nextError();
        void prevError();
        [[nodiscard]] bool errorActive() const noexcept;
        void setErrorButtonsDown(bool nextDown, bool prevDown, bool comboDown = false);
        void setErrorButtonDown(bool down);

        [[nodiscard]] ConfigureStatusBarFluent configureStatusBar();

        void setStatusBarStyle(StatusBarStyle style) noexcept
        {
            _status.style = style;
            _status.dirtyMask = detail::StatusBarDirtyAll;
        }

        [[nodiscard]] SetStatusBarTextFluent setStatusBarText();
        void setStatusBarBattery(int8_t levelPercent, BatteryStyle style);
        [[nodiscard]] SetStatusBarIconFluent setStatusBarIcon();
        void clearStatusBarIcon(TextAlign side);
        void setStatusBarCustom(StatusBarCustomCallback cb);
        [[nodiscard]] int16_t statusBarHeight() const noexcept;
        void updateStatusBar();
        void renderStatusBar();

        [[nodiscard]] static constexpr uint32_t rgb888(uint8_t r, uint8_t g, uint8_t b) noexcept
        {
            return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
        }

        [[nodiscard]] uint16_t screenWidth() const noexcept { return _render.screenWidth; }
        [[nodiscard]] uint16_t screenHeight() const noexcept { return _render.screenHeight; }

    private:
        friend struct detail::GuiAccess;
        friend struct detail::TextFontGuard;
        using DirtyRect = detail::DirtyRect;
        using ClipState = detail::ClipState;
        using ScreenshotEntry = detail::ScreenshotEntry;
        static constexpr uint8_t DIRTY_RECT_MAX = detail::DIRTY_RECT_MAX;
        detail::DisplayState _disp;
        detail::RenderState _render;
        detail::ClipState _clip;
        detail::DirtyState _dirty;
        detail::ScreenState _screen;
        detail::BootState _boot;
        detail::TypographyState _typo;
        detail::ErrorState _error;
        detail::NotificationState _notif;
        detail::ToastState _toast;
        detail::PopupMenuState _popup;
        detail::StatusBarState _status;
        detail::BlurState _blur;
        detail::Flags _flags = {};
        detail::DiagnosticsState _diag;
        detail::ButtonCacheState _buttonCache;
        detail::SliderCacheState _sliderCache;
        detail::TextCacheState _textCache;
        detail::ToggleCacheState _toggleCache;
        detail::DrumRollCacheState _drumRollCache;
        detail::ScreenshotGalleryState _shots;
        detail::ScreenshotStreamState _shotStream;

        uint32_t nowMs() const;

        void initFonts();
        void applyClip(int16_t x, int16_t y, int16_t w, int16_t h);
        void startLogo(const String &t, const String &s,
                       BootAnimation a = None);
        void configDisplay(const pipcore::DisplayConfig &cfg);
        void configureBacklight(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12);
        void setBackgroundColorCache(uint16_t color565) noexcept;
        void resetDisplayRuntime() noexcept;
        void clearReportedPlatformError();
        void reportPlatformErrorOnce(const char *stage);
        void handleScreenshotShortcut(bool comboDown);
        [[nodiscard]] bool statusBarAnimationActive() const noexcept;
        void configureStatusBar(bool enabled, uint32_t bgColor, uint8_t height, StatusBarPosition pos);
        void setStatusBarText(const String &left, const String &center, const String &right);
        void setStatusBarIcon(TextAlign side, IconId iconId, int32_t color, uint16_t sizePx);
        void freeScreenshotGallery(pipcore::Platform *plat) noexcept;
        void releaseScreenshotGalleryCache(pipcore::Platform *plat) noexcept;
        void freeScreenshotStream(pipcore::Platform *plat) noexcept;
        void resetScreenshotStreamState(pipcore::Platform *plat) noexcept;
        bool ensureScreenshotGallery(pipcore::Platform *plat);
        void captureScreenshotToGallery();
        void insertShotToGalleryFrom565(const uint16_t *src565be, uint16_t w, uint16_t h, uint32_t stamp, const char *path);
        void renderScreenshotGallery(int16_t x, int16_t y, int16_t w, int16_t h,
                                     uint8_t cols, uint8_t rows, uint16_t padding);
        void serviceScreenshotGalleryFlash();
        void startScreenshotStream();
        void serviceScreenshotStream();
        bool presentSprite(int16_t x, int16_t y, int16_t w, int16_t h, const char *stage);
        bool presentSpriteRegion(int16_t dstX, int16_t dstY,
                                 int16_t srcX, int16_t srcY,
                                 int16_t w, int16_t h,
                                 const char *stage);

        pipcore::Sprite *getDrawTarget();
        detail::ButtonState &resolveButtonState(const String &label, int16_t x, int16_t y,
                                               int16_t w, int16_t h, uint16_t baseColor, uint8_t radius,
                                               IconId iconId);
        detail::SliderState &resolveSliderState(int16_t x, int16_t y, int16_t w, int16_t h,
                                                int16_t minValue, int16_t maxValue, int16_t step,
                                                uint16_t activeColor, int32_t inactiveColor, int32_t thumbColor);
        void stepButtonState(detail::ButtonState &s, bool isDown);
        bool stepSliderState(detail::SliderState &state, int16_t &value, bool nextDown, bool prevDown);
        detail::ToggleState &resolveToggleState(int16_t x, int16_t y, int16_t w, int16_t h,
                                               uint16_t activeColor, int32_t inactiveColor, int32_t knobColor);
        detail::DrumRollAnimState &resolveDrumRollState(uint32_t key, uint8_t selectedIndex, uint16_t durationMs);
        bool stepToggleState(detail::ToggleState &state, bool &value, bool pressed);
        void flushDirty();
        void invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h);
        void drawProgressTextSpan(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const String &text, uint16_t textColor565, uint16_t bgColor565,
                                  TextAlign align, uint16_t fontPx,
                                  int16_t clipX, int16_t clipW);
        void drawProgressAttachedText(int16_t x, int16_t y, int16_t w, int16_t h,
                                      int16_t fillLeft, int16_t fillRight,
                                      uint16_t baseColor, uint16_t fillColor,
                                      const String &text, uint16_t textColor565,
                                      TextAlign align, uint16_t fontPx);
        void drawProgressDecorated(int16_t x, int16_t y, int16_t w, int16_t h,
                                   uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius,
                                   ProgressAnim anim,
                                   const String *label, uint16_t labelColor, TextAlign labelAlign, uint16_t labelFontPx,
                                   bool showPercent, uint16_t percentColor, TextAlign percentAlign, uint16_t percentFontPx);
        void updateProgressDecorated(int16_t x, int16_t y, int16_t w, int16_t h,
                                     uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius,
                                     ProgressAnim anim,
                                     const String *label, uint16_t labelColor, TextAlign labelAlign, uint16_t labelFontPx,
                                     bool showPercent, uint16_t percentColor, TextAlign percentAlign, uint16_t percentFontPx);

        template <typename Fn>
        void renderToSpriteAndInvalidate(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall);

        void ensureScreenState(uint8_t id);
        void syncRegisteredScreens();
        void setScreenId(uint8_t id);
        void activateScreenId(uint8_t id, int8_t transDir);
        void pushScreenHistory(uint8_t screenId);
        void renderScreenToMainSprite(ScreenCallback cb, uint8_t screenId = INVALID_SCREEN_ID);
        void freeScreenState(pipcore::Platform *plat) noexcept;
        void renderScreenTransition(uint32_t now);
        void renderBootFrame(uint32_t now);

        void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, uint16_t color);
        void drawLineCore(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                          uint8_t thickness, uint16_t color,
                          bool roundStart, bool roundEnd, bool invalidate);
        void drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
        void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
        void drawArc(int16_t cx, int16_t cy, int16_t r, uint8_t thickness, float startDeg, float endDeg, uint16_t color);
        void drawArcShaded(int16_t cx, int16_t cy, int16_t r, uint8_t thickness,
                           float startDeg, float endDeg,
                           uint16_t (*shader)(const void *, float),
                           const void *shaderCtx,
                           bool needsColorAngle,
                           bool invalidate);
        void drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);
        void fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);
        void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
        void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
        void drawSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color);
        void drawSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h,
                              uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                              uint16_t color);
        void fillSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color);
        void fillSquircleRect(int16_t x, int16_t y, int16_t w, int16_t h,
                              uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                              uint16_t color);
        void drawRoundTriangle(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               uint8_t radius, uint16_t color);
        void fillRoundTriangle(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               uint8_t radius, uint16_t color);
        void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color565);
        void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                           uint16_t color565);
        void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color565);
        void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                           uint16_t color565);
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        void fillRectGradientVertical(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint32_t topColor, uint32_t bottomColor);
        void fillRectGradientHorizontal(int16_t x, int16_t y, int16_t w, int16_t h,
                                        uint32_t leftColor, uint32_t rightColor);
        void fillRectGradientCorners(int16_t x, int16_t y, int16_t w, int16_t h,
                                     uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11);
        void fillRectGradientDiagonal(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint32_t tlColor, uint32_t brColor);
        void fillRectGradientRadial(int16_t x, int16_t y, int16_t w, int16_t h,
                                    int16_t cx, int16_t cy, int16_t radius,
                                    uint32_t innerColor, uint32_t outerColor);

        int16_t AutoX(int32_t contentWidth) const;
        int16_t AutoY(int32_t contentHeight) const;
        bool measureText(const String &text, int16_t &outW, int16_t &outH) const;
        void drawTextAligned(const String &text,
                             int16_t x, int16_t y,
                             uint16_t fg565, uint16_t bg565,
                             TextAlign align);
        void drawTextImmediate(const String &text,
                               int16_t rx, int16_t ry,
                               int16_t tw, int16_t th,
                               uint16_t fg565, uint16_t bg565,
                               TextAlign align);
        void drawTextImmediateMasked(const String &text,
                                     int16_t rx, int16_t ry,
                                     int16_t tw, int16_t th,
                                     uint16_t fg565, uint16_t bg565,
                                     TextAlign align,
                                     int16_t fadeBoxX, int16_t fadeBoxW,
                                     uint8_t fadePx);
        void drawIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565);
        void updateIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565, uint16_t bg565);
        void drawAnimatedIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565, uint32_t nowMs);
        void updateAnimatedIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565, uint16_t bg565, uint32_t nowMs);
        void drawBootTitleBlock(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565);
        void drawText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = TextAlign::Left);
        void updateText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = TextAlign::Left);
        bool drawTextMarquee(const String &text,
                             int16_t x, int16_t y,
                             int16_t maxWidth,
                             uint16_t fg565,
                             TextAlign align = TextAlign::Left,
                             const MarqueeTextOptions &opts = MarqueeTextOptions());
        bool drawTextEllipsized(const String &text,
                                int16_t x, int16_t y,
                                int16_t maxWidth,
                                uint16_t fg565,
                                TextAlign align = TextAlign::Left);

        bool ensureBlurWorkBuffers(uint32_t smallLen, int16_t sw, int16_t sh, int16_t w, int16_t h) noexcept;
        void freeBlurBuffers(pipcore::Platform *plat) noexcept;
        void drawBlurRegion(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t radius, BlurDirection dir,
                            bool gradient, uint8_t materialStrength,
                            int32_t materialColor);
        void updateBlurRegion(int16_t x, int16_t y, int16_t w, int16_t h,
                              uint8_t radius, BlurDirection dir,
                              bool gradient, uint8_t materialStrength,
                              int32_t materialColor);

        void drawScrollDotsImpl(int16_t x, int16_t y,
                                uint8_t count, uint8_t activeIndex,
                                uint16_t activeColor, uint16_t inactiveColor,
                                uint8_t radius, uint8_t spacing);

        void updateScrollDotsImpl(int16_t x, int16_t y,
                                  uint8_t count, uint8_t activeIndex,
                                  uint16_t activeColor, uint16_t inactiveColor,
                                  uint8_t radius, uint8_t spacing);

        void drawGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radius, GraphDirection dir, uint32_t bgColor,
                           float speed = 1.0f);
        void updateGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, GraphDirection dir, uint32_t bgColor,
                             float speed = 1.0f);
        void drawGraphLine(uint8_t lineIndex, int16_t value, uint32_t color, int16_t valueMin, int16_t valueMax, uint8_t thickness = 1);
        void updateGraphLine(uint8_t lineIndex, int16_t value, uint32_t color, int16_t valueMin, int16_t valueMax, uint8_t thickness = 1);
        void drawGraphSamples(uint8_t lineIndex, const int16_t *samples, uint16_t sampleCount,
                              uint32_t color, int16_t valueMin, int16_t valueMax, uint8_t thickness = 1);
        void updateGraphSamples(uint8_t lineIndex, const int16_t *samples, uint16_t sampleCount,
                                uint32_t color, int16_t valueMin, int16_t valueMax, uint8_t thickness = 1);
        void configGraphScope(uint8_t screenId,
                              uint16_t sampleRateHz,
                              uint16_t timebaseMs,
                              uint16_t visibleSamples = 0);

        struct ProgressState
        {
            bool inited = false;
            uint8_t value = 0;
            ProgressAnim anim = None;
        };

        void drawProgress(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
                          ProgressAnim anim = None);
        void drawProgress(int16_t x, int16_t y, int16_t w, int16_t h,
                          uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None);
        void updateProgress(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
                            ProgressAnim anim = None);
        void updateProgress(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None);
        void updateProgress(ProgressState &s, int16_t x, int16_t y, int16_t w,
                            int16_t h, uint8_t value, uint16_t baseColor, uint16_t fillColor,
                            uint8_t radius = 6, ProgressAnim anim = None);
        void drawLinearProgressRange(int16_t x, int16_t y, int16_t w, int16_t h,
                                     int16_t fillLeft, int16_t fillRight,
                                     uint16_t baseColor, uint16_t fillColor,
                                     uint8_t radius);

        void drawCircleProgress(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None);
        void drawCircleProgress(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                uint8_t value, uint16_t color, ProgressAnim anim = None);
        void updateCircleProgress(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                  uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None);
        void updateCircleProgress(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                  uint8_t value, uint16_t color, ProgressAnim anim = None);

        void drawButton(const String &label, int16_t x, int16_t y, int16_t w,
                        int16_t h, uint16_t baseColor, uint8_t radius,
                        IconId iconId, detail::ButtonState &state);
        void updateButton(const String &label, int16_t x, int16_t y, int16_t w,
                          int16_t h, uint16_t baseColor, uint8_t radius,
                          IconId iconId, detail::ButtonState &state);
        void drawSlider(int16_t x, int16_t y, int16_t w, int16_t h,
                        detail::SliderState &state,
                        uint16_t activeColor,
                        int32_t inactiveColor = -1,
                        int32_t thumbColor = -1);
        void updateSlider(int16_t x, int16_t y, int16_t w, int16_t h,
                          detail::SliderState &state,
                          uint16_t activeColor,
                          int32_t inactiveColor = -1,
                          int32_t thumbColor = -1);
        void drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                              detail::ToggleState &state, uint16_t activeColor,
                              int32_t inactiveColor = -1, int32_t knobColor = -1);
        void updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                detail::ToggleState &state, uint16_t activeColor,
                                int32_t inactiveColor = -1, int32_t knobColor = -1);

        void drawGlowCircle(int16_t x, int16_t y,
                            int16_t r, uint16_t fillColor,
                            int16_t bgColor, int16_t glowColor,
                            uint8_t glowSize, uint8_t glowStrength,
                            GlowAnim anim, uint16_t pulsePeriodMs);
        void updateGlowCircle(int16_t x, int16_t y,
                              int16_t r, uint16_t fillColor,
                              int16_t bgColor, int16_t glowColor,
                              uint8_t glowSize, uint8_t glowStrength,
                              GlowAnim anim, uint16_t pulsePeriodMs);

        GraphArea *ensureGraphArea(uint8_t screenId);
        void beginGraphFrame(uint8_t screenId) noexcept;
        void endGraphFrame(uint8_t screenId) noexcept;
        void releaseGraphBuffers(uint8_t screenId) noexcept;
        void flushPendingGraphRender(uint8_t screenId) noexcept;
        ListState *ensureList(uint8_t screenId);
        TileState *ensureTile(uint8_t screenId);
        ListState *getList(uint8_t screenId);
        TileState *getTile(uint8_t screenId);

        void handleListInput(uint8_t screenId, bool nextDown, bool prevDown);
        bool renderListState(ListState &menu,
                             int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             uint16_t bgColor565,
                             bool invalidate);
        bool updateList(uint8_t screenId,
                        int16_t x, int16_t y,
                        int16_t w, int16_t h,
                        uint16_t bgColor565);
        void updateTile(uint8_t screenId, uint8_t prevSelectedIndex);
        void handleTileInput(uint8_t screenId, bool nextDown, bool prevDown);
        void configureTile(uint8_t screenId,
                           const TileItemDef *items,
                           uint8_t itemCount,
                           const TileStyle &style);

        void freeGraphAreas(pipcore::Platform *plat) noexcept;
        void freeLists(pipcore::Platform *plat) noexcept;
        void freeTiles(pipcore::Platform *plat) noexcept;
        void freePopupMenu(pipcore::Platform *plat) noexcept;

        void renderToastOverlay(uint32_t now);
        bool computeToastBounds(uint32_t now, DirtyRect &outRect);
        bool computePopupBounds(uint32_t now, DirtyRect &outRect);
        void renderNotificationOverlay();
        void renderPopupMenuOverlay(uint32_t now);
        void showPopupMenuInternal(PopupMenuItemFn itemFn,
                                   void *itemUser,
                                   uint8_t count,
                                   uint8_t selectedIndex,
                                   int16_t w,
                                   uint8_t maxVisible,
                                   int16_t anchorX,
                                   int16_t anchorY,
                                   int16_t anchorW,
                                   int16_t anchorH);
        void handlePopupMenuInput(bool nextDown, bool prevDown);
        void showToastInternal(const String &text,
                               bool fromTop,
                               IconId iconId);
        void showNotificationInternal(const String &title,
                                      const String &message,
                                      const String &buttonText,
                                      uint16_t delaySeconds,
                                      NotificationType type,
                                      IconId iconId);

        void renderErrorFrame(uint32_t now);
        void startError(const String &message,
                        const String &code,
                        ErrorType type = ErrorTypeWarning,
                        const String &buttonText = "OK");
        bool ensureErrorCapacity(uint8_t need);
        void freeErrors(pipcore::Platform *plat) noexcept;
    };

    using InputState = GUI::InputState;
}

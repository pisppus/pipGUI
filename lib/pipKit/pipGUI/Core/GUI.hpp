#pragma once

#include <pipCore/Display.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <pipGUI/Core/Types.hpp>
#include <pipGUI/Core/Internal/GuiState.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Systems/Update/Ota.hpp>

#if (PIPGUI_SCREENSHOT_MODE == 2)
#include <FS.h>
#include <LittleFS.h>
#endif

namespace pipgui
{
    struct ConfigureDisplayFluent;
    struct ConfigureBacklightFluent;
    struct SetClipFluent;
    struct ShowLogoFluent;

    struct FillRectFluent;
    struct DrawRectFluent;
    struct GradientVerticalFluent;
    struct GradientHorizontalFluent;
    struct GradientCornersFluent;
    struct GradientDiagonalFluent;
    struct GradientRadialFluent;
    struct DrawLineFluent;
    struct DrawCircleFluent;
    struct FillCircleFluent;
    struct DrawArcFluent;
    struct DrawEllipseFluent;
    struct FillEllipseFluent;
    struct DrawTriangleFluent;
    struct FillTriangleFluent;
    struct DrawSquircleFluent;
    struct FillSquircleFluent;

    template <bool IsUpdate>
    struct BlurRegionFluentT;
    using DrawBlurFluent = BlurRegionFluentT<false>;
    using UpdateBlurFluent = BlurRegionFluentT<true>;

    template <bool IsUpdate>
    struct GlowCircleFluentT;
    using DrawGlowCircleFluent = GlowCircleFluentT<false>;
    using UpdateGlowCircleFluent = GlowCircleFluentT<true>;

    template <bool IsUpdate>
    struct GlowRectFluentT;
    using DrawGlowRectFluent = GlowRectFluentT<false>;
    using UpdateGlowRectFluent = GlowRectFluentT<true>;

    template <bool IsUpdate>
    struct ToggleSwitchFluentT;
    using DrawToggleSwitchFluent = ToggleSwitchFluentT<false>;
    using UpdateToggleSwitchFluent = ToggleSwitchFluentT<true>;

    template <bool IsUpdate>
    struct ButtonFluentT;
    using DrawButtonFluent = ButtonFluentT<false>;
    using UpdateButtonFluent = ButtonFluentT<true>;

    template <bool IsUpdate>
    struct ProgressBarFluentT;
    using DrawProgressBarFluent = ProgressBarFluentT<false>;
    using UpdateProgressBarFluent = ProgressBarFluentT<true>;

    template <bool IsUpdate>
    struct CircularProgressBarFluentT;
    using DrawCircularProgressBarFluent = CircularProgressBarFluentT<false>;
    using UpdateCircularProgressBarFluent = CircularProgressBarFluentT<true>;

    template <bool IsUpdate>
    struct ScrollDotsFluentT;
    using DrawScrollDotsFluent = ScrollDotsFluentT<false>;
    using UpdateScrollDotsFluent = ScrollDotsFluentT<true>;

    struct ToastFluent;
    struct NotificationFluent;
    struct PopupMenuFluent;
    struct DrawIconFluent;
    struct DrawScreenshotFluent;

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

    enum class ScreenshotFormat : uint8_t
    {
        QoiRgb = 3,
    };

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

        [[nodiscard]] ConfigureDisplayFluent configureDisplay();
        [[nodiscard]] ConfigureBacklightFluent setBacklight();
        [[nodiscard]] SetClipFluent setClip();
        [[nodiscard]] ShowLogoFluent showLogo();
        void configureDisplay(const pipcore::DisplayConfig &cfg);
        void configureBacklight(uint8_t pin, uint8_t channel = 0, uint32_t freqHz = 5000, uint8_t resolutionBits = 12);
        void begin(uint8_t rotation = 0, uint16_t bgColor = 0x0000);

        void setBacklightCallback(BacklightCallback cb) noexcept;
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
        void setScreenshotShortcut(Button *next, Button *prev, uint16_t holdMs = 500);
        InputState pollInput(Button &next, Button &prev);

        // WiFi
        void requestWiFi(bool enabled) noexcept;
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

        [[nodiscard]] FillRectFluent fillRect();
        [[nodiscard]] DrawRectFluent drawRect();
        [[nodiscard]] GradientVerticalFluent gradientVertical();
        [[nodiscard]] GradientHorizontalFluent gradientHorizontal();
        [[nodiscard]] GradientCornersFluent gradientCorners();
        [[nodiscard]] GradientDiagonalFluent gradientDiagonal();
        [[nodiscard]] GradientRadialFluent gradientRadial();

        [[nodiscard]] DrawLineFluent drawLine();
        [[nodiscard]] DrawCircleFluent drawCircle();
        [[nodiscard]] FillCircleFluent fillCircle();
        [[nodiscard]] DrawArcFluent drawArc();
        [[nodiscard]] DrawEllipseFluent drawEllipse();
        [[nodiscard]] FillEllipseFluent fillEllipse();
        [[nodiscard]] DrawTriangleFluent drawTriangle();
        [[nodiscard]] FillTriangleFluent fillTriangle();
        [[nodiscard]] DrawSquircleFluent drawSquircle();
        [[nodiscard]] FillSquircleFluent fillSquircle();

        [[nodiscard]] DrawBlurFluent drawBlur();
        [[nodiscard]] UpdateBlurFluent updateBlur();

        [[nodiscard]] DrawGlowCircleFluent drawGlowCircle();
        [[nodiscard]] UpdateGlowCircleFluent updateGlowCircle();
        [[nodiscard]] DrawGlowRectFluent drawGlowRect();
        [[nodiscard]] UpdateGlowRectFluent updateGlowRect();

        [[nodiscard]] DrawScrollDotsFluent drawScrollDots();
        [[nodiscard]] UpdateScrollDotsFluent updateScrollDots();

        [[nodiscard]] DrawToggleSwitchFluent drawToggleSwitch();
        [[nodiscard]] UpdateToggleSwitchFluent updateToggleSwitch();

        [[nodiscard]] DrawButtonFluent drawButton();
        [[nodiscard]] UpdateButtonFluent updateButton();

        [[nodiscard]] DrawProgressBarFluent drawProgressBar();
        [[nodiscard]] UpdateProgressBarFluent updateProgressBar();

        [[nodiscard]] DrawCircularProgressBarFluent drawCircularProgressBar();
        [[nodiscard]] UpdateCircularProgressBarFluent updateCircularProgressBar();

        [[nodiscard]] ToastFluent showToast();
        [[nodiscard]] NotificationFluent showNotification();
        [[nodiscard]] PopupMenuFluent showPopupMenu();
        [[nodiscard]] PopupMenuInputFluent popupMenuInput();
        [[nodiscard]] bool popupMenuActive() const noexcept { return _flags.popupActive; }
        [[nodiscard]] bool popupMenuHasResult() const noexcept { return _popup.resultReady; }
        [[nodiscard]] int16_t popupMenuTakeResult() noexcept
        {
            if (!_popup.resultReady)
                return -1;
            _popup.resultReady = false;
            return _popup.resultIndex;
        }

        void drawGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radius, GraphDirection dir, uint32_t bgColor,
                           uint32_t gridColor, float speed = 1.0f);

        void drawGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t radius, GraphDirection dir, uint16_t bgColor565,
                           uint16_t gridColor565, float speed = 1.0f)
        {
            drawGraphGrid(x, y, w, h, radius, dir, detail::color565To888(bgColor565), detail::color565To888(gridColor565), speed);
        }

        void updateGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, GraphDirection dir, uint32_t bgColor, uint32_t gridColor,
                             float speed = 1.0f);

        void updateGraphGrid(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t radius, GraphDirection dir, uint16_t bgColor565, uint16_t gridColor565,
                             float speed = 1.0f)
        {
            updateGraphGrid(x, y, w, h, radius, dir, detail::color565To888(bgColor565), detail::color565To888(gridColor565), speed);
        }

        void setGraphAutoScale(bool enabled);
        void drawGraphLine(uint8_t lineIndex, int16_t value, uint32_t color, int16_t valueMin, int16_t valueMax);
        void updateGraphLine(uint8_t lineIndex, int16_t value, uint32_t color, int16_t valueMin, int16_t valueMax);

        void drawGraphLine(uint8_t lineIndex, int16_t value, uint16_t color565, int16_t valueMin, int16_t valueMax)
        {
            drawGraphLine(lineIndex, value, detail::color565To888(color565), valueMin, valueMax);
        }

        void updateGraphLine(uint8_t lineIndex, int16_t value, uint16_t color565, int16_t valueMin, int16_t valueMax)
        {
            updateGraphLine(lineIndex, value, detail::color565To888(color565), valueMin, valueMax);
        }

        void drawProgressText(int16_t x, int16_t y, int16_t w, int16_t h,
                              const String &text, TextAlign align = AlignCenter,
                              uint32_t textColor = 0xFFFFFF, uint32_t bgColor = 0x000000,
                              uint16_t fontPx = 0);

        void drawProgressText(int16_t x, int16_t y, int16_t w, int16_t h,
                              const String &text, uint16_t textColor565, uint16_t bgColor565,
                              TextAlign align = AlignCenter, uint16_t fontPx = 0)
        {
            drawProgressText(x, y, w, h, text, align, detail::color565To888(textColor565), detail::color565To888(bgColor565), fontPx);
        }

        void drawProgressPercent(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t value, TextAlign align = AlignCenter,
                                 uint32_t textColor = 0xFFFFFF, uint32_t bgColor = 0x000000,
                                 uint16_t fontPx = 0);

        void drawProgressPercent(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint8_t value, uint16_t textColor565, uint16_t bgColor565,
                                 TextAlign align = AlignCenter, uint16_t fontPx = 0)
        {
            drawProgressPercent(x, y, w, h, value, align, detail::color565To888(textColor565), detail::color565To888(bgColor565), fontPx);
        }

        void updateButtonPress(ButtonVisualState &s, bool isDown);
        bool updateToggleSwitch(ToggleSwitchState &s, bool pressed);

        [[nodiscard]] DrawIconFluent drawIcon();
        [[nodiscard]] DrawTextFluent drawText();
        [[nodiscard]] UpdateTextFluent updateText();
        [[nodiscard]] DrawTextMarqueeFluent drawTextMarquee();
        [[nodiscard]] DrawTextEllipsizedFluent drawTextEllipsized();

        void drawDrumRollHorizontal(int16_t x, int16_t y, int16_t w, int16_t h,
                                    const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
                                    float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

        void drawDrumRollVertical(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
                                  float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

        FontId registerFont(const uint8_t *atlasData,
                            uint16_t atlasWidth, uint16_t atlasHeight,
                            float distanceRange, float nominalSizePx,
                            float ascender, float descender, float lineHeight,
                            const void *glyphs, uint16_t glyphCount);

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
        void applyClipRect(int16_t x, int16_t y, int16_t w, int16_t h);
        void clearClip();

        void startLogo(const String &t, const String &s,
                       BootAnimation a = None, uint32_t fg = 0xFFFFFF,
                       uint32_t bg = 0x000000, uint32_t dur = 0,
                       int16_t x = -1, int16_t y = -1);

        void logoTitleSizePx(uint16_t sizePx) noexcept { _typo.logoTitleSizePx = sizePx; }
        void logoSubtitleSizePx(uint16_t sizePx) noexcept { _typo.logoSubtitleSizePx = sizePx; }
        void logoSizesPx(uint16_t titleSizePx, uint16_t subtitleSizePx) noexcept
        {
            _typo.logoTitleSizePx = titleSizePx;
            _typo.logoSubtitleSizePx = subtitleSizePx;
        }

        void setNotificationButtonDown(bool down);
        [[nodiscard]] bool notificationActive() const noexcept;
        [[nodiscard]] bool toastActive() const;

        void showError(const String &title, const String &message,
                       ErrorType type = Warning, const String &buttonText = "OK");
        void nextError();
        [[nodiscard]] bool errorActive() const noexcept;
        void setErrorButtonDown(bool down);

        void configureStatusBar(bool enabled, uint32_t bgColor = 0x000000,
                                uint8_t height = 0, StatusBarPosition pos = Top);

        void setStatusBarStyle(StatusBarStyle style) noexcept
        {
            _status.style = style;
            _status.dirtyMask = detail::StatusBarDirtyAll;
        }

        void setStatusBarText(const String &left, const String &center, const String &right);
        void setStatusBarBattery(int8_t levelPercent, BatteryStyle style);
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
        detail::ScreenshotGalleryState _shots;
        detail::ScreenshotStreamState _shotStream;

        uint32_t nowMs() const;

        void initFonts();
        void setBackgroundColorCache(uint16_t color565) noexcept;
        void resetDisplayRuntime() noexcept;
        void clearReportedPlatformError();
        void reportPlatformErrorOnce(const char *stage);
        void handleScreenshotShortcut(bool nextDown, bool prevDown);
        void freeScreenshotGallery(pipcore::Platform *plat) noexcept;
        void freeScreenshotStream(pipcore::Platform *plat) noexcept;
        void resetScreenshotStreamState(pipcore::Platform *plat, bool keepBuffer) noexcept;
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
        void flushDirty();
        void invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h);

        template <typename Fn>
        void renderToSpriteAndInvalidate(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall);

        void ensureScreenState(uint8_t id);
        void syncRegisteredScreens();
        void setScreenId(uint8_t id);
        void activateScreenId(uint8_t id, int8_t transDir);
        void renderScreenToMainSprite(ScreenCallback cb, uint8_t screenId = INVALID_SCREEN_ID);
        void freeScreenState(pipcore::Platform *plat) noexcept;
        void renderScreenTransition(uint32_t now);
        void renderBootFrame(uint32_t now);

        void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
        void drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
        void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
        void drawArc(int16_t cx, int16_t cy, int16_t r, float startDeg, float endDeg, uint16_t color);
        void drawEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);
        void fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);
        void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
        void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
        void drawRoundTriangle(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               uint8_t radius, uint16_t color);
        void fillRoundTriangle(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               uint8_t radius, uint16_t color);
        void drawSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
        void fillSquircle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
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
        void drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565);
        void drawText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = Left);
        void updateText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = Left);
        bool drawTextMarquee(const String &text,
                             int16_t x, int16_t y,
                             int16_t maxWidth,
                             uint16_t fg565,
                             TextAlign align = Left,
                             const MarqueeTextOptions &opts = MarqueeTextOptions());
        bool drawTextEllipsized(const String &text,
                                int16_t x, int16_t y,
                                int16_t maxWidth,
                                uint16_t fg565,
                                TextAlign align = Left);

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
                                uint8_t prevIndex, float animProgress,
                                bool animate, int8_t animDirection,
                                uint16_t activeColor, uint16_t inactiveColor,
                                uint8_t dotRadius, uint8_t spacing,
                                uint8_t activeWidth);

        void updateScrollDotsImpl(int16_t x, int16_t y,
                                  uint8_t count, uint8_t activeIndex,
                                  uint8_t prevIndex, float animProgress,
                                  bool animate, int8_t animDirection,
                                  uint16_t activeColor, uint16_t inactiveColor,
                                  uint8_t dotRadius, uint8_t spacing,
                                  uint8_t activeWidth);

        struct ProgressBarState
        {
            bool inited;
            uint8_t value;
            ProgressAnim anim;
            int16_t segL;
            int16_t segR;
        };

        void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
                             ProgressAnim anim = None);
        void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None);
        void updateProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
                               ProgressAnim anim = None, bool doFlush = true);
        void updateProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                               uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None,
                               bool doFlush = true);
        void updateProgressBar(ProgressBarState &s, int16_t x, int16_t y, int16_t w,
                               int16_t h, uint8_t value, uint16_t baseColor, uint16_t fillColor,
                               uint8_t radius = 6, ProgressAnim anim = None);

        void drawCircularProgressBar(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                     uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None);
        void drawCircularProgressBar(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                     uint8_t value, uint16_t color, ProgressAnim anim = None);
        void updateCircularProgressBar(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                       uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None,
                                       bool doFlush = true);
        void updateCircularProgressBar(int16_t x, int16_t y, int16_t r, uint8_t thickness,
                                       uint8_t value, uint16_t color, ProgressAnim anim = None,
                                       bool doFlush = true);

        void drawButton(const String &label, int16_t x, int16_t y, int16_t w,
                        int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);
        void updateButton(const String &label, int16_t x, int16_t y, int16_t w,
                          int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);
        void drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                              ToggleSwitchState &state, uint16_t activeColor,
                              int32_t inactiveColor = -1, int32_t knobColor = -1);
        void updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                ToggleSwitchState &state, uint16_t activeColor,
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
        void drawGlowRect(int16_t x, int16_t y,
                          int16_t w, int16_t h,
                          uint8_t radius, uint16_t fillColor,
                          int16_t bgColor, int16_t glowColor,
                          uint8_t glowSize, uint8_t glowStrength,
                          GlowAnim anim, uint16_t pulsePeriodMs);
        void updateGlowRect(int16_t x, int16_t y,
                            int16_t w, int16_t h,
                            uint8_t radius, uint16_t fillColor,
                            int16_t bgColor, int16_t glowColor,
                            uint8_t glowSize, uint8_t glowStrength,
                            GlowAnim anim, uint16_t pulsePeriodMs);
        void drawGlowRect(int16_t x, int16_t y,
                          int16_t w, int16_t h,
                          uint8_t radiusTL, uint8_t radiusTR,
                          uint8_t radiusBR, uint8_t radiusBL,
                          uint16_t fillColor, int16_t bgColor,
                          int16_t glowColor, uint8_t glowSize,
                          uint8_t glowStrength, GlowAnim anim,
                          uint16_t pulsePeriodMs);
        void updateGlowRect(int16_t x, int16_t y,
                            int16_t w, int16_t h,
                            uint8_t radiusTL, uint8_t radiusTR,
                            uint8_t radiusBR, uint8_t radiusBL,
                            uint16_t fillColor, int16_t bgColor,
                            int16_t glowColor, uint8_t glowSize,
                            uint8_t glowStrength, GlowAnim anim,
                            uint16_t pulsePeriodMs);

        GraphArea *ensureGraphArea(uint8_t screenId);
        ListState *ensureList(uint8_t screenId);
        TileState *ensureTile(uint8_t screenId);
        ListState *getList(uint8_t screenId);
        TileState *getTile(uint8_t screenId);

        void handleListInput(uint8_t screenId, bool nextDown, bool prevDown);
        bool updateList(uint8_t screenId,
                        int16_t x, int16_t y,
                        int16_t w, int16_t h,
                        uint16_t bgColor565);
        void updateTile(uint8_t screenId, uint8_t prevSelectedIndex);
        void handleTileInput(uint8_t screenId, bool nextDown, bool prevDown);
        void configureTile(uint8_t screenId,
                           uint8_t parentScreen,
                           const TileItemDef *items,
                           uint8_t itemCount,
                           const TileStyle &style);

        void freeGraphAreas(pipcore::Platform *plat) noexcept;
        void freeLists(pipcore::Platform *plat) noexcept;
        void freeTiles(pipcore::Platform *plat) noexcept;

        void renderToastOverlay(uint32_t now);
        bool computeToastBounds(uint32_t now, DirtyRect &outRect);
        void renderNotificationOverlay();
        void renderPopupMenuOverlay(uint32_t now);
        void showPopupMenuInternal(PopupMenuItemFn itemFn,
                                   void *itemUser,
                                   uint8_t count,
                                   uint8_t selectedIndex,
                                   int16_t x,
                                   int16_t y,
                                   int16_t w,
                                   uint8_t maxVisible);
        void handlePopupMenuInput(bool nextDown, bool prevDown);
        void showToastInternal(const String &text,
                               uint32_t durationMs,
                               bool fromTop,
                               IconId iconId,
                               uint16_t iconSizePx);
        void showNotificationInternal(const String &title,
                                      const String &message,
                                      const String &buttonText,
                                      uint16_t delaySeconds,
                                      NotificationType type,
                                      IconId iconId);

        void renderErrorFrame(uint32_t now);
        bool ensureErrorCapacity(uint8_t need);
        void freeErrors(pipcore::Platform *plat) noexcept;
        void drawErrorCard(int16_t x, int16_t y, int16_t w, int16_t h,
                           const String &title, const String &detail,
                           uint32_t accentColor);
    };
}

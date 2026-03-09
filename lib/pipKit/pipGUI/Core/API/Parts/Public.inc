#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

#include <pipGUI/Core/Utils/Colors.hpp>

DrawLineFluent drawLine() { return DrawLineFluent(this); }
DrawCircleFluent drawCircle() { return DrawCircleFluent(this); }
FillCircleFluent fillCircle() { return FillCircleFluent(this); }
DrawArcFluent drawArc() { return DrawArcFluent(this); }
DrawEllipseFluent drawEllipse() { return DrawEllipseFluent(this); }
FillEllipseFluent fillEllipse() { return FillEllipseFluent(this); }
DrawTriangleFluent drawTriangle() { return DrawTriangleFluent(this); }
FillTriangleFluent fillTriangle() { return FillTriangleFluent(this); }
DrawSquircleFluent drawSquircle() { return DrawSquircleFluent(this); }
FillSquircleFluent fillSquircle() { return FillSquircleFluent(this); }

DrawBlurFluent drawBlur() { return DrawBlurFluent(this); }
UpdateBlurFluent updateBlur() { return UpdateBlurFluent(this); }

DrawGlowCircleFluent drawGlowCircle() { return DrawGlowCircleFluent(this); }
UpdateGlowCircleFluent updateGlowCircle() { return UpdateGlowCircleFluent(this); }
DrawGlowRectFluent drawGlowRect() { return DrawGlowRectFluent(this); }
UpdateGlowRectFluent updateGlowRect() { return UpdateGlowRectFluent(this); }

DrawScrollDotsFluent drawScrollDots() { return DrawScrollDotsFluent(this); }
UpdateScrollDotsFluent updateScrollDots() { return UpdateScrollDotsFluent(this); }

DrawToggleSwitchFluent drawToggleSwitch() { return DrawToggleSwitchFluent(this); }
UpdateToggleSwitchFluent updateToggleSwitch() { return UpdateToggleSwitchFluent(this); }

DrawButtonFluent drawButton() { return DrawButtonFluent(this); }
UpdateButtonFluent updateButton() { return UpdateButtonFluent(this); }

DrawProgressBarFluent drawProgressBar() { return DrawProgressBarFluent(this); }
UpdateProgressBarFluent updateProgressBar() { return UpdateProgressBarFluent(this); }

DrawCircularProgressBarFluent drawCircularProgressBar() { return DrawCircularProgressBarFluent(this); }
UpdateCircularProgressBarFluent updateCircularProgressBar() { return UpdateCircularProgressBarFluent(this); }

ToastFluent showToast() { return ToastFluent(this); }
NotificationFluent showNotification() { return NotificationFluent(this); }

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

// Progress Helpers
void drawProgressText(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String &text, TextAlign align = AlignCenter,
    uint32_t textColor = 0xFFFFFF, uint32_t bgColor = 0x000000,
    uint16_t fontPx = 0);

void drawProgressText(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String &text, uint16_t textColor565, uint16_t bgColor565,
    TextAlign align = AlignCenter, uint16_t fontPx = 0)
{
    drawProgressText(x, y, w, h, text, align, detail::color565To888(textColor565), detail::color565To888(bgColor565), fontPx);
}

void drawProgressPercent(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, TextAlign align = AlignCenter,
    uint32_t textColor = 0xFFFFFF, uint32_t bgColor = 0x000000,
    uint16_t fontPx = 0);

void drawProgressPercent(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, uint16_t textColor565, uint16_t bgColor565,
    TextAlign align = AlignCenter, uint16_t fontPx = 0)
{
    drawProgressPercent(x, y, w, h, value, align, detail::color565To888(textColor565), detail::color565To888(bgColor565), fontPx);
}

void updateButtonPress(
    ButtonVisualState &s, bool isDown);

bool updateToggleSwitch(ToggleSwitchState &s, bool pressed);

DrawIconFluent drawIcon() { return DrawIconFluent(this); }
DrawTextFluent drawText() { return DrawTextFluent(this); }
UpdateTextFluent updateText() { return UpdateTextFluent(this); }
DrawTextMarqueeFluent drawTextMarquee() { return DrawTextMarqueeFluent(this); }
DrawTextEllipsizedFluent drawTextEllipsized() { return DrawTextEllipsizedFluent(this); }

// Other UI Elements
void drawDrumRollHorizontal(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
    float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

void drawDrumRollVertical(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
    float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

// Font Registration (for custom fonts)
FontId registerFont(const uint8_t *atlasData,
                    uint16_t atlasWidth, uint16_t atlasHeight,
                    float distanceRange, float nominalSizePx,
                    float ascender, float descender, float lineHeight,
                    const void *glyphs, uint16_t glyphCount);

bool setFont(FontId fontId);
void setFontSize(uint16_t px);
uint16_t fontSize() const;
void setFontWeight(uint16_t weight);
uint16_t fontWeight() const;

// Text
void configureTextStyles(
    uint16_t h1Px = 24, uint16_t h2Px = 18,
    uint16_t bodyPx = 14, uint16_t captionPx = 12);

void setTextStyle(TextStyle style);

// List Menu
ConfigureListFluent configureList() { return ConfigureListFluent(this); }

bool updateList(uint8_t screenId);

ListInputFluent listInput(uint8_t screenId) { return ListInputFluent(this, screenId); }

// Tile Menu
ConfigureTileFluent configureTile() { return ConfigureTileFluent(this); }

void renderTile(uint8_t screenId);

TileInputFluent tileInput(uint8_t screenId) { return TileInputFluent(this, screenId); }

// Screen Control
void setScreen(uint8_t id);
uint8_t currentScreen() const;
bool screenTransitionActive() const;
void nextScreen();
void prevScreen();
void loop();
void loopWithInput(Button &next, Button &prev);
void requestRedraw();
void setScreenAnimation(ScreenAnim anim, uint32_t durationMs);
void setClip(
    int16_t x, int16_t y,
    int16_t w, int16_t h);

void clearClip();

// Boot Logo
void showLogo(
    const String &t, const String &s,
    BootAnimation a = None, uint32_t fg = 0xFFFFFF,
    uint32_t bg = 0x000000, uint32_t dur = 0,
    int16_t x = -1, int16_t y = -1);

void logoTitleSizePx(uint16_t sizePx)
{
    _typo.logoTitleSizePx = sizePx;
}

void logoSubtitleSizePx(uint16_t sizePx)
{
    _typo.logoSubtitleSizePx = sizePx;
}

void logoSizesPx(uint16_t titleSizePx, uint16_t subtitleSizePx)
{
    _typo.logoTitleSizePx = titleSizePx;
    _typo.logoSubtitleSizePx = subtitleSizePx;
}

void setNotificationButtonDown(bool down);
bool notificationActive() const;

// Toasts
bool toastActive() const;

// Errors
void showError(
    const String &title, const String &message,
    ErrorType type = Warning, const String &buttonText = "OK");

void nextError();
bool errorActive() const;
void setErrorButtonDown(bool down);

// Status Bar
void configureStatusBar(
    bool enabled, uint32_t bgColor = 0x000000,
    uint8_t height = 0, StatusBarPosition pos = Top);

void setStatusBarStyle(StatusBarStyle style)
{
    _status.style = style;
    _status.dirtyMask = StatusBarDirtyAll;
}

void setStatusBarText(
    const String &left, const String &center,
    const String &right);

void setStatusBarBattery(int8_t levelPercent, BatteryStyle style);
void setStatusBarCustom(StatusBarCustomCallback cb);
int16_t statusBarHeight() const;
void updateStatusBar();
void renderStatusBar();

// Color Helpers
static constexpr uint32_t rgb888(
    uint8_t r, uint8_t g,
    uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// Screen Properties
uint16_t screenWidth() const { return _render.screenWidth; }
uint16_t screenHeight() const { return _render.screenHeight; }

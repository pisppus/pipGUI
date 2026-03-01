#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

#include <pipGUI/core/utils/Colors.hpp>

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

struct ProgressBarState
{
    bool inited;
    uint8_t value;
    ProgressAnim anim;
    int16_t segL;
    int16_t segR;
};

// Linear Progress Bar
void drawProgressBar(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
    ProgressAnim anim = None);

void drawProgressBar(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None);

void updateProgressBar(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, uint16_t baseColor, uint16_t fillColor, uint8_t radius = 6,
    ProgressAnim anim = None, bool doFlush = true);

void updateProgressBar(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t value, uint16_t color, uint8_t radius = 6, ProgressAnim anim = None,
    bool doFlush = true);

void updateProgressBar(
    ProgressBarState &s, int16_t x, int16_t y, int16_t w,
    int16_t h, uint8_t value, uint16_t baseColor, uint16_t fillColor,
    uint8_t radius = 6, ProgressAnim anim = None);

// Circular Progress Bar
void drawCircularProgressBar(
    int16_t x, int16_t y, int16_t r, uint8_t thickness,
    uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None);

void drawCircularProgressBar(
    int16_t x, int16_t y, int16_t r, uint8_t thickness,
    uint8_t value, uint16_t color, ProgressAnim anim = None);

void updateCircularProgressBar(
    int16_t x, int16_t y, int16_t r, uint8_t thickness,
    uint8_t value, uint16_t baseColor, uint16_t fillColor, ProgressAnim anim = None,
    bool doFlush = true);

void updateCircularProgressBar(
    int16_t x, int16_t y, int16_t r, uint8_t thickness,
    uint8_t value, uint16_t color, ProgressAnim anim = None,
    bool doFlush = true);

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

// Buttons
void drawButton(
    const String &label, int16_t x, int16_t y, int16_t w,
    int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);

void updateButton(
    const String &label, int16_t x, int16_t y, int16_t w,
    int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);

void updateButtonPress(
    ButtonVisualState &s, bool isDown);

// Toggle Switches
void drawToggleSwitch(
    int16_t x, int16_t y, int16_t w, int16_t h,
    ToggleSwitchState &state, uint16_t activeColor,
    int32_t inactiveColor = -1, int32_t knobColor = -1);

void updateToggleSwitch(
    int16_t x, int16_t y, int16_t w, int16_t h,
    ToggleSwitchState &state, uint16_t activeColor,
    int32_t inactiveColor = -1, int32_t knobColor = -1);

bool updateToggleSwitch(ToggleSwitchState &s, bool pressed);

DrawIconFluent drawIcon() { return DrawIconFluent(this); }
DrawTextFluent drawText() { return DrawTextFluent(this); }

// Other UI Elements
void drawDrumRollHorizontal(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
    float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

void drawDrumRollVertical(
    int16_t x, int16_t y, int16_t w, int16_t h,
    const String *options, uint8_t count, uint8_t selectedIndex, uint8_t prevIndex,
    float animProgress, uint32_t fgColor, uint32_t bgColor, uint16_t fontPx = 0);

bool loadBuiltinPSDF();
void enablePSDF(bool enabled);
bool psdfEnabled() const;
bool psdfFontLoaded() const;

// Font Registration (for custom fonts)
FontId registerFont(const uint8_t *atlasData,
                    uint16_t atlasWidth, uint16_t atlasHeight,
                    float distanceRange, float nominalSizePx,
                    float ascender, float descender, float lineHeight,
                    const void *glyphs, uint16_t glyphCount);
uint8_t getFontCount() const;

void setPSDFFont(FontId fontId);
FontId psdfFontId() const;
void setPSDFFontSize(uint16_t px);
uint16_t psdfFontSize() const;
void setPSDFWeight(uint16_t weight);
uint16_t psdfWeight() const;
void setPSDFWeightBias(float bias);
float psdfWeightBias() const;

// Text
void configureTextStyles(
    uint16_t h1Px = 24, uint16_t h2Px = 18,
    uint16_t bodyPx = 14, uint16_t captionPx = 12);

void setTextStyle(TextStyle style);

void drawPSDFText(
    const String &text, int16_t x,
    int16_t y, uint16_t fg565,
    uint16_t bg565, TextAlign align = Left);

void updatePSDFText(
    const String &text, int16_t x,
    int16_t y, uint16_t fg565,
    uint16_t bg565, TextAlign align = Left);

// List Menu
void configureListMenu(
    uint8_t screenId, uint8_t parentScreen,
    std::initializer_list<ListMenuItemDef> items);

template <size_t N>
void configureListMenu(
    uint8_t screenId, uint8_t parentScreen,
    const ListMenuItemDef (&items)[N]) { configureListMenu(screenId, parentScreen, items, static_cast<uint8_t>(N)); }

void setListMenuStyle(
    uint8_t screenId, uint32_t cardColor,
    uint32_t cardActiveColor, uint8_t spacing = 8,
    uint8_t radius = 8, int16_t cardWidth = 0,
    int16_t cardHeight = 0, uint16_t titleFontPx = 0,
    uint16_t subtitleFontPx = 0, uint16_t lineGapPx = 0,
    ListMenuMode mode = Cards);

void setListMenuStyle(
    uint8_t screenId, uint16_t cardColor565,
    uint16_t cardActiveColor565, uint8_t spacing = 8,
    uint8_t radius = 8, int16_t cardWidth = 0,
    int16_t cardHeight = 0, uint16_t titleFontPx = 0,
    uint16_t subtitleFontPx = 0, uint16_t lineGapPx = 0,
    ListMenuMode mode = Cards)
{
    setListMenuStyle(screenId,
                     detail::color565To888(cardColor565),
                     detail::color565To888(cardActiveColor565),
                     spacing, radius, cardWidth, cardHeight,
                     titleFontPx, subtitleFontPx, lineGapPx,
                     mode);
}

void renderListMenu(uint8_t screenId);

void renderListMenu(
    uint8_t screenId, int16_t x,
    int16_t y, int16_t w,
    int16_t h, uint32_t bgColor);

void renderListMenu(
    uint8_t screenId, int16_t x,
    int16_t y, int16_t w,
    int16_t h, uint16_t bgColor565)
{
    renderListMenu(screenId, x, y, w, h, detail::color565To888(bgColor565));
}

bool updateListMenu(uint8_t screenId);

bool updateListMenu(
    uint8_t screenId, int16_t x,
    int16_t y, int16_t w,
    int16_t h, uint32_t bgColor);

bool updateListMenu(
    uint8_t screenId, int16_t x,
    int16_t y, int16_t w,
    int16_t h, uint16_t bgColor565)
{
    return updateListMenu(screenId, x, y, w, h, detail::color565To888(bgColor565));
}

void handleListMenuInput(
    uint8_t screenId, bool nextPressed,
    bool prevPressed, bool nextDown,
    bool prevDown);

void invalidateListMenuCache(uint8_t screenId);

// Tile Menu
void configureTileMenu(
    uint8_t screenId, uint8_t parentScreen,
    std::initializer_list<TileMenuItemDef> items);

template <size_t N>
void configureTileMenu(
    uint8_t screenId, uint8_t parentScreen,
    const TileMenuItemDef (&items)[N])
{
    configureTileMenu(screenId, parentScreen, items, static_cast<uint8_t>(N));
}

void setTileMenuLayout(
    uint8_t screenId, uint8_t layoutCols,
    uint8_t layoutRows, std::initializer_list<TileLayoutCell> tiles);

template <size_t N>
void setTileMenuLayout(
    uint8_t screenId, uint8_t layoutCols,
    uint8_t layoutRows, const TileLayoutCell (&tiles)[N])
{
    setTileMenuLayout(screenId, layoutCols, layoutRows, tiles, static_cast<uint8_t>(N));
}

void setTileMenuStyle(
    uint8_t screenId, uint32_t cardColor,
    uint32_t cardActiveColor, uint8_t radius = 8,
    uint8_t spacing = 8, uint8_t columns = 2,
    int16_t tileWidth = 0, int16_t tileHeight = 0,
    uint16_t titleFontPx = 0, uint16_t subtitleFontPx = 0,
    uint16_t lineGapPx = 0, TileContentMode contentMode = TextSubtitle);

void renderTileMenu(uint8_t screenId);
void updateTileMenu(uint8_t screenId, uint8_t prevSelectedIndex);

void handleTileMenuInput(
    uint8_t screenId, bool nextPressed,
    bool prevPressed, bool nextDown,
    bool prevDown);

void invalidateTileMenuCache(uint8_t screenId);

// Screen Control
void regScreen(uint8_t id, ScreenCallback cb);
void setScreen(uint8_t id);
uint8_t currentScreen() const;
void nextScreen();
void prevScreen();
void loop();
void loopWithInput(Button &next, Button &prev);
void requestRedraw();
void setScreenAnimation(ScreenAnim anim, uint32_t durationMs);
void setClipWindow(
    int16_t x, int16_t y,
    int16_t w, int16_t h);

void resetClipWindow();

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

// Notifications
void showNotification(
    const String &t, const String &m,
    const String &btn = "OK", uint16_t delay = 0,
    NotificationType type = Normal);

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
void updateStatusBarBattery();
void updateStatusBarCenter();

// Debug
void setDebugShowDirtyRects(bool enabled) { Debug::setDirtyRectEnabled(enabled); }

bool debugShowDirtyRects() const { return Debug::dirtyRectEnabled(); }

void setDebugDirtyRectColor(uint16_t color) { Debug::setDirtyRectColor(color); }

uint16_t debugDirtyRectColor() const { return Debug::dirtyRectColor(); }

void setDebugDirtyRectActiveColor(uint16_t color) { Debug::setDirtyRectActiveColor(color); }

uint16_t debugDirtyRectActiveColor() const { return Debug::dirtyRectActiveColor(); }

void setDebugMetrics(bool enabled);

bool debugStatusBarMetrics() const { return _flags.statusBarDebugMetrics; }

// Color Helpers
static constexpr uint32_t rgb888(
    uint8_t r, uint8_t g,
    uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
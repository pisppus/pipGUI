#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

#define PIPGUI_FRIEND_FLUENT_BUILDERS               \
    friend struct FillRectFluent;                   \
    friend struct DrawRectFluent;                   \
    template <bool IsUpdate>                        \
    friend struct TextFluentT;                      \
    friend struct DrawTextMarqueeFluent;            \
    friend struct DrawTextEllipsizedFluent;         \
    friend struct GradientVerticalFluent;        \
    friend struct GradientHorizontalFluent;       \
    friend struct GradientCornersFluent;                 \
    friend struct GradientDiagonalFluent;         \
    friend struct GradientRadialFluent;     \
    friend struct DrawLineFluent;                   \
    friend struct DrawCircleFluent;                 \
    friend struct FillCircleFluent;                 \
    friend struct DrawArcFluent;                    \
    friend struct DrawEllipseFluent;                \
    friend struct FillEllipseFluent;                \
    friend struct DrawTriangleFluent;               \
    friend struct FillTriangleFluent;               \
    friend struct DrawSquircleFluent;               \
    friend struct FillSquircleFluent;               \
    friend struct DrawIconFluent;                   \
    friend struct ToastFluent;                      \
    friend struct NotificationFluent;               \
    friend struct detail::TextFontGuard;            \
    friend struct ConfigureListFluent;              \
    friend struct ListInputFluent;                  \
    friend struct ConfigureTileFluent;          \
    friend struct TileInputFluent;                  \
    template <bool IsUpdate>                        \
    friend struct ToggleSwitchFluentT;              \
    template <bool IsUpdate>                        \
    friend struct ButtonFluentT;                    \
    template <bool IsUpdate>                        \
    friend struct ProgressBarFluentT;               \
    template <bool IsUpdate>                        \
    friend struct CircularProgressBarFluentT;       \
    template <bool IsUpdate>                        \
    friend struct BlurRegionFluentT;                 \
    template <bool IsUpdate>                        \
    friend struct ScrollDotsFluentT;                \
    template <bool IsUpdate>                        \
    friend struct GlowCircleFluentT;                \
    template <bool IsUpdate>                        \
    friend struct GlowRectFluentT;

PIPGUI_FRIEND_FLUENT_BUILDERS
#undef PIPGUI_FRIEND_FLUENT_BUILDERS

struct DisplayState
{
    pipcore::GuiDisplay *display = nullptr;
    pipcore::GuiDisplayConfig cfg;
    bool cfgConfigured = false;
    BacklightCallback backlightCb = nullptr;
    uint8_t brightnessMax = 100;
} _disp;

struct RenderState
{
    pipcore::Sprite sprite;
    pipcore::Sprite *activeSprite = nullptr;
    uint16_t screenWidth = 0;
    uint16_t screenHeight = 0;
    uint32_t bgColor = 0;
    uint16_t bgColor565 = 0;
} _render;

struct ClipState
{
    bool enabled = false;
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
} _clip;

struct DirtyRect
{
    int16_t x, y, w, h;
};
static constexpr uint8_t DIRTY_RECT_MAX = 8;
struct DirtyState
{
    DirtyRect rects[DIRTY_RECT_MAX] = {};
    uint8_t count = 0;
} _dirty;

struct ScreenState
{
    ScreenCallback *callbacks = nullptr;
    GraphArea **graphAreas = nullptr;
    ListState **lists = nullptr;
    TileState **tiles = nullptr;
    uint16_t capacity = 0;
    uint8_t current = INVALID_SCREEN_ID;
    bool registrySynced = false;

    ScreenAnim anim = None;
    uint8_t to = 0;
    int8_t transDir = 0;
    uint32_t animStartMs = 0;
    uint32_t animDurationMs = 0;
} _screen;

struct BootState
{
    BootAnimation anim = None;
    String title;
    String subtitle;
    uint32_t fgColor = 0;
    uint32_t bgColor = 0;
    uint32_t startMs = 0;
    uint32_t durationMs = 0;
    int16_t x = 0;
    int16_t y = 0;
} _boot;

struct TypographyState
{
    uint16_t logoTitleSizePx = 0;
    uint16_t logoSubtitleSizePx = 0;
    uint16_t h1Px = 24;
    uint16_t h2Px = 18;
    uint16_t bodyPx = 14;
    uint16_t captionPx = 12;
    uint16_t psdfSizePx = 0;
    uint16_t psdfWeight = 500;
} _typo;

struct ErrorEntry
{
    String title, detail;
    ErrorType type;
};
struct ErrorState
{
    ErrorEntry *entries = nullptr;
    uint8_t count = 0;
    uint8_t capacity = 0;
    uint8_t currentIndex = 0;
    uint8_t nextIndex = 0;
    uint32_t animStartMs = 0;
    ErrorType type = Warning;
    String buttonText;
    ButtonVisualState buttonState;
} _error;

struct NotificationState
{
    String title;
    String message;
    String buttonText;
    NotificationType type = NotificationType::Normal;
    IconId iconId = (IconId)0xFFFF;
    uint32_t startMs = 0;
    uint32_t animDurationMs = 0;
    uint32_t unlockMs = 0;
    ButtonVisualState buttonState;
} _notif;

struct ToastState
{
    String text;
    IconId iconId = WarningLayer0;
    uint16_t iconSizePx = 20;
    uint32_t startMs = 0;
    uint32_t animDurMs = 420;
    uint32_t displayMs = 2500;
    bool fromTop = false;
} _toast;

enum StatusBarDirty : uint8_t
{
    StatusBarDirtyLeft = 1 << 0,
    StatusBarDirtyCenter = 1 << 1,
    StatusBarDirtyRight = 1 << 2,
    StatusBarDirtyBattery = 1 << 3,
    StatusBarDirtyAll = 0xFF,
};

struct StatusBarState
{
    StatusBarPosition pos = Top;
    StatusBarStyle style = StatusBarStyleSolid;
    uint8_t height = 0;
    uint32_t bg = 0x000000;
    uint32_t fg = 0xFFFFFF;

    String textLeft;
    String textCenter;
    String textRight;
    StatusBarCustomCallback custom = nullptr;

    uint8_t dirtyMask = 0;
    DirtyRect lastLeft = {};
    DirtyRect lastCenter = {};
    DirtyRect lastRight = {};
    DirtyRect lastBattery = {};

    int8_t batteryLevel = -1;
    BatteryStyle batteryStyle = Hidden;
} _status;

struct BlurState
{
    uint16_t *smallIn = nullptr;
    uint16_t *smallTmp = nullptr;
    uint32_t *rowR = nullptr;
    uint32_t *rowG = nullptr;
    uint32_t *rowB = nullptr;
    uint32_t *colR = nullptr;
    uint32_t *colG = nullptr;
    uint32_t *colB = nullptr;
    uint32_t workLen = 0;
    uint16_t rowCap = 0;
    uint16_t colCap = 0;
} _blur;

struct Flags
{
    unsigned spriteEnabled : 1;
    unsigned inSpritePass : 1;
    unsigned needRedraw : 1;
    unsigned bootActive : 1;
    unsigned screenTransition : 1;
    unsigned errorActive : 1;
    unsigned errorTransition : 1;
    unsigned errorButtonDown : 1;
    unsigned errorAwaitRelease : 1;
    unsigned notifActive : 1;
    unsigned notifButtonDown : 1;
    unsigned notifClosing : 1;
    unsigned notifDelayed : 1;
    unsigned notifAwaitRelease : 1;
    unsigned statusBarEnabled : 1;
    unsigned statusBarDebugMetrics : 1;
    unsigned toastActive : 1;
} _flags = {};

uint32_t nowMs() const;

// --- Initialization helpers ---
void initFonts();

pipcore::Sprite *getDrawTarget();

void flushDirty();

void invalidateRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h);

// --- Update Effects Rendering Helpers ---
template <typename Fn>
void renderToSpriteAndFlush(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall);

// --- Screen Management ---
void ensureScreenState(uint8_t id);
void syncRegisteredScreens();
void setScreenId(uint8_t id);
void activateScreenId(uint8_t id, int8_t transDir);
void renderScreenToMainSprite(ScreenCallback cb, uint8_t screenId = INVALID_SCREEN_ID);
void freeScreenState(pipcore::GuiPlatform *plat) noexcept;
void renderScreenTransition(uint32_t now);
void renderBootFrame(uint32_t now);

// --- Graphics: Basic Primitives ---
void drawLine(
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void drawCircle(
    int16_t cx, int16_t cy, int16_t r, uint16_t color);

void fillCircle(
    int16_t cx, int16_t cy, int16_t r, uint16_t color);

void drawArc(
    int16_t cx, int16_t cy, int16_t r, float startDeg, float endDeg, uint16_t color);

void drawEllipse(
    int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);

void fillEllipse(
    int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t color);

void drawTriangle(
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void fillTriangle(
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void drawRoundTriangle(
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t radius, uint16_t color);

void fillRoundTriangle(
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t radius, uint16_t color);

void drawSquircle(
    int16_t cx, int16_t cy, int16_t r, uint16_t color);

void fillSquircle(
    int16_t cx, int16_t cy, int16_t r, uint16_t color);

void drawRoundRect(
    int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color565);

void drawRoundRect(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
    uint16_t color565);

void fillRoundRect(
    int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color565);

void fillRoundRect(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
    uint16_t color565);

// --- Graphics: Rects & Gradients ---
void fillRect(
    int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void fillRectGradientVertical(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t topColor, uint32_t bottomColor);

void fillRectGradientHorizontal(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t leftColor, uint32_t rightColor);

void fillRectGradientCorners(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11);

void fillRectGradientDiagonal(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tlColor, uint32_t brColor);

void fillRectGradientRadial(
    int16_t x, int16_t y, int16_t w, int16_t h, int16_t cx, int16_t cy, int16_t radius, uint32_t innerColor, uint32_t outerColor);

// --- Text & Layout ---
int16_t AutoX(int32_t contentWidth) const;
int16_t AutoY(int32_t contentHeight) const;
bool measureText(const String &text, int16_t &outW, int16_t &outH) const;
void drawTextAligned(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align);
void drawTextImmediate(const String &text, int16_t rx, int16_t ry, int16_t tw, int16_t th, uint16_t fg565, uint16_t bg565, TextAlign align);
void drawTextImmediateMasked(const String &text, int16_t rx, int16_t ry, int16_t tw, int16_t th, uint16_t fg565, uint16_t bg565, TextAlign align, int16_t fadeBoxX, int16_t fadeBoxW, uint8_t fadePx);
void drawIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565);
void updateIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565, uint16_t bg565);
void drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565);
void drawText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = Left);
void updateText(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align = Left);
bool drawTextMarquee(const String &text, int16_t x, int16_t y, int16_t maxWidth, uint16_t fg565, TextAlign align = Left, const MarqueeTextOptions &opts = MarqueeTextOptions());
bool drawTextEllipsized(const String &text, int16_t x, int16_t y, int16_t maxWidth, uint16_t fg565, TextAlign align = Left);

// --- Visual Effects: Blur ---
bool ensureBlurWorkBuffers(uint32_t smallLen, int16_t sw, int16_t sh) noexcept;
void freeBlurBuffers(pipcore::GuiPlatform *plat) noexcept;

void drawBlurRegion(
    int16_t x, int16_t y, int16_t w, int16_t h,
    uint8_t radius, BlurDirection dir,
    bool gradient, uint8_t materialStrength,
    uint8_t noiseAmount, int32_t materialColor);

void updateBlurRegion(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint8_t radius, BlurDirection dir,
    bool gradient, uint8_t materialStrength,
    uint8_t noiseAmount, int32_t materialColor);

// --- Visual Effects: Scroll Dots ---
void drawScrollDotsImpl(
    int16_t x, int16_t y,
    uint8_t count, uint8_t activeIndex,
    uint8_t prevIndex, float animProgress,
    bool animate, int8_t animDirection,
    uint16_t activeColor, uint16_t inactiveColor,
    uint8_t dotRadius, uint8_t spacing,
    uint8_t activeWidth);

void updateScrollDotsImpl(
    int16_t x, int16_t y,
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

void drawButton(
    const String &label, int16_t x, int16_t y, int16_t w,
    int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);

void updateButton(
    const String &label, int16_t x, int16_t y, int16_t w,
    int16_t h, uint16_t baseColor, uint8_t radius, const ButtonVisualState &state);

void drawToggleSwitch(
    int16_t x, int16_t y, int16_t w, int16_t h,
    ToggleSwitchState &state, uint16_t activeColor,
    int32_t inactiveColor = -1, int32_t knobColor = -1);

void updateToggleSwitch(
    int16_t x, int16_t y, int16_t w, int16_t h,
    ToggleSwitchState &state, uint16_t activeColor,
    int32_t inactiveColor = -1, int32_t knobColor = -1);

// --- Visual Effects: Glow ---
void drawGlowCircle(
    int16_t x, int16_t y,
    int16_t r, uint16_t fillColor,
    int16_t bgColor, int16_t glowColor,
    uint8_t glowSize, uint8_t glowStrength,
    GlowAnim anim, uint16_t pulsePeriodMs);

void updateGlowCircle(
    int16_t x, int16_t y,
    int16_t r, uint16_t fillColor,
    int16_t bgColor, int16_t glowColor,
    uint8_t glowSize, uint8_t glowStrength,
    GlowAnim anim, uint16_t pulsePeriodMs);

void drawGlowRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint8_t radius, uint16_t fillColor,
    int16_t bgColor, int16_t glowColor,
    uint8_t glowSize, uint8_t glowStrength,
    GlowAnim anim, uint16_t pulsePeriodMs);

void updateGlowRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint8_t radius, uint16_t fillColor,
    int16_t bgColor, int16_t glowColor,
    uint8_t glowSize, uint8_t glowStrength,
    GlowAnim anim, uint16_t pulsePeriodMs);

void drawGlowRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint8_t radiusTL, uint8_t radiusTR,
    uint8_t radiusBR, uint8_t radiusBL,
    uint16_t fillColor, int16_t bgColor,
    int16_t glowColor, uint8_t glowSize,
    uint8_t glowStrength, GlowAnim anim,
    uint16_t pulsePeriodMs);

void updateGlowRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint8_t radiusTL, uint8_t radiusTR,
    uint8_t radiusBR, uint8_t radiusBL,
    uint16_t fillColor, int16_t bgColor,
    int16_t glowColor, uint8_t glowSize,
    uint8_t glowStrength, GlowAnim anim,
    uint16_t pulsePeriodMs);

// --- UI: Menus & Graphs ---
GraphArea *ensureGraphArea(uint8_t screenId);
ListState *ensureList(uint8_t screenId);
TileState *ensureTile(uint8_t screenId);
ListState *getList(uint8_t screenId);
TileState *getTile(uint8_t screenId);

void handleListInput(
    uint8_t screenId, bool nextDown,
    bool prevDown);
bool updateList(
    uint8_t screenId, int16_t x,
    int16_t y, int16_t w,
    int16_t h, uint16_t bgColor565);
void updateTile(uint8_t screenId, uint8_t prevSelectedIndex);
void handleTileInput(
    uint8_t screenId, bool nextDown,
    bool prevDown);

void configureTile(uint8_t screenId, uint8_t parentScreen, const TileItemDef *items, uint8_t itemCount, const TileStyle &style);

void freeGraphAreas(pipcore::GuiPlatform *plat) noexcept;
void freeLists(pipcore::GuiPlatform *plat) noexcept;
void freeTiles(pipcore::GuiPlatform *plat) noexcept;

// --- UI: Toasts & Notifications ---
void renderToastOverlay();
void renderNotificationOverlay();
void showToastInternal(const String &text, uint32_t durationMs, bool fromTop, IconId iconId, uint16_t iconSizePx);
void showNotificationInternal(const String &title, const String &message, const String &buttonText, uint16_t delaySeconds, NotificationType type, IconId iconId);

// --- UI: Error Handling ---
void renderErrorFrame(uint32_t now);
bool ensureErrorCapacity(uint8_t need);
void freeErrors(pipcore::GuiPlatform *plat) noexcept;
void drawErrorCard(int16_t x, int16_t y, int16_t w, int16_t h,
                   const String &title, const String &detail,
                   uint32_t accentColor);

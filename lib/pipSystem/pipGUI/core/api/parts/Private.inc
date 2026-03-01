#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

#define PIPGUI_FRIEND_FLUENT_BUILDERS               \
    friend struct FillRectFluent;                   \
    friend struct DrawRectFluent;                   \
    friend struct FillRectAlphaFluent;              \
    friend struct FillRectGradientVerticalFluent;   \
    friend struct FillRectGradientHorizontalFluent; \
    friend struct FillRectGradient4Fluent;          \
    friend struct FillRectGradientDiagonalFluent;   \
    friend struct FillRectGradientRadialFluent;     \
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
    pipcore::GuiPlatform *platform = nullptr;
    pipcore::GuiDisplay *display = nullptr;
    pipcore::GuiDisplayConfig cfg;
    bool cfgConfigured = false;
    BacklightCallback backlightCb = nullptr;
    uint8_t brightnessMax = 100;
} _disp;

struct RenderState
{
    pipcore::Sprite sprite;
    pipcore::Sprite screenFromSprite;
    pipcore::Sprite screenToSprite;
    pipcore::Sprite *activeSprite = nullptr;
    uint16_t screenWidth = 0;
    uint16_t screenHeight = 0;
    uint32_t bgColor = 0;
} _render;

struct SavedRenderState
{
    unsigned renderToSprite : 1;
    pipcore::Sprite *activeSprite = nullptr;
} _savedRenderState = {};

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
struct DirtyState
{
    DirtyRect rects[8] = {};
    uint8_t count = 0;
} _dirty;

uint32_t _bnsdSeed = 0x6D2B79F5U;

struct ScreenState
{
    ScreenCallback *callbacks = nullptr;
    GraphArea **graphAreas = nullptr;
    ListMenuState **listMenus = nullptr;
    TileMenuState **tileMenus = nullptr;
    uint16_t capacity = 0;
    uint8_t current = 255;

    ScreenAnim anim = None;
    uint8_t from = 0;
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
    float psdfWeightBias = 0.083f;
    FontId psdfFontId = WixMadeForDisplay;
} _typo;

struct ErrorEntry
{
    String title, detail;
    ErrorType type;
    int16_t scrollPos, textWidth;
};
struct ErrorState
{
    ErrorEntry *entries = nullptr;
    uint8_t count = 0;
    uint8_t capacity = 0;
    uint8_t currentIndex = 0;
    uint8_t nextIndex = 0;
    uint32_t animStartMs = 0;
    uint32_t lastScrollMs = 0;
    ErrorType type = Warning;
    String title;
    String message;
    String buttonText;
    uint32_t lastToggleMs = 0;
    uint32_t flashIntervalMs = 0;
    uint32_t endMs = 0;
    ButtonVisualState buttonState;
} _error;

struct NotificationState
{
    String title;
    String message;
    String buttonText;
    NotificationType type = Normal;
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
    uint16_t *prevOrig = nullptr;
    uint32_t prevOrigLen = 0;
    int16_t prevX = 0, prevY = 0;
    int16_t prevW = 0, prevH = 0;
    uint8_t prevScale = 1;

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
    unsigned inited : 1;
    unsigned displayInited : 1;
    unsigned spriteEnabled : 1;
    unsigned renderToSprite : 1;
    unsigned screenSpritesEnabled : 1;
    unsigned needRedraw : 1;
    unsigned bootActive : 1;
    unsigned showBoot : 1;
    unsigned showScreenTransition : 1;
    unsigned screenTransition : 1;
    unsigned showError : 1;
    unsigned errorActive : 1;
    unsigned errorTransition : 1;
    unsigned errorFlashState : 1;
    unsigned errorButtonDown : 1;
    unsigned errorAwaitRelease : 1;
    unsigned notifActive : 1;
    unsigned notifButtonDown : 1;
    unsigned notifClosing : 1;
    unsigned notifDelayed : 1;
    unsigned notifAwaitRelease : 1;
    unsigned statusBarEnabled : 1;
    unsigned statusBarDebugMetrics : 1;
    unsigned psdfLoaded : 1;
    unsigned psdfEnabled : 1;
    unsigned toastActive : 1;
} _flags = {};

uint32_t nowMs() const;

// --- Initialization helpers ---
bool initDisplay(
    pipcore::GuiPlatform *plat, uint8_t rotation,
    uint16_t bgColor);

bool initSprites();
void initFonts();

pipcore::Sprite *getDrawTarget();

void flushDirty();

void invalidateRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h);

void tickDebugDirtyOverlay(uint32_t now);

// --- Update Effects Rendering Helpers ---
void beginDirectRender();
void endDirectRender();
template <typename Fn>
void renderToSpriteAndFlush(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall);

// --- Screen Management ---
void ensureScreenStorage(uint8_t id);
void freeScreenStorage(pipcore::GuiPlatform *plat) noexcept;
void prepareScreenTransition(ScreenCallback fromCb, ScreenCallback toCb);
bool ensureScreenTransitionSprites();
void freeScreenTransitionSprites();
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

void fillRectAlpha(
    int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t alpha);

void fillRectGradientVertical(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t topColor, uint32_t bottomColor);

void fillRectGradientHorizontal(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t leftColor, uint32_t rightColor);

void fillRectGradient4(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11);

void fillRectGradientDiagonal(
    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tlColor, uint32_t brColor);

void fillRectGradientRadial(
    int16_t x, int16_t y, int16_t w, int16_t h, int16_t cx, int16_t cy, int16_t radius, uint32_t innerColor, uint32_t outerColor);

// --- Text & Layout ---
int16_t AutoX(int32_t contentWidth) const;
int16_t AutoY(int32_t contentHeight) const;
bool psdfMeasureText(const String &text, int16_t &outW, int16_t &outH) const;
void psdfDrawTextInternal(const String &text, int16_t x, int16_t y, uint16_t fg565, uint16_t bg565, TextAlign align);
void psdfDrawTextDirect(const String &text, int16_t rx, int16_t ry, int16_t tw, int16_t th, uint16_t fg565, uint16_t bg565, TextAlign align);
void drawIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565);
void updateIconInternal(uint16_t iconId, int16_t x, int16_t y, uint16_t sizePx, uint16_t fg565, uint16_t bg565);
void drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565);

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
GraphArea &ensureGraphArea(uint8_t screenId);
ListMenuState &ensureListMenu(uint8_t screenId);
TileMenuState &ensureTileMenu(uint8_t screenId);
ListMenuState *getListMenu(uint8_t screenId);
TileMenuState *getTileMenu(uint8_t screenId);

void configureListMenu(uint8_t screenId, uint8_t parentScreen, const ListMenuItemDef *items, uint8_t itemCount);
void configureTileMenu(uint8_t screenId, uint8_t parentScreen, const TileMenuItemDef *items, uint8_t itemCount);
bool ensureTileCapacity(TileMenuState &m, uint8_t need);
void setTileMenuLayout(uint8_t screenId, uint8_t layoutCols, uint8_t layoutRows, const TileLayoutCell *tiles, uint8_t count);

void freeGraphAreas(pipcore::GuiPlatform *plat) noexcept;
void freeListMenus(pipcore::GuiPlatform *plat) noexcept;
void freeTileMenus(pipcore::GuiPlatform *plat) noexcept;

// --- UI: Status Bar ---
void renderStatusBar(bool useSprite);
void freeStatusBarIcons(pipcore::GuiPlatform *plat) noexcept;

// --- UI: Toasts & Notifications ---
void renderToastOverlay();
void renderNotificationOverlay();
void showToastInternal(const String &text, uint32_t durationMs, bool fromTop, IconId iconId, uint16_t iconSizePx);

// --- UI: Error Handling ---
void renderErrorFrame(uint32_t now);
bool ensureErrorCapacity(uint8_t need);
void freeErrors(pipcore::GuiPlatform *plat) noexcept;
void drawErrorCard(pipcore::Sprite &t, int16_t x, int16_t y, int16_t w, int16_t h,
                   const String &title, const String &detail,
                   uint32_t accentColor, uint8_t opacity, int scrollX);
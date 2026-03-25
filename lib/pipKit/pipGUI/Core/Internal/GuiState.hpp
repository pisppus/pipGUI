#pragma once

#include <pipCore/Display.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <pipGUI/Core/Types.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <pipGUI/Systems/Update/Ota.hpp>

#if (PIPGUI_SCREENSHOT_MODE == 2)
#include <FS.h>
#include <LittleFS.h>
#endif

namespace pipgui::detail
{
    struct ButtonState
    {
        bool enabled = false;
        uint8_t pressLevel = 0;
        uint8_t fadeLevel = 0;
        bool prevEnabled = false;
        bool loading = false;
        uint32_t lastUpdateMs = 0;
    };

    struct ToggleState
    {
        bool initialized = false;
        bool value = false;
        bool enabled = true;
        uint8_t pos = 0;
        uint8_t animFrom = 0;
        uint8_t animTo = 0;
        uint32_t animStartMs = 0;
        uint8_t enabledLevel = 255;
        uint8_t enabledAnimFrom = 255;
        uint8_t enabledAnimTo = 255;
        uint32_t enabledAnimStartMs = 0;
    };

    struct DisplayState
    {
        pipcore::Display *display = nullptr;
        pipcore::DisplayConfig cfg;
        bool cfgConfigured = false;
        BacklightCallback backlightCb = nullptr;
        uint8_t brightness = 100;
        uint8_t brightnessMax = 100;
    };

    struct RenderState
    {
        pipcore::Sprite sprite;
        pipcore::Sprite *activeSprite = nullptr;
        uint16_t screenWidth = 0;
        uint16_t screenHeight = 0;
        uint32_t bgColor = 0;
        uint16_t bgColor565 = 0;
    };

    struct ClipState
    {
        bool enabled = false;
        int16_t x = 0;
        int16_t y = 0;
        int16_t w = 0;
        int16_t h = 0;
    };

    struct DirtyRect
    {
        int16_t x = 0;
        int16_t y = 0;
        int16_t w = 0;
        int16_t h = 0;
    };

    inline constexpr uint8_t DIRTY_RECT_MAX = 8;

    struct DirtyState
    {
        DirtyRect rects[DIRTY_RECT_MAX] = {};
        uint8_t count = 0;
    };

    struct ScreenState
    {
        static constexpr uint8_t HISTORY_MAX = 16;
        ScreenCallback *callbacks = nullptr;
        GraphArea **graphAreas = nullptr;
        ListState **lists = nullptr;
        TileState **tiles = nullptr;
        uint16_t capacity = 0;
        uint8_t current = INVALID_SCREEN_ID;
        uint8_t history[HISTORY_MAX] = {};
        uint8_t historyCount = 0;
        bool suppressHistory = false;
        bool registrySynced = false;

        ScreenAnim anim = None;
        uint8_t to = 0;
        int8_t transDir = 0;
        uint32_t animStartMs = 0;
        uint32_t animDurationMs = 0;
    };

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
    };

    struct TypographyState
    {
        uint16_t psdfSizePx = 0;
        uint16_t psdfWeight = 500;
        FontId currentFontId = static_cast<FontId>(0);
        float subpixelOffsetX = 0.0f;
        float subpixelOffsetY = 0.0f;
    };

    struct ErrorEntry
    {
        String message;
        String code;
        ErrorType type = ErrorTypeWarning;
        String buttonText;
    };

    struct ErrorState
    {
        ErrorEntry *entries = nullptr;
        uint8_t count = 0;
        uint8_t capacity = 0;
        uint8_t currentIndex = 0;
        uint8_t nextIndex = 0;
        uint32_t animStartMs = 0;
        uint32_t showStartMs = 0;
        uint32_t contentStartMs = 0;
        uint32_t layoutAnimStartMs = 0;
        int8_t transitionDir = 0;
        uint8_t layoutFromDotsVisible = 0;
        uint8_t layoutToDotsVisible = 0;
        ButtonState buttonState;
        uint32_t nextHoldStartMs = 0;
        uint32_t prevHoldStartMs = 0;
        bool nextLongFired = false;
        bool prevLongFired = false;
        bool lastNextDown = false;
        bool lastPrevDown = false;
    };

    struct NotificationState
    {
        String title;
        String message;
        String buttonText;
        NotificationType type = NotificationType::Normal;
        IconId iconId = static_cast<IconId>(0xFFFF);
        uint32_t startMs = 0;
        uint32_t animDurationMs = 0;
        uint32_t unlockMs = 0;
        ButtonState buttonState;
    };

    struct ToastState
    {
        String text;
        IconId iconId = static_cast<IconId>(0xFFFF);
        uint32_t startMs = 0;
        uint32_t animDurMs = 420;
        bool fromTop = false;
        int16_t textW = 0;
        int16_t textH = 0;
        bool textMetricsValid = false;
        DirtyRect lastRect = {};
        bool lastRectValid = false;
    };

    struct PopupMenuState
    {
        PopupMenuItemFn itemFn = nullptr;
        void *itemUser = nullptr;
        uint8_t count = 0;
        uint8_t selectedIndex = 0;
        uint8_t scrollIndex = 0;
        uint8_t maxVisible = 6;
        int16_t x = 0;
        int16_t y = 0;
        int16_t w = 0;
        uint8_t itemHeight = 28;
        uint8_t radius = 12;
        uint16_t bg565 = 0x0000;
        uint16_t fg565 = 0xFFFF;
        uint16_t selBg565 = 0x39E7;
        uint16_t border565 = 0x0000;
        uint32_t startMs = 0;
        uint32_t animDurationMs = 220;
        int16_t resultIndex = -1;
        bool resultReady = false;

        uint32_t nextHoldStartMs = 0;
        uint32_t prevHoldStartMs = 0;
        bool nextLongFired = false;
        bool prevLongFired = false;
        bool lastNextDown = false;
        bool lastPrevDown = false;
    };

    enum StatusBarDirty : uint8_t
    {
        StatusBarDirtyLeft = 1 << 0,
        StatusBarDirtyCenter = 1 << 1,
        StatusBarDirtyRight = 1 << 2,
        StatusBarDirtyBattery = 1 << 3,
        StatusBarDirtyAll = 0xFF,
    };

    struct StatusBarIconState
    {
        IconId iconId = static_cast<IconId>(0xFFFF);
        uint16_t color565 = 0xFFFF;
        uint16_t sizePx = 0;
        uint8_t alpha = 0;
        uint8_t alphaFrom = 0;
        uint8_t alphaTo = 0;
        uint32_t animStartMs = 0;
        bool visible = false;
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

        StatusBarIconState leftIcon;
        StatusBarIconState centerIcon;
        StatusBarIconState rightIcon;

        int8_t batteryLevel = -1;
        BatteryStyle batteryStyle = Hidden;
    };

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
        uint8_t *lookup = nullptr;
        uint32_t workLen = 0;
        uint16_t rowCap = 0;
        uint16_t colCap = 0;
        uint16_t lookupSw = 0;
        uint16_t lookupSh = 0;
        uint16_t lookupW = 0;
        uint16_t lookupH = 0;
        uint32_t lastUseMs = 0;
    };

    struct Flags
    {
        unsigned spriteEnabled : 1;
        unsigned inSpritePass : 1;
        unsigned needRedraw : 1;
        unsigned dirtyRedrawPending : 1;
        unsigned bootActive : 1;
        unsigned screenTransition : 1;
        unsigned errorActive : 1;
        unsigned errorTransition : 1;
        unsigned errorEntering : 1;
        unsigned errorButtonDown : 1;
        unsigned errorAwaitRelease : 1;
        unsigned notifActive : 1;
        unsigned notifButtonDown : 1;
        unsigned notifClosing : 1;
        unsigned notifDelayed : 1;
        unsigned notifAwaitRelease : 1;
        unsigned statusBarEnabled : 1;
        unsigned statusBarConfigured : 1;
        unsigned statusBarDebugMetrics : 1;
        unsigned toastActive : 1;
        unsigned popupActive : 1;
        unsigned popupClosing : 1;
    };

    struct DiagnosticsState
    {
        pipcore::PlatformError lastReportedError = pipcore::PlatformError::None;
        uint8_t otaOkFrames = 0;
        bool otaAutoConfirmed = false;
        uint32_t screenshotHoldStartMs = 0;
        bool screenshotCaptured = false;
    };

    struct ButtonCacheEntry
    {
        uint32_t key = 0;
        uint32_t lastUseMs = 0;
        bool used = false;
        ButtonState state{};
    };

    inline constexpr uint8_t BUTTON_CACHE_MAX = 16;

    struct ButtonCacheState
    {
        ButtonCacheEntry entries[BUTTON_CACHE_MAX] = {};
    };

    struct TextCacheEntry
    {
        uint32_t key = 0;
        uint32_t lastUseMs = 0;
        bool used = false;
        DirtyRect rect{};
    };

    inline constexpr uint8_t TEXT_CACHE_MAX = 24;

    struct TextCacheState
    {
        TextCacheEntry entries[TEXT_CACHE_MAX] = {};
    };

    struct ToggleCacheEntry
    {
        uint32_t key = 0;
        uint32_t lastUseMs = 0;
        bool used = false;
        ToggleState state{};
    };

    inline constexpr uint8_t TOGGLE_CACHE_MAX = 16;

    struct ToggleCacheState
    {
        ToggleCacheEntry entries[TOGGLE_CACHE_MAX] = {};
    };

    struct DrumRollAnimState
    {
        float fromIndex = 0.0f;
        uint8_t toIndex = 0;
        uint16_t durationMs = 280;
        uint32_t startMs = 0;
    };

    struct DrumRollCacheEntry
    {
        uint32_t key = 0;
        uint32_t lastUseMs = 0;
        bool used = false;
        DrumRollAnimState state{};
    };

    inline constexpr uint8_t DRUM_ROLL_CACHE_MAX = 8;

    struct DrumRollCacheState
    {
        DrumRollCacheEntry entries[DRUM_ROLL_CACHE_MAX] = {};
    };

    struct ScreenshotEntry
    {
        uint16_t *pixels = nullptr;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        uint32_t stamp = 0;
        char path[64] = {};
        bool thumbOnFlash = false;
#endif
    };

    struct ScreenshotGalleryState
    {
        ScreenshotEntry *entries = nullptr;
        uint16_t thumbW = 64;
        uint16_t thumbH = 40;
        uint16_t padding = 6;
        uint8_t maxShots = 12;
        uint8_t count = 0;
        uint32_t lastUseMs = 0;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        uint8_t flashLoadIndex = 0;
        bool fsReady = false;
        bool fsDirsReady = false;
        bool flashScanDone = false;
        bool flashScanActive = false;
        bool flashThumbsDone = false;
        bool thumbIndexReady = false;
        uint16_t thumbIndexW = 0;
        uint16_t thumbIndexH = 0;
        fs::File scanDir;
        uint8_t *rowBuf = nullptr;
        uint32_t rowBufSize = 0;
#endif
    };

    struct ScreenshotStreamState
    {
        uint16_t width = 0;
        uint16_t height = 0;
        uint8_t header[12] = {};
        uint8_t headerOffset = 0;
        uint32_t payloadCrc = 0;
        uint8_t *buffer = nullptr;
        bool active = false;
        bool headerReady = false;
        bool notifyOnComplete = false;

        uint32_t encPos = 0;
        uint32_t encPayloadBytes = 0;
        uint16_t encPrev565 = 0;
        uint16_t encIndex[64] = {};
        uint16_t encLiteral565[32] = {};
        uint8_t encRun = 0;
        uint8_t encLiteralLen = 0;
        uint16_t encOutOff = 0;
        uint16_t encOutLen = 0;
        uint8_t encOut[1024] = {};
#if (PIPGUI_SCREENSHOT_MODE == 2)
        fs::File file;
        uint32_t stamp = 0;
        char path[64] = {};
#endif
    };
}

#pragma once

#include <Arduino.h>
#include <assert.h>
#include <initializer_list>
#include <stdlib.h>
#include <algorithm>
#include <utility>
#include <pipCore/Platforms/GUIPlatform.h>
#include <pipCore/Button.h>
#include <pipGUI/core/components/FRC.h>
#include "UiLayout.h"

namespace pipgui
{

    namespace detail
    {
        static inline void *guiAlloc(pipcore::GuiPlatform *plat, size_t bytes, pipcore::GuiAllocCaps caps) noexcept
        {
            if (plat)
            {
                void *p = plat->guiAlloc(bytes, caps);
                if (p)
                    return p;
                if (caps != pipcore::GuiAllocCaps::Default)
                    return plat->guiAlloc(bytes, pipcore::GuiAllocCaps::Default);
                return nullptr;
            }
            return malloc(bytes);
        }

        static inline void guiFree(pipcore::GuiPlatform *plat, void *ptr) noexcept
        {
            if (!ptr)
                return;
            if (plat)
                plat->guiFree(ptr);
            else
                free(ptr);
        }

        template <typename T>
        static bool ensureCapacity(pipcore::GuiPlatform *plat, T *&arr, uint8_t &capacity, uint8_t need) noexcept
        {
            if (capacity >= need && arr)
                return true;

            if (capacity >= need && !arr)
            {
                T *newArr = (T *)guiAlloc(plat, sizeof(T) * capacity, pipcore::GuiAllocCaps::Default);
                if (!newArr)
                    return false;
                for (uint8_t i = 0; i < capacity; ++i)
                    new (&newArr[i]) T();
                arr = newArr;
                return true;
            }

            uint8_t newCap = capacity ? (uint8_t)std::max<uint8_t>(capacity * 2, need) : (uint8_t)std::max<uint8_t>(4, need);
            T *newArr = (T *)guiAlloc(plat, sizeof(T) * newCap, pipcore::GuiAllocCaps::Default);
            if (!newArr)
                return false;

            for (uint8_t i = 0; i < newCap; ++i)
                new (&newArr[i]) T();

            if (arr)
            {
                for (uint8_t i = 0; i < capacity; ++i)
                {
                    newArr[i] = std::move(arr[i]);
                    arr[i].~T();
                }
                guiFree(plat, arr);
            }

            arr = newArr;
            capacity = newCap;
            return true;
        }
    }

    using Button = pipcore::Button;

    enum BootAnimation : uint8_t
    {
        BootAnimNone,
        FadeIn,
        SlideUp,
        LightFade,
        ZoomIn
    };

    enum TextAlign : uint8_t
    {
        AlignLeft,
        AlignCenter,
        AlignRight
    };

    enum TextStyle : uint8_t
    {
        H1,
        H2,
        Body,
        Caption
    };

    static constexpr int16_t center = -1;

    enum StatusBarPosition : uint8_t
    {
        StatusBarTop,
        StatusBarBottom,
        StatusBarLeft,
        StatusBarRight
    };

    enum StatusBarIconPos : uint8_t
    {
        StatusBarIconLeft,
        StatusBarIconCenter,
        StatusBarIconRight
    };

    enum ScreenAnim : uint8_t
    {
        ScreenAnimNone,
        Slide
    };

    enum GraphDirection : uint8_t
    {
        LeftToRight,
        RightToLeft,
        Oscilloscope
    };

    enum ProgressAnim : uint8_t
    {
        ProgressAnimNone,
        Shimmer,
        Stripes,
        Indeterminate
    };

    enum BlurDirection : uint8_t
    {
        TopDown,
        BottomUp,
        LeftRight,
        RightLeft
    };

    enum GlowAnim : uint8_t
    {
        GlowNone,
        GlowPulse
    };

    enum ErrorType : uint8_t
    {
        ErrorTypeWarning,
        Crash
    };

    enum NotificationType : uint8_t
    {
        Normal,
        NotificationTypeWarning,
        Error
    };

    enum BatteryStyle : uint8_t
    {
        Hidden,
        Numeric,
        Bar,
        WarningBar,
        ErrorBar
    };

    namespace detail
    {
        enum AlignTokenCode : uint8_t
        {
            TokLeft,
            TokCenter,
            TokRight,
            TokTop,
            TokBottom
        };

        struct AlignToken
        {
            constexpr AlignToken(AlignTokenCode c) : code(c) {}

            AlignTokenCode code;

            constexpr operator TextAlign() const
            {
                return (code == TokCenter) ? AlignCenter : ((code == TokRight) ? AlignRight : AlignLeft);
            }

            constexpr operator StatusBarPosition() const
            {
                return (code == TokLeft) ? StatusBarLeft : ((code == TokRight) ? StatusBarRight : ((code == TokBottom) ? StatusBarBottom : StatusBarTop));
            }

            constexpr operator StatusBarIconPos() const
            {
                return (code == TokCenter) ? StatusBarIconCenter : ((code == TokRight) ? StatusBarIconRight : StatusBarIconLeft);
            }
        };

        static inline uint32_t blend888(uint32_t bg, uint32_t fg, uint8_t alpha)
        {
            if (alpha == 0)
                return bg;
            if (alpha >= 255)
                return fg;

            uint32_t invAlpha = 255 - alpha;
            uint8_t br = (uint8_t)((bg >> 16) & 0xFF);
            uint8_t bgc = (uint8_t)((bg >> 8) & 0xFF);
            uint8_t bb = (uint8_t)(bg & 0xFF);
            uint8_t fr = (uint8_t)((fg >> 16) & 0xFF);
            uint8_t fgc = (uint8_t)((fg >> 8) & 0xFF);
            uint8_t fb = (uint8_t)(fg & 0xFF);

            uint8_t r = (uint8_t)((br * invAlpha + fr * alpha) >> 8);
            uint8_t g = (uint8_t)((bgc * invAlpha + fgc * alpha) >> 8);
            uint8_t b = (uint8_t)((bb * invAlpha + fb * alpha) >> 8);
            return (uint32_t)((r << 16) | (g << 8) | b);
        }

        static inline uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha)
        {
            if (alpha == 0)
                return bg;
            if (alpha >= 255)
                return fg;

            uint16_t invAlpha = 255 - alpha;
            uint16_t r = (((bg >> 11) & 0x1F) * invAlpha + ((fg >> 11) & 0x1F) * alpha) >> 8;
            uint16_t g = (((bg >> 5) & 0x3F) * invAlpha + ((fg >> 5) & 0x3F) * alpha) >> 8;
            uint16_t b = ((bg & 0x1F) * invAlpha + (fg & 0x1F) * alpha) >> 8;
            return (uint16_t)((r << 11) | (g << 5) | b);
        }

        struct NoneToken
        {
            constexpr operator BootAnimation() const { return BootAnimNone; }
            constexpr operator ScreenAnim() const { return ScreenAnimNone; }
            constexpr operator ProgressAnim() const { return ProgressAnimNone; }
            constexpr operator GlowAnim() const { return GlowNone; }
        };

        struct WarningToken
        {
            constexpr operator ErrorType() const { return ErrorTypeWarning; }
            constexpr operator NotificationType() const { return NotificationTypeWarning; }
        };

        struct PulseToken
        {
            constexpr operator GlowAnim() const { return GlowPulse; }
        };
    }

    static constexpr detail::AlignToken Left{detail::TokLeft};
    static constexpr detail::AlignToken Center{detail::TokCenter};
    static constexpr detail::AlignToken Right{detail::TokRight};
    static constexpr detail::AlignToken Top{detail::TokTop};
    static constexpr detail::AlignToken Bottom{detail::TokBottom};

    static constexpr detail::NoneToken None{};
    static constexpr detail::WarningToken Warning{};
    static constexpr detail::PulseToken Pulse{};

    class GUI;

    using ScreenCallback = void (*)(GUI &ui);
    using BacklightCallback = void (*)(uint16_t level);
    using StatusBarCustomCallback = void (*)(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h);
    using RecoveryActionCallback = void (*)(GUI &ui);

    struct RecoveryItemDef
    {
        const char *title;
        const char *subtitle;
        RecoveryActionCallback action;
    };

    using PipLGFX = lgfx::LovyanGFX;

    struct ButtonVisualStyle
    {
        uint16_t bgColor, bgPressedColor, bgDisabledColor, textColor;
        uint8_t radius, font;
    };

    struct ButtonVisualState
    {
        bool enabled;
        uint8_t pressLevel;
        uint8_t fadeLevel;
        bool prevEnabled;
        bool loading;
        uint32_t lastUpdateMs;
    };

    struct ToggleSwitchState
    {
        bool value;
        uint8_t pos;
        uint32_t lastUpdateMs;
    };

    struct StatusIcon
    {
        const uint16_t *bitmap;
        uint8_t w;
        uint8_t h;
    };

    struct GraphArea
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;

        int16_t innerX;
        int16_t innerY;
        int16_t innerW;
        int16_t innerH;

        uint32_t bgColor;
        uint32_t gridColor;

        uint8_t gridCellsX;
        uint8_t gridCellsY;

        uint8_t radius;
        GraphDirection direction;
        float speed;
        bool autoScaleEnabled;
        bool autoScaleInitialized;
        int16_t autoMin;
        int16_t autoMax;
        uint32_t lastPeakMs;

        uint16_t lineCount;
        uint16_t sampleCapacity;
        int16_t **samples;
        uint16_t *sampleCounts;
        uint16_t *sampleHead;
    };

    struct ListMenuItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
    };

    enum ListMenuMode : uint8_t
    {
        Cards,
        Plain
    };

    enum TileContentMode : uint8_t
    {
        TextOnly,
        TextSubtitle
    };

    struct TileMenuItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
        const uint16_t *iconBitmap;
        uint8_t iconW;
        uint8_t iconH;
    };

    struct TileLayoutCell
    {
        uint8_t col;
        uint8_t row;
        uint8_t colSpan;
        uint8_t rowSpan;
    };

    struct ListMenuStyle
    {
        uint32_t cardColor;
        uint32_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        int16_t cardWidth;
        int16_t cardHeight;
        uint16_t titleFontPx;
        uint16_t subtitleFontPx;
        uint16_t lineGapPx;
        ListMenuMode mode;
    };

    struct TileMenuStyle
    {
        uint32_t cardColor;
        uint32_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        uint8_t columns;
        int16_t tileWidth;
        int16_t tileHeight;
        uint16_t titleFontPx;
        uint16_t subtitleFontPx;
        uint16_t lineGapPx;
        TileContentMode contentMode;
    };

    struct ListMenuState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
        };

        bool configured;
        uint8_t parentScreen;
        uint8_t itemCount;
        uint8_t selectedIndex;

        float scrollPos;
        float targetScroll;

        uint32_t nextHoldStartMs;
        uint32_t prevHoldStartMs;
        bool nextLongFired;
        bool prevLongFired;
        bool lastNextDown;
        bool lastPrevDown;

        ListMenuStyle style;

        uint8_t scrollbarAlpha;
        uint32_t lastScrollActivityMs;

        uint8_t capacity;
        Item *items;

        uint16_t **cacheNormal;
        uint16_t **cacheActive;

        uint16_t cacheW;
        uint16_t cacheH;
        bool cacheValid;

        lgfx::LGFX_Sprite *viewportSprite;
        int16_t viewportX;
        int16_t viewportY;
        int16_t viewportW;
        int16_t viewportH;
        int16_t viewportCardW;
        int16_t viewportCardH;
        int16_t viewportItemSpanPx;
        uint16_t viewportBgColor;
        float viewportScrollPos;
        int32_t viewportScrollPx;
        uint8_t viewportSelectedIndex;
        bool viewportValid;
    };

    struct TileMenuState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
            const uint16_t *iconBitmap;
            uint8_t iconW;
            uint8_t iconH;
        };

        bool configured;
        uint8_t parentScreen;
        uint8_t itemCount;
        uint8_t selectedIndex;

        uint32_t nextHoldStartMs;
        uint32_t prevHoldStartMs;
        bool nextLongFired;
        bool prevLongFired;
        bool lastNextDown;
        bool lastPrevDown;

        TileMenuStyle style;

        bool customLayout;
        uint8_t layoutCols;
        uint8_t layoutRows;

        TileLayoutCell *layout = nullptr;
        Item *items = nullptr;
        uint8_t itemCapacity = 0;
    };

#include "parts/Gui.inc"
}
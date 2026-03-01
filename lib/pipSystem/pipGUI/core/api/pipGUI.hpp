#pragma once

#include <Arduino.h>
#include <cstdlib>
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <pipCore/Platforms/GuiPlatform.hpp>
#include <pipCore/Platforms/PlatformFactory.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Input/Button.hpp>
#include <pipGUI/core/Debug.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipGUI/icons/metrics.hpp>
#include "UiLayout.hpp"

namespace pipgui
{

    struct DisplayPins
    {
        int8_t mosi = -1;
        int8_t sclk = -1;
        int8_t cs = -1;
        int8_t dc = -1;
        int8_t rst = -1;

        constexpr DisplayPins(int8_t mosi_,
                              int8_t sclk_,
                              int8_t cs_,
                              int8_t dc_,
                              int8_t rst_ = -1)
            : mosi(mosi_),
              sclk(sclk_),
              cs(cs_),
              dc(dc_),
              rst(rst_) {}
    };

    namespace detail
    {
        static inline void *guiAlloc(pipcore::GuiPlatform *plat, size_t bytes, pipcore::GuiAllocCaps caps) noexcept
        {
            (void)plat;
            pipcore::GuiPlatform *p = pipcore::GetPlatform();
            void *ptr = p->guiAlloc(bytes, caps);
            if (ptr)
                return ptr;
            if (caps != pipcore::GuiAllocCaps::Default)
                return p->guiAlloc(bytes, pipcore::GuiAllocCaps::Default);
            return nullptr;
        }

        static inline void guiFree(pipcore::GuiPlatform *plat, void *ptr) noexcept
        {
            if (!ptr)
                return;
            (void)plat;
            pipcore::GetPlatform()->guiFree(ptr);
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
    using PullMode = pipcore::PullMode;
    using pipcore::Pulldown;
    using pipcore::Pullup;

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

    enum FontId : uint8_t
    {
        WixMadeForDisplay,
        KronaOne
    };

    enum TextStyle : uint8_t
    {
        H1,
        H2,
        Body,
        Caption
    };

    struct WeightToken
    {
        uint16_t value;
        constexpr operator uint16_t() const { return value; }
    };

    static constexpr WeightToken Thin{100};
    static constexpr WeightToken Light{300};
    static constexpr WeightToken Regular{400};
    static constexpr WeightToken Medium{500};
    static constexpr WeightToken Semibold{600};
    static constexpr WeightToken Bold{700};
    static constexpr WeightToken Black{900};

    static constexpr int16_t center = -1;

    enum StatusBarPosition : uint8_t
    {
        StatusBarTop,
        StatusBarBottom,
        StatusBarLeft,
        StatusBarRight
    };
    enum StatusBarStyle : uint8_t
    {
        StatusBarStyleSolid,
        StatusBarStyleBlurGradient
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

        struct AlignToken
        {
            enum Code : uint8_t
            {
                TokLeft,
                TokCenter,
                TokRight,
                TokTop,
                TokBottom
            };

            constexpr AlignToken(Code c) : code(c) {}

            Code code;

            constexpr operator TextAlign() const
            {
                return (code == TokCenter) ? AlignCenter : ((code == TokRight) ? AlignRight : AlignLeft);
            }

            constexpr operator StatusBarPosition() const
            {
                return (code == TokLeft) ? StatusBarLeft : ((code == TokRight) ? StatusBarRight : ((code == TokBottom) ? StatusBarBottom : StatusBarTop));
            }

            constexpr operator UiAlign() const
            {
                return (code == TokCenter) ? UiAlign::Center : ((code == TokRight) ? UiAlign::End : UiAlign::Start);
            }

            constexpr operator UiJustify() const
            {
                return (code == TokCenter) ? UiJustify::Center : ((code == TokRight) ? UiJustify::End : UiJustify::Start);
            }
        };

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

        static inline uint32_t autoTextColor888(uint32_t bg, uint8_t threshold = 140)
        {
            uint8_t r = (uint8_t)((bg >> 16) & 0xFF);
            uint8_t g = (uint8_t)((bg >> 8) & 0xFF);
            uint8_t b = (uint8_t)(bg & 0xFF);
            uint32_t lum = (uint32_t)r * 30U + (uint32_t)g * 59U + (uint32_t)b * 11U;
            lum /= 100U;
            return (lum > (uint32_t)threshold) ? 0x000000U : 0xFFFFFFU;
        }
    }

    static constexpr detail::AlignToken Left{detail::AlignToken::TokLeft};
    static constexpr detail::AlignToken Center{detail::AlignToken::TokCenter};
    static constexpr detail::AlignToken Right{detail::AlignToken::TokRight};
    static constexpr detail::AlignToken Top{detail::AlignToken::TokTop};
    static constexpr detail::AlignToken Bottom{detail::AlignToken::TokBottom};

    static constexpr detail::NoneToken None{};
    static constexpr detail::WarningToken Warning{};
    static constexpr detail::PulseToken Pulse{};

    class GUI;

    using ScreenCallback = void (*)(GUI &ui);
    using BacklightCallback = void (*)(uint16_t level);
    using StatusBarCustomCallback = void (*)(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h);

    struct ButtonVisualStyle
    {
        uint16_t fontPx = 0;
        uint8_t iconScale = 1;
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

        pipcore::Sprite *viewportSprite;
        int16_t viewportX;
        int16_t viewportY;
        int16_t viewportW;
        int16_t viewportH;
        int16_t viewportCardW;
        int16_t viewportCardH;
        int16_t viewportItemSpanPx;
        uint32_t viewportBgColor;
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

    struct ConfigureDisplayFluent
    {
        GUI *_gui;
        pipcore::GuiDisplayConfig _cfg;
        bool _consumed;

        ConfigureDisplayFluent(GUI *g) : _gui(g), _cfg(), _consumed(false)
        {
            _cfg.mosi = -1;
            _cfg.sclk = -1;
            _cfg.cs = -1;
            _cfg.dc = -1;
            _cfg.rst = -1;
            _cfg.width = 0;
            _cfg.height = 0;
            _cfg.hz = 80000000;
            _cfg.order = 0;
            _cfg.invert = true;
            _cfg.swap = false;
        }

        ConfigureDisplayFluent &pins(const DisplayPins &p)
        {
            if (_consumed)
                return *this;
            _cfg.mosi = p.mosi;
            _cfg.sclk = p.sclk;
            _cfg.cs = p.cs;
            _cfg.dc = p.dc;
            _cfg.rst = p.rst;
            return *this;
        }

        ConfigureDisplayFluent &pins(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst)
        {
            if (_consumed)
                return *this;
            _cfg.mosi = mosi;
            _cfg.sclk = sclk;
            _cfg.cs = cs;
            _cfg.dc = dc;
            _cfg.rst = rst;
            return *this;
        }

        ConfigureDisplayFluent &size(uint16_t w, uint16_t h)
        {
            if (_consumed)
                return *this;
            _cfg.width = w;
            _cfg.height = h;
            return *this;
        }

        ConfigureDisplayFluent &hz(uint32_t v)
        {
            if (_consumed)
                return *this;
            _cfg.hz = v;
            return *this;
        }

        ConfigureDisplayFluent &order(const char *v)
        {
            if (_consumed)
                return *this;
            if (!v)
                return *this;

            if ((v[0] == 'B' || v[0] == 'b') && (v[1] == 'G' || v[1] == 'g') && (v[2] == 'R' || v[2] == 'r') && v[3] == 0)
                _cfg.order = 1;
            else
                _cfg.order = 0;
            return *this;
        }

        ConfigureDisplayFluent &invert(bool v = true)
        {
            if (_consumed)
                return *this;
            _cfg.invert = v;
            return *this;
        }

        ConfigureDisplayFluent &swap(bool v = true)
        {
            if (_consumed)
                return *this;
            _cfg.swap = v;
            return *this;
        }

        void apply();
    };

#define PIPGUI_FLUENT_BUILDERS_INC 1
#include "parts/Builders.inc"
#include "parts/Gui.inc"
}
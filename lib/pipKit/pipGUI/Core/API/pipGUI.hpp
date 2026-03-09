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
#include <pipGUI/Core/Debug.hpp>
#include <pipGUI/Core/Utils/Colors.hpp>
#include <pipGUI/Icons/metrics.hpp>
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

        static inline bool assignString(String &text, const char *src)
        {
            if (!src)
            {
                text = String();
                return true;
            }

            text = src;
            return text.length() > 0 || src[0] == '\0';
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
    static constexpr uint8_t INVALID_SCREEN_ID = 255;

    enum StatusBarPosition : uint8_t
    {
        StatusBarTop,
        StatusBarBottom
    };
    enum StatusBarStyle : uint8_t
    {
        StatusBarStyleSolid,
        StatusBarStyleBlurGradient
    };
    enum ScreenAnim : uint8_t
    {
        ScreenAnimNone,
        SlideX,
        SlideY
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
    enum class NotificationType : uint8_t
    {
        Normal,
        Warning,
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
                return (code == TokBottom) ? StatusBarBottom : StatusBarTop;
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
            constexpr operator NotificationType() const { return NotificationType::Warning; }
        };

        struct NormalToken
        {
            constexpr operator NotificationType() const { return NotificationType::Normal; }
        };

        struct ErrorToken
        {
            constexpr operator NotificationType() const { return NotificationType::Error; }
        };

        struct PulseToken
        {
            constexpr operator GlowAnim() const { return GlowPulse; }
        };

    }

    static constexpr detail::AlignToken Left{detail::AlignToken::TokLeft};
    static constexpr detail::AlignToken Center{detail::AlignToken::TokCenter};
    static constexpr detail::AlignToken Right{detail::AlignToken::TokRight};
    static constexpr detail::AlignToken Top{detail::AlignToken::TokTop};
    static constexpr detail::AlignToken Bottom{detail::AlignToken::TokBottom};

    static constexpr detail::NoneToken None{};
    static constexpr detail::NormalToken Normal{};
    static constexpr detail::WarningToken Warning{};
    static constexpr detail::ErrorToken Error{};
    static constexpr detail::PulseToken Pulse{};

    namespace detail
    {
        struct TextFontGuard;
    }

    class GUI;

    using ScreenCallback = void (*)(GUI &ui);
    using BacklightCallback = void (*)(uint16_t level);
    using StatusBarCustomCallback = void (*)(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h);

    namespace detail
    {
        struct ScreenRegistration
        {
            ScreenCallback callback;
            uint8_t order;
            ScreenRegistration *next;

            static ScreenRegistration *&head() noexcept
            {
                static ScreenRegistration *listHead = nullptr;
                return listHead;
            }

            ScreenRegistration(ScreenCallback cb, uint8_t screenOrder) noexcept
                : callback(cb), order(screenOrder), next(head())
            {
                head() = this;
            }
        };
    }

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

    struct ListItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
        uint16_t iconId;

        constexpr ListItemDef(const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(0xFFFF)
        {
        }

        constexpr ListItemDef(uint16_t icon, const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(icon)
        {
        }
    };

    enum ListMode : uint8_t
    {
        Cards,
        Plain
    };

    enum TileMode : uint8_t
    {
        TextOnly,
        TextSubtitle
    };

    struct MarqueeTextOptions
    {
        uint16_t speedPxPerSec;
        uint16_t holdStartMs;
        uint32_t phaseStartMs;

        constexpr MarqueeTextOptions(uint16_t speed = 28,
                                     uint16_t holdStart = 700,
                                     uint32_t phaseStart = 0) noexcept
            : speedPxPerSec(speed), holdStartMs(holdStart), phaseStartMs(phaseStart)
        {
        }
    };

    struct TileItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
        uint16_t iconId;

        constexpr TileItemDef(const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(0xFFFF)
        {
        }

        constexpr TileItemDef(uint16_t icon, const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(icon)
        {
        }
    };

    struct ListStyle
    {
        uint16_t cardColor;
        uint16_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        int16_t cardWidth;
        int16_t cardHeight;
        uint16_t titleFontPx;
        uint16_t subtitleFontPx;
        uint16_t lineGapPx;
        ListMode mode;
    };

    struct TileStyle
    {
        uint32_t cardColor;
        uint32_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        uint8_t columns;
        int16_t tileWidth;
        int16_t tileHeight;
        uint16_t lineGapPx;
        TileMode contentMode;
    };

    struct ListState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
            uint16_t iconId = 0xFFFF;

            int16_t titleW = 0;
            int16_t titleH = 0;
            int16_t subW = 0;
            int16_t subH = 0;
            uint16_t cachedTitlePx = 0;
            uint16_t cachedSubPx = 0;
            uint16_t cachedTitleWeight = 0;
            uint16_t cachedSubWeight = 0;
        };

        bool configured;
        uint8_t parentScreen;
        uint8_t itemCount;
        uint8_t selectedIndex;

        float scrollPos;
        float targetScroll;
        float scrollVel;

        uint32_t nextHoldStartMs;
        uint32_t prevHoldStartMs;
        bool nextLongFired;
        bool prevLongFired;
        bool lastNextDown;
        bool lastPrevDown;

        ListStyle style;

        uint8_t scrollbarAlpha;
        uint32_t lastScrollActivityMs;
        uint32_t marqueeStartMs;

        uint8_t capacity;
        Item *items;

        uint32_t lastUpdateMs;
    };

    struct TileState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
            uint16_t iconId = 0xFFFF;
            uint8_t layoutCol = 0;
            uint8_t layoutRow = 0;
            uint8_t layoutColSpan = 1;
            uint8_t layoutRowSpan = 1;

            int16_t titleW = 0;
            int16_t titleH = 0;
            int16_t subW = 0;
            int16_t subH = 0;
            uint16_t cachedTitlePx = 0;
            uint16_t cachedSubPx = 0;
            uint16_t cachedTitleWeight = 0;
            uint16_t cachedSubWeight = 0;
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
        uint32_t marqueeStartMs;

        TileStyle style;

        bool customLayout;
        uint8_t layoutCols;
        uint8_t layoutRows;

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

    struct ListInputFluent
    {
        GUI *_gui;
        uint8_t _screenId;
        bool _nextDown;
        bool _prevDown;
        bool _consumed;

        ListInputFluent(GUI *g, uint8_t screenId)
            : _gui(g), _screenId(screenId), _nextDown(false), _prevDown(false), _consumed(false)
        {
        }

        ListInputFluent &nextDown(bool v)
        {
            if (_consumed) return *this;
            _nextDown = v;
            return *this;
        }

        ListInputFluent &prevDown(bool v)
        {
            if (_consumed) return *this;
            _prevDown = v;
            return *this;
        }

        ~ListInputFluent() { apply(); }

        void apply();
    };

    struct ConfigureListFluent
    {
        GUI *_gui;
        uint8_t _screenId;
        const ListItemDef *_items;
        uint8_t _itemCount;
        uint8_t _parentScreen;
        uint16_t _cardColor;
        uint16_t _cardActiveColor;
        uint8_t _radius;
        int16_t _cardWidth;
        int16_t _cardHeight;
        ListMode _mode;
        bool _consumed;

        ConfigureListFluent(GUI *g)
            : _gui(g), _screenId(INVALID_SCREEN_ID), _items(nullptr), _itemCount(0),
              _parentScreen(INVALID_SCREEN_ID), _cardColor(0), _cardActiveColor(0), _radius(8),
              _cardWidth(0), _cardHeight(0), _mode(Cards), _consumed(false)
        {
        }

        ConfigureListFluent &screen(uint8_t screenId)
        {
            if (_consumed) return *this;
            _screenId = screenId;
            return *this;
        }

        ConfigureListFluent &items(std::initializer_list<ListItemDef> items)
        {
            if (_consumed) return *this;
            _items = items.begin();
            _itemCount = static_cast<uint8_t>(items.size());
            return *this;
        }

        ConfigureListFluent &parent(uint8_t parentScreen)
        {
            if (_consumed) return *this;
            _parentScreen = parentScreen;
            return *this;
        }

        ConfigureListFluent &cardColor(uint16_t color565)
        {
            if (_consumed) return *this;
            _cardColor = color565;
            return *this;
        }

        ConfigureListFluent &activeColor(uint16_t color565)
        {
            if (_consumed) return *this;
            _cardActiveColor = color565;
            return *this;
        }

        ConfigureListFluent &radius(uint8_t px)
        {
            if (_consumed) return *this;
            _radius = px;
            return *this;
        }

        ConfigureListFluent &cardSize(int16_t width, int16_t height)
        {
            if (_consumed) return *this;
            _cardWidth = width;
            _cardHeight = height;
            return *this;
        }

        ConfigureListFluent &mode(ListMode m)
        {
            if (_consumed) return *this;
            _mode = m;
            return *this;
        }

        ~ConfigureListFluent() { apply(); }

        void apply();
    };

    struct TileInputFluent
    {
        GUI *_gui;
        uint8_t _screenId;
        bool _nextDown;
        bool _prevDown;
        bool _consumed;

        TileInputFluent(GUI *g, uint8_t screenId)
            : _gui(g), _screenId(screenId), _nextDown(false), _prevDown(false), _consumed(false)
        {
        }

        TileInputFluent &nextDown(bool v)
        {
            if (_consumed) return *this;
            _nextDown = v;
            return *this;
        }

        TileInputFluent &prevDown(bool v)
        {
            if (_consumed) return *this;
            _prevDown = v;
            return *this;
        }

        ~TileInputFluent() { apply(); }

        void apply();
    };

    struct ConfigureTileFluent
    {
        GUI *_gui;
        uint8_t _screenId;
        const TileItemDef *_items;
        uint8_t _itemCount;
        const char *const *_layoutRowsSpec;
        uint8_t _layoutRowCount;
        bool _layoutConfigured;
        uint8_t _parentScreen;
        uint32_t _cardColor;
        uint32_t _cardActiveColor;
        uint8_t _radius;
        uint8_t _spacing;
        uint8_t _columns;
        int16_t _tileWidth;
        int16_t _tileHeight;
        uint16_t _lineGapPx;
        TileMode _contentMode;
        bool _consumed;

        ConfigureTileFluent(GUI *g)
            : _gui(g), _screenId(INVALID_SCREEN_ID), _items(nullptr), _itemCount(0),
              _layoutRowsSpec(nullptr), _layoutRowCount(0), _layoutConfigured(false),
              _parentScreen(INVALID_SCREEN_ID), _cardColor(0), _cardActiveColor(0), _radius(8), _spacing(8),
              _columns(2), _tileWidth(0), _tileHeight(0), _lineGapPx(0),
              _contentMode(TextSubtitle), _consumed(false)
        {
        }

        ConfigureTileFluent &screen(uint8_t screenId)
        {
            if (_consumed) return *this;
            _screenId = screenId;
            return *this;
        }

        ConfigureTileFluent &items(std::initializer_list<TileItemDef> items)
        {
            if (_consumed) return *this;
            _items = items.begin();
            _itemCount = static_cast<uint8_t>(items.size());
            return *this;
        }

        ConfigureTileFluent &layout(std::initializer_list<const char *> rows)
        {
            if (_consumed) return *this;
            _layoutRowsSpec = rows.begin();
            _layoutRowCount = static_cast<uint8_t>(rows.size());
            _layoutConfigured = true;
            return *this;
        }

        ConfigureTileFluent &parent(uint8_t parentScreen)
        {
            if (_consumed) return *this;
            _parentScreen = parentScreen;
            return *this;
        }

        ConfigureTileFluent &cardColor(uint32_t color888)
        {
            if (_consumed) return *this;
            _cardColor = color888;
            return *this;
        }

        ConfigureTileFluent &activeColor(uint32_t color888)
        {
            if (_consumed) return *this;
            _cardActiveColor = color888;
            return *this;
        }

        ConfigureTileFluent &radius(uint8_t px)
        {
            if (_consumed) return *this;
            _radius = px;
            return *this;
        }

        ConfigureTileFluent &spacing(uint8_t px)
        {
            if (_consumed) return *this;
            _spacing = px;
            return *this;
        }

        ConfigureTileFluent &columns(uint8_t value)
        {
            if (_consumed) return *this;
            _columns = value;
            return *this;
        }

        ConfigureTileFluent &tileSize(int16_t width, int16_t height)
        {
            if (_consumed) return *this;
            _tileWidth = width;
            _tileHeight = height;
            return *this;
        }

        ConfigureTileFluent &lineGap(uint16_t px)
        {
            if (_consumed) return *this;
            _lineGapPx = px;
            return *this;
        }

        ConfigureTileFluent &mode(TileMode value)
        {
            if (_consumed) return *this;
            _contentMode = value;
            return *this;
        }

        ~ConfigureTileFluent() { apply(); }

        void apply();
    };


#define PIPGUI_FLUENT_BUILDERS_INC 1
#include "Parts/Builders.inc"
#include "Parts/Gui.inc"
}

#define SCREEN(name, order)                                                                \
    static void name##_screen_callback(::pipgui::GUI &ui);                                 \
    static ::pipgui::detail::ScreenRegistration name##_screen_registration(                \
        name##_screen_callback, static_cast<uint8_t>(order));                              \
    static constexpr uint8_t name = static_cast<uint8_t>(order);                           \
    static void name##_screen_callback(::pipgui::GUI &ui)

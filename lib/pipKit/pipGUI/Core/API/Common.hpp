#pragma once

#include <Arduino.h>
#include <pipCore/Input/Button.hpp>
#include <pipGUI/Core/API/UiLayout.hpp>
#include <pipGUI/Icons/metrics.hpp>

namespace pipgui
{
    class GUI;

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

    using ScreenCallback = void (*)(GUI &ui);
    using BacklightCallback = void (*)(uint16_t level);
    using StatusBarCustomCallback = void (*)(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h);

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

    struct ListItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
        uint16_t iconId;

        constexpr ListItemDef(const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(0xFFFF) {}

        constexpr ListItemDef(uint16_t icon, const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(icon) {}
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
            : speedPxPerSec(speed), holdStartMs(holdStart), phaseStartMs(phaseStart) {}
    };

    struct TileItemDef
    {
        const char *title;
        const char *subtitle;
        uint8_t targetScreen;
        uint16_t iconId;

        constexpr TileItemDef(const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(0xFFFF) {}

        constexpr TileItemDef(uint16_t icon, const char *t, const char *s, uint8_t target) noexcept
            : title(t), subtitle(s), targetScreen(target), iconId(icon) {}
    };
}

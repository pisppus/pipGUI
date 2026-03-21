#pragma once

#include <Arduino.h>
#include <pipGUI/Core/Config/Select.hpp>
#include <pipCore/Input/Button.hpp>
#include <pipGUI/Core/UiLayout.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Text/Icons/metrics.hpp>

namespace pipgui
{
    class GUI;

    using PopupMenuItemFn = const char *(*)(void *user, uint8_t idx);

    struct DisplayPins
    {
        int8_t mosi = -1;
        int8_t sclk = -1;
        int8_t cs = -1;
        int8_t dc = -1;
        int8_t rst = -1;

        constexpr DisplayPins() noexcept = default;

        constexpr DisplayPins(int8_t mosi_,
                              int8_t sclk_,
                              int8_t cs_,
                              int8_t dc_,
                              int8_t rst_ = -1) noexcept
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
        LightFade
    };

    enum class TextAlign : uint8_t
    {
        Left,
        Center,
        Right
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
        constexpr operator uint16_t() const noexcept { return value; }
    };

    inline constexpr WeightToken Thin{100};
    inline constexpr WeightToken Light{300};
    inline constexpr WeightToken Regular{400};
    inline constexpr WeightToken Medium{500};
    inline constexpr WeightToken Semibold{600};
    inline constexpr WeightToken Bold{700};
    inline constexpr WeightToken Black{900};

    inline constexpr int16_t center = -1;
    inline constexpr uint8_t INVALID_SCREEN_ID = 255;

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

            constexpr operator TextAlign() const noexcept
            {
                return (code == TokCenter) ? TextAlign::Center : ((code == TokRight) ? TextAlign::Right : TextAlign::Left);
            }

            constexpr operator StatusBarPosition() const noexcept
            {
                return (code == TokBottom) ? StatusBarBottom : StatusBarTop;
            }

            constexpr operator UiAlign() const noexcept
            {
                return (code == TokCenter) ? UiAlign::Center : ((code == TokRight) ? UiAlign::End : UiAlign::Start);
            }

            constexpr operator UiJustify() const noexcept
            {
                return (code == TokCenter) ? UiJustify::Center : ((code == TokRight) ? UiJustify::End : UiJustify::Start);
            }
        };

        struct NoneToken
        {
            constexpr operator BootAnimation() const noexcept { return BootAnimNone; }
            constexpr operator ScreenAnim() const noexcept { return ScreenAnimNone; }
            constexpr operator ProgressAnim() const noexcept { return ProgressAnimNone; }
            constexpr operator GlowAnim() const noexcept { return GlowNone; }
        };

        struct WarningToken
        {
            constexpr operator ErrorType() const noexcept { return ErrorTypeWarning; }
            constexpr operator NotificationType() const noexcept { return NotificationType::Warning; }
            constexpr operator uint16_t() const noexcept { return detail::color888To565(0xFF6200u); }
        };

        struct NormalToken
        {
            constexpr operator NotificationType() const noexcept { return NotificationType::Normal; }
        };

        struct ErrorToken
        {
            constexpr operator NotificationType() const noexcept { return NotificationType::Error; }
            constexpr operator uint16_t() const noexcept { return detail::color888To565(0xFF0048u); }
        };

        struct PulseToken
        {
            constexpr operator GlowAnim() const noexcept { return GlowPulse; }
        };
    }

    inline constexpr detail::AlignToken Left{detail::AlignToken::TokLeft};
    inline constexpr detail::AlignToken Center{detail::AlignToken::TokCenter};
    inline constexpr detail::AlignToken Right{detail::AlignToken::TokRight};
    inline constexpr detail::AlignToken Top{detail::AlignToken::TokTop};
    inline constexpr detail::AlignToken Bottom{detail::AlignToken::TokBottom};

    inline constexpr detail::NoneToken None{};
    inline constexpr detail::NormalToken Normal{};
    inline constexpr detail::WarningToken Warning{};
    inline constexpr detail::ErrorToken Error{};
    inline constexpr detail::PulseToken Pulse{};

    using ScreenCallback = void (*)(GUI &ui);
    using BacklightCallback = void (*)(uint16_t level);
    using StatusBarCustomCallback = void (*)(GUI &ui, int16_t x, int16_t y, int16_t w, int16_t h);

    struct ButtonVisualState
    {
        bool enabled = false;
        uint8_t pressLevel = 0;
        uint8_t fadeLevel = 0;
        bool prevEnabled = false;
        bool loading = false;
        uint32_t lastUpdateMs = 0;
    };

    struct ToggleSwitchState
    {
        bool value = false;
        uint8_t pos = 0;
        uint32_t lastUpdateMs = 0;
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

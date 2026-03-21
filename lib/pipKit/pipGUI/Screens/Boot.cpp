#include <pipGUI/Core/pipGUI.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint16_t kBootMinTitlePx = 16;
        constexpr uint16_t kBootMaxTitlePx = 36;
        constexpr uint16_t kBootEndHoldMs = 180;

        struct BootTextMetrics
        {
            bool hasSubtitle = false;
            uint16_t titlePx = kBootMinTitlePx;
            uint16_t subtitlePx = (kBootMinTitlePx * 3U) / 4U;
            int16_t titleH = 0;
            int16_t subtitleH = 0;
            int16_t spacing = 0;
            int16_t totalH = 0;
        };

        static inline uint16_t resolveTitlePx(uint16_t screenWidth, uint16_t screenHeight) noexcept
        {
            const uint16_t minSide = screenWidth < screenHeight ? screenWidth : screenHeight;
            uint16_t titlePx = static_cast<uint16_t>(minSide / 8U);
            if (titlePx < kBootMinTitlePx)
                titlePx = kBootMinTitlePx;
            if (titlePx > kBootMaxTitlePx)
                titlePx = kBootMaxTitlePx;
            return titlePx;
        }

        static inline int16_t resolveSubtitleSpacing(uint16_t titlePx) noexcept
        {
            int16_t spacing = static_cast<int16_t>(titlePx / 10U);
            return spacing < 3 ? 3 : spacing;
        }

        static inline uint32_t clampElapsed(uint32_t elapsed, uint32_t duration) noexcept
        {
            return elapsed < duration ? elapsed : duration;
        }
    }

    void GUI::drawBootTitleBlock(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565)
    {
        const FontId prevFont = fontId();
        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();
        clear(bg565);

        BootTextMetrics layout{};
        layout.hasSubtitle = subtitle.length() > 0;
        layout.titlePx = resolveTitlePx(_render.screenWidth, _render.screenHeight);
        layout.subtitlePx = static_cast<uint16_t>((layout.titlePx * 2U) / 3U);
        layout.spacing = layout.hasSubtitle ? resolveSubtitleSpacing(layout.titlePx) : 0;

        int16_t unusedW = 0;
        static_cast<void>(setFont(KronaOne));
        setFontSize(layout.titlePx);
        measureText(title, unusedW, layout.titleH);

        if (layout.hasSubtitle)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(layout.subtitlePx);
            measureText(subtitle, unusedW, layout.subtitleH);
        }

        layout.totalH = layout.titleH + (layout.hasSubtitle ? layout.spacing + layout.subtitleH : 0);
        const int16_t topY = (_boot.y != -1) ? _boot.y : static_cast<int16_t>((_render.screenHeight - layout.totalH) >> 1);
        const int16_t centerX = (_boot.x != -1) ? _boot.x : static_cast<int16_t>(_render.screenWidth >> 1);

        static_cast<void>(setFont(KronaOne));
        setFontSize(layout.titlePx);
        drawTextAligned(title, centerX, topY, fg565, bg565, TextAlign::Center);

        if (layout.hasSubtitle)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(layout.subtitlePx);
            drawTextAligned(subtitle, centerX, topY + layout.titleH + layout.spacing, fg565, bg565, TextAlign::Center);
        }

        static_cast<void>(setFont(prevFont));
        setFontSize(prevSize);
        setFontWeight(prevWeight);
    }

    void GUI::startLogo(const String &t, const String &s, BootAnimation a, uint32_t fg, uint32_t bg, uint32_t dur, int16_t x, int16_t y)
    {
        _flags.bootActive = 1;
        _boot.anim = a;
        _boot.title = t;
        _boot.subtitle = s;
        _boot.fgColor = fg;
        _boot.bgColor = bg;
        _boot.durationMs = dur;
        _boot.x = x;
        _boot.y = y;
        _boot.startMs = nowMs();
    }

    void GUI::renderBootFrame(uint32_t now)
    {
        if (!_flags.bootActive)
            return;

        const uint32_t elapsed = now - _boot.startMs;
        const uint32_t duration = _boot.durationMs > 0 ? _boot.durationMs : 1;
        const uint32_t totalDuration = duration + kBootEndHoldMs;
        bool done = elapsed >= totalDuration;
        const uint16_t bg565 = detail::color888To565(_boot.bgColor);

        auto renderBootScreen = [&](uint16_t fg565)
        {
            const bool useSprite = _flags.spriteEnabled;
            const bool prevSpritePass = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;

            if (useSprite)
            {
                _render.activeSprite = &_render.sprite;
                _flags.inSpritePass = 1;
            }

            drawBootTitleBlock(_boot.title, _boot.subtitle, fg565, bg565);

            _flags.inSpritePass = prevSpritePass;
            _render.activeSprite = prevActive;

            if (useSprite && _disp.display)
                presentSprite(0, 0, static_cast<int16_t>(_render.screenWidth), static_cast<int16_t>(_render.screenHeight), "present");
        };

        auto draw = [&](uint32_t fg888)
        {
            renderBootScreen(detail::color888To565(fg888));
        };

        switch (_boot.anim)
        {
        case BootAnimNone:
            draw(_boot.fgColor);
            done = true;
            break;

        case LightFade:
        {
            const uint32_t animElapsed = clampElapsed(elapsed, duration);
            const uint32_t brightness = done ? _disp.brightnessMax : (animElapsed * static_cast<uint32_t>(_disp.brightnessMax)) / duration;
            if (_disp.backlightCb)
                _disp.backlightCb(min(static_cast<uint32_t>(_disp.brightnessMax), brightness));
            draw(_boot.fgColor);
            break;
        }

        case FadeIn:
        {
            const uint32_t animElapsed = clampElapsed(elapsed, duration);
            const uint8_t alpha = done ? 255 : static_cast<uint8_t>((animElapsed * 255U) / duration);
            draw(detail::blend888(_boot.bgColor, _boot.fgColor, alpha));
            break;
        }
        }

        if (done)
        {
            _flags.bootActive = 0;
            _flags.needRedraw = 1;
            if (_boot.anim == LightFade && _disp.backlightCb)
                _disp.backlightCb(_disp.brightnessMax);
        }
    }
}

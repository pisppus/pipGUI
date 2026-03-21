#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kMaxAnimDtMs = 100U;
        constexpr float kFadeSpeed = 600.0f;
        constexpr float kDisabledReleaseSpeed = 1200.0f;
        constexpr float kPressSpeed = 1600.0f;
        constexpr float kReleaseSpeed = 1400.0f;
        constexpr float kPressedScale = 0.05f;
        constexpr float kMinScale = 0.85f;

        struct ButtonFrame
        {
            int16_t centerX;
            int16_t centerY;
            int16_t x;
            int16_t y;
            int16_t w;
            int16_t h;
            int16_t radius;
        };

        [[nodiscard]] uint32_t clampAnimDt(uint32_t now, uint32_t last) noexcept
        {
            if (last == 0 || now <= last)
                return 0;
            const uint32_t dt = now - last;
            return dt > kMaxAnimDtMs ? kMaxAnimDtMs : dt;
        }

        [[nodiscard]] uint8_t animStep(float unitsPerSecond, uint32_t dtMs) noexcept
        {
            if (dtMs == 0)
                return 0;
            const uint8_t step = static_cast<uint8_t>(unitsPerSecond * (static_cast<float>(dtMs) / 1000.0f) + 0.5f);
            return step > 0 ? step : 1;
        }

        void approachLevel(uint8_t &value, uint8_t target, uint8_t step) noexcept
        {
            if (step == 0 || value == target)
                return;
            if (value < target)
            {
                const uint16_t next = static_cast<uint16_t>(value) + step;
                value = next > target ? target : static_cast<uint8_t>(next);
                return;
            }
            value = value > step ? static_cast<uint8_t>(value - step) : 0;
            if (value < target)
                value = target;
        }

        [[nodiscard]] int16_t resolveAxis(int16_t v, int16_t extent, int16_t total) noexcept
        {
            return v == center ? static_cast<int16_t>((total - extent) / 2) : v;
        }

        [[nodiscard]] ButtonFrame resolveButtonFrame(int16_t x, int16_t y, int16_t w, int16_t h,
                                                     uint8_t radius, uint8_t pressLevel,
                                                     int16_t screenW, int16_t screenH) noexcept
        {
            const int16_t x0 = resolveAxis(x, w, screenW);
            const int16_t y0 = resolveAxis(y, h, screenH);
            const int16_t centerX = static_cast<int16_t>(x0 + w / 2);
            const int16_t centerY = static_cast<int16_t>(y0 + h / 2);

            float scale = 1.0f - kPressedScale * (pressLevel / 255.0f);
            if (scale < kMinScale)
                scale = kMinScale;

            const int16_t outW = (w * scale) > 4 ? static_cast<int16_t>(w * scale) : 4;
            const int16_t outH = (h * scale) > 4 ? static_cast<int16_t>(h * scale) : 4;
            int16_t outR = static_cast<int16_t>(radius * scale + 0.5f);
            if (outR < 1)
                outR = 1;

            return {
                centerX,
                centerY,
                static_cast<int16_t>(centerX - outW / 2),
                static_cast<int16_t>(centerY - outH / 2),
                outW,
                outH,
                outR,
            };
        }

        [[nodiscard]] uint16_t resolveButtonTextColor(uint16_t bg, const detail::ButtonState &state, uint16_t fgActive) noexcept
        {
            if (state.loading)
                return fgActive;

            if (!state.enabled)
                return static_cast<uint16_t>(detail::blend565(bg, fgActive, 132));

            if (state.fadeLevel < 255)
                return static_cast<uint16_t>(detail::blend565(bg, fgActive, 224));

            return fgActive;
        }

        void fillButtonBody(GUI &gui, const ButtonFrame &frame, uint16_t color565)
        {
            gui.fillSquircleRect(frame.x, frame.y, frame.w, frame.h, frame.radius, color565);
        }

        [[nodiscard]] String loadingLabel(const String &label, bool loading, uint32_t now) noexcept
        {
            if (!loading)
                return label;

            String out = label;
            const uint8_t phase = static_cast<uint8_t>((now / 300U) % 6U);
            const uint8_t dots = phase <= 3U ? phase : static_cast<uint8_t>(6U - phase);
            for (uint8_t i = 0; i < dots; ++i)
                out += '.';
            return out;
        }

        [[nodiscard]] uint16_t resolveButtonIconSize(int16_t buttonH, bool hasLabel) noexcept
        {
            const float ratio = hasLabel ? 0.48f : 0.58f;
            const uint16_t sizePx = static_cast<uint16_t>(buttonH * ratio + 0.5f);
            return sizePx > 6 ? sizePx : 6;
        }
    }

    void GUI::stepButtonState(detail::ButtonState &s, bool isDown)
    {
        const uint32_t now = nowMs();
        const uint32_t dt = clampAnimDt(now, s.lastUpdateMs);
        const bool visualEnabled = s.enabled && !s.loading;

        if (visualEnabled != s.prevEnabled)
        {
            s.fadeLevel = visualEnabled ? 0 : 255;
            s.prevEnabled = visualEnabled;
        }

        approachLevel(s.fadeLevel, visualEnabled ? 255 : 0, animStep(kFadeSpeed, dt));

        if (!visualEnabled)
        {
            approachLevel(s.pressLevel, 0, animStep(kDisabledReleaseSpeed, dt));
            s.lastUpdateMs = now;
            return;
        }

        approachLevel(s.pressLevel, isDown ? 255 : 0, animStep(isDown ? kPressSpeed : kReleaseSpeed, dt));
        s.lastUpdateMs = now;
    }

    void GUI::drawButton(const String &label,
                         int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t baseColor,
                         uint8_t radius,
                         IconId iconId,
                         const detail::ButtonState &state)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateButton(label, x, y, w, h, baseColor, radius, iconId, state);
            return;
        }

        if (!getDrawTarget())
            return;

        const ButtonFrame frame = resolveButtonFrame(x, y, w, h, radius, state.pressLevel, _render.screenWidth, _render.screenHeight);

        const uint16_t surfaceBg = _render.bgColor565;
        const uint16_t pressedBg = static_cast<uint16_t>(detail::blend565(surfaceBg, baseColor, 190));
        const uint16_t disabledBg = static_cast<uint16_t>(detail::blend565(surfaceBg, baseColor, 76));
        const uint16_t activeBg = state.pressLevel > 0 ? pressedBg : baseColor;

        uint16_t bg = activeBg;
        if (state.fadeLevel == 0)
            bg = disabledBg;
        else if (state.fadeLevel < 255)
        {
            const float eased = detail::motion::easeOutCubic(static_cast<float>(state.fadeLevel) / 255.0f);
            const uint8_t a = static_cast<uint8_t>(eased * 255.0f + 0.5f);
            bg = static_cast<uint16_t>(detail::blend565(disabledBg, activeBg, a));
        }

        fillButtonBody(*this, frame, bg);

        const uint16_t fgActive = detail::autoTextColor(bg, 140);
        const uint16_t fg = resolveButtonTextColor(bg, state, fgActive);

        const String labelToDraw = loadingLabel(label, state.loading, nowMs());
        const bool hasText = labelToDraw.length() > 0;
        const bool hasIcon = (iconId != static_cast<IconId>(0xFFFF) && iconId < psdf_icons::IconCount);

        constexpr int16_t paddingX = 6;
        constexpr int16_t paddingY = 2;
        const int16_t maxW = (frame.w - paddingX * 2) > 4 ? static_cast<int16_t>(frame.w - paddingX * 2) : 4;
        const int16_t maxH = (frame.h - paddingY * 2) > 4 ? static_cast<int16_t>(frame.h - paddingY * 2) : 4;

        uint16_t px = (frame.h * 0.55f) > 8 ? static_cast<uint16_t>(frame.h * 0.55f) : 8;
        const uint16_t maxPx = (frame.h * 0.8f) > 8 ? static_cast<uint16_t>(frame.h * 0.8f) : 8;
        if (px > maxPx)
            px = maxPx;

        int16_t tw = 0;
        int16_t th = 0;
        const uint16_t resolvedIconSize = hasIcon ? resolveButtonIconSize(frame.h, hasText) : 0;
        const int16_t iconGap = (hasIcon && hasText) ? static_cast<int16_t>(resolvedIconSize > 16 ? 7 : 5) : 0;
        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();

        const auto fitLabel = [&](uint16_t &sizePx, int16_t &outW, int16_t &outH)
        {
            setFontWeight(Medium);
            setFontSize(sizePx);
            measureText(labelToDraw, outW, outH);

            const int16_t availW = hasIcon && hasText
                                       ? static_cast<int16_t>(maxW - resolvedIconSize - iconGap)
                                       : maxW;

            if (outW <= availW && outH <= maxH)
                return;

            const float scaleX = outW > 0 ? (static_cast<float>((availW > 4) ? availW : 4) / static_cast<float>(outW)) : 1.0f;
            const float scaleY = outH > 0 ? (static_cast<float>(maxH) / static_cast<float>(outH)) : 1.0f;
            const float scale = scaleX < scaleY ? scaleX : scaleY;
            if (scale >= 1.0f)
                return;

            const uint16_t nextPx = (sizePx * scale) > 7.0f ? static_cast<uint16_t>(sizePx * scale) : 7;
            if (nextPx == sizePx)
                return;

            sizePx = nextPx;
            setFontSize(sizePx);
            measureText(labelToDraw, outW, outH);
        };

        if (hasText)
            fitLabel(px, tw, th);

        const int16_t iconY = static_cast<int16_t>(frame.centerY - resolvedIconSize / 2);
        int16_t textY = static_cast<int16_t>(frame.centerY - th / 2);
        if (textY < frame.y)
            textY = frame.y;

        if (hasIcon && !hasText)
        {
            const int16_t iconX = static_cast<int16_t>(frame.centerX - resolvedIconSize / 2);
            drawIconInternal(iconId, iconX, iconY, resolvedIconSize, fg);
        }
        else if (hasText)
        {
            const int16_t contentW = hasIcon ? static_cast<int16_t>(resolvedIconSize + iconGap + tw) : tw;
            const int16_t contentX = static_cast<int16_t>(frame.centerX - contentW / 2);

            if (hasIcon)
                drawIconInternal(iconId, contentX, iconY, resolvedIconSize, fg);

            setFontWeight(Medium);
            setFontSize(px);
            const int16_t textCenterX = hasIcon
                                            ? static_cast<int16_t>(contentX + resolvedIconSize + iconGap + tw / 2)
                                            : frame.centerX;
            drawTextAligned(labelToDraw, textCenterX, textY, fg, bg, TextAlign::Center);
        }
        setFontWeight(prevWeight);
        setFontSize(prevSize);
    }

    void GUI::updateButton(const String &label,
                           int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t baseColor,
                           uint8_t radius,
                           IconId iconId,
                           const detail::ButtonState &state)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawButton(label, x, y, w, h, baseColor, radius, iconId, state);
            return;
        }

        const int16_t rx = resolveAxis(x, w, _render.screenWidth);
        const int16_t ry = resolveAxis(y, h, _render.screenHeight);
        constexpr int16_t pad = 2;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
            .color(_render.bgColor565)
            .draw();
        drawButton(label, x, y, w, h, baseColor, radius, iconId, state);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
            flushDirty();
        }
    }
}

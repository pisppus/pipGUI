#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kMaxAnimDtMs = 100U;
        constexpr uint16_t kPressInDurationMs = 250U;
        constexpr uint16_t kPressOutDurationMs = 340U;
        constexpr uint16_t kLabelTransitionMs = 360U;
        constexpr uint16_t kProgressTransitionMs = 320U;
        constexpr float kFadeSpeed = 600.0f;
        constexpr float kDisabledFadeSpeed = 1200.0f;
        constexpr uint16_t kDisabledReleaseDurationMs = 120U;
        constexpr float kPressedScale = 0.032f;
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
            float scale;
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

        [[nodiscard]] int16_t centerExtent(int16_t centerPos, int16_t extent) noexcept
        {
            return static_cast<int16_t>(lroundf(static_cast<float>(centerPos) - static_cast<float>(extent) * 0.5f));
        }

        [[nodiscard]] float resolveButtonScale(uint8_t pressLevel) noexcept
        {
            float scale = 1.0f - kPressedScale * (pressLevel / 255.0f);
            if (scale < kMinScale)
                scale = kMinScale;
            return scale;
        }

        [[nodiscard]] ButtonFrame resolveButtonFrame(int16_t x, int16_t y, int16_t w, int16_t h,
                                                     uint8_t radius, uint8_t pressLevel,
                                                     int16_t screenW, int16_t screenH) noexcept
        {
            const int16_t x0 = resolveAxis(x, w, screenW);
            const int16_t y0 = resolveAxis(y, h, screenH);
            const int16_t centerX = static_cast<int16_t>(x0 + w / 2);
            const int16_t centerY = static_cast<int16_t>(y0 + h / 2);

            const float scale = resolveButtonScale(pressLevel);

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
                scale,
            };
        }

        [[nodiscard]] uint8_t samplePressLevel(const detail::ButtonState &state, uint32_t now) noexcept
        {
            if (state.pressAnimDurationMs == 0 || state.pressFrom == state.pressTo || now <= state.pressAnimStartMs)
                return state.pressLevel;

            const uint32_t elapsed = now - state.pressAnimStartMs;
            if (elapsed >= state.pressAnimDurationMs)
                return state.pressTo;

            const float t = static_cast<float>(elapsed) / static_cast<float>(state.pressAnimDurationMs);
            const float eased = (state.pressTo > state.pressFrom)
                                    ? detail::motion::cubicBezierEase(t, 0.24f, 0.84f, 0.22f, 1.0f)
                                    : detail::motion::cubicBezierEase(t, 0.30f, 0.0f, 0.16f, 1.0f);
            const float value = static_cast<float>(state.pressFrom) + (static_cast<float>(state.pressTo) - static_cast<float>(state.pressFrom)) * eased;
            return static_cast<uint8_t>(value + 0.5f);
        }

        [[nodiscard]] uint8_t clampProgressValue(uint8_t value) noexcept
        {
            return value > 100 ? 100 : value;
        }

        [[nodiscard]] uint8_t sampleProgressValue(const detail::ButtonState &state, uint32_t now) noexcept
        {
            if (state.progressAnimDurationMs == 0 || state.progressFrom == state.progressTo || now <= state.progressAnimStartMs)
                return state.progressValue;

            const uint32_t elapsed = now - state.progressAnimStartMs;
            if (elapsed >= state.progressAnimDurationMs)
                return state.progressTo;

            const float t = static_cast<float>(elapsed) / static_cast<float>(state.progressAnimDurationMs);
            const float eased = detail::motion::cubicBezierEase(t, 0.22f, 1.0f, 0.30f, 1.0f);
            const float value = static_cast<float>(state.progressFrom) + (static_cast<float>(state.progressTo) - static_cast<float>(state.progressFrom)) * eased;
            return static_cast<uint8_t>(value + 0.5f);
        }

        [[nodiscard]] int16_t progressWidthForValue(int16_t width, uint8_t value) noexcept
        {
            if (width <= 0)
                return 0;
            return static_cast<int16_t>((static_cast<int32_t>(width) * clampProgressValue(value)) / 100);
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
            gui.drawSquircleRect().pos(frame.x, frame.y).size(frame.w, frame.h).radius({(uint8_t)frame.radius}).fill(color565);
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

        void syncButtonLabel(detail::ButtonState &state, const String &label, uint32_t now)
        {
            if (!state.labelInitialized)
            {
                state.currentLabel = label;
                state.previousLabel = String();
                state.labelInitialized = true;
                state.labelTransitionActive = false;
                return;
            }

            if (!state.labelTransitionActive && state.currentLabel == label)
                return;

            if (state.labelTransitionActive && (now - state.labelAnimStartMs) >= kLabelTransitionMs)
            {
                state.previousLabel = String();
                state.labelTransitionActive = false;
            }

            if (state.currentLabel == label)
                return;

            state.previousLabel = state.currentLabel;
            state.currentLabel = label;
            state.labelAnimStartMs = now;
            state.labelTransitionActive = state.previousLabel != state.currentLabel;
        }

        void syncButtonLabelDirect(detail::ButtonState &state, const String &label) noexcept
        {
            state.currentLabel = label;
            state.previousLabel = String();
            state.labelInitialized = true;
            state.labelTransitionActive = false;
        }

        [[nodiscard]] float buttonLabelTransition(detail::ButtonState &state, uint32_t now) noexcept
        {
            if (!state.labelTransitionActive)
                return 1.0f;

            const uint32_t elapsed = now - state.labelAnimStartMs;
            if (elapsed >= kLabelTransitionMs)
            {
                state.previousLabel = String();
                state.labelTransitionActive = false;
                return 1.0f;
            }

            const float t = static_cast<float>(elapsed) / static_cast<float>(kLabelTransitionMs);
            return detail::motion::cubicBezierEase(t, 0.18f, 0.0f, 0.0f, 1.0f);
        }

    }

    void GUI::stepButtonState(detail::ButtonState &s, bool isDown)
    {
        const uint32_t now = nowMs();
        const uint32_t dt = clampAnimDt(now, s.lastUpdateMs);
        const bool visualEnabled = s.enabled && !s.loading;

        if (s.lastUpdateMs == 0)
        {
            s.prevEnabled = visualEnabled;
            s.fadeLevel = visualEnabled ? 255 : 0;
            s.pressLevel = 0;
            s.pressFrom = 0;
            s.pressTo = 0;
            s.progressInitialized = s.progressEnabled;
            s.progressTarget = clampProgressValue(s.progressTarget);
            s.progressValue = s.progressTarget;
            s.progressFrom = s.progressTarget;
            s.progressTo = s.progressTarget;
            s.progressAnimStartMs = now;
            s.progressAnimDurationMs = 0;
            s.lastUpdateMs = now;
            return;
        }

        if (visualEnabled != s.prevEnabled)
        {
            s.fadeLevel = visualEnabled ? 0 : 255;
            s.prevEnabled = visualEnabled;
        }

        approachLevel(s.fadeLevel, visualEnabled ? 255 : 0, animStep(visualEnabled ? kFadeSpeed : kDisabledFadeSpeed, dt));

        s.pressLevel = samplePressLevel(s, now);
        const uint8_t targetPress = visualEnabled && isDown ? 255 : 0;
        if (s.pressTo != targetPress)
        {
            s.pressLevel = samplePressLevel(s, now);
            s.pressFrom = s.pressLevel;
            s.pressTo = targetPress;
            s.pressAnimStartMs = now;
            s.pressAnimDurationMs = visualEnabled ? (targetPress > s.pressFrom ? kPressInDurationMs : kPressOutDurationMs) : kDisabledReleaseDurationMs;
        }

        s.progressTarget = clampProgressValue(s.progressTarget);
        if (!s.progressEnabled)
        {
            s.progressInitialized = false;
            s.progressValue = 0;
            s.progressFrom = 0;
            s.progressTo = 0;
            s.progressAnimDurationMs = 0;
        }
        else
        {
            if (!s.progressInitialized)
            {
                s.progressInitialized = true;
                s.progressValue = s.progressTarget;
                s.progressFrom = s.progressTarget;
                s.progressTo = s.progressTarget;
                s.progressAnimStartMs = now;
                s.progressAnimDurationMs = 0;
            }
            else if (s.progressTo != s.progressTarget)
            {
                s.progressValue = sampleProgressValue(s, now);
                s.progressFrom = s.progressValue;
                s.progressTo = s.progressTarget;
                s.progressAnimStartMs = now;
                s.progressAnimDurationMs = kProgressTransitionMs;
            }

            if (s.progressValue != s.progressTo)
            {
                s.progressValue = sampleProgressValue(s, now);
                if ((now - s.progressAnimStartMs) >= s.progressAnimDurationMs)
                {
                    s.progressValue = s.progressTo;
                    s.progressFrom = s.progressTo;
                }
            }
            else
            {
                s.progressFrom = s.progressTo;
            }
        }
        s.lastUpdateMs = now;
    }

    void GUI::drawButton(const String &label,
                         int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t baseColor,
                         uint8_t radius,
                         IconId iconId,
                         detail::ButtonState &state)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateButton(label, x, y, w, h, baseColor, radius, iconId, state);
            return;
        }

        if (!getDrawTarget())
            return;

        const uint32_t now = nowMs();
        const uint8_t pressLevel = samplePressLevel(state, now);
        const ButtonFrame frame = resolveButtonFrame(x, y, w, h, radius, pressLevel, _render.screenWidth, _render.screenHeight);

        const uint16_t surfaceBg = _render.bgColor565;
        const uint16_t pressedBg = static_cast<uint16_t>(detail::blend565(surfaceBg, baseColor, 190));
        const uint16_t disabledBg = static_cast<uint16_t>(detail::blend565(surfaceBg, baseColor, 76));
        const uint16_t activeBg = pressLevel > 0 ? pressedBg : baseColor;

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

        if (state.progressEnabled)
        {
            const uint16_t progressFill = static_cast<uint16_t>(detail::blend565(bg, state.progressFillColor, state.fadeLevel));
            const int16_t progressW = progressWidthForValue(frame.w, state.progressValue);
            if (progressW > 0)
                drawSquircleRect().pos(frame.x, frame.y).size(progressW, frame.h).radius({(uint8_t)frame.radius}).fill(progressFill);
        }

        const uint16_t fgActive = detail::autoTextColor(bg, 140);
        const uint16_t fg = resolveButtonTextColor(bg, state, fgActive);

        const String displayLabel = loadingLabel(label, state.loading, now);
        const String layoutLabel = state.loading ? (label + "...") : displayLabel;
        if (state.loading || state.progressEnabled)
            syncButtonLabelDirect(state, state.loading ? layoutLabel : displayLabel);
        else
            syncButtonLabel(state, displayLabel, now);
        const float labelBlend = buttonLabelTransition(state, now);
        const bool loadingDotsOnly = state.loading || state.progressEnabled;
        const String &labelToDraw = loadingDotsOnly ? displayLabel : state.currentLabel;
        const String &labelToFadeOut = state.previousLabel;
        const bool hasPrevText = !loadingDotsOnly && labelToFadeOut.length() > 0;
        const bool hasText = labelToDraw.length() > 0 || hasPrevText;
        const bool hasIcon = (iconId != static_cast<IconId>(0xFFFF) && iconId < psdf_icons::IconCount);

        constexpr int16_t paddingX = 6;
        constexpr int16_t paddingY = 2;
        const int16_t baseMaxW = (w - paddingX * 2) > 4 ? static_cast<int16_t>(w - paddingX * 2) : 4;
        const int16_t baseMaxH = (h - paddingY * 2) > 4 ? static_cast<int16_t>(h - paddingY * 2) : 4;

        uint16_t basePx = (h * 0.55f) > 8 ? static_cast<uint16_t>(h * 0.55f) : 8;
        const uint16_t baseMaxPx = (h * 0.8f) > 8 ? static_cast<uint16_t>(h * 0.8f) : 8;
        if (basePx > baseMaxPx)
            basePx = baseMaxPx;

        int16_t tw = 0;
        int16_t th = 0;
        int16_t prevTw = 0;
        int16_t prevTh = 0;
        const uint16_t baseIconSize = hasIcon ? resolveButtonIconSize(h, hasText) : 0;
        const uint16_t resolvedIconSize = hasIcon ? static_cast<uint16_t>(max<int16_t>(6, static_cast<int16_t>(baseIconSize * frame.scale + 0.5f))) : 0;
        const int16_t baseIconGap = (hasIcon && hasText) ? static_cast<int16_t>(baseIconSize > 16 ? 7 : 5) : 0;
        const int16_t iconGap = (hasIcon && hasText) ? static_cast<int16_t>(max<int16_t>(4, static_cast<int16_t>(baseIconGap * frame.scale + 0.5f))) : 0;
        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();

        const auto fitLabel = [&](uint16_t &sizePx, int16_t &outW, int16_t &outH, const String &text, int16_t availW, int16_t availH)
        {
            setFontWeight(Medium);
            setFontSize(sizePx);
            measureText(text, outW, outH);

            if (outW <= availW && outH <= availH)
                return;

            const float scaleX = outW > 0 ? (static_cast<float>((availW > 4) ? availW : 4) / static_cast<float>(outW)) : 1.0f;
            const float scaleY = outH > 0 ? (static_cast<float>(availH) / static_cast<float>(outH)) : 1.0f;
            const float scale = scaleX < scaleY ? scaleX : scaleY;
            if (scale >= 1.0f)
                return;

            const uint16_t nextPx = (sizePx * scale) > 7.0f ? static_cast<uint16_t>(sizePx * scale) : 7;
            if (nextPx == sizePx)
                return;

            sizePx = nextPx;
            setFontSize(sizePx);
            measureText(text, outW, outH);
        };

        if (hasText)
        {
            const String &fitSource = (labelToDraw.length() >= labelToFadeOut.length()) ? labelToDraw : labelToFadeOut;
            const int16_t fitAvailW = hasIcon && hasText ? static_cast<int16_t>(baseMaxW - baseIconSize - iconGap) : baseMaxW;
            fitLabel(basePx, tw, th, fitSource, fitAvailW, baseMaxH);
            uint16_t drawPx = static_cast<uint16_t>(basePx * frame.scale + 0.5f);
            if (drawPx < 7)
                drawPx = 7;
            setFontWeight(Medium);
            setFontSize(drawPx);
            if (labelToDraw.length() > 0)
                measureText(labelToDraw, tw, th);
            if (hasPrevText)
                measureText(labelToFadeOut, prevTw, prevTh);
            basePx = drawPx;
        }

        const int16_t iconY = static_cast<int16_t>(frame.centerY - resolvedIconSize / 2);
        const int16_t textH = (th > prevTh) ? th : prevTh;
        int16_t textY = static_cast<int16_t>(frame.centerY - textH / 2);
        if (textY < frame.y)
            textY = frame.y;

        if (hasIcon && !hasText)
        {
            const int16_t iconX = centerExtent(frame.centerX, resolvedIconSize);
            drawIconInternal(iconId, iconX, iconY, resolvedIconSize, fg);
        }
        else if (hasText)
        {
            const int16_t contentTextW = (tw > prevTw) ? tw : prevTw;
            const int16_t contentW = hasIcon ? static_cast<int16_t>(resolvedIconSize + iconGap + contentTextW) : contentTextW;
            const int16_t contentX = centerExtent(frame.centerX, contentW);

            if (hasIcon)
                drawIconInternal(iconId, contentX, iconY, resolvedIconSize, fg);

            setFontWeight(Medium);
            setFontSize(basePx);
            const int16_t textCenterX = hasIcon
                                            ? static_cast<int16_t>(contentX + resolvedIconSize + iconGap + lroundf(static_cast<float>(contentTextW) * 0.5f))
                                            : frame.centerX;
            const auto drawTransitionLabel = [&](const String &text, uint8_t alpha, int16_t blurOffset)
            {
                if (text.length() == 0 || alpha == 0)
                    return;

                const uint16_t mainColor = static_cast<uint16_t>(detail::blend565(bg, fg, alpha));
                drawTextAligned(text, textCenterX, textY, mainColor, bg, TextAlign::Center);
            };
            if (state.labelTransitionActive && !loadingDotsOnly)
            {
                const uint8_t currentAlpha = static_cast<uint8_t>(labelBlend * 255.0f + 0.5f);
                const uint8_t previousAlpha = static_cast<uint8_t>((1.0f - labelBlend) * 255.0f + 0.5f);
                drawTransitionLabel(labelToFadeOut, previousAlpha, 0);
                drawTransitionLabel(labelToDraw, currentAlpha, 0);
            }
            else
            {
                drawTextAligned(labelToDraw, textCenterX, textY, fg, bg, TextAlign::Center);
            }
        }
        setFontWeight(prevWeight);
        setFontSize(prevSize);
    }

    void GUI::updateButton(const String &label,
                           int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t baseColor,
                           uint8_t radius,
                           IconId iconId,
                           detail::ButtonState &state)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawButton(label, x, y, w, h, baseColor, radius, iconId, state);
            return;
        }

        const int16_t rx = resolveAxis(x, w, _render.screenWidth);
        const int16_t ry = resolveAxis(y, h, _render.screenHeight);
        constexpr int16_t pad = 3;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
            .fill(_render.bgColor565)
            .draw();
        drawButton(label, x, y, w, h, baseColor, radius, iconId, state);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
    }
}

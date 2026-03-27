#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint16_t kSliderAnimMs = 240;
        constexpr uint16_t kSliderEnabledAnimMs = 160;
        constexpr uint16_t kSliderHoldDelayMs = 420;
        constexpr uint16_t kSliderRepeatMs = 92;
        constexpr uint16_t kSliderRepeatFastMs = 36;
        constexpr uint16_t kSliderRepeatRampMs = 1200;
        constexpr int16_t kSliderUpdatePad = 5;

        [[nodiscard]] inline uint32_t elapsedMs(uint32_t now, uint32_t then) noexcept
        {
            return (now >= then) ? (now - then) : 0U;
        }

        [[nodiscard]] inline uint8_t lerpByte(uint8_t from, uint8_t to, float t) noexcept
        {
            if (from == to)
                return from;
            const float value = (float)from + ((float)to - (float)from) * t;
            int16_t rounded = (int16_t)(value + 0.5f);
            if (rounded < 0)
                rounded = 0;
            if (rounded > 255)
                rounded = 255;
            return (uint8_t)rounded;
        }

        [[nodiscard]] inline uint8_t sliderTarget(int16_t value, int16_t minValue, int16_t maxValue) noexcept
        {
            if (maxValue <= minValue)
                return 0;
            if (value <= minValue)
                return 0;
            if (value >= maxValue)
                return 255;
            const int32_t span = (int32_t)maxValue - minValue;
            const int32_t pos = (int32_t)value - minValue;
            return static_cast<uint8_t>((pos * 255 + (span / 2)) / span);
        }

        [[nodiscard]] inline uint16_t resolveSliderInactiveTrack(uint16_t bg565, uint16_t active565, int32_t inactiveColor) noexcept
        {
            if (inactiveColor >= 0)
                return (uint16_t)inactiveColor;
            return detail::blend565(bg565, active565, 44);
        }

        [[nodiscard]] inline uint16_t resolveSliderThumb(uint16_t active565, int32_t thumbColor) noexcept
        {
            if (thumbColor >= 0)
                return (uint16_t)thumbColor;
            return 0xFFFF;
        }

        [[nodiscard]] int16_t clampValueToRange(int16_t value, int16_t minValue, int16_t maxValue) noexcept
        {
            if (value < minValue)
                return minValue;
            if (value > maxValue)
                return maxValue;
            return value;
        }

        [[nodiscard]] inline uint16_t sliderRepeatInterval(uint32_t heldMs) noexcept
        {
            if (heldMs == 0)
                return kSliderRepeatMs;
            const float t = detail::motion::clamp01((float)heldMs / (float)kSliderRepeatRampMs);
            const float eased = detail::motion::easeOutCubic(t);
            const float interval = (float)kSliderRepeatMs + ((float)kSliderRepeatFastMs - (float)kSliderRepeatMs) * eased;
            return (uint16_t)(interval + 0.5f);
        }

        [[nodiscard]] inline int16_t sliderHoldMultiplier(uint32_t heldMs) noexcept
        {
            if (heldMs < 700U)
                return 1;
            if (heldMs < 1200U)
                return 2;
            if (heldMs < 1800U)
                return 3;
            return 4;
        }
    }

    bool GUI::stepSliderState(detail::SliderState &state, int16_t &value, bool nextDown, bool prevDown)
    {
        const uint32_t now = nowMs();
        bool changed = false;

        if (state.maxValue < state.minValue)
        {
            const int16_t tmp = state.minValue;
            state.minValue = state.maxValue;
            state.maxValue = tmp;
        }
        if (state.step <= 0)
            state.step = 1;

        value = clampValueToRange(value, state.minValue, state.maxValue);

        if (!state.initialized)
        {
            const uint8_t target = sliderTarget(value, state.minValue, state.maxValue);
            const uint8_t enabledTarget = state.enabled ? 255U : 0U;
            state.initialized = true;
            state.value = value;
            state.pos = target;
            state.animFrom = target;
            state.animTo = target;
            state.animStartMs = now;
            state.animDurationMs = kSliderAnimMs;
            state.enabledLevel = enabledTarget;
            state.enabledAnimFrom = enabledTarget;
            state.enabledAnimTo = enabledTarget;
            state.enabledAnimStartMs = now;
            state.lastNextDown = nextDown;
            state.lastPrevDown = prevDown;
            return false;
        }

        const uint8_t enabledTarget = state.enabled ? 255U : 0U;
        if (enabledTarget != state.enabledAnimTo)
        {
            state.enabledAnimFrom = state.enabledLevel;
            state.enabledAnimTo = enabledTarget;
            state.enabledAnimStartMs = now;
            changed = true;
        }

        auto applyDelta = [&](int8_t dir, int16_t multiplier = 1)
        {
            const int16_t step = state.step > 0 ? state.step : 1;
            const int32_t raw = (int32_t)value + (int32_t)dir * step * multiplier;
            const int16_t clamped = clampValueToRange((int16_t)raw, state.minValue, state.maxValue);
            if (clamped != value)
            {
                value = clamped;
                changed = true;
            }
        };

        if (state.enabled)
        {
            if (nextDown)
            {
                if (!state.lastNextDown)
                {
                    applyDelta(+1);
                    state.nextHoldStartMs = now;
                    state.nextRepeatMs = now + kSliderHoldDelayMs;
                }
                else if (state.nextRepeatMs != 0 && now >= state.nextRepeatMs)
                {
                    const uint32_t heldMs = elapsedMs(now, state.nextHoldStartMs);
                    applyDelta(+1, sliderHoldMultiplier(heldMs));
                    state.nextRepeatMs = now + sliderRepeatInterval(heldMs);
                }
            }
            else
            {
                state.nextHoldStartMs = 0;
                state.nextRepeatMs = 0;
            }

            if (prevDown)
            {
                if (!state.lastPrevDown)
                {
                    applyDelta(-1);
                    state.prevHoldStartMs = now;
                    state.prevRepeatMs = now + kSliderHoldDelayMs;
                }
                else if (state.prevRepeatMs != 0 && now >= state.prevRepeatMs)
                {
                    const uint32_t heldMs = elapsedMs(now, state.prevHoldStartMs);
                    applyDelta(-1, sliderHoldMultiplier(heldMs));
                    state.prevRepeatMs = now + sliderRepeatInterval(heldMs);
                }
            }
            else
            {
                state.prevHoldStartMs = 0;
                state.prevRepeatMs = 0;
            }
        }
        else
        {
            state.nextHoldStartMs = 0;
            state.prevHoldStartMs = 0;
            state.nextRepeatMs = 0;
            state.prevRepeatMs = 0;
        }

        state.lastNextDown = nextDown;
        state.lastPrevDown = prevDown;

        if (value != state.value)
        {
            state.value = value;
            state.animFrom = state.pos;
            state.animTo = sliderTarget(value, state.minValue, state.maxValue);
            state.animStartMs = now;
            state.animDurationMs = kSliderAnimMs;
            changed = true;
        }

        if (state.pos != state.animTo)
        {
            const float t = detail::motion::clamp01((float)elapsedMs(now, state.animStartMs) / (float)(state.animDurationMs ? state.animDurationMs : 1));
            const float eased = detail::motion::cubicBezierEase(t, 0.22f, 1.0f, 0.30f, 1.0f);
            const uint8_t nextPos = lerpByte(state.animFrom, state.animTo, eased);
            if (nextPos != state.pos)
            {
                state.pos = nextPos;
                changed = true;
            }
            if (t >= 1.0f)
            {
                state.pos = state.animTo;
                state.animFrom = state.animTo;
            }
        }
        else
        {
            state.animFrom = state.animTo;
        }

        if (state.enabledLevel != state.enabledAnimTo)
        {
            const float t = detail::motion::clamp01((float)elapsedMs(now, state.enabledAnimStartMs) / (float)kSliderEnabledAnimMs);
            const float eased = detail::motion::easeInOutCubic(t);
            const uint8_t nextLevel = lerpByte(state.enabledAnimFrom, state.enabledAnimTo, eased);
            if (nextLevel != state.enabledLevel)
            {
                state.enabledLevel = nextLevel;
                changed = true;
            }
            if (t >= 1.0f)
            {
                state.enabledLevel = state.enabledAnimTo;
                state.enabledAnimFrom = state.enabledAnimTo;
            }
        }
        else
        {
            state.enabledAnimFrom = state.enabledAnimTo;
        }

        return changed;
    }

    void GUI::drawSlider(int16_t x, int16_t y, int16_t w, int16_t h,
                         detail::SliderState &state,
                         uint16_t activeColor,
                         int32_t inactiveColor,
                         int32_t thumbColor)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateSlider(x, y, w, h, state, activeColor, inactiveColor, thumbColor);
            return;
        }

        int16_t x0 = x;
        int16_t y0 = y;
        if (x0 == center)
            x0 = AutoX(w);
        if (y0 == center)
            y0 = AutoY(h);

        const uint16_t bg565 = _render.bgColor565;
        const uint16_t trackInactive565 = resolveSliderInactiveTrack(bg565, activeColor, inactiveColor);
        const uint16_t thumbBase565 = resolveSliderThumb(activeColor, thumbColor);
        const float k = (float)state.pos / 255.0f;

        int16_t trackH = h / 4;
        if (trackH < 5)
            trackH = 5;
        if (trackH > h)
            trackH = h;
        const int16_t trackY = (int16_t)(y0 + (h - trackH) / 2);
        const uint8_t trackRadius = (uint8_t)(trackH / 2);

        int16_t thumbW = (int16_t)(h * 1.58f + 0.5f);
        if (thumbW < trackH + 10)
            thumbW = trackH + 10;
        if (thumbW > w)
            thumbW = w;
        int16_t thumbH = (int16_t)(h * 0.72f + 0.5f);
        if (thumbH < trackH + 2)
            thumbH = trackH + 2;
        if (thumbH > h)
            thumbH = h;
        const uint8_t thumbRadius = (uint8_t)(thumbH / 2);

        const int16_t thumbTravel = w - thumbW;
        const int16_t thumbX = x0 + (thumbTravel > 0 ? (int16_t)(thumbTravel * k + 0.5f) : 0);
        const int16_t thumbY = (int16_t)(y0 + (h - thumbH) / 2);
        const int16_t thumbCenterX = thumbX + thumbW / 2;
        int16_t fillW = thumbCenterX - x0;
        if (fillW < 0)
            fillW = 0;
        if (fillW > w)
            fillW = w;

        const uint8_t enabledAlpha = state.enabledLevel;
        const uint16_t inactive565 = detail::blend565(bg565, trackInactive565, enabledAlpha);
        const uint16_t fill565 = detail::blend565(bg565, activeColor, enabledAlpha);
        const uint16_t thumb565 = detail::blend565(bg565, thumbBase565, enabledAlpha);
        const uint16_t shadow565 = detail::blend565(bg565, static_cast<uint16_t>(0x0000), (uint8_t)(enabledAlpha / 4));
        const uint16_t border565 = detail::blend565(thumb565, static_cast<uint16_t>(0x0000), 22);

        drawLinearProgressRange(x0, trackY, w, trackH, x0, (int16_t)(x0 + fillW), inactive565, fill565, trackRadius);

        const int16_t shadowX = (int16_t)(thumbX + 1);
        const int16_t shadowY = (int16_t)(thumbY + 1);
        const int16_t shadowW = thumbW > 1 ? (int16_t)(thumbW - 1) : thumbW;
        const int16_t shadowH = thumbH > 1 ? (int16_t)(thumbH - 1) : thumbH;
        if (shadowW > 0 && shadowH > 0)
            fillRoundRect(shadowX, shadowY, shadowW, shadowH, (uint8_t)(shadowH / 2), shadow565);
        fillRoundRect(thumbX, thumbY, thumbW, thumbH, thumbRadius, border565);
        fillRoundRect((int16_t)(thumbX + 1), (int16_t)(thumbY + 1), (int16_t)(thumbW - 2), (int16_t)(thumbH - 2),
                      (uint8_t)((thumbRadius > 1) ? (thumbRadius - 1) : thumbRadius), thumb565);
    }

    void GUI::updateSlider(int16_t x, int16_t y, int16_t w, int16_t h,
                           detail::SliderState &state,
                           uint16_t activeColor,
                           int32_t inactiveColor,
                           int32_t thumbColor)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawSlider(x, y, w, h, state, activeColor, inactiveColor, thumbColor);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;
        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *const prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect()
            .pos((int16_t)(rx - kSliderUpdatePad), (int16_t)(ry - kSliderUpdatePad))
            .size((int16_t)(w + kSliderUpdatePad * 2), (int16_t)(h + kSliderUpdatePad * 2))
            .fill(_render.bgColor565);
        drawSlider(x, y, w, h, state, activeColor, inactiveColor, thumbColor);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - kSliderUpdatePad),
                           (int16_t)(ry - kSliderUpdatePad),
                           (int16_t)(w + kSliderUpdatePad * 2),
                           (int16_t)(h + kSliderUpdatePad * 2));
        }
    }
}

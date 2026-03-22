#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint16_t kToggleAnimMs = 200;
        constexpr uint16_t kToggleEnabledAnimMs = 150;
        constexpr int16_t kToggleUpdatePad = 3;

        [[nodiscard]] inline uint8_t toggleTarget(bool value) noexcept
        {
            return value ? 255U : 0U;
        }

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

        [[nodiscard]] inline uint16_t resolveInactiveTrackColor(uint16_t bg565,
                                                                uint16_t activeColor,
                                                                int32_t inactiveColor) noexcept
        {
            if (inactiveColor >= 0)
                return (uint16_t)inactiveColor;
            return detail::blend565(bg565, activeColor, 40);
        }

        [[nodiscard]] inline uint16_t resolveKnobFillColor(uint16_t track565,
                                                           int32_t knobColor) noexcept
        {
            if (knobColor >= 0)
                return (uint16_t)knobColor;
            return detail::blend565WithWhite(track565, 220);
        }
    }

    bool GUI::stepToggleState(detail::ToggleState &state, bool &value, bool pressed)
    {
        const uint32_t now = nowMs();
        bool changed = false;

        if (!state.initialized)
        {
            const uint8_t target = toggleTarget(value);
            const uint8_t enabledTarget = state.enabled ? 255U : 0U;
            state.initialized = true;
            state.value = value;
            state.pos = target;
            state.animFrom = target;
            state.animTo = target;
            state.animStartMs = now;
            state.enabledLevel = enabledTarget;
            state.enabledAnimFrom = enabledTarget;
            state.enabledAnimTo = enabledTarget;
            state.enabledAnimStartMs = now;
        }

        const uint8_t enabledTarget = state.enabled ? 255U : 0U;
        if (enabledTarget != state.enabledAnimTo)
        {
            state.enabledAnimFrom = state.enabledLevel;
            state.enabledAnimTo = enabledTarget;
            state.enabledAnimStartMs = now;
            changed = true;
        }

        if (pressed && state.enabled)
        {
            value = !value;
            changed = true;
        }

        if (value != state.value)
        {
            state.value = value;
            state.animFrom = state.pos;
            state.animTo = toggleTarget(value);
            state.animStartMs = now;
            changed = true;
        }

        if (state.pos != state.animTo)
        {
            const float t = detail::motion::clamp01((float)elapsedMs(now, state.animStartMs) / (float)kToggleAnimMs);
            const float eased = detail::motion::easeInOutCubic(t);
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
            const float t = detail::motion::clamp01((float)elapsedMs(now, state.enabledAnimStartMs) / (float)kToggleEnabledAnimMs);
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

    void GUI::drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                               detail::ToggleState &state,
                               uint16_t activeColor,
                               int32_t inactiveColor,
                               int32_t knobColor)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            return;
        }

        int16_t x0 = x;
        int16_t y0 = y;

        if (x0 == center)
        {
            int16_t availW = _render.screenWidth;
            if (availW < w)
                availW = w;
            x0 = (availW - w) / 2;
        }

        if (y0 == center)
        {
            int16_t top = 0;
            int16_t availH = _render.screenHeight;
            const int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0)
            {
                if (_status.pos == Top)
                {
                    top += sb;
                    availH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    availH -= sb;
                }
            }

            if (availH < h)
                availH = h;
            y0 = top + (availH - h) / 2;
        }

        const uint8_t r = (uint8_t)(h / 2);
        const uint16_t bg565 = _render.bgColor565;
        const uint16_t inactive565 = resolveInactiveTrackColor(bg565, activeColor, inactiveColor);
        const float k = (float)state.pos / 255.0f;
        uint16_t track565 = detail::lerp565(inactive565, activeColor, k);

        int16_t pad = h / 6;
        if (pad < 4)
            pad = 4;

        int16_t knobR = (h / 2) - pad;
        if (knobR < 2)
            knobR = 2;

        const int16_t leftCx = x0 + pad + knobR;
        int16_t rightCx = x0 + w - pad - knobR;
        if (rightCx < leftCx)
            rightCx = leftCx;

        const int16_t cx = leftCx + (int16_t)((rightCx - leftCx) * k + 0.5f);
        const int16_t cy = y0 + h / 2;
        uint16_t knob565 = resolveKnobFillColor(track565, knobColor);
        const uint8_t trackAlpha = lerpByte(68, 255, (float)state.enabledLevel / 255.0f);
        const uint8_t knobAlpha = lerpByte(84, 255, (float)state.enabledLevel / 255.0f);
        track565 = detail::blend565(bg565, track565, trackAlpha);
        knob565 = detail::blend565(bg565, knob565, knobAlpha);
        fillRoundRect(x0, y0, w, h, r, track565);
        const uint16_t border565 = detail::blend565(track565, knob565, 80);

        fillCircle(cx, cy, knobR, border565);
        int16_t innerR = knobR - 1;
        if (innerR < 1)
            innerR = 1;
        fillCircle(cx, cy, innerR, knob565);
    }

    void GUI::updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                 detail::ToggleState &state,
                                 uint16_t activeColor,
                                 int32_t inactiveColor,
                                 int32_t knobColor)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            return;
        }

        if (!state.initialized)
        {
            const uint8_t target = toggleTarget(state.value);
            state.initialized = true;
            state.pos = target;
            state.animFrom = target;
            state.animTo = target;
            state.animStartMs = nowMs();
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

        fillRect()
            .pos((int16_t)(rx - kToggleUpdatePad), (int16_t)(ry - kToggleUpdatePad))
            .size((int16_t)(w + kToggleUpdatePad * 2), (int16_t)(h + kToggleUpdatePad * 2))
            .color(_render.bgColor565);
        drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - kToggleUpdatePad),
                           (int16_t)(ry - kToggleUpdatePad),
                           (int16_t)(w + kToggleUpdatePad * 2),
                           (int16_t)(h + kToggleUpdatePad * 2));
            flushDirty();
        }
    }
}

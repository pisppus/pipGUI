#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>

namespace pipgui
{
    bool GUI::updateToggleSwitch(ToggleSwitchState &s, bool pressed)
    {
        bool changed = false;
        if (pressed)
        {
            s.value = !s.value;
            changed = true;
        }

        uint32_t now = nowMs();
        uint32_t last = s.lastUpdateMs;
        uint32_t dt = 0;

        if (last == 0 || now <= last)
        {
            uint8_t target = s.value ? 255U : 0U;
            if (s.pos != target)
            {
                s.pos = target;
                changed = true;
            }
            s.lastUpdateMs = now;
        }
        else
        {
            dt = now - last;
            if (dt > 100U)
                dt = 100U;
        }

        uint8_t target = s.value ? 255U : 0U;

        if (dt > 0 && s.pos != target)
        {
            float speed = 900.0f;
            uint8_t step = (uint8_t)(speed * (float)dt / 1000.0f + 0.5f);
            if (step < 1)
                step = 1;

            if (s.pos < target)
            {
                uint16_t nv = (uint16_t)s.pos + step;
                s.pos = (nv >= target) ? target : (uint8_t)nv;
            }
            else
            {
                s.pos = (s.pos > step) ? (uint8_t)(s.pos - step) : 0U;
                if (s.pos < target)
                    s.pos = target;
            }

            changed = true;
        }

        s.lastUpdateMs = now;
        return changed;
    }

    void GUI::drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                               ToggleSwitchState &state,
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

        auto t = getDrawTarget();

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
            int16_t sb = statusBarHeight();
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

        uint8_t r = (uint8_t)(h / 2);

        uint16_t inactive;
        if (inactiveColor < 0)
            inactive = detail::lerp565(activeColor, (uint16_t)0x7BEF, 0.75f);
        else
            inactive = (uint16_t)inactiveColor;

        const float k = (float)state.pos / 255.0f;
        const float eased = detail::motion::cubicBezier1D(k, 0.08f, 1.00f);

        const uint16_t bg = detail::lerp565(inactive, activeColor, eased);
        fillRoundRect(x0, y0, w, h, r, bg);

        int16_t pad = (int16_t)max((int16_t)3, (int16_t)(h / 8));
        int16_t knobR = (h / 2) - pad;
        if (knobR < 2)
            knobR = 2;

        int16_t leftCx = x0 + pad + knobR;
        int16_t rightCx = x0 + w - pad - knobR;
        if (rightCx < leftCx)
            rightCx = leftCx;

        int16_t cx = leftCx + (int16_t)((rightCx - leftCx) * eased + 0.5f);
        int16_t cy = y0 + h / 2;

        uint16_t knob;
        if (knobColor < 0)
            knob = detail::autoTextColor(bg, 140);
        else
            knob = (uint16_t)knobColor;

        const uint16_t border = detail::lerp565(knob, bg, 0.55f);
        fillCircle(cx, cy, knobR, border);
        int16_t innerR = knobR - 2;
        if (innerR < 1)
            innerR = knobR - 1;
        if (innerR < 1)
            innerR = 1;
        fillCircle(cx, cy, innerR, knob);
    }

    void GUI::updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                 ToggleSwitchState &state,
                                 uint16_t activeColor,
                                 int32_t inactiveColor,
                                 int32_t knobColor)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            return;
        }

        if (state.lastUpdateMs == 0)
            state.pos = state.value ? 255U : 0U;

        int16_t rx = x;
        int16_t ry = y;

        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t pad = 2;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
            .color((uint16_t)detail::color888To565(_render.bgColor))
            .draw();
        drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
            flushDirty();
        }
    }
}

#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{
    static float cubicBezier1D(float t, float p1, float p2)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;

        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        return (3.0f * uu * t * p1) + (3.0f * u * tt * p2) + (tt * t);
    }

    static float easeBezierSw(float t)
    {
        return cubicBezier1D(t, 0.08f, 1.00f);
    }

    static uint16_t lerpColor565Sw(uint16_t c0, uint16_t c1, float t)
    {
        if (t <= 0.0f)
            return c0;
        if (t >= 1.0f)
            return c1;

        uint8_t r0 = (c0 >> 11) & 0x1F;
        uint8_t g0 = (c0 >> 5) & 0x3F;
        uint8_t b0 = c0 & 0x1F;

        uint8_t r1 = (c1 >> 11) & 0x1F;
        uint8_t g1 = (c1 >> 5) & 0x3F;
        uint8_t b1 = c1 & 0x1F;

        uint8_t r = (uint8_t)(r0 + (r1 - r0) * t + 0.5f);
        uint8_t g = (uint8_t)(g0 + (g1 - g0) * t + 0.5f);
        uint8_t b = (uint8_t)(b0 + (b1 - b0) * t + 0.5f);

        return (uint16_t)((r << 11) | (g << 5) | b);
    }

    static uint16_t autoKnobColor(uint16_t bg)
    {
        uint8_t r5 = (bg >> 11) & 0x1F;
        uint8_t g6 = (bg >> 5) & 0x3F;
        uint8_t b5 = bg & 0x1F;

        uint16_t r8 = (uint16_t)((r5 * 255U) / 31U);
        uint16_t g8 = (uint16_t)((g6 * 255U) / 63U);
        uint16_t b8 = (uint16_t)((b5 * 255U) / 31U);

        uint16_t lum = (uint16_t)((r8 * 30U + g8 * 59U + b8 * 11U) / 100U);
        return (lum > 140U) ? 0x0000 : 0xFFFF;
    }

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
                               uint32_t activeColor,
                               int32_t inactiveColor,
                               int32_t knobColor)
    {
        auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };
        uint16_t active565 = to565(activeColor);
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
        {
            updateToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            return;
        }

        auto t = getDrawTarget();

        int16_t x0 = x;
        int16_t y0 = y;

        if (x0 == center)
        {
            int16_t left = 0;
            int16_t availW = _screenWidth;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Left)
                {
                    left += _statusBarHeight;
                    availW -= _statusBarHeight;
                }
                else if (_statusBarPos == Right)
                {
                    availW -= _statusBarHeight;
                }
            }
            if (availW < w)
                availW = w;
            x0 = left + (availW - w) / 2;
        }
        if (y0 == center)
        {
            int16_t top = 0;
            int16_t availH = _screenHeight;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Top)
                {
                    top += _statusBarHeight;
                    availH -= _statusBarHeight;
                }
                else if (_statusBarPos == Bottom)
                {
                    availH -= _statusBarHeight;
                }
            }
            if (availH < h)
                availH = h;
            y0 = top + (availH - h) / 2;
        }

        uint8_t r = (uint8_t)(h / 2);

        uint16_t inactive;
        if (inactiveColor < 0)
        {
            inactive = lerpColor565Sw(active565, 0x8410, 0.75f);
        }
        else
        {
            inactive = (uint16_t)inactiveColor;
        }

        float k = (float)state.pos / 255.0f;
        float eased = easeBezierSw(k);

        uint16_t bg = lerpColor565Sw(inactive, active565, eased);
        t->fillRoundRect(x0, y0, w, h, r, bg);

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
        {
            knob = autoKnobColor(bg);
        }
        else
        {
            knob = (uint16_t)knobColor;
        }

        uint16_t border = lerpColor565Sw(knob, bg, 0.55f);
        t->fillCircle(cx, cy, knobR, border);
        int16_t innerR = knobR - 2;
        if (innerR < 1)
            innerR = knobR - 1;
        if (innerR < 1)
            innerR = 1;
        t->fillCircle(cx, cy, innerR, knob);
    }

    void GUI::updateToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h,
                                 ToggleSwitchState &state,
                                 uint32_t activeColor,
                                 int32_t inactiveColor,
                                 int32_t knobColor)
    {
        if (!_flags.spriteEnabled || !_display)
        {
            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        int16_t rx = x;
        int16_t ry = y;

        if (rx == center)
            rx = AutoX(w);
        if (ry == center)
            ry = AutoY(h);

        int16_t pad = 2;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2), _bgColor);
        drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
        flushDirty();
    }
}

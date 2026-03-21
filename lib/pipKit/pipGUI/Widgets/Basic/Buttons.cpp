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
    }

    void GUI::updateButtonPress(ButtonVisualState &s, bool isDown)
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
                         const ButtonVisualState &state)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateButton(label, x, y, w, h, baseColor, radius, state);
            return;
        }

        auto t = getDrawTarget();
        if (!t)
            return;

        const int16_t x0 = resolveAxis(x, w, _render.screenWidth);
        const int16_t y0 = resolveAxis(y, h, _render.screenHeight);

        float sc = 1.0f - kPressedScale * (state.pressLevel / 255.0f);
        if (sc < kMinScale)
            sc = kMinScale;

        const int16_t bw = (w * sc) > 4 ? static_cast<int16_t>(w * sc) : 4;
        const int16_t bh = (h * sc) > 4 ? static_cast<int16_t>(h * sc) : 4;
        int16_t bx = x0 + (w - bw) / 2;
        int16_t by = y0 + (h - bh) / 2;

        int16_t br = (int16_t)(radius * sc + 0.5f);
        if (br < 1)
            br = 1;

        const uint16_t pressedBg = static_cast<uint16_t>(detail::blend565(baseColor, static_cast<uint16_t>(0x0000), 56));
        const uint16_t disabledBg = static_cast<uint16_t>(detail::blend565(baseColor, static_cast<uint16_t>(0x7BEF), 160));
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

        fillRoundRect(bx, by, bw, bh, (uint8_t)br, bg);

        const uint16_t fgActive = detail::autoTextColor(bg, 140);
        const bool disabledVis = !state.enabled || state.fadeLevel < 40;
        const uint16_t fg = disabledVis
                                ? (fgActive == 0xFFFF ? static_cast<uint16_t>(0x8410) : static_cast<uint16_t>(0x630C))
                                : fgActive;

        const String labelToDraw = loadingLabel(label, state.loading, nowMs());

        constexpr int16_t paddingX = 6;
        constexpr int16_t paddingY = 2;
        const int16_t maxW = (bw - paddingX * 2) > 4 ? static_cast<int16_t>(bw - paddingX * 2) : 4;
        const int16_t maxH = (bh - paddingY * 2) > 4 ? static_cast<int16_t>(bh - paddingY * 2) : 4;

        uint16_t px = (bh * 0.55f) > 8 ? static_cast<uint16_t>(bh * 0.55f) : 8;
        const uint16_t maxPx = (bh * 0.8f) > 8 ? static_cast<uint16_t>(bh * 0.8f) : 8;
        if (px > maxPx)
            px = maxPx;

        int16_t tw = 0;
        int16_t th = 0;
        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();

        const auto fitLabel = [&](uint16_t &sizePx, int16_t &outW, int16_t &outH)
        {
            setFontWeight(Medium);
            setFontSize(sizePx);
            measureText(labelToDraw, outW, outH);

            if (outW <= maxW && outH <= maxH)
                return;

            const float scaleX = outW > 0 ? (static_cast<float>(maxW) / static_cast<float>(outW)) : 1.0f;
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

        fitLabel(px, tw, th);

        int16_t ty = by + (bh - th) / 2;
        if (ty < by)
            ty = by;

        setFontWeight(Medium);
        setFontSize(px);
        drawTextAligned(labelToDraw, (int16_t)(bx + bw / 2), ty, fg, bg, TextAlign::Center);
        setFontWeight(prevWeight);
        setFontSize(prevSize);
    }

    void GUI::updateButton(const String &label,
                           int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t baseColor,
                           uint8_t radius,
                           const ButtonVisualState &state)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawButton(label, x, y, w, h, baseColor, radius, state);
            return;
        }

        int16_t rx = x;
        int16_t ry = y;

        rx = resolveAxis(rx, w, _render.screenWidth);
        ry = resolveAxis(ry, h, _render.screenHeight);

        int16_t pad = 2;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect()
            .pos((int16_t)(rx - pad), (int16_t)(ry - pad))
            .size((int16_t)(w + pad * 2), (int16_t)(h + pad * 2))
            .color(detail::color888To565(_render.bgColor))
            .draw();
        drawButton(label, x, y, w, h, baseColor, radius, state);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
        {
            invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
            flushDirty();
        }
    }
}

#include <pipGUI/core/api/pipGUI.h>

#include <pipGUI/fonts/WixMadeforDisplay-SemiBold.h>

namespace pipgui
{

    static OpenFontRender g_buttonFont;
    static bool g_buttonFontInited = false;

    static void ensureButtonFont()
    {
        if (g_buttonFontInited)
            return;

        pipcore::GuiPlatform *p = pipgui::GUI::sharedPlatform();
        if (p)
            p->configureOpenFontRender();

        FT_Error e = g_buttonFont.loadFont(gBuiltinWixSemiBoldTTF, sizeof(gBuiltinWixSemiBoldTTF), 0);
        if (!e)
        {
            g_buttonFont.setBackgroundFillMethod(BgFillMethod::None);
            g_buttonFontInited = true;
        }
    }

    static float easeOutCubicBtn(float t)
    {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    static uint16_t lerpColor565Btn(uint16_t c0, uint16_t c1, float t)
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

    void GUI::updateButtonPress(ButtonVisualState &s, bool isDown)
    {
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

        bool visEnabled = s.enabled && !s.loading;

        if (visEnabled != s.prevEnabled)
        {
            s.fadeLevel = visEnabled ? 0 : 255;
            s.prevEnabled = visEnabled;
        }

        uint8_t targetFade = visEnabled ? 255 : 0;

        if (dt > 0 && s.fadeLevel != targetFade)
        {
            float speed = 600.0f;
            uint8_t step = (uint8_t)(speed * (float)dt / 1000.0f + 0.5f);
            if (step < 1)
                step = 1;

            if (s.fadeLevel < targetFade)
            {
                uint16_t nv = (uint16_t)s.fadeLevel + step;
                s.fadeLevel = (nv > targetFade) ? targetFade : (uint8_t)nv;
            }
            else
            {
                if (s.fadeLevel > step)
                    s.fadeLevel = (uint8_t)(s.fadeLevel - step);
                else
                    s.fadeLevel = 0;
                if (s.fadeLevel < targetFade)
                    s.fadeLevel = targetFade;
            }
        }

        if (!visEnabled)
        {
            if (dt > 0 && s.pressLevel > 0)
            {
                float speedRelease = 1200.0f;
                uint8_t step = (uint8_t)(speedRelease * (float)dt / 1000.0f + 0.5f);
                if (step < 1)
                    step = 1;
                s.pressLevel = (s.pressLevel > step) ? (uint8_t)(s.pressLevel - step) : 0;
            }
            s.lastUpdateMs = now;
            return;
        }

        if (dt > 0)
        {
            if (isDown)
            {
                float speedPress = 1600.0f;
                uint8_t step = (uint8_t)(speedPress * (float)dt / 1000.0f + 0.5f);
                if (step < 1)
                    step = 1;
                uint16_t nv = (uint16_t)s.pressLevel + step;
                s.pressLevel = (nv > 255U) ? 255U : (uint8_t)nv;
            }
            else if (s.pressLevel > 0)
            {
                float speedRelease = 1400.0f;
                uint8_t step = (uint8_t)(speedRelease * (float)dt / 1000.0f + 0.5f);
                if (step < 1)
                    step = 1;
                s.pressLevel = (s.pressLevel > step) ? (uint8_t)(s.pressLevel - step) : 0;
            }
        }

        s.lastUpdateMs = now;
    }

    void GUI::drawButton(const String &label,
                         const uint16_t *iconBitmap, uint8_t iconW, uint8_t iconH,
                         int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t baseColor,
                         uint8_t radius,
                         const ButtonVisualState &state)
    {
        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            updateButton(label, iconBitmap, iconW, iconH, x, y, w, h, baseColor, radius, state);
            return;
        }

        auto t = getDrawTarget();

        int16_t x0 = x;
        int16_t y0 = y;

        if (x0 == center)
            x0 = (_screenWidth - w) / 2;
        if (y0 == center)
            y0 = (_screenHeight - h) / 2;

        float sc = 1.0f - 0.05f * (state.pressLevel / 255.0f);
        if (sc < 0.85f)
            sc = 0.85f;

        int16_t bw = (w * sc) > 4 ? (int16_t)(w * sc) : 4;
        int16_t bh = (h * sc) > 4 ? (int16_t)(h * sc) : 4;
        int16_t bx = x0 + (w - bw) / 2;
        int16_t by = y0 + (h - bh) / 2;

        int16_t br = (int16_t)(radius * sc + 0.5f);
        if (br < 1)
            br = 1;

        uint16_t mainColor = baseColor;
        uint16_t autoPressedBg = lerpColor565Btn(mainColor, 0x0000, 0.22f);
        uint16_t autoDisabledBg = lerpColor565Btn(mainColor, 0x8410, 0.6f);

        uint16_t activeBg = (state.pressLevel > 0 ? autoPressedBg : mainColor);
        uint16_t disabledBg = autoDisabledBg;

        uint16_t bg;

        if (state.fadeLevel == 0)
        {
            bg = disabledBg;
        }
        else if (state.fadeLevel >= 255)
        {
            bg = activeBg;
        }
        else
        {
            float k = (float)state.fadeLevel / 255.0f;
            float eased = easeOutCubicBtn(k);
            bg = lerpColor565Btn(disabledBg, activeBg, eased);
        }

        t->fillSmoothRoundRect(bx, by, bw, bh, br, bg);

        uint8_t r5 = (bg >> 11) & 0x1F;
        uint8_t g6 = (bg >> 5) & 0x3F;
        uint8_t b5 = bg & 0x1F;

        uint16_t r8 = (uint16_t)((r5 * 255U) / 31U);
        uint16_t g8 = (uint16_t)((g6 * 255U) / 63U);
        uint16_t b8 = (uint16_t)((b5 * 255U) / 31U);

        uint16_t lum = (uint16_t)((r8 * 30U + g8 * 59U + b8 * 11U) / 100U);
        uint16_t fgActive = (lum > 140U) ? 0x0000 : 0xFFFF;

        bool disabledVis = !state.enabled || (state.fadeLevel < 40);
        uint16_t fg;
        if (disabledVis)
        {
            fg = (fgActive == 0xFFFF) ? 0xC618 : 0x4208;
        }
        else
        {
            fg = fgActive;
        }

        String labelToDraw = label;
        if (state.loading)
        {
            uint32_t now = nowMs();
            const uint32_t stepMs = 300U;
            uint8_t phase = (uint8_t)((now / stepMs) % 6U);
            uint8_t dots = (phase <= 3U) ? phase : (uint8_t)(6U - phase);
            for (uint8_t i = 0; i < dots; ++i)
            {
                labelToDraw += '.';
            }
        }

        ensureButtonFont();

        bool hasIcon = (iconBitmap && iconW > 0 && iconH > 0);

        if (g_buttonFontInited)
        {
            g_buttonFont.setDrawer(*t);
            g_buttonFont.setFontColor(fg, bg);

            int16_t paddingX = 6;
            int16_t paddingY = 2;
            int16_t maxW = (bw - paddingX * 2) > 4 ? (int16_t)(bw - paddingX * 2) : 4;
            int16_t maxH = (bh - paddingY * 2) > 4 ? (int16_t)(bh - paddingY * 2) : 4;

            int16_t textMaxW = maxW;
            int16_t iconGap = hasIcon ? 4 : 0;
            if (hasIcon)
            {
                textMaxW = maxW - iconW - iconGap;
                if (textMaxW < 4)
                    textMaxW = 4;
            }

            uint16_t px = (bh * 0.55f) > 8 ? (uint16_t)(bh * 0.55f) : 8;
            uint16_t maxPx = (bh * 0.8f) > 8 ? (uint16_t)(bh * 0.8f) : 8;

            if (px > maxPx)
                px = maxPx;

            g_buttonFont.setFontSize(px);
            uint32_t tw = g_buttonFont.getTextWidth("%s", labelToDraw.c_str());
            uint32_t th = g_buttonFont.getTextHeight("%s", labelToDraw.c_str());

            if (tw > (uint32_t)textMaxW || th > (uint32_t)maxH)
            {
                float scaleX = (float)textMaxW / (float)tw;
                float scaleY = (float)maxH / (float)th;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;

                if (scale < 1.0f)
                {
                    uint16_t newPx = (px * scale) > 7 ? (uint16_t)(px * scale) : 7;

                    g_buttonFont.setFontSize(newPx);
                    tw = g_buttonFont.getTextWidth("%s", labelToDraw.c_str());
                    th = g_buttonFont.getTextHeight("%s", labelToDraw.c_str());
                }
            }

            int16_t contentW = (int16_t)tw;
            int16_t iconGap2 = hasIcon ? iconGap : 0;
            if (hasIcon)
                contentW += iconW + iconGap2;

            int16_t contentX = bx + (bw - contentW) / 2;

            int16_t tx = contentX;
            int16_t iconX = contentX;
            if (hasIcon)
            {
                iconX = contentX;
                tx = iconX + iconW + iconGap2;
            }

            int16_t ty = by + (bh - (int16_t)th) / 2;
            if (ty - 3 >= by)
                ty -= 3;

            if (hasIcon)
            {
                int16_t iconY = by + (bh - iconH) / 2;
                if (iconY < by)
                    iconY = by;
                t->pushImage(iconX, iconY, iconW, iconH, iconBitmap);
            }

            g_buttonFont.drawString(labelToDraw.c_str(), tx, ty, fg, bg, Layout::Horizontal);
        }
        else
        {
            t->setTextFont(2);
            t->setTextColor(fg, bg);

            int16_t tw = t->textWidth(labelToDraw);

            int16_t th = t->fontHeight();

            int16_t contentW = tw;
            int16_t iconGap = hasIcon ? 4 : 0;
            if (hasIcon)
            {
                contentW += iconW + iconGap;
            }

            int16_t contentX = bx + (bw - contentW) / 2;

            int16_t tx = contentX;
            int16_t iconX = contentX;
            if (hasIcon)
            {
                iconX = contentX;
                tx = iconX + iconW + iconGap;
            }

            int16_t ty0 = by + (bh - th) / 2;
            int16_t ty = ty0 > by ? ty0 : by;
            if (ty - 3 >= by)
                ty -= 3;

            if (hasIcon)
            {
                int16_t iconY = by + (bh - iconH) / 2;
                if (iconY < by)
                    iconY = by;
                t->pushImage(iconX, iconY, iconW, iconH, iconBitmap);
            }

            t->drawString(labelToDraw, tx, ty);
        }
    }

    void GUI::drawButton(const String &label,
                         int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t baseColor,
                         uint8_t radius,
                         const ButtonVisualState &state)
    {
        drawButton(label, nullptr, 0, 0, x, y, w, h, baseColor, radius, state);
    }

    void GUI::updateButton(const String &label,
                           const uint16_t *iconBitmap, uint8_t iconW, uint8_t iconH,
                           int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t baseColor,
                           uint8_t radius,
                           const ButtonVisualState &state)
    {
        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawButton(label, iconBitmap, iconW, iconH, x, y, w, h, baseColor, radius, state);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        int16_t rx = x;
        int16_t ry = y;

        if (rx == center)
            rx = (int16_t)((_screenWidth - w) / 2);
        if (ry == center)
            ry = (int16_t)((_screenHeight - h) / 2);

        int16_t pad = 2;

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        _sprite.fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2), _bgColor);
        drawButton(label, iconBitmap, iconW, iconH, x, y, w, h, baseColor, radius, state);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(w + pad * 2), (int16_t)(h + pad * 2));
        flushDirty();
    }

    void GUI::updateButton(const String &label,
                           int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t baseColor,
                           uint8_t radius,
                           const ButtonVisualState &state)
    {
        updateButton(label, nullptr, 0, 0, x, y, w, h, baseColor, radius, state);
    }
}
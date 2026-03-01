// Alert dialog overlay: modal with title, message and button (showNotification API).

#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{

    static float easeOutCubic(float t)
    {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    static uint16_t lerpColor565(uint16_t c0, uint16_t c1, float t)
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

    static inline uint32_t expand565(uint16_t c)
    {
        uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
        uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
        uint8_t b5 = (uint8_t)(c & 0x1F);
        uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
        uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
        uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
        return (uint32_t)((r8 << 16) | (g8 << 8) | b8);
    }

    void GUI::showNotification(const String &t, const String &m, const String &btn, uint16_t delay, NotificationType type)
    {
        _notif.title = t;
        _notif.message = m;
        _notif.buttonText = btn;
        _notif.type = type;
        uint32_t now = nowMs();
        if (!_flags.notifActive)
        {
            _flags.notifButtonDown = 0;
            _flags.notifAwaitRelease = 1;
            _flags.notifClosing = 0;
            _notif.startMs = now;
            _notif.animDurationMs = 350;
            _flags.notifActive = 1;
            bool enabledNow = (delay == 0);
            _notif.buttonState.enabled = enabledNow;
            _notif.buttonState.pressLevel = 0;
            _notif.buttonState.fadeLevel = enabledNow ? 255 : 0;
            _notif.buttonState.prevEnabled = enabledNow;
        }
        else
            _flags.notifClosing = 0;
        _flags.notifDelayed = (delay > 0);
        _notif.unlockMs = _flags.notifDelayed ? (now + delay * 1000UL) : 0;
    }

    void GUI::setNotificationButtonDown(bool down)
    {
        bool canConfirm = !_flags.notifDelayed || (nowMs() >= _notif.unlockMs);
        _notif.buttonState.enabled = canConfirm;
        bool effectiveDown = canConfirm && down;

        if (_flags.notifAwaitRelease)
        {
            if (!down)
            {
                _flags.notifAwaitRelease = 0;
                _flags.notifButtonDown = 0;
                updateButtonPress(_notif.buttonState, false);
            }
            return;
        }

        bool was = _flags.notifButtonDown;
        _flags.notifButtonDown = effectiveDown;
        updateButtonPress(_notif.buttonState, effectiveDown);

        if (_flags.notifActive && !_flags.notifClosing && was && !effectiveDown)
        {
            _flags.notifClosing = 1;
            _notif.startMs = nowMs();
            _notif.animDurationMs = 300;
        }
    }

    bool GUI::notificationActive() const { return _flags.notifActive; }

    void GUI::renderNotificationOverlay()
    {
        if (!_flags.notifActive)
            return;

        uint32_t now = nowMs(), elapsed = now - _notif.startMs;
        uint32_t dur = _notif.animDurationMs ? _notif.animDurationMs : 1;
        if (elapsed > dur)
            elapsed = dur;

        float t = (float)elapsed / dur;
        float p = _flags.notifClosing ? (1.0f - easeOutCubic(t)) : easeOutCubic(t);

        if (_flags.notifClosing && elapsed >= dur)
        {
            _flags.notifActive = 0;
            _flags.notifClosing = 0;
            _flags.needRedraw = 1;
            return;
        }

        if (_flags.spriteEnabled)
        {
            float df = p * 220.0f;
            if (df > 255)
                df = 255;
            uint32_t factor = (uint32_t)(256.0f - df);

            uint16_t *buf = (uint16_t *)_render.sprite.getBuffer();
            if (buf)
            {
                size_t len = _render.screenWidth * _render.screenHeight;

                uint8_t lut5[32];
                uint8_t lut6[64];

                for (uint8_t v = 0; v < 32; ++v)
                    lut5[v] = (uint8_t)(((uint32_t)v * factor) >> 8);
                for (uint8_t v = 0; v < 64; ++v)
                    lut6[v] = (uint8_t)(((uint32_t)v * factor) >> 8);

                for (size_t i = 0; i < len; ++i)
                {
                    uint16_t px = __builtin_bswap16(buf[i]);
                    uint16_t r = lut5[(px >> 11) & 0x1F];
                    uint16_t g = lut6[(px >> 5) & 0x3F];
                    uint16_t b = lut5[px & 0x1F];
                    px = (uint16_t)((r << 11) | (g << 5) | b);
                    buf[i] = __builtin_bswap16(px);
                }
            }

            uint32_t cardColor = 0xFFFFFF;
            uint32_t titleColor = 0x000000;
            uint32_t msgColor = 0x808080;

            uint32_t btnColor;

            if (_notif.type == Error)
                btnColor = 0xFF0000;
            else if (_notif.type == NotificationTypeWarning)
                btnColor = 0xFF8C00;
            else
                btnColor = 0x00D604;

            int16_t finalW = 148;
            int16_t finalH = 96;

            // AppleвЂ‘style feel: alert \"Р»РѕР¶РёС‚СЃСЏ\" РЅР° СЌРєСЂР°РЅ СЃ Р»С‘РіРєРёРј СѓРјРµРЅСЊС€РµРЅРёРµРј
            float scale;
            if (_flags.notifClosing)
            {
                // Close: subtle shrink and fade out
                scale = 1.0f - 0.06f * p; // 1.00 -> 0.94
            }
            else
            {
                // Open: start СЃР»РµРіРєР° СѓРІРµР»РёС‡РµРЅРЅРѕР№ Рё РјСЏРіРєРѕ РїСЂРёР·РµРјР»СЏРµС‚СЃСЏ Рє 1.0
                scale = 1.06f - 0.06f * p; // 1.06 -> 1.00
            }

            float cardWf = finalW * scale;
            float cardHf = finalH * scale;
            int16_t cardW = (int16_t)(cardWf + 0.5f);
            int16_t cardH = (int16_t)(cardHf + 0.5f);

            float cardXf = (_render.screenWidth - cardWf) * 0.5f;

            int16_t cTop = 0;
            int16_t cH = (int16_t)_render.screenHeight;
            int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0 && _status.style == StatusBarStyleSolid)
            {
                if (_status.pos == Top)
                {
                    cTop += sb;
                    cH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    cH -= sb;
                }
            }
            if (cH <= 0)
            {
                cTop = 0;
                cH = (int16_t)_render.screenHeight;
            }

            float contentHf = (float)cH;
            if (contentHf < cardHf)
                contentHf = cardHf;
            float cardYf = (float)cTop + (contentHf - cardHf) * 0.5f;

            int16_t cardX = (int16_t)(cardXf + 0.5f);
            int16_t cardY = (int16_t)(cardYf + 0.5f);

            int16_t cardCenterY = cardY + cardH / 2;

            int16_t cardRadius = (int16_t)(12.0f * scale + 0.5f);
            if (cardRadius < 4)
                cardRadius = 4;

            uint16_t cardBg565 = detail::color888To565(cardColor);
            uint16_t cardBorder565 = (uint16_t)detail::blend565(cardBg565, (uint16_t)0x0000, 31);

            fillRoundRect(cardX, cardY, cardW, cardH, (uint8_t)cardRadius, cardBorder565);
            fillRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2,
                          (uint8_t)(cardRadius > 2 ? (cardRadius - 2) : cardRadius),
                          cardBg565);

            uint16_t titleSz = (uint16_t)max(9, (int)(19 * scale));
            uint16_t msgSz = (uint16_t)max(7, (int)(13 * scale));

            int16_t tW = 0;
            int16_t tH = 0;
            int16_t mW = 0;
            int16_t mH = 0;

            uint16_t prevW = psdfWeight();
            setPSDFWeight(Semibold);
            setPSDFFontSize(titleSz);
            psdfMeasureText(_notif.title, tW, tH);

            setPSDFWeight(Medium);
            setPSDFFontSize(msgSz);
            psdfMeasureText(_notif.message, mW, mH);

            int16_t gap = (int16_t)(11 * scale);
            int16_t blockH = tH + gap + mH;
            int16_t startY = cardCenterY - blockH / 2 - (int16_t)(20 * scale);

            bool prevRenderText = _flags.renderToSprite;
            pipcore::Sprite *prevActiveText = _render.activeSprite;
            _flags.renderToSprite = 1;
            _render.activeSprite = &_render.sprite;

            uint16_t bg565Title = detail::color888To565(cardColor);
            uint16_t fg565Title = detail::color888To565(titleColor);

            setPSDFWeight(Semibold);
            setPSDFFontSize(titleSz);
            psdfDrawTextInternal(_notif.title,
                                cardX + (cardW - tW) / 2,
                                startY,
                                fg565Title,
                                bg565Title,
                                AlignLeft);

            setPSDFWeight(Medium);
            setPSDFFontSize(msgSz);

            int16_t msgX = (int16_t)(cardX + (cardW - mW) / 2);
            int16_t msgY = (int16_t)(startY + tH + gap);
            uint16_t bg565Msg = detail::color888To565(cardColor);
            uint16_t fg565Msg = detail::color888To565(msgColor);
            psdfDrawTextInternal(_notif.message,
                                msgX,
                                msgY,
                                fg565Msg,
                                bg565Msg,
                                AlignLeft);

            setPSDFWeight(prevW);
            _flags.renderToSprite = prevRenderText;
            _render.activeSprite = prevActiveText;

            int16_t btnBaseW = 123;
            int16_t btnBaseH = 22;
            int16_t marginBot = 8;

            int16_t btnW = (int16_t)(btnBaseW * scale + 0.5f);
            int16_t btnH = (int16_t)(btnBaseH * scale + 0.5f);
            if (btnW < 18)
                btnW = 18;
            if (btnH < 10)
                btnH = 10;

            int16_t btnX = cardX + (cardW - btnW) / 2;
            int16_t btnY = cardY + cardH - btnH - (int16_t)(marginBot * scale + 0.5f);

            int16_t btnRadius = (int16_t)(10.0f * scale + 0.5f);
            if (btnRadius < 4)
                btnRadius = 4;

            String label = _notif.buttonText.length() ? _notif.buttonText : String("OK");

            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _render.activeSprite;
            _flags.renderToSprite = 1;
            _render.activeSprite = &_render.sprite;

            drawButton(label,
                       btnX,
                       btnY,
                       btnW,
                       btnH,
                       btnColor,
                       btnRadius,
                       _notif.buttonState);

            _flags.renderToSprite = prevRender;
            _render.activeSprite = prevActive;

            if (_disp.display && _flags.spriteEnabled)
                _render.sprite.writeToDisplay(*_disp.display, 0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
        }
        else
        {
            clear(0x000000);
        }
    }
}

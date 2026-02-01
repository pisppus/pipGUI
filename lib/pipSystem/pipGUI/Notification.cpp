#include <pipGUI/core/api/pipGUI.h>

#include <pipGUI/fonts/WixMadeforDisplay-Bold.h>
#include <pipGUI/fonts/WixMadeforDisplay-SemiBold.h>

namespace pipgui
{

    static OpenFontRender g_notifTitleFont;
    static OpenFontRender g_notifBodyFont;
    static bool g_notifFontsInited = false;

    static void ensureNotificationFonts()
    {
        if (g_notifFontsInited)
            return;

        pipcore::GuiPlatform *p = pipgui::GUI::sharedPlatform();
        if (p)
            p->configureOpenFontRender();

        FT_Error e1 = g_notifTitleFont.loadFont(gBuiltinWixBoldTTF, sizeof(gBuiltinWixBoldTTF), 0);
        FT_Error e2 = g_notifBodyFont.loadFont(gBuiltinWixSemiBoldTTF, sizeof(gBuiltinWixSemiBoldTTF), 0);

        if (!e1 && !e2)
        {
            g_notifTitleFont.setBackgroundFillMethod(BgFillMethod::None);
            g_notifBodyFont.setBackgroundFillMethod(BgFillMethod::None);
            g_notifFontsInited = true;
        }
    }

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

    void GUI::showNotification(const String &t, const String &m, const String &btn, uint16_t delay, NotificationType type)
    {
        _notifTitle = t;
        _notifMessage = m;
        _notifButtonText = btn;
        _notifType = type;
        uint32_t now = nowMs();
        if (!_flags.notifActive)
        {
            _flags.notifButtonDown = 0;
            _flags.notifAwaitRelease = 1;
            _flags.notifClosing = 0;
            _notifStartMs = now;
            _notifAnimDurationMs = 350;
            _flags.notifActive = 1;
            bool enabledNow = (delay == 0);
            _notifButtonState.enabled = enabledNow;
            _notifButtonState.pressLevel = 0;
            _notifButtonState.fadeLevel = enabledNow ? 255 : 0;
            _notifButtonState.prevEnabled = enabledNow;
        }
        else
            _flags.notifClosing = 0;
        _flags.notifDelayed = (delay > 0);
        _notifUnlockMs = _flags.notifDelayed ? (now + delay * 1000UL) : 0;
    }

    void GUI::setNotificationButtonDown(bool down)
    {
        bool canConfirm = !_flags.notifDelayed || (nowMs() >= _notifUnlockMs);
        _notifButtonState.enabled = canConfirm;
        bool effectiveDown = canConfirm && down;

        if (_flags.notifAwaitRelease)
        {
            if (!down)
            {
                _flags.notifAwaitRelease = 0;
                _flags.notifButtonDown = 0;
                updateButtonPress(_notifButtonState, false);
            }
            return;
        }

        bool was = _flags.notifButtonDown;
        _flags.notifButtonDown = effectiveDown;
        updateButtonPress(_notifButtonState, effectiveDown);

        if (_flags.notifActive && !_flags.notifClosing && was && !effectiveDown)
        {
            _flags.notifClosing = 1;
            _notifStartMs = nowMs();
            _notifAnimDurationMs = 300;
        }
    }

    bool GUI::notificationActive() const { return _flags.notifActive; }

    void GUI::renderNotificationOverlay()
    {
        if (!_flags.notifActive)
            return;
        uint32_t now = nowMs(), elapsed = now - _notifStartMs;
        uint32_t dur = _notifAnimDurationMs ? _notifAnimDurationMs : 1;
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

            uint16_t *buf = (uint16_t *)_sprite.getBuffer();
            if (buf)
            {
                size_t len = _screenWidth * _screenHeight;

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

            uint16_t cardColor = 0xFFFF;  
            uint16_t titleColor = 0x0000; 
            uint16_t msgColor = 0x7BEF;   

            uint16_t btnColor;

            if (_notifType == Error)
            {
                btnColor = 0xF80A;
            }
            else if (_notifType == NotificationTypeWarning)
            {
                btnColor = 0xFD24;
            }
            else
            {
                btnColor = 0xE75C;
            }

            int16_t finalW = 138;
            int16_t finalH = 92;

            float scale = 0.5f + 0.5f * p;

            float cardWf = finalW * scale;
            float cardHf = finalH * scale;
            int16_t cardW = (int16_t)(cardWf + 0.5f);
            int16_t cardH = (int16_t)(cardHf + 0.5f);

            float cardXf = (_screenWidth - cardWf) * 0.5f;

            int16_t cTop = 0;
            int16_t cH = (int16_t)_screenHeight;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Top)
                {
                    cTop += (int16_t)_statusBarHeight;
                    cH -= (int16_t)_statusBarHeight;
                }
                else if (_statusBarPos == Bottom)
                {
                    cH -= (int16_t)_statusBarHeight;
                }
            }
            if (cH <= 0)
            {
                cTop = 0;
                cH = (int16_t)_screenHeight;
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

            uint16_t cardBorderColor = lerpColor565(cardColor, 0x0000, 0.12f);

            _sprite.fillSmoothRoundRect(cardX, cardY, cardW, cardH, cardRadius, cardBorderColor);
            _sprite.fillSmoothRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2,
                                        cardRadius > 2 ? (cardRadius - 2) : cardRadius,
                                        cardColor);

            int16_t btnBaseW = 123;
            int16_t btnBaseH = 22;

            int16_t marginBot = 8;

            int16_t btnW = (int16_t)(btnBaseW * scale + 0.5f);
            int16_t btnH = (int16_t)(btnBaseH * scale + 0.5f);

            int16_t btnX = cardX + (cardW - btnW) / 2;
            int16_t btnY = cardY + cardH - btnH - (int16_t)(marginBot * scale + 0.5f);

            int16_t btnRadius = (int16_t)(7.0f * scale + 0.5f);
            if (btnRadius < 3)
                btnRadius = 3;

            ensureNotificationFonts();

            if (g_notifFontsInited)
            {
                uint16_t titleSz = (uint16_t)max(9, (int)(19 * scale));
                uint16_t msgSz = (uint16_t)max(7, (int)(13 * scale));

                g_notifTitleFont.setDrawer(_sprite);
                g_notifBodyFont.setDrawer(_sprite);

                g_notifTitleFont.setFontSize(titleSz);
                g_notifBodyFont.setFontSize(msgSz);

                int16_t tH = (int16_t)g_notifTitleFont.getTextHeight("A");
                int16_t tW = (int16_t)g_notifTitleFont.getTextWidth("%s", _notifTitle.c_str());

                int16_t mH = (int16_t)g_notifBodyFont.getTextHeight("A");
                int16_t mW = (int16_t)g_notifBodyFont.getTextWidth("%s", _notifMessage.c_str());

                int16_t gap = (int16_t)(11 * scale);
                int16_t blockH = tH + gap + mH;
                int16_t startY = cardCenterY - blockH / 2 - (int16_t)(20 * scale);

                g_notifTitleFont.setFontColor(titleColor, cardColor);
                g_notifTitleFont.drawString(_notifTitle.c_str(),
                                            cardX + (cardW - tW) / 2,
                                            startY,
                                            titleColor,
                                            cardColor,
                                            Layout::Horizontal);

                g_notifBodyFont.setFontColor(msgColor, cardColor);
                g_notifBodyFont.drawString(_notifMessage.c_str(),
                                           cardX + (cardW - mW) / 2,
                                           startY + tH + gap,
                                           msgColor,
                                           cardColor,
                                           Layout::Horizontal);
            }
            else
            {
                _sprite.setTextFont(2);
                _sprite.setTextColor(titleColor, cardColor);
                int16_t th = _sprite.fontHeight();
                _sprite.setTextColor(msgColor, cardColor);
                int16_t mh = _sprite.fontHeight();

                int16_t gap = 5;
                int16_t blockH = th + gap + mh;
                int16_t sy = cardCenterY - blockH / 2 - (int16_t)(20 * scale);

                _sprite.setTextFont(2);
                _sprite.drawString(_notifTitle, cardX + (cardW - _sprite.textWidth(_notifTitle)) / 2, sy);

                _sprite.setTextFont(1);
                _sprite.drawString(_notifMessage, cardX + (cardW - _sprite.textWidth(_notifMessage)) / 2, sy + th + gap);
            }

            String label = _notifButtonText;
            if (_flags.notifDelayed && now < _notifUnlockMs)
            {
                label += " (";
                label += String((_notifUnlockMs - now + 999) / 1000);
                label += ")";
            }

            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;
            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;

            drawButton(label,
                       btnX,
                       btnY,
                       btnW,
                       btnH,
                       btnColor,
                       btnRadius,
                       _notifButtonState);

            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;

            if (_tft && _flags.spriteEnabled)
                _sprite.pushSprite(_tft, 0, 0);
        }
        else
        {
            if (_tft)
                _tft->fillScreen(0);
        }
    }
}
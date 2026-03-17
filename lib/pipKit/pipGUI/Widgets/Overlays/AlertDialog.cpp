#include <pipGUI/Core/pipGUI.hpp>

namespace pipgui
{
    namespace
    {
        float cubicBezierPoint(float u, float p1, float p2)
        {
            const float inv = 1.0f - u;
            return 3.0f * inv * inv * u * p1 + 3.0f * inv * u * u * p2 + u * u * u;
        }

        float cubicBezierEase(float t, float x1, float y1, float x2, float y2)
        {
            if (t <= 0.0f)
                return 0.0f;
            if (t >= 1.0f)
                return 1.0f;

            float lo = 0.0f;
            float hi = 1.0f;
            float u = t;
            for (uint8_t i = 0; i < 10; ++i)
            {
                const float x = cubicBezierPoint(u, x1, x2);
                if (x < t)
                    lo = u;
                else
                    hi = u;
                u = (lo + hi) * 0.5f;
            }

            return cubicBezierPoint(u, y1, y2);
        }

        float notificationEnterEase(float t)
        {
            return cubicBezierEase(t, 0.16f, 1.0f, 0.30f, 1.0f);
        }

        float notificationExitEase(float t)
        {
            return cubicBezierEase(t, 0.40f, 0.0f, 1.0f, 1.0f);
        }

        constexpr uint8_t kAlertMaxMessageLines = 2;
        constexpr float kNotificationBackdropDim = 182.0f;
        constexpr uint16_t kAlertCardBg565 = 0x0861;
        constexpr uint16_t kAlertTitleFg565 = 0xFFFF;
        constexpr uint16_t kAlertMessageFg565 = 0x8410;
        constexpr uint16_t kAlertNormalBtn565 = 0xDEFB;
        constexpr uint16_t kAlertWarningBtn565 = 0xFBC4;
        constexpr uint16_t kAlertErrorBtn565 = 0xF80A;
        constexpr IconId kAlertNoIcon = (IconId)0xFFFF;

        uint16_t notificationButtonColor(NotificationType type)
        {
            if (type == NotificationType::Error)
                return kAlertErrorBtn565;
            if (type == NotificationType::Warning)
                return kAlertWarningBtn565;
            return kAlertNormalBtn565;
        }

        IconId notificationDefaultIcon(NotificationType type)
        {
            if (type == NotificationType::Error)
                return error_layer0;
            if (type == NotificationType::Warning)
                return warning_layer0;
            return kAlertNoIcon;
        }

        IconId notificationResolvedIcon(NotificationType type, IconId customIconId)
        {
            if (customIconId != kAlertNoIcon)
                return customIconId;
            return notificationDefaultIcon(type);
        }
    }

    void GUI::showNotificationInternal(const String &t, const String &m, const String &btn, uint16_t delay, NotificationType type, IconId iconId)
    {
        _notif.title = t;
        _notif.message = m;
        _notif.buttonText = btn;
        _notif.type = type;
        _notif.iconId = iconId;

        const uint32_t now = nowMs();
        if (!_flags.notifActive)
        {
            _flags.notifButtonDown = 0;
            _flags.notifAwaitRelease = 1;
            _flags.notifClosing = 0;
            _notif.startMs = now;
            _notif.animDurationMs = 360;
            _flags.notifActive = 1;

            const bool enabledNow = (delay == 0);
            _notif.buttonState.enabled = enabledNow;
            _notif.buttonState.pressLevel = 0;
            _notif.buttonState.fadeLevel = enabledNow ? 255 : 0;
            _notif.buttonState.prevEnabled = enabledNow;
        }
        else
        {
            _flags.notifClosing = 0;
        }

        _flags.notifDelayed = (delay > 0);
        _notif.unlockMs = _flags.notifDelayed ? (now + delay * 1000UL) : 0;
    }

    void GUI::setNotificationButtonDown(bool down)
    {
        const bool canConfirm = !_flags.notifDelayed || (nowMs() >= _notif.unlockMs);
        _notif.buttonState.enabled = canConfirm;
        const bool effectiveDown = canConfirm && down;

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

        const bool was = _flags.notifButtonDown;
        _flags.notifButtonDown = effectiveDown;
        updateButtonPress(_notif.buttonState, effectiveDown);

        if (_flags.notifActive && !_flags.notifClosing && was && !effectiveDown)
        {
            _flags.notifClosing = 1;
            _notif.startMs = nowMs();
            _notif.animDurationMs = 260;
        }
    }

    bool GUI::notificationActive() const noexcept { return _flags.notifActive; }

    void GUI::renderNotificationOverlay()
    {
        if (!_flags.notifActive)
            return;

        uint32_t now = nowMs();
        uint32_t elapsed = (now >= _notif.startMs) ? (now - _notif.startMs) : 0;
        uint32_t dur = _notif.animDurationMs ? _notif.animDurationMs : 1;
        if (elapsed > dur)
            elapsed = dur;

        const float t = (float)elapsed / dur;
        const float visualP = _flags.notifClosing ? (1.0f - notificationExitEase(t)) : notificationEnterEase(t);

        if (_flags.notifClosing && elapsed >= dur)
        {
            _flags.notifActive = 0;
            _flags.notifClosing = 0;
            _flags.needRedraw = 1;
            return;
        }

        if (_flags.spriteEnabled)
        {
            float dim = visualP * kNotificationBackdropDim;
            if (dim > 255.0f)
                dim = 255.0f;
            const uint32_t factor = (uint32_t)(256.0f - dim);

            uint16_t *buf = (uint16_t *)_render.sprite.getBuffer();
            if (buf)
            {
                const size_t len = _render.screenWidth * _render.screenHeight;
                uint8_t lut5[32];
                uint8_t lut6[64];

                for (uint8_t v = 0; v < 32; ++v)
                    lut5[v] = (uint8_t)(((uint32_t)v * factor) >> 8);
                for (uint8_t v = 0; v < 64; ++v)
                    lut6[v] = (uint8_t)(((uint32_t)v * factor) >> 8);

                for (size_t i = 0; i < len; ++i)
                {
                    uint16_t px = __builtin_bswap16(buf[i]);
                    const uint16_t r = lut5[(px >> 11) & 0x1F];
                    const uint16_t g = lut6[(px >> 5) & 0x3F];
                    const uint16_t b = lut5[px & 0x1F];
                    px = (uint16_t)((r << 11) | (g << 5) | b);
                    buf[i] = __builtin_bswap16(px);
                }
            }

            const uint16_t cardBg565 = kAlertCardBg565;
            const uint16_t cardBorder565 = (uint16_t)detail::blend565(cardBg565, (uint16_t)0xFFFF, 12);
            const uint16_t titleFg565 = kAlertTitleFg565;
            const uint16_t msgFg565 = kAlertMessageFg565;

            const uint16_t btnColor565 = notificationButtonColor(_notif.type);
            const IconId resolvedIconId = notificationResolvedIcon(_notif.type, _notif.iconId);
            const bool hasIcon = (resolvedIconId != kAlertNoIcon);

            const float scale = 0.88f + 0.12f * visualP;
            const int16_t motionY = 0;

            int16_t cTop = 0;
            int16_t cH = (int16_t)_render.screenHeight;
            const int16_t sb = statusBarHeight();
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

            int16_t baseCardW = ((int16_t)_render.screenWidth > 196) ? 188 : (int16_t)(_render.screenWidth - 26);
            if (baseCardW < 148)
                baseCardW = 148;

            int16_t cardW = (int16_t)(baseCardW * scale + 0.5f);
            const int16_t maxCardW = (int16_t)(_render.screenWidth - 12);
            if (cardW > maxCardW)
                cardW = maxCardW;

            const uint16_t prevSize = fontSize();
            const uint16_t prevWeight = fontWeight();

            const uint16_t titleSz = (uint16_t)max(12, (int)(20 * scale + 0.5f));
            const uint16_t msgSz = (uint16_t)max(9, (int)(12 * scale + 0.5f));
            const int16_t innerPadX = (int16_t)max(9, (int)(12 * scale + 0.5f));
            const int16_t topPad = (int16_t)max(7, (int)(9 * scale + 0.5f));
            const int16_t bottomPad = (int16_t)max(8, (int)(9 * scale + 0.5f));
            const int16_t iconSize = hasIcon ? (int16_t)max(30, (int)(38 * scale + 0.5f)) : 0;
            const int16_t iconOffsetY = hasIcon ? (int16_t)max(1, (int)(2 * scale + 0.5f)) : 0;
            const int16_t iconTitleGap = hasIcon ? (int16_t)max(4, (int)(6 * scale + 0.5f)) : 0;
            const int16_t titleNudgeY = hasIcon ? 2 : 1;
            const int16_t titleMsgGap = (int16_t)max(3, (int)(3 * scale + 0.5f));
            const int16_t msgLineGap = (int16_t)max(1, (int)(2 * scale + 0.5f));
            const int16_t msgBtnGap = (int16_t)max(6, (int)(7 * scale + 0.5f));
            const int16_t messageMaxW = (int16_t)max(24, cardW - innerPadX * 2);

            int16_t titleW = 0;
            int16_t titleH = 0;
            setFontWeight(Black);
            setFontSize(titleSz);
            measureText(_notif.title.length() ? _notif.title : String(" "), titleW, titleH);
            if (titleH <= 0)
                titleH = (int16_t)titleSz;

            int16_t msgLineH = 0;
            int16_t tmpW = 0;
            setFontWeight(Medium);
            setFontSize(msgSz);
            measureText(String("Ag"), tmpW, msgLineH);
            if (msgLineH <= 0)
                msgLineH = (int16_t)msgSz;

            String msgLines[kAlertMaxMessageLines];
            uint8_t msgLineCount = 0;

            const auto fitsMessageWidth = [&](const String &s) -> bool
            {
                int16_t w = 0;
                int16_t h = 0;
                return measureText(s, w, h) && w <= messageMaxW;
            };

            const auto pushMessageLine = [&](const String &line) -> bool
            {
                if (msgLineCount >= kAlertMaxMessageLines)
                    return false;
                msgLines[msgLineCount++] = line;
                return true;
            };

            if (_notif.message.length() > 0)
            {
                bool truncated = false;
                int32_t start = 0;
                const int32_t textLen = _notif.message.length();

                while (start <= textLen && !truncated)
                {
                    const int32_t lineBreak = _notif.message.indexOf('\n', start);
                    const String segment = (lineBreak >= 0) ? _notif.message.substring(start, lineBreak) : _notif.message.substring(start);
                    start = (lineBreak >= 0) ? (lineBreak + 1) : (textLen + 1);

                    if (segment.length() == 0)
                    {
                        if (!pushMessageLine(String()))
                            truncated = true;
                        continue;
                    }

                    String currentLine;
                    int32_t wordStart = 0;
                    while (wordStart < segment.length() && !truncated)
                    {
                        while (wordStart < segment.length() && segment[wordStart] == ' ')
                            ++wordStart;
                        if (wordStart >= segment.length())
                            break;

                        int32_t wordEnd = wordStart;
                        while (wordEnd < segment.length() && segment[wordEnd] != ' ')
                            ++wordEnd;

                        const String word = segment.substring(wordStart, wordEnd);
                        const String trial = currentLine.length() ? (currentLine + " " + word) : word;

                        if (currentLine.length() == 0 && !fitsMessageWidth(word))
                        {
                            if (msgLineCount + 1 >= kAlertMaxMessageLines)
                            {
                                pushMessageLine(segment.substring(wordStart));
                                truncated = true;
                                break;
                            }

                            pushMessageLine(word);
                            wordStart = wordEnd + 1;
                            continue;
                        }

                        if (currentLine.length() == 0 || fitsMessageWidth(trial))
                        {
                            currentLine = trial;
                        }
                        else
                        {
                            if (msgLineCount + 1 >= kAlertMaxMessageLines)
                            {
                                pushMessageLine(currentLine + " " + segment.substring(wordStart));
                                truncated = true;
                                break;
                            }

                            pushMessageLine(currentLine);
                            currentLine = word;
                        }

                        wordStart = wordEnd + 1;
                    }

                    if (!truncated && currentLine.length())
                    {
                        if (!pushMessageLine(currentLine))
                        {
                            truncated = true;
                        }
                    }
                }
            }

            const int16_t msgBlockH = (msgLineCount > 0)
                                          ? (int16_t)(msgLineCount * msgLineH + (msgLineCount - 1) * msgLineGap)
                                          : 0;
            const int16_t btnH = (int16_t)max(20, (int)(23 * scale + 0.5f));
            const int16_t btnInsetX = (int16_t)max(10, (int)(12 * scale + 0.5f));
            const int16_t btnInsetBottom = (int16_t)max<int16_t>(8, bottomPad);
            const uint16_t btnDisabledBg = (uint16_t)detail::blend565(btnColor565, cardBg565, 150);
            uint16_t btnBg = btnColor565;
            if (_notif.buttonState.fadeLevel == 0)
            {
                btnBg = btnDisabledBg;
            }
            else if (_notif.buttonState.fadeLevel < 255)
            {
                btnBg = (uint16_t)detail::blend565(btnDisabledBg, btnColor565, _notif.buttonState.fadeLevel);
            }
            const uint16_t iconColor565 = btnColor565;

            int16_t cardH = topPad + iconOffsetY + iconSize + iconTitleGap + titleH + (msgBlockH > 0 ? (titleMsgGap + msgBlockH) : 0) + msgBtnGap + btnH + btnInsetBottom;
            const int16_t minCardH = (int16_t)max(78, (int)(86 * scale + 0.5f));
            if (cardH < minCardH)
                cardH = minCardH;
            if (cardH > cH - 8)
                cardH = cH - 8;

            int16_t cardX = (int16_t)((_render.screenWidth - cardW) / 2);
            int16_t cardY = (int16_t)(cTop + (cH - cardH) / 2 + motionY);
            if (cardY < cTop + 4)
                cardY = cTop + 4;

            const int16_t cardRadius = (int16_t)max(11, (int)(16 * scale + 0.5f));
            const int16_t btnW = cardW - btnInsetX * 2;
            const int16_t btnY = cardY + cardH - btnH - btnInsetBottom;
            const int16_t titleMaxW = cardW - innerPadX * 2;
            const int16_t iconX = (int16_t)(cardX + (cardW - iconSize) / 2);
            const int16_t iconY = cardY + topPad + iconOffsetY;
            const int16_t titleY = iconY + iconSize + iconTitleGap - titleNudgeY;
            const int16_t msgY = titleY + titleH + (msgBlockH > 0 ? titleMsgGap : 0);

            fillRoundRect(cardX, cardY, cardW, cardH, (uint8_t)cardRadius, cardBorder565);
            fillRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2,
                          (uint8_t)(cardRadius > 2 ? (cardRadius - 2) : cardRadius),
                          cardBg565);

            const String label = _notif.buttonText.length() ? _notif.buttonText : String("OK");
            const bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;
            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;

            MarqueeTextOptions marqueeOpts;
            marqueeOpts.speedPxPerSec = 24;
            marqueeOpts.holdStartMs = 650;
            marqueeOpts.phaseStartMs = _notif.startMs;

            const auto drawAlertTitle = [&](const String &text, int16_t x, int16_t y, int16_t maxWidth, uint16_t fg565, TextAlign align)
            {
                if (!drawTextEllipsized(text, x, y, maxWidth, fg565, align))
                    drawTextAligned(text, x, y, fg565, cardBg565, align);
            };

            const auto drawAlertBodyText = [&](const String &text, int16_t x, int16_t y, int16_t maxWidth, uint16_t fg565, TextAlign align)
            {
                if (!drawTextMarquee(text, x, y, maxWidth, fg565, align, marqueeOpts) &&
                    !drawTextEllipsized(text, x, y, maxWidth, fg565, align))
                    drawTextAligned(text, x, y, fg565, cardBg565, align);
            };

            if (hasIcon)
            {
                drawIcon()
                    .pos(iconX, iconY)
                    .size((uint16_t)iconSize)
                    .icon(resolvedIconId)
                    .color(iconColor565)
                    .bgColor(cardBg565)
                    .draw();
            }

            setFontWeight(Black);
            setFontSize(titleSz);
            drawAlertTitle(_notif.title, (int16_t)(cardX + cardW / 2), titleY, titleMaxW, titleFg565, AlignCenter);

            setFontWeight(Medium);
            setFontSize(msgSz);
            if (msgLineCount == 1)
            {
                drawAlertBodyText(msgLines[0], (int16_t)(cardX + cardW / 2), msgY, messageMaxW, msgFg565, AlignCenter);
            }
            else
            {
                for (uint8_t i = 0; i < msgLineCount; ++i)
                {
                    drawAlertBodyText(msgLines[i],
                                      (int16_t)(cardX + cardW / 2),
                                      (int16_t)(msgY + i * (msgLineH + msgLineGap)),
                                      messageMaxW,
                                      msgFg565,
                                      AlignCenter);
                }
            }

            const float btnScale = 1.0f - 0.035f * (_notif.buttonState.pressLevel / 255.0f);
            int16_t drawBtnW = (int16_t)(btnW * btnScale + 0.5f);
            int16_t drawBtnH = (int16_t)(btnH * btnScale + 0.5f);
            if (drawBtnW < 18)
                drawBtnW = 18;
            if (drawBtnH < 12)
                drawBtnH = 12;

            int16_t drawBtnX = (int16_t)(cardX + btnInsetX + (btnW - drawBtnW) / 2);
            int16_t drawBtnY = (int16_t)(btnY + (btnH - drawBtnH) / 2);
            int16_t drawBtnRadius = (int16_t)max(5, (int)(9 * scale + 0.5f));
            if (drawBtnRadius < 5)
                drawBtnRadius = 5;

            fillRoundRect(drawBtnX, drawBtnY, drawBtnW, drawBtnH, (uint8_t)drawBtnRadius, btnBg);

            int16_t btnTextW = 0;
            int16_t btnTextH = 0;
            const uint16_t btnTextPx = (uint16_t)max(9, (int)(drawBtnH * 0.58f));
            setFontWeight(Semibold);
            setFontSize(btnTextPx);
            measureText(label, btnTextW, btnTextH);

            const uint16_t btnTextColor = detail::autoTextColor(btnBg);
            int16_t btnTextY = (int16_t)(drawBtnY + (drawBtnH - btnTextH) / 2);
            if (btnTextY < drawBtnY)
                btnTextY = drawBtnY;

            drawTextAligned(label,
                            (int16_t)(drawBtnX + drawBtnW / 2),
                            btnTextY,
                            btnTextColor,
                            btnBg,
                            AlignCenter);

            setFontWeight(prevWeight);
            setFontSize(prevSize);
            _flags.inSpritePass = prevRender;
            _render.activeSprite = prevActive;
        }
        else
        {
            clear((uint16_t)0x0000);
        }
    }
}

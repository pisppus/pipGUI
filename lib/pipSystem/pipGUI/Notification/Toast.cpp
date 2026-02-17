// Toast notification: short-lived message sliding in from top or bottom with Bezier easing.
// Renders on top of all content (including alert dialog if any).

#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{
    // Cubic Bezier ease-out — smooth decelerate into position (softer than 0.18/0.99)
    static float bezierEaseOut(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        float u = 1.0f - t;
        float uu = u * u;
        float tt = t * t;
        return 3.0f * uu * t * 0.22f + 3.0f * u * tt * 0.92f + tt * t;
    }

    // Ease-in for exit — smooth accelerate off-screen (smoothstep)
    static float bezierEaseIn(float t)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;
        return t * t * (3.0f - 2.0f * t);
    }

    void GUI::showToastInternal(const String &text,
                                uint32_t durationMs,
                                bool fromTop,
                                IconId iconId,
                                uint16_t iconSizePx)
    {
        _toastText = text;
        _toastDisplayMs = (durationMs != 0) ? durationMs : 2500;
        _toastFromTop = fromTop;
        _toastIconId = iconId;
        _toastIconSizePx = iconSizePx ? iconSizePx : 20;
        _toastStartMs = nowMs();
        _toastAnimDurMs = 420;
        _flags.toastActive = 1;
    }

    bool GUI::toastActive() const { return _flags.toastActive; }

    void GUI::renderToastOverlay()
    {
        if (!_flags.toastActive || _toastText.length() == 0)
            return;

        if (!_flags.spriteEnabled || !_display)
            return;

        uint32_t now = nowMs();
        uint32_t elapsed = now - _toastStartMs;
        uint32_t totalDur = _toastAnimDurMs * 2 + _toastDisplayMs;

        if (elapsed >= totalDur)
        {
            _flags.toastActive = 0;
            _flags.needRedraw = 1;
            return;
        }

        // Phase: 0.._toastAnimDurMs = enter, _toastAnimDurMs..(_toastAnimDurMs+_toastDisplayMs) = hold, then exit
        float phaseProgress;
        float slideT; // 0 = off-screen, 1 = on-screen (visible)
        if (elapsed < _toastAnimDurMs)
        {
            phaseProgress = (float)elapsed / (float)_toastAnimDurMs;
            slideT = bezierEaseOut(phaseProgress);
        }
        else if (elapsed < _toastAnimDurMs + _toastDisplayMs)
        {
            slideT = 1.0f;
        }
        else
        {
            uint32_t exitElapsed = elapsed - _toastAnimDurMs - _toastDisplayMs;
            phaseProgress = (float)exitElapsed / (float)_toastAnimDurMs;
            if (phaseProgress >= 1.0f)
            {
                _flags.toastActive = 0;
                _flags.needRedraw = 1;
                return;
            }
            slideT = 1.0f - bezierEaseIn(phaseProgress);
        }

        // Slightly larger text for better legibility
        uint16_t fontPx = 16;
        setPSDFFontSize(fontPx);
        setPSDFWeight(PSDF_WEIGHT_MEDIUM);
        int16_t tw = 0, th = 0;
        psdfMeasureText(_toastText, tw, th);

        const bool hasIcon = true; // we always reserve space for icon when using fluent toast
        const int16_t padH = 18;
        const int16_t padV = 9;
        const int16_t iconGap = hasIcon ? 8 : 0;
        const int16_t radius = 10;

        int16_t iconSide = (int16_t)_toastIconSizePx;
        int16_t contentW = (int16_t)(tw + (hasIcon ? (iconSide + iconGap) : 0));

        int16_t boxW = (int16_t)(contentW + padH * 2);
        int16_t boxH = (int16_t)(th + padV * 2);
        if (boxW > (int16_t)_screenWidth - 24)
            boxW = (int16_t)_screenWidth - 24;
        int16_t boxX = ((int16_t)_screenWidth - boxW) / 2;

        // Slightly longer travel for more \"inertia\"
        int16_t fullTravel = (int16_t)(boxH + 28);
        int16_t startY = _toastFromTop ? (int16_t)(-fullTravel) : (int16_t)(_screenHeight + fullTravel);
        int16_t endY = _toastFromTop ? (int16_t)(20) : (int16_t)(_screenHeight - boxH - 20);
        int16_t sb = statusBarHeight();
        if (_flags.statusBarEnabled && sb > 0 && _statusBarStyle == StatusBarStyleSolid)
        {
            if (_statusBarPos == Top && _toastFromTop)
                endY = (int16_t)(sb + 12);
            else if (_statusBarPos == Bottom && !_toastFromTop)
                endY = (int16_t)(_screenHeight - sb - boxH - 12);
        }
        int16_t boxY = (int16_t)((float)startY + (float)(endY - startY) * slideT + 0.5f);

        uint32_t bgColor = 0x2D2D2D;
        uint32_t borderColor = detail::blend888(bgColor, 0x000000, 40);
        uint32_t fgColor = 0xFFFFFF;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;
        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        fillRoundRectFrc(boxX, boxY, boxW, boxH, (uint8_t)radius, borderColor);
        fillRoundRectFrc(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (uint8_t)(radius > 2 ? radius - 2 : radius), bgColor);

        int16_t textX = boxX + (boxW - tw) / 2;
        int16_t textY = boxY + (boxH - th) / 2;

        if (hasIcon)
        {
            int16_t iconX = (int16_t)(boxX + padH);
            int16_t iconY = (int16_t)(boxY + (boxH - iconSide) / 2);
            drawIcon(_toastIconId, iconX, iconY, (uint16_t)iconSide, fgColor, bgColor);
            textX = (int16_t)(iconX + iconSide + iconGap);
        }

        uint16_t fg565 = color888To565(textX, textY, fgColor);
        uint16_t bg565 = color888To565(textX, textY, bgColor);
        psdfDrawTextInternal(_toastText, textX, textY, fg565, bg565, AlignLeft);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        _sprite.writeToDisplay(*_display, 0, 0, (int16_t)_screenWidth, (int16_t)_screenHeight);
    }
}

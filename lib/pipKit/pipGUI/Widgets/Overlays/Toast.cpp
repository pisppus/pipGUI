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

        float toastEnterEase(float t)
        {
            return cubicBezierEase(t, 0.16f, 1.0f, 0.30f, 1.0f);
        }

        float toastExitEase(float t)
        {
            return cubicBezierEase(t, 0.40f, 0.0f, 1.0f, 1.0f);
        }
    }

    void GUI::showToastInternal(const String &text,
                                uint32_t durationMs,
                                bool fromTop,
                                IconId iconId,
                                uint16_t iconSizePx)
    {
        _toast.text = text;
        _toast.textMetricsValid = false;
        _toast.lastRectValid = false;
        if (_toast.text.length() == 0)
        {
            _flags.toastActive = 0;
            return;
        }
        _toast.displayMs = (durationMs != 0) ? durationMs : 2500;
        _toast.fromTop = fromTop;
        _toast.iconId = iconId;
        _toast.iconSizePx = iconSizePx ? iconSizePx : 20;
        _toast.startMs = nowMs();
        _toast.animDurMs = 420;
        _flags.toastActive = 1;
        _flags.needRedraw = 1;

        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();
        setFontSize(16);
        setFontWeight(Medium);
        int16_t tw = 0;
        int16_t th = 0;
        if (measureText(_toast.text, tw, th))
        {
            _toast.textW = tw;
            _toast.textH = th;
            _toast.textMetricsValid = true;
        }
        setFontWeight(prevWeight);
        setFontSize(prevSize);
    }

    bool GUI::toastActive() const { return _flags.toastActive && _toast.text.length() > 0; }

    bool GUI::computeToastBounds(uint32_t now, DirtyRect &outRect)
    {
        outRect = {0, 0, 0, 0};

        if (!_flags.toastActive)
            return false;
        if (_toast.text.length() == 0)
        {
            _flags.toastActive = 0;
            return false;
        }

        const uint32_t elapsed = (now >= _toast.startMs) ? (now - _toast.startMs) : 0;
        const uint32_t totalDur = _toast.animDurMs * 2 + _toast.displayMs;
        if (elapsed >= totalDur)
        {
            _flags.toastActive = 0;
            return false;
        }

        float visualP = 1.0f;
        if (elapsed < _toast.animDurMs)
        {
            const float phaseProgress = (float)elapsed / (float)_toast.animDurMs;
            visualP = toastEnterEase(phaseProgress);
        }
        else if (elapsed >= _toast.animDurMs + _toast.displayMs)
        {
            const uint32_t exitElapsed = elapsed - _toast.animDurMs - _toast.displayMs;
            const float phaseProgress = (float)exitElapsed / (float)_toast.animDurMs;
            if (phaseProgress >= 1.0f)
            {
                _flags.toastActive = 0;
                return false;
            }
            visualP = 1.0f - toastExitEase(phaseProgress);
        }

        int16_t tw = _toast.textW;
        int16_t th = _toast.textH;
        if (!_toast.textMetricsValid)
        {
            const uint16_t prevSize = fontSize();
            const uint16_t prevWeight = fontWeight();
            const uint16_t fontPx = 16;
            setFontSize(fontPx);
            setFontWeight(Medium);

            tw = 0;
            th = 0;
            if (measureText(_toast.text, tw, th))
            {
                _toast.textW = tw;
                _toast.textH = th;
                _toast.textMetricsValid = true;
            }

            setFontWeight(prevWeight);
            setFontSize(prevSize);
        }

        const bool hasIcon = (_toast.iconId < psdf_icons::IconCount) && (_toast.iconSizePx > 0);
        const int16_t padH = 18;
        const int16_t padV = 9;
        const int16_t iconGap = hasIcon ? 8 : 0;
        const int16_t iconSide = (int16_t)_toast.iconSizePx;
        const int16_t contentW = (int16_t)(tw + (hasIcon ? (iconSide + iconGap) : 0));

        int16_t boxW = (int16_t)(contentW + padH * 2);
        int16_t boxH = (int16_t)(th + padV * 2);
        if (boxW > (int16_t)_render.screenWidth - 24)
            boxW = (int16_t)_render.screenWidth - 24;

        int16_t boxX = ((int16_t)_render.screenWidth - boxW) / 2;
        int16_t startY = _toast.fromTop ? (int16_t)(-boxH - 18) : (int16_t)(_render.screenHeight + 18);
        int16_t endY = _toast.fromTop ? 18 : (int16_t)(_render.screenHeight - boxH - 18);

        const int16_t sb = statusBarHeight();
        if (_flags.statusBarEnabled && sb > 0 && _status.style == StatusBarStyleSolid)
        {
            if (_status.pos == Top && _toast.fromTop)
                endY = (int16_t)(sb + 10);
            else if (_status.pos == Bottom && !_toast.fromTop)
                endY = (int16_t)(_render.screenHeight - sb - boxH - 10);
        }

        int16_t boxY = (int16_t)lroundf((float)startY + (float)(endY - startY) * visualP);
        const int16_t motionOffset = (int16_t)lroundf((1.0f - visualP) * 10.0f);
        if (_toast.fromTop)
            boxY = (int16_t)(boxY + motionOffset);
        else
            boxY = (int16_t)(boxY - motionOffset);

        int16_t x1 = boxX;
        int16_t y1 = boxY;
        int16_t x2 = (int16_t)(boxX + boxW);
        int16_t y2 = (int16_t)(boxY + boxH);

        if (x1 < 0)
            x1 = 0;
        if (y1 < 0)
            y1 = 0;
        if (x2 > (int16_t)_render.screenWidth)
            x2 = (int16_t)_render.screenWidth;
        if (y2 > (int16_t)_render.screenHeight)
            y2 = (int16_t)_render.screenHeight;

        outRect.x = x1;
        outRect.y = y1;
        outRect.w = x2 - x1;
        outRect.h = y2 - y1;

        return outRect.w > 0 && outRect.h > 0;
    }

    void GUI::renderToastOverlay(uint32_t now)
    {
        if (!_flags.toastActive)
            return;
        if (_toast.text.length() == 0)
        {
            _flags.toastActive = 0;
            return;
        }

        if (!_flags.spriteEnabled || !_disp.display)
            return;

        const uint32_t elapsed = (now >= _toast.startMs) ? (now - _toast.startMs) : 0;
        const uint32_t totalDur = _toast.animDurMs * 2 + _toast.displayMs;

        if (elapsed >= totalDur)
        {
            _flags.toastActive = 0;
            return;
        }

        float visualP = 1.0f;
        if (elapsed < _toast.animDurMs)
        {
            const float phaseProgress = (float)elapsed / (float)_toast.animDurMs;
            visualP = toastEnterEase(phaseProgress);
        }
        else if (elapsed >= _toast.animDurMs + _toast.displayMs)
        {
            const uint32_t exitElapsed = elapsed - _toast.animDurMs - _toast.displayMs;
            const float phaseProgress = (float)exitElapsed / (float)_toast.animDurMs;
            if (phaseProgress >= 1.0f)
            {
                _flags.toastActive = 0;
                return;
            }
            visualP = 1.0f - toastExitEase(phaseProgress);
        }

        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();
        const uint16_t fontPx = 16;
        setFontSize(fontPx);
        setFontWeight(Medium);

        int16_t tw = _toast.textW;
        int16_t th = _toast.textH;
        if (!_toast.textMetricsValid)
        {
            tw = 0;
            th = 0;
            if (measureText(_toast.text, tw, th))
            {
                _toast.textW = tw;
                _toast.textH = th;
                _toast.textMetricsValid = true;
            }
        }

        const bool hasIcon = (_toast.iconId < psdf_icons::IconCount) && (_toast.iconSizePx > 0);
        const int16_t padH = 18;
        const int16_t padV = 9;
        const int16_t iconGap = hasIcon ? 8 : 0;
        const int16_t baseRadius = 10;
        const int16_t iconSide = (int16_t)_toast.iconSizePx;
        const int16_t contentW = (int16_t)(tw + (hasIcon ? (iconSide + iconGap) : 0));

        int16_t boxW = (int16_t)(contentW + padH * 2);
        int16_t boxH = (int16_t)(th + padV * 2);
        if (boxW > (int16_t)_render.screenWidth - 24)
            boxW = (int16_t)_render.screenWidth - 24;

        const int16_t drawW = boxW;
        const int16_t drawH = boxH;

        int16_t boxX = ((int16_t)_render.screenWidth - drawW) / 2;
        int16_t startY = _toast.fromTop ? (int16_t)(-drawH - 18) : (int16_t)(_render.screenHeight + 18);
        int16_t endY = _toast.fromTop ? 18 : (int16_t)(_render.screenHeight - drawH - 18);

        const int16_t sb = statusBarHeight();
        if (_flags.statusBarEnabled && sb > 0 && _status.style == StatusBarStyleSolid)
        {
            if (_status.pos == Top && _toast.fromTop)
                endY = (int16_t)(sb + 10);
            else if (_status.pos == Bottom && !_toast.fromTop)
                endY = (int16_t)(_render.screenHeight - sb - drawH - 10);
        }

        int16_t boxY = (int16_t)lroundf((float)startY + (float)(endY - startY) * visualP);
        const int16_t motionOffset = (int16_t)lroundf((1.0f - visualP) * 10.0f);
        if (_toast.fromTop)
            boxY = (int16_t)(boxY + motionOffset);
        else
            boxY = (int16_t)(boxY - motionOffset);

        const uint16_t bgColor = 0x2965;
        const uint16_t borderColor = (uint16_t)detail::blend565(bgColor, (uint16_t)0x0000, 40);
        const uint16_t fgColor = 0xFFFF;

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        const int16_t drawRadius = baseRadius;
        fillRoundRect(boxX, boxY, drawW, drawH, (uint8_t)drawRadius, borderColor);
        fillRoundRect(boxX + 1, boxY + 1, drawW - 2, drawH - 2,
                      (uint8_t)(drawRadius > 2 ? drawRadius - 2 : drawRadius), bgColor);

        int16_t textX = boxX + (drawW - tw) / 2;
        const int16_t textY = boxY + (drawH - th) / 2;
        const int16_t textMaxW = (int16_t)max<int16_t>(24, drawW - padH * 2 - (hasIcon ? (iconSide + iconGap) : 0));

        if (hasIcon)
        {
            const int16_t iconX = (int16_t)(boxX + padH);
            const int16_t iconY = (int16_t)(boxY + (drawH - iconSide) / 2);
            drawIcon()
                .pos(iconX, iconY)
                .size((uint16_t)iconSide)
                .icon(_toast.iconId)
                .color(0xFFFF)
                .bgColor(bgColor)
                .draw();
            textX = (int16_t)(iconX + iconSide + iconGap);
        }

        MarqueeTextOptions marqueeOpts;
        marqueeOpts.speedPxPerSec = 24;
        marqueeOpts.holdStartMs = 700;
        marqueeOpts.phaseStartMs = _toast.startMs;

        if (!drawTextMarquee(_toast.text, textX, textY, textMaxW, fgColor, AlignLeft, marqueeOpts) &&
            !drawTextEllipsized(_toast.text, textX, textY, textMaxW, fgColor, AlignLeft))
        {
            drawTextAligned(_toast.text, textX, textY, fgColor, bgColor, AlignLeft);
        }

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
        setFontWeight(prevWeight);
        setFontSize(prevSize);
    }
}

#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/icons/metrics.hpp>

namespace pipgui
{

    static float easeInOutQuint(float t)
    {
        if (t < 0.5f)
            return 16.0f * t * t * t * t * t;
        t -= 1.0f;
        return 1.0f + 16.0f * t * t * t * t * t;
    }

    void GUI::drawErrorCard(pipcore::Sprite &t,
                            int16_t x, int16_t y, int16_t w, int16_t h,
                            const String &title, const String &detail,
                            uint32_t accentColor,
                            uint8_t opacity,
                            int scrollX)
    {
        if (w <= 0 || h <= 0)
            return;

        uint32_t bg = 0;
        uint32_t dim = 0xC0C0C0;
        uint32_t accent = accentColor;

        int16_t innerX = x;
        int16_t innerY = y;
        int16_t innerW = w;
        int16_t innerH = h;

        {
            uint16_t msgPx = (uint16_t)max<int16_t>(16, _screenHeight / 18);
            uint16_t codePx = msgPx;
            int16_t gap = 10;

            int16_t mw = 0;
            int16_t mh = 0;
            int16_t tmpW = 0;
            setPSDFFontSize(msgPx);
            psdfMeasureText(title, mw, mh);
            psdfMeasureText(String("Ag"), tmpW, mh);

            int16_t mx = innerX + (innerW - mw) / 2;
            if (mx < innerX)
                mx = innerX;

            psdfDrawTextInternal(title, mx, innerY, dim, bg, AlignLeft);

            int16_t cy = innerY + mh + gap;
            setPSDFFontSize(codePx);

            if (detail.startsWith("Code:"))
            {
                String label = "Code:";
                String code = detail.substring(5);
                code.trim();
                code = String(" ") + code;

                int16_t lw = 0;
                int16_t cw = 0;
                int16_t lh = 0;
                int16_t ch = 0;
                psdfMeasureText(label, lw, lh);
                psdfMeasureText(code, cw, ch);

                int16_t total = lw + cw;
                int16_t sx = innerX + (innerW - total) / 2;
                if (sx < innerX)
                    sx = innerX;

                psdfDrawTextInternal(label, sx, cy, dim, bg, AlignLeft);
                psdfDrawTextInternal(code, sx + lw, cy, accent, bg, AlignLeft);
            }
            else
            {
                int16_t dw = 0;
                int16_t dh = 0;
                psdfMeasureText(detail, dw, dh);
                int16_t vw = innerW;
                int16_t vx = innerX;

                if (dw > vw)
                {
                    int tsw = dw + 20;
                    int effX = scrollX % tsw;
                    if (effX > 0)
                        effX -= tsw;
                    psdfDrawTextInternal(detail, (int16_t)(vx + effX + 10), cy, dim, bg, AlignLeft);
                    if (vx + effX + dw + 10 < vx + vw)
                        psdfDrawTextInternal(detail, (int16_t)(vx + effX + dw + 20), cy, dim, bg, AlignLeft);
                }
                else
                {
                    int16_t dx = innerX + (innerW - dw) / 2;
                    if (dx < innerX)
                        dx = innerX;
                    psdfDrawTextInternal(detail, dx, cy, dim, bg, AlignLeft);
                }
            }
        }
    }

    void GUI::showError(const String &title, const String &message, ErrorType type, const String &buttonText)
    {
        uint32_t now = nowMs();
        _errorTitle = title;
        _errorMessage = message;

        _errorType = type;
        _errorButtonText = buttonText;

        uint8_t idx;
        if (!ensureErrorCapacity((uint8_t)(_errorCount + 1)))
        {
            if (_errorCapacity == 0)
                return; // allocation failed and no capacity

            // shift left, drop oldest
            for (uint8_t i = 0; i + 1 < _errorCapacity; ++i)
                _errorEntries[i] = std::move(_errorEntries[i + 1]);
            idx = _errorCapacity - 1;
        }
        else
        {
            idx = (_errorCount < _errorCapacity) ? _errorCount++ : (uint8_t)(_errorCapacity - 1);
        }

        _errorEntries[idx] = {title, message, type, 0, 0};

        if (!_flags.errorActive)
        {
            _flags.errorActive = 1;
            _errorCurrentIndex = 0;
            _errorNextIndex = 0;
            _flags.errorTransition = 0;
            _errorAnimStartMs = now;
            _errorLastScrollMs = now;

            _flags.errorFlashState = 0;
            _errorLastToggleMs = 0;
            _errorFlashIntervalMs = 0;
            _errorEndMs = 0;

            _flags.errorButtonDown = 0;
            _flags.errorAwaitRelease = 1;
            _errorButtonState.enabled = (type == Warning);

            _errorButtonState.pressLevel = 0;
            _errorButtonState.fadeLevel = 255;
            _errorButtonState.prevEnabled = _errorButtonState.enabled;
            _errorButtonState.loading = false;
            _errorButtonState.lastUpdateMs = now;
        }
    }

    bool GUI::ensureErrorCapacity(uint8_t need)
    {
        return detail::ensureCapacity(platform(), _errorEntries, _errorCapacity, need);
    }

    void GUI::nextError()
    {
        if (!_flags.errorActive || _flags.errorTransition || _errorCount <= 1)
            return;
        _flags.errorTransition = 1;
        _errorAnimStartMs = nowMs();
        _errorNextIndex = (_errorCurrentIndex + 1) % _errorCount;
    }

    void GUI::renderErrorFrame(uint32_t now)
    {
        if (!_flags.errorActive)
            return;

        if (_errorCount == 0)
        {
            if (!_flags.errorFlashState)
            {
                clear(0xFF0000);
            }
            return;
        }

        bool prevRenderFrame = _flags.renderToSprite;
        pipcore::Sprite *prevActiveFrame = _activeSprite;

        if (_flags.spriteEnabled)
        {
            _flags.renderToSprite = 1;
            _activeSprite = &_sprite;
        }
        else
        {
            _flags.renderToSprite = 0;
            _activeSprite = nullptr;
        }

        auto t = getDrawTarget();
        clear(0x000000);

        int16_t sbh = statusBarHeight();
        renderStatusBar(_flags.spriteEnabled);

        uint32_t accent888 = (_errorType == Warning) ? rgb(255, 98, 0) : rgb(255, 0, 72);
        uint32_t accentColor = accent888;
        uint32_t iconColor = accentColor;
        uint16_t iconSize = (uint16_t)max<int16_t>(32, _screenHeight / 6);
        uint32_t dotsActiveColor = rgb(235, 235, 235);
        uint32_t dotsInactiveColor = rgb(60, 60, 60);

        const char *header = (_errorType == Warning) ? "WARNING" : "ERROR";
        uint16_t iconId = (_errorType == Warning) ? warning_layer0 : error_layer0;

        int16_t contentTop = sbh;
        int16_t contentH = (int16_t)(_screenHeight - contentTop);

        int16_t iconY = (int16_t)(contentTop + (int16_t)(contentH * 0.26f));
        if (iconY < (int16_t)(contentTop + (int16_t)iconSize / 2 + 6))
            iconY = (int16_t)(contentTop + (int16_t)iconSize / 2 + 6);

        {
            int16_t ix = (int16_t)(_screenWidth / 2) - (int16_t)(iconSize / 2);
            int16_t iy = iconY - (int16_t)(iconSize / 2);
            drawIcon(iconId, ix, iy, iconSize, iconColor, 0);

            uint16_t px = (uint16_t)max<int16_t>(18, _screenHeight / 12);
            int16_t hw = 0;
            int16_t hh = 0;
            setPSDFFontSize(px);
            psdfMeasureText(String(header), hw, hh);

            int16_t ty = iconY + (int16_t)(iconSize / 2 + 10);
            if (ty < (int16_t)(contentTop + 2))
                ty = (int16_t)(contentTop + 2);

            uint16_t headerFg = color888To565((int16_t)(_screenWidth / 2), ty, 0xFFFFFF);
            psdfDrawTextInternal(String(header), (int16_t)(_screenWidth / 2), ty, headerFg, 0, AlignCenter);
        }

        int16_t messageTop = (int16_t)(iconY + (int16_t)iconSize / 2 + 32);
        if (messageTop < contentTop)
            messageTop = contentTop;

        int16_t cardW = (int16_t)(_screenWidth - 34);
        if (cardW < 140)
            cardW = (int16_t)(_screenWidth - 16);

        int16_t bottomReserve = (_errorType == Warning) ? 120 : 90;
        int16_t cardH = (int16_t)max<int16_t>(60, (_screenHeight - bottomReserve) - messageTop);
        int16_t baseX = max<int16_t>(0, (_screenWidth - cardW) / 2);
        int16_t baseY = (int16_t)max<int16_t>(contentTop, messageTop);

        float dotsP = 0.0f;
        bool dotsAnim = false;
        uint8_t dotsActive = _errorCurrentIndex;
        uint8_t dotsPrev = _errorCurrentIndex;
        int8_t dotsDir = 0;

        if (_flags.errorTransition)
        {
            uint32_t el = now - _errorAnimStartMs;
            if (el >= 700)
            {
                _flags.errorTransition = 0;
                _errorCurrentIndex = _errorNextIndex;
                dotsP = 1.0f;
            }
            else
            {
                dotsP = easeInOutQuint(min(1.0f, el / 700.0f));
            }

            float p = dotsP;
            int curX = (int)(baseX - p * (float)cardW * 1.1f + 0.5f);
            int nxtX = (int)((float)baseX + (1.0f - p) * (float)_screenWidth + 0.5f);

            drawErrorCard(*t,
                          (int16_t)curX, baseY, cardW, cardH,
                          _errorEntries[_errorCurrentIndex].title,
                          _errorEntries[_errorCurrentIndex].detail,
                          accentColor,
                          (uint8_t)(255 * (1.0f - p)),
                          _errorEntries[_errorCurrentIndex].scrollPos);

            drawErrorCard(*t,
                          (int16_t)nxtX, baseY, cardW, cardH,
                          _errorEntries[_errorNextIndex].title,
                          _errorEntries[_errorNextIndex].detail,
                          accentColor,
                          (uint8_t)(255 * p),
                          _errorEntries[_errorNextIndex].scrollPos);

            dotsActive = _errorNextIndex;
            dotsPrev = _errorCurrentIndex;
            dotsAnim = true;
            dotsDir = 1;
        }
        else
        {
            drawErrorCard(*t,
                          baseX, baseY, cardW, cardH,
                          _errorEntries[_errorCurrentIndex].title,
                          _errorEntries[_errorCurrentIndex].detail,
                          accentColor,
                          255,
                          _errorEntries[_errorCurrentIndex].scrollPos);
        }

        int16_t btnY = -1;
        if (_errorType == Warning)
        {
            int16_t btnW = (int16_t)(_screenWidth - 80);
            if (btnW < 80)
                btnW = 80;
            int16_t btnH = 30;
            int16_t btnX = (_screenWidth - btnW) / 2;
            btnY = (int16_t)(_screenHeight - btnH - 18);

            uint32_t btnColor = accentColor;
            uint8_t radius = 10;

            bool prevRenderBtn = _flags.renderToSprite;
            pipcore::Sprite *prevActiveBtn = _activeSprite;

            if (_flags.spriteEnabled)
            {
                _flags.renderToSprite = 1;
                _activeSprite = &_sprite;
            }
            else
            {
                _flags.renderToSprite = 0;
                _activeSprite = nullptr;
            }

            String label = _errorButtonText.length() ? _errorButtonText : String("OK");
            drawButton(label, btnX, btnY, btnW, btnH, btnColor, radius, _errorButtonState);

            _flags.renderToSprite = prevRenderBtn;
            _activeSprite = prevActiveBtn;
        }

        if (_errorCount > 1)
        {
            int16_t dotsY = (int16_t)(baseY + ((_errorType == Warning) ? 64 : 54));
            if (btnY >= 0 && dotsY > (int16_t)(btnY - 26))
                dotsY = (int16_t)(btnY - 26);

            drawScrollDots(center,
                           dotsY,
                           _errorCount,
                           dotsActive,
                           dotsPrev,
                           dotsP,
                           dotsAnim,
                           dotsDir,
                           dotsActiveColor,
                           dotsInactiveColor,
                           3, 14, 18);
        }

        if (_flags.spriteEnabled && _display)
            _sprite.writeToDisplay(*_display, 0, 0, (int16_t)_screenWidth, (int16_t)_screenHeight);

        _flags.renderToSprite = prevRenderFrame;
        _activeSprite = prevActiveFrame;
    }

    bool GUI::errorActive() const
    {
        return _flags.errorActive;
    }

    void GUI::setErrorButtonDown(bool down)
    {
        if (!_flags.errorActive || _errorType != Warning)
            return;

        if (_flags.errorAwaitRelease)
        {
            if (!down)
            {
                _flags.errorAwaitRelease = 0;
                _flags.errorButtonDown = 0;
                updateButtonPress(_errorButtonState, false);
            }
            return;
        }

        bool was = _flags.errorButtonDown;
        _flags.errorButtonDown = down;
        updateButtonPress(_errorButtonState, down);

        if (was && !down)
        {
            _flags.errorActive = 0;
            _errorCount = 0;
            _errorCurrentIndex = 0;
            _errorNextIndex = 0;
            _flags.needRedraw = 1;
        }
    }
}
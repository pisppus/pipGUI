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
            uint16_t msgPx = (uint16_t)max<int16_t>(16, _render.screenHeight / 18);
            uint16_t codePx = msgPx;
            int16_t gap = 10;

            int16_t mw = 0;
            int16_t mh = 0;
            int16_t tmpW = 0;
            setFontSize(msgPx);
            measureText(title, mw, mh);
            measureText(String("Ag"), tmpW, mh);

            int16_t mx = innerX + (innerW - mw) / 2;
            if (mx < innerX)
                mx = innerX;

            drawTextAligned(title, mx, innerY, dim, bg, AlignLeft);

            int16_t cy = innerY + mh + gap;
            setFontSize(codePx);

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
                measureText(label, lw, lh);
                measureText(code, cw, ch);

                int16_t total = lw + cw;
                int16_t sx = innerX + (innerW - total) / 2;
                if (sx < innerX)
                    sx = innerX;

                drawTextAligned(label, sx, cy, dim, bg, AlignLeft);
                drawTextAligned(code, sx + lw, cy, accent, bg, AlignLeft);
            }
            else
            {
                int16_t dw = 0;
                int16_t dh = 0;
                measureText(detail, dw, dh);
                int16_t vw = innerW;
                int16_t vx = innerX;

                if (dw > vw)
                {
                    int tsw = dw + 20;
                    int effX = scrollX % tsw;
                    if (effX > 0)
                        effX -= tsw;
                    drawTextAligned(detail, (int16_t)(vx + effX + 10), cy, dim, bg, AlignLeft);
                    if (vx + effX + dw + 10 < vx + vw)
                        drawTextAligned(detail, (int16_t)(vx + effX + dw + 20), cy, dim, bg, AlignLeft);
                }
                else
                {
                    int16_t dx = innerX + (innerW - dw) / 2;
                    if (dx < innerX)
                        dx = innerX;
                    drawTextAligned(detail, dx, cy, dim, bg, AlignLeft);
                }
            }
        }
    }

    void GUI::showError(const String &title, const String &message, ErrorType type, const String &buttonText)
    {
        uint32_t now = nowMs();
        _error.title = title;
        _error.message = message;

        _error.type = type;
        _error.buttonText = buttonText;

        uint8_t idx;
        if (!ensureErrorCapacity((uint8_t)(_error.count + 1)))
        {
            if (_error.capacity == 0)
                return; // allocation failed and no capacity

            // shift left, drop oldest
            for (uint8_t i = 0; i + 1 < _error.capacity; ++i)
                _error.entries[i] = std::move(_error.entries[i + 1]);
            idx = _error.capacity - 1;
        }
        else
        {
            idx = (_error.count < _error.capacity) ? _error.count++ : (uint8_t)(_error.capacity - 1);
        }

        _error.entries[idx] = {title, message, type, 0, 0};

        if (!_flags.errorActive)
        {
            _flags.errorActive = 1;
            _error.currentIndex = 0;
            _error.nextIndex = 0;
            _flags.errorTransition = 0;
            _error.animStartMs = now;
            _error.lastScrollMs = now;

            _flags.errorFlashState = 0;
            _error.lastToggleMs = 0;
            _error.flashIntervalMs = 0;
            _error.endMs = 0;

            _flags.errorButtonDown = 0;
            _flags.errorAwaitRelease = 1;
            _error.buttonState.enabled = (type == Warning);

            _error.buttonState.pressLevel = 0;
            _error.buttonState.fadeLevel = 255;
            _error.buttonState.prevEnabled = _error.buttonState.enabled;
            _error.buttonState.loading = false;
            _error.buttonState.lastUpdateMs = now;
        }
    }

    bool GUI::ensureErrorCapacity(uint8_t need)
    {
        return detail::ensureCapacity(platform(), _error.entries, _error.capacity, need);
    }

    void GUI::nextError()
    {
        if (!_flags.errorActive || _flags.errorTransition || _error.count <= 1)
            return;
        _flags.errorTransition = 1;
        _error.animStartMs = nowMs();
        _error.nextIndex = (_error.currentIndex + 1) % _error.count;
    }

    void GUI::renderErrorFrame(uint32_t now)
    {
        if (!_flags.errorActive)
            return;

        if (_error.count == 0)
        {
            if (!_flags.errorFlashState)
            {
                clear(rgb(255, 0, 0));
            }
            return;
        }

        bool prevRenderFrame = _flags.renderToSprite;
        pipcore::Sprite *prevActiveFrame = _render.activeSprite;

        if (_flags.spriteEnabled)
        {
            _flags.renderToSprite = 1;
            _render.activeSprite = &_render.sprite;
        }
        else
        {
            _flags.renderToSprite = 0;
            _render.activeSprite = nullptr;
        }

        auto t = getDrawTarget();
        clear(rgb(0, 0, 0));

        int16_t sbh = statusBarHeight();
        renderStatusBar();

        uint16_t accentColor = (_error.type == Warning) ? rgb(255, 98, 0) : rgb(255, 0, 72);
        uint16_t iconColor = accentColor;
        uint16_t iconSize = (uint16_t)max<int16_t>(32, _render.screenHeight / 6);
        uint16_t dotsActiveColor = rgb(235, 235, 235);
        uint16_t dotsInactiveColor = rgb(60, 60, 60);

        const char *header = (_error.type == Warning) ? "WARNING" : "ERROR";
        uint16_t iconId = (_error.type == Warning) ? warning_layer0 : error_layer0;

        int16_t contentTop = sbh;
        int16_t contentH = (int16_t)(_render.screenHeight - contentTop);

        int16_t iconY = (int16_t)(contentTop + (int16_t)(contentH * 0.26f));
        if (iconY < (int16_t)(contentTop + (int16_t)iconSize / 2 + 6))
            iconY = (int16_t)(contentTop + (int16_t)iconSize / 2 + 6);

        {
            int16_t ix = (int16_t)(_render.screenWidth / 2) - (int16_t)(iconSize / 2);
            int16_t iy = iconY - (int16_t)(iconSize / 2);
            drawIcon()
                .at(ix, iy)
                .size(iconSize)
                .icon(iconId)
                .color(iconColor)
                .bgColor(0)
                .draw();

            uint16_t px = (uint16_t)max<int16_t>(18, _render.screenHeight / 12);
            int16_t hw = 0;
            int16_t hh = 0;
            setFontSize(px);

            uint16_t prevW = fontWeight();
            setFontWeight(Semibold);
            measureText(String(header), hw, hh);

            int16_t ty = iconY + (int16_t)(iconSize / 2 + 10);
            if (ty < (int16_t)(contentTop + 2))
                ty = (int16_t)(contentTop + 2);

            uint16_t headerFg = detail::color888To565(0xFFFFFF);
            drawTextAligned(String(header), (int16_t)(_render.screenWidth / 2), ty, headerFg, 0, AlignCenter);
            setFontWeight(prevW);
        }

        int16_t messageTop = (int16_t)(iconY + (int16_t)iconSize / 2 + 32);
        if (messageTop < contentTop)
            messageTop = contentTop;

        int16_t cardW = (int16_t)(_render.screenWidth - 34);
        if (cardW < 140)
            cardW = (int16_t)(_render.screenWidth - 16);

        int16_t bottomReserve = (_error.type == Warning) ? 120 : 90;
        int16_t cardH = (int16_t)max<int16_t>(60, (_render.screenHeight - bottomReserve) - messageTop);
        int16_t baseX = max<int16_t>(0, (_render.screenWidth - cardW) / 2);
        int16_t baseY = (int16_t)max<int16_t>(contentTop, messageTop);

        float dotsP = 0.0f;
        bool dotsAnim = false;
        uint8_t dotsActive = _error.currentIndex;
        uint8_t dotsPrev = _error.currentIndex;
        int8_t dotsDir = 0;

        if (_flags.errorTransition)
        {
            uint32_t el = now - _error.animStartMs;
            if (el >= 700)
            {
                _flags.errorTransition = 0;
                _error.currentIndex = _error.nextIndex;
                dotsP = 1.0f;
            }
            else
            {
                dotsP = easeInOutQuint(min(1.0f, el / 700.0f));
            }

            float p = dotsP;
            int curX = (int)(baseX - p * (float)cardW * 1.1f + 0.5f);
            int nxtX = (int)((float)baseX + (1.0f - p) * (float)_render.screenWidth + 0.5f);

            drawErrorCard(*t,
                          (int16_t)curX, baseY, cardW, cardH,
                          _error.entries[_error.currentIndex].title,
                          _error.entries[_error.currentIndex].detail,
                          accentColor,
                          (uint8_t)(255 * (1.0f - p)),
                          _error.entries[_error.currentIndex].scrollPos);

            drawErrorCard(*t,
                          (int16_t)nxtX, baseY, cardW, cardH,
                          _error.entries[_error.nextIndex].title,
                          _error.entries[_error.nextIndex].detail,
                          accentColor,
                          (uint8_t)(255 * p),
                          _error.entries[_error.nextIndex].scrollPos);

            dotsActive = _error.nextIndex;
            dotsPrev = _error.currentIndex;
            dotsAnim = true;
            dotsDir = 1;
        }
        else
        {
            drawErrorCard(*t,
                          baseX, baseY, cardW, cardH,
                          _error.entries[_error.currentIndex].title,
                          _error.entries[_error.currentIndex].detail,
                          accentColor,
                          255,
                          _error.entries[_error.currentIndex].scrollPos);
        }

        int16_t btnY = -1;
        if (_error.type == Warning)
        {
            int16_t btnW = (int16_t)(_render.screenWidth - 80);
            if (btnW < 80)
                btnW = 80;
            int16_t btnH = 30;
            int16_t btnX = (_render.screenWidth - btnW) / 2;
            btnY = (int16_t)(_render.screenHeight - btnH - 18);

            uint32_t btnColor = accentColor;
            uint8_t radius = 10;

            bool prevRenderBtn = _flags.renderToSprite;
            pipcore::Sprite *prevActiveBtn = _render.activeSprite;

            if (_flags.spriteEnabled)
            {
                _flags.renderToSprite = 1;
                _render.activeSprite = &_render.sprite;
            }
            else
            {
                _flags.renderToSprite = 0;
                _render.activeSprite = nullptr;
            }

            String label = _error.buttonText.length() ? _error.buttonText : String("OK");
            drawButton(label, btnX, btnY, btnW, btnH, btnColor, radius, _error.buttonState);

            _flags.renderToSprite = prevRenderBtn;
            _render.activeSprite = prevActiveBtn;
        }

        if (_error.count > 1)
        {
            int16_t dotsY = (int16_t)(baseY + ((_error.type == Warning) ? 64 : 54));
            if (btnY >= 0 && dotsY > (int16_t)(btnY - 26))
                dotsY = (int16_t)(btnY - 26);

            drawScrollDots().at(center, dotsY)
                .count(_error.count)
                .activeIndex(dotsActive)
                .prevIndex(dotsPrev)
                .animProgress(dotsP)
                .animate(dotsAnim)
                .animDirection(dotsDir)
                .activeColor(dotsActiveColor)
                .inactiveColor(dotsInactiveColor)
                .dotRadius(3)
                .spacing(14)
                .activeWidth(18)
                .draw();
        }

        if (_flags.spriteEnabled && _disp.display)
            _render.sprite.writeToDisplay(*_disp.display, 0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);

        _flags.renderToSprite = prevRenderFrame;
        _render.activeSprite = prevActiveFrame;
    }

    bool GUI::errorActive() const
    {
        return _flags.errorActive;
    }

    void GUI::setErrorButtonDown(bool down)
    {
        if (!_flags.errorActive || _error.type != Warning)
            return;

        if (_flags.errorAwaitRelease)
        {
            if (!down)
            {
                _flags.errorAwaitRelease = 0;
                _flags.errorButtonDown = 0;
                updateButtonPress(_error.buttonState, false);
            }
            return;
        }

        bool was = _flags.errorButtonDown;
        _flags.errorButtonDown = down;
        updateButtonPress(_error.buttonState, down);

        if (was && !down)
        {
            _flags.errorActive = 0;
            _error.count = 0;
            _error.currentIndex = 0;
            _error.nextIndex = 0;
            _flags.needRedraw = 1;
        }
    }
}
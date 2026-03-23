#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>
#include <pipGUI/Graphics/Text/Icons/metrics.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kErrorTransitionMs = 320;
        constexpr uint32_t kErrorEnterMs = 360;
        constexpr uint32_t kErrorLayoutMs = 220;
        constexpr uint16_t kErrorMarqueeHoldMs = 700;
        constexpr uint16_t kErrorMarqueeSpeedPx = 28;

        struct ErrorTheme
        {
            uint16_t accent565 = 0;
            uint16_t text565 = 0;
            uint16_t detail565 = 0;
            uint16_t dotsActive565 = 0;
            uint16_t dotsInactive565 = 0;
            uint16_t iconId = 0;
            const char *label = "";
            bool dismissible = false;
        };

        struct ErrorLayout
        {
            int16_t centerX = 0;
            int16_t iconTop = 0;
            int16_t headerTop = 0;
            int16_t messageTop = 0;
            int16_t detailTop = 0;
            int16_t dotsY = 0;
            int16_t buttonX = 0;
            int16_t buttonY = 0;
            int16_t buttonW = 0;
            int16_t buttonH = 0;
            int16_t textMaxW = 0;
            uint16_t iconSize = 0;
            uint16_t headerPx = 0;
            uint16_t bodyPx = 0;
            uint16_t detailPx = 0;
            uint8_t buttonRadius = 0;
        };

        static inline uint8_t clampIndex(uint8_t index, uint8_t count) noexcept
        {
            return (count == 0 || index >= count) ? 0 : index;
        }
    }

    void GUI::renderErrorFrame(uint32_t now)
    {
        if (!_flags.errorActive)
            return;

        if (_error.count == 0)
        {
            _flags.errorActive = 0;
            _flags.errorTransition = 0;
            _flags.errorEntering = 0;
            _flags.needRedraw = 1;
            return;
        }

        const FontId prevFont = fontId();
        const uint16_t prevSize = fontSize();
        const uint16_t prevWeight = fontWeight();
        const bool prevSpritePass = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        clear(rgb(0, 0, 0));
        if (_flags.statusBarEnabled)
            renderStatusBar();

        auto resolveTheme = [&](const detail::ErrorEntry &entry) -> ErrorTheme
        {
            const bool isWarning = entry.type == ErrorTypeWarning;
            return {
                isWarning ? static_cast<uint16_t>(Warning) : static_cast<uint16_t>(Error),
                rgb(255, 255, 255),
                rgb(166, 166, 171),
                rgb(235, 235, 235),
                rgb(88, 88, 92),
                isWarning ? warning : error,
                isWarning ? "WARNING" : "ERROR",
                isWarning};
        };

        auto lerpPos = [](int16_t a, int16_t b, float t) -> int16_t
        {
            return (int16_t)(a + (int16_t)(((float)(b - a) * t) + ((b >= a) ? 0.5f : -0.5f)));
        };

        auto buildLayoutFor = [&](bool dismissible, bool includeDots, bool includeCode) -> ErrorLayout
        {
            ErrorLayout layout{};
            const int16_t top = _flags.statusBarEnabled ? statusBarHeight() : 0;
            const int16_t screenW = (int16_t)_render.screenWidth;
            const int16_t screenH = (int16_t)_render.screenHeight;
            constexpr int16_t iconGap = 5;
            constexpr int16_t titleGap = 8;
            constexpr int16_t detailGap = 5;
            constexpr int16_t dotsGap = 14;
            constexpr int16_t dotsH = 7;

            layout.iconSize = 40;
            layout.headerPx = 20;
            layout.bodyPx = 16;
            layout.detailPx = 16;
            layout.buttonH = 30;
            layout.buttonW = 220;
            if (layout.buttonW > screenW - 8)
                layout.buttonW = (int16_t)(screenW - 8);
            layout.centerX = (int16_t)(screenW / 2);
            layout.buttonX = (int16_t)((screenW - layout.buttonW) / 2);
            layout.buttonY = (int16_t)(screenH - layout.buttonH - 15);
            layout.buttonRadius = 10;
            layout.textMaxW = layout.buttonW;

            const int16_t contentBottom = dismissible ? layout.buttonY : screenH;
            const int16_t codeBlockH = includeCode ? (int16_t)(detailGap + layout.detailPx) : 0;
            const int16_t dotsBlockH = includeDots ? (int16_t)(dotsGap + dotsH) : 0;
            const int16_t blockH = (int16_t)(layout.iconSize + iconGap + layout.headerPx + titleGap + layout.bodyPx + codeBlockH + dotsBlockH);
            int16_t blockTop = (int16_t)(top + ((contentBottom - top - blockH) / 2));
            if (blockTop < top)
                blockTop = top;

            layout.iconTop = blockTop;
            layout.headerTop = (int16_t)(layout.iconTop + layout.iconSize + iconGap);
            layout.messageTop = (int16_t)(layout.headerTop + layout.headerPx + titleGap);
            layout.detailTop = (int16_t)(layout.messageTop + layout.bodyPx + (includeCode ? detailGap : 0));
            const int16_t detailBottom = includeCode ? (int16_t)(layout.detailTop + layout.detailPx) : (int16_t)(layout.messageTop + layout.bodyPx);
            layout.dotsY = includeDots ? (int16_t)(detailBottom + dotsGap) : detailBottom;

            if (dismissible && layout.dotsY > (int16_t)(layout.buttonY - dotsH))
                layout.dotsY = (int16_t)(layout.buttonY - dotsH);

            return layout;
        };

        auto dotsVisibleProgress = [&]() -> float
        {
            if (_error.layoutAnimStartMs == 0 || _error.layoutFromDotsVisible == _error.layoutToDotsVisible)
                return (_error.layoutToDotsVisible > 0) ? 1.0f : 0.0f;

            const uint32_t elapsed = now - _error.layoutAnimStartMs;
            if (elapsed >= kErrorLayoutMs)
                return (_error.layoutToDotsVisible > 0) ? 1.0f : 0.0f;

            _flags.needRedraw = 1;
            const float p = detail::motion::easeInOutCubic((float)elapsed / (float)kErrorLayoutMs);
            const float from = (float)_error.layoutFromDotsVisible * (1.0f / 255.0f);
            const float to = (float)_error.layoutToDotsVisible * (1.0f / 255.0f);
            return from + (to - from) * p;
        };

        auto buildLayout = [&](bool dismissible, bool includeCode, float dotsVisible) -> ErrorLayout
        {
            const ErrorLayout withDots = buildLayoutFor(dismissible, true, includeCode);
            const ErrorLayout withoutDots = buildLayoutFor(dismissible, false, includeCode);

            ErrorLayout layout = withDots;
            layout.iconTop = lerpPos(withoutDots.iconTop, withDots.iconTop, dotsVisible);
            layout.headerTop = lerpPos(withoutDots.headerTop, withDots.headerTop, dotsVisible);
            layout.messageTop = lerpPos(withoutDots.messageTop, withDots.messageTop, dotsVisible);
            layout.detailTop = lerpPos(withoutDots.detailTop, withDots.detailTop, dotsVisible);
            layout.dotsY = lerpPos(withoutDots.dotsY, withDots.dotsY, dotsVisible);
            return layout;
        };

        auto withClip = [&](int16_t x, int16_t y, int16_t w, int16_t h, auto &&drawFn)
        {
            pipcore::Sprite *sprite = getDrawTarget();
            if (!sprite)
                return;

            int32_t clipX = 0;
            int32_t clipY = 0;
            int32_t clipW = 0;
            int32_t clipH = 0;
            sprite->getClipRect(&clipX, &clipY, &clipW, &clipH);
            sprite->setClipRect(x, y, w, h);
            drawFn();
            sprite->setClipRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
        };

        auto measureLine = [&](const String &text, uint16_t px, uint16_t weight, int16_t &width, int16_t &height)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(px);
            setFontWeight(weight);
            width = 0;
            height = 0;
            if (!measureText(text.length() ? text : String("Ag"), width, height))
                height = (int16_t)px;
            if (height < (int16_t)px)
                height = (int16_t)px;
        };

        auto measureLineHeight = [&](const String &text, uint16_t px, uint16_t weight) -> int16_t
        {
            int16_t width = 0;
            int16_t height = 0;
            measureLine(text, px, weight, width, height);
            return height;
        };

        auto drawCenteredLine = [&](const String &text,
                                    int16_t centerX,
                                    int16_t topY,
                                    int16_t maxWidth,
                                    uint16_t fg565,
                                    uint16_t bg565,
                                    uint16_t px,
                                    uint16_t weight,
                                    bool allowMarquee,
                                    uint32_t phaseStartMs)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(px);
            setFontWeight(weight);
            const uint32_t marqueeElapsedMs = (now >= phaseStartMs) ? (now - phaseStartMs) : 0U;
            if (allowMarquee && marqueeElapsedMs >= kErrorMarqueeHoldMs)
            {
                MarqueeTextOptions marqueeOpts{};
                marqueeOpts.speedPxPerSec = kErrorMarqueeSpeedPx;
                marqueeOpts.holdStartMs = kErrorMarqueeHoldMs;
                marqueeOpts.phaseStartMs = phaseStartMs;
                if (drawTextMarquee(text, centerX, topY, maxWidth, fg565, TextAlign::Center, marqueeOpts))
                    return;
            }
            if (!drawTextEllipsized(text, centerX, topY, maxWidth, fg565, TextAlign::Center))
                drawTextAligned(text, centerX, topY, fg565, bg565, TextAlign::Center);
        };

        auto measureCodeLine = [&](const String &code, uint16_t px, int16_t &prefixW, int16_t &codeW, int16_t &lineH)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(px);
            setFontWeight(Medium);
            prefixW = 0;
            codeW = 0;
            lineH = 0;
            int16_t tmpH = 0;
            measureText(String("Code:"), prefixW, tmpH);
            lineH = tmpH;
            if (code.length() > 0)
                measureText(String(" ") + code, codeW, tmpH);
            if (tmpH > lineH)
                lineH = tmpH;
            if (lineH < (int16_t)px)
                lineH = (int16_t)px;
        };

        auto drawCodeLineAt = [&](const String &code,
                                  int16_t startX,
                                  int16_t topY,
                                  uint16_t prefix565,
                                  uint16_t accent565,
                                  uint16_t bg565,
                                  uint16_t px)
        {
            static_cast<void>(setFont(WixMadeForDisplay));
            setFontSize(px);
            setFontWeight(Medium);
            int16_t prefixW = 0;
            int16_t codeW = 0;
            int16_t lineH = 0;
            measureCodeLine(code, px, prefixW, codeW, lineH);
            drawTextAligned(String("Code:"), startX, topY, prefix565, bg565, TextAlign::Left);
            if (code.length() > 0)
                drawTextAligned(String(" ") + code, (int16_t)(startX + prefixW), topY, accent565, bg565, TextAlign::Left);
        };

        auto drawCodeLine = [&](const String &code,
                                int16_t centerX,
                                int16_t topY,
                                int16_t maxWidth,
                                uint16_t prefix565,
                                uint16_t accent565,
                                uint16_t bg565,
                                uint16_t px,
                                bool allowMarquee,
                                uint32_t phaseStartMs)
        {
            if (code.length() == 0)
                return;

            int16_t prefixW = 0;
            int16_t codeW = 0;
            int16_t lineH = 0;
            measureCodeLine(code, px, prefixW, codeW, lineH);
            const String fullText = String("Code: ") + code;
            const int16_t totalW = (int16_t)(prefixW + codeW);
            const uint32_t marqueeElapsedMs = (now >= phaseStartMs) ? (now - phaseStartMs) : 0U;
            if (allowMarquee && totalW > maxWidth && marqueeElapsedMs >= kErrorMarqueeHoldMs)
            {
                const int16_t bandX = (int16_t)(centerX - (maxWidth / 2));
                const int16_t bandY = (int16_t)(topY - 3);
                const int16_t bandH = (int16_t)(lineH + 6);
                const int16_t loopPx = (int16_t)(totalW + 18);
                int16_t offsetPx = 0;
                if (loopPx > 0)
                {
                    const uint64_t distanceMilliPx = (uint64_t)(marqueeElapsedMs - kErrorMarqueeHoldMs) * kErrorMarqueeSpeedPx;
                    const uint64_t loopMilliPx = (uint64_t)loopPx * 1000ULL;
                    const uint64_t wrappedMilliPx = distanceMilliPx % loopMilliPx;
                    offsetPx = (int16_t)((wrappedMilliPx + 500ULL) / 1000ULL);
                    if (offsetPx >= loopPx)
                        offsetPx = 0;
                    requestRedraw();
                }

                withClip(bandX, bandY, maxWidth, bandH, [&]
                         {
                    const int16_t drawX = (int16_t)(bandX - offsetPx);
                    drawCodeLineAt(code, drawX, topY, prefix565, accent565, bg565, px);
                    drawCodeLineAt(code, (int16_t)(drawX + loopPx), topY, prefix565, accent565, bg565, px); });
                return;
            }

            if (totalW <= maxWidth)
            {
                drawCodeLineAt(code, (int16_t)(centerX - (totalW / 2)), topY, prefix565, accent565, bg565, px);
                return;
            }

            if (!drawTextEllipsized(fullText, centerX, topY, maxWidth, prefix565, TextAlign::Center))
                drawTextAligned(fullText, centerX, topY, prefix565, bg565, TextAlign::Center);
        };

        auto drawRollingText = [&](const String &from,
                                   const String &to,
                                   int16_t centerX,
                                   int16_t topY,
                                   int16_t maxWidth,
                                   uint16_t fg565,
                                   uint16_t bg565,
                                   uint16_t px,
                                   uint16_t weight,
                                   float progress,
                                   int8_t direction,
                                   uint32_t phaseStartMs)
        {
            const int16_t lineH = measureLineHeight(from.length() >= to.length() ? from : to, px, weight);
            const int16_t bandH = (int16_t)(lineH + 6);
            const int16_t bandY = (int16_t)(topY - 3);
            const int16_t bandX = (int16_t)(centerX - (maxWidth / 2));

            if (progress <= 0.0f || from == to)
            {
                drawCenteredLine(from, centerX, topY, maxWidth, fg565, bg565, px, weight, false, phaseStartMs);
                return;
            }

            const int16_t travel = (int16_t)(maxWidth + 12);
            const int16_t shift = (int16_t)(travel * progress + 0.5f);
            const int16_t currentX = (int16_t)(centerX - direction * shift);
            const int16_t nextX = (int16_t)(centerX + direction * (travel - shift));

            withClip(bandX, bandY, maxWidth, bandH, [&]
                     {
                drawCenteredLine(from, currentX, topY, maxWidth, fg565, bg565, px, weight, false, phaseStartMs);
                drawCenteredLine(to, nextX, topY, maxWidth, fg565, bg565, px, weight, false, phaseStartMs); });
        };

        auto drawRollingCode = [&](const String &from,
                                   const String &to,
                                   int16_t centerX,
                                   int16_t topY,
                                   int16_t maxWidth,
                                   uint16_t detail565,
                                   uint16_t accent565,
                                   uint16_t bg565,
                                   uint16_t px,
                                   float progress,
                                   int8_t direction,
                                   uint32_t phaseStartMs)
        {
            if (from.length() == 0 && to.length() == 0)
                return;

            const String measureTextValue = (from.length() >= to.length()) ? String("Code: ") + from : String("Code: ") + to;
            const int16_t lineH = measureLineHeight(measureTextValue, px, Medium);
            const int16_t bandH = (int16_t)(lineH + 6);
            const int16_t bandY = (int16_t)(topY - 3);
            const int16_t bandX = (int16_t)(centerX - (maxWidth / 2));

            if (progress <= 0.0f || from == to)
            {
                drawCodeLine(from, centerX, topY, maxWidth, detail565, accent565, bg565, px, false, phaseStartMs);
                return;
            }

            const int16_t travel = (int16_t)(maxWidth + 12);
            const int16_t shift = (int16_t)(travel * progress + 0.5f);
            const int16_t currentX = (int16_t)(centerX - direction * shift);
            const int16_t nextX = (int16_t)(centerX + direction * (travel - shift));

            withClip(bandX, bandY, maxWidth, bandH, [&]
                     {
                drawCodeLine(from, currentX, topY, maxWidth, detail565, accent565, bg565, px, false, phaseStartMs);
                drawCodeLine(to, nextX, topY, maxWidth, detail565, accent565, bg565, px, false, phaseStartMs); });
        };

        const detail::ErrorEntry &currentEntry = _error.entries[clampIndex(_error.currentIndex, _error.count)];
        const detail::ErrorEntry &visualEntry = _flags.errorTransition
                                                    ? _error.entries[clampIndex(_error.nextIndex, _error.count)]
                                                    : currentEntry;
        const ErrorTheme theme = resolveTheme(visualEntry);
        const bool hasCode = visualEntry.code.length() > 0;
        const float dotsVisible = dotsVisibleProgress();
        const ErrorLayout layout = buildLayout(theme.dismissible, hasCode, dotsVisible);
        const uint16_t bg565 = 0x0000;

        if (_flags.errorTransition)
        {
            const uint32_t elapsed = now - _error.animStartMs;
            if (elapsed >= kErrorTransitionMs)
            {
                _flags.errorTransition = 0;
                _error.currentIndex = clampIndex(_error.nextIndex, _error.count);
                _error.transitionDir = 0;
                _error.contentStartMs = now;
            }
            else
            {
                _flags.needRedraw = 1;
            }
        }

        if (_flags.errorEntering)
        {
            const uint32_t elapsed = now - _error.showStartMs;
            if (elapsed >= kErrorEnterMs)
            {
                _flags.errorEntering = 0;
                _error.contentStartMs = now;
            }
            else
            {
                _flags.needRedraw = 1;
            }
        }

        if (_error.layoutAnimStartMs != 0 && (now - _error.layoutAnimStartMs) >= kErrorLayoutMs)
        {
            _error.layoutAnimStartMs = 0;
            _error.layoutFromDotsVisible = _error.layoutToDotsVisible;
        }

        const float rawProgress = _flags.errorTransition
                                      ? detail::motion::clamp01((float)(now - _error.animStartMs) / (float)kErrorTransitionMs)
                                      : 0.0f;
        const float textProgress = detail::motion::easeInOutCubic(rawProgress);
        const float enterRaw = _flags.errorEntering
                                   ? detail::motion::clamp01((float)(now - _error.showStartMs) / (float)kErrorEnterMs)
                                   : 1.0f;
        const float enterProgress = detail::motion::cubicBezierEase(enterRaw, 0.16f, 1.0f, 0.30f, 1.0f);
        const float enterShiftF = (1.0f - enterProgress) * 18.0f;
        const int16_t enterShift = (int16_t)(enterShiftF + 0.5f);
        uint16_t iconSize = (uint16_t)(layout.iconSize * (0.78f + 0.22f * enterProgress) + 0.5f);
        if (iconSize < 24)
            iconSize = 24;
        const int16_t iconX = (int16_t)(layout.centerX - (iconSize / 2));
        const int16_t iconY = (int16_t)(layout.iconTop + enterShift + (layout.iconSize - iconSize) / 2);
        const int16_t headerY = (int16_t)(layout.headerTop + enterShift);
        const int16_t messageY = (int16_t)(layout.messageTop + enterShift);
        const int16_t detailY = (int16_t)(layout.detailTop + enterShift);
        const int16_t dotsY = (int16_t)(layout.dotsY + enterShift);
        const int16_t buttonY = (int16_t)(layout.buttonY + enterShift);

        drawIcon()
            .pos(iconX, iconY)
            .size(iconSize)
            .icon(theme.iconId)
            .color(theme.accent565)
            .bgColor(bg565)
            .draw();

        static_cast<void>(setFont(WixMadeForDisplay));
        setFontSize(layout.headerPx);
        setFontWeight(Bold);
        drawTextAligned(String(theme.label), layout.centerX, headerY, theme.text565, bg565, TextAlign::Center);

        if (_flags.errorTransition)
        {
            const detail::ErrorEntry &nextEntry = _error.entries[clampIndex(_error.nextIndex, _error.count)];
            drawRollingText(currentEntry.message, nextEntry.message,
                            layout.centerX, messageY, layout.textMaxW,
                            theme.detail565, bg565, layout.bodyPx, Medium,
                            textProgress, _error.transitionDir, _error.contentStartMs);
            drawRollingCode(currentEntry.code, nextEntry.code,
                            layout.centerX, detailY, layout.textMaxW,
                            theme.detail565, theme.accent565, bg565, layout.detailPx,
                            textProgress, _error.transitionDir, _error.contentStartMs);
        }
        else
        {
            drawCenteredLine(currentEntry.message, layout.centerX, messageY, layout.textMaxW,
                             theme.detail565, bg565, layout.bodyPx, Medium,
                             !_flags.errorEntering, _error.contentStartMs);
            drawCodeLine(currentEntry.code, layout.centerX, detailY, layout.textMaxW,
                         theme.detail565, theme.accent565, bg565, layout.detailPx,
                         !_flags.errorEntering, _error.contentStartMs);
        }

        const uint8_t dotsCount = (_error.count > 1) ? _error.count : 0;
        const uint8_t dotsActive = (dotsCount > 0) ? (_flags.errorTransition ? clampIndex(_error.nextIndex, _error.count) : clampIndex(_error.currentIndex, _error.count)) : 0;
        const uint8_t dotsPrev = (dotsCount > 0) ? clampIndex(_error.currentIndex, _error.count) : 0;
        drawScrollDots()
            .pos(center, dotsY)
            .count(dotsCount)
            .activeIndex(dotsActive)
            .prevIndex(dotsPrev)
            .animProgress(rawProgress)
            .animDirection((_flags.errorTransition && _error.transitionDir < 0) ? -1 : 1)
            .activeColor(theme.dotsActive565)
            .inactiveColor(theme.dotsInactive565)
            .radius(3)
            .spacing(14)
            .draw();

        _error.buttonState.enabled = theme.dismissible;
        if (theme.dismissible)
        {
            const String label = currentEntry.buttonText.length() ? currentEntry.buttonText : String("OK");
            drawButton(label, layout.buttonX, buttonY, layout.buttonW, layout.buttonH,
                       theme.accent565, layout.buttonRadius, static_cast<IconId>(0xFFFF), _error.buttonState);
        }

        if (_flags.spriteEnabled && _disp.display)
        {
            invalidateRect(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight);
            flushDirty();
        }

        static_cast<void>(setFont(prevFont));
        setFontSize(prevSize);
        setFontWeight(prevWeight);
        _flags.inSpritePass = prevSpritePass;
        _render.activeSprite = prevActive;
    }

}

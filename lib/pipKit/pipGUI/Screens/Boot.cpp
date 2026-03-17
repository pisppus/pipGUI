#include <pipGUI/Core/pipGUI.hpp>
#include <math.h>

namespace pipgui
{

    static float logoEase(float t)
    {
        if (t < 0.5f)
            return 2.0f * t * t;
        return -2.0f * t * t + 3.0f * t - 0.5f;
    }

    static float bezierEase01(float t, float y1, float y2)
    {
        if (t <= 0.0f)
            return 0.0f;
        if (t >= 1.0f)
            return 1.0f;

        float u = 1.0f - t;
        float t2 = t * t;
        float u2 = u * u;

        return 3.0f * u2 * t * y1 + 3.0f * u * t2 * y2 + t2 * t;
    }

    void GUI::drawCenteredTitle(const String &title, const String &subtitle, uint16_t fg565, uint16_t bg565)
    {
        if (!_flags.spriteEnabled)
            return;

        clear(bg565);

        const uint16_t titlePx = _typo.logoTitleSizePx ? _typo.logoTitleSizePx : _typo.h1Px;
        const uint16_t subPx = _typo.logoSubtitleSizePx ? _typo.logoSubtitleSizePx : (uint16_t)((titlePx * 3) >> 2);

        // Title with KronaOne
        (void)setFont(KronaOne);
        int16_t titleW, titleH;
        setFontSize(titlePx);
        measureText(title, titleW, titleH);

        // Subtitle with WixMadeForDisplay
        (void)setFont(WixMadeForDisplay);
        int16_t subW = 0, subH = 0;
        const bool hasSub = subtitle.length() > 0;
        if (hasSub)
        {
            setFontSize(subPx);
            measureText(subtitle, subW, subH);
        }

        const int16_t totalH = titleH + (hasSub ? 4 + subH : 0);
        const int16_t topY = (_boot.y != -1) ? _boot.y : (int16_t)((_render.screenHeight - totalH) >> 1);
        const int16_t cx = (_boot.x != -1) ? _boot.x : (int16_t)(_render.screenWidth >> 1);

        // Draw title with KronaOne
        (void)setFont(KronaOne);
        setFontSize(titlePx);
        drawTextAligned(title, cx, topY, fg565, bg565, AlignCenter);

        // Draw subtitle with WixMadeForDisplay
        if (hasSub)
        {
            (void)setFont(WixMadeForDisplay);
            setFontSize(subPx);
            drawTextAligned(subtitle, cx, topY + titleH + 4, fg565, bg565, AlignCenter);
        }
    }

    void GUI::showLogo(const String &t, const String &s, BootAnimation a, uint32_t fg, uint32_t bg, uint32_t dur, int16_t x, int16_t y)
    {
        _flags.bootActive = 1;
        _boot.anim = a;
        _boot.title = t;
        _boot.subtitle = s;
        _boot.fgColor = fg;
        _boot.bgColor = bg;
        _boot.durationMs = dur;
        _boot.x = x;
        _boot.y = y;
        _boot.startMs = nowMs();
    }

    void GUI::renderBootFrame(uint32_t now)
    {
        if (!_flags.bootActive)
            return;

        uint32_t el = now - _boot.startMs, dur = _boot.durationMs ? _boot.durationMs : 1;
        bool done = (el >= dur);

        auto draw = [&](uint32_t fg)
        {
            const uint16_t fg565 = detail::color888To565(fg);
            const uint16_t bg565 = detail::color888To565(_boot.bgColor);
            if (_flags.spriteEnabled)
            {
                bool prevRender = _flags.inSpritePass;
                pipcore::Sprite *prevActive = _render.activeSprite;

                _render.activeSprite = &_render.sprite;
                _flags.inSpritePass = 1;
                drawCenteredTitle(_boot.title, _boot.subtitle, fg565, bg565);

                _flags.inSpritePass = prevRender;
                _render.activeSprite = prevActive;
                if (_disp.display && _flags.spriteEnabled)
                    presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
            }
            else
            {
                drawCenteredTitle(_boot.title, _boot.subtitle, fg565, bg565);
            }
        }; 

        switch (_boot.anim)
        {
        case BootAnimNone:
            draw(_boot.fgColor);
            done = true;
            break;

        case LightFade:
        {
            uint32_t p = done ? _disp.brightnessMax : (el * (uint32_t)_disp.brightnessMax) / dur;
            if (_disp.backlightCb)
                _disp.backlightCb(min((uint32_t)_disp.brightnessMax, p));
            draw(_boot.fgColor);
            break;
        }

        case FadeIn:
        {
            uint8_t p = done ? 255 : (el * 255U) / dur;
            uint32_t c = (p << 16) | (p << 8) | p;
            draw(c);
            break;
        }

        case ZoomIn:
        {
            float tn = (float)((el > dur) ? dur : el) / (float)dur;
            float zoomEase = bezierEase01(tn, 0.30f, 0.80f);
            float fadeEase = bezierEase01(tn, 0.20f, 0.90f);

            bool useSprite = _flags.spriteEnabled;
            bool prevRender = _flags.inSpritePass;
            pipcore::Sprite *prevActive = _render.activeSprite;

            if (useSprite)
            {
                _render.activeSprite = &_render.sprite;
                _flags.inSpritePass = 1;
            }

            clear(detail::color888To565(_boot.bgColor));

            uint8_t alpha = (uint8_t)(fadeEase * 255.0f + 0.5f);
            uint32_t fgBlend = detail::blend888(_boot.bgColor, _boot.fgColor, alpha);

            uint16_t baseTitlePx = _typo.logoTitleSizePx ? _typo.logoTitleSizePx : _typo.h1Px;
            uint16_t baseSubPx = _typo.logoSubtitleSizePx ? _typo.logoSubtitleSizePx : (uint16_t)((baseTitlePx * 3U) / 4U);

            float scale = 0.50f + 0.50f * zoomEase;

            uint16_t titlePx = (uint16_t)max(4, (int)(baseTitlePx * scale + 0.5f));
            uint16_t subPx = (uint16_t)max(4, (int)(baseSubPx * scale + 0.5f));

            int16_t titleW = 0;
            int16_t titleH = 0;
            (void)setFont(KronaOne);
            (void)setFontSize(titlePx);
            measureText(_boot.title, titleW, titleH);

            bool hasSub = _boot.subtitle.length() > 0;
            int16_t subW = 0;
            int16_t subH = 0;

            int16_t spacing = (int16_t)((float)baseTitlePx * 0.12f);
            if (spacing < 4)
                spacing = 4;

            if (hasSub)
            {
                (void)setFont(WixMadeForDisplay);
                (void)setFontSize(subPx);
                measureText(_boot.subtitle, subW, subH);
            }

            int16_t totalH = hasSub ? (titleH + spacing + subH) : titleH;
            int16_t topY = (_boot.y != -1) ? _boot.y : (int16_t)((_render.screenHeight - totalH) / 2);
            int16_t cx = (_boot.x != -1) ? _boot.x : (int16_t)(_render.screenWidth / 2);

            uint16_t fgBlend565 = detail::color888To565(fgBlend);
            uint16_t bg565 = detail::color888To565(_boot.bgColor);
            (void)setFont(KronaOne);
            (void)setFontSize(titlePx);
            drawTextAligned(_boot.title, cx, topY, fgBlend565, bg565, AlignCenter);

            if (hasSub)
            {
                int16_t subY = topY + titleH + spacing;
                uint16_t fgBlendSub = detail::color888To565(fgBlend);
                uint16_t bg565Sub = detail::color888To565(_boot.bgColor);
                (void)setFont(WixMadeForDisplay);
                (void)setFontSize(subPx);
                drawTextAligned(_boot.subtitle, cx, subY, fgBlendSub, bg565Sub, AlignCenter);
            }

            if (useSprite)
            {
                _flags.inSpritePass = prevRender;
                _render.activeSprite = prevActive;
                if (_disp.display && _flags.spriteEnabled)
                    presentSprite(0, 0, (int16_t)_render.screenWidth, (int16_t)_render.screenHeight, "present");
            }
            break;
        }

        case SlideUp:
        {
            auto t = getDrawTarget();

            int16_t th = 0, sh = 0, sp = 4;
            bool sub = _boot.subtitle.length() > 0;

            uint16_t titlePx = _typo.logoTitleSizePx ? _typo.logoTitleSizePx : _typo.h1Px;
            uint16_t subPx = _typo.logoSubtitleSizePx ? _typo.logoSubtitleSizePx : (uint16_t)((titlePx * 3U) / 4U);

            int16_t tmpW = 0;
            (void)setFont(KronaOne);
            (void)setFontSize(titlePx);
            measureText(_boot.title, tmpW, th);

            if (sub)
            {
                (void)setFont(WixMadeForDisplay);
                (void)setFontSize(subPx);
                measureText(_boot.subtitle, tmpW, sh);
            }

            int16_t totH = sub ? (th + sp + sh) : th;
            int16_t targetY = (_boot.y != -1) ? _boot.y : (_render.screenHeight - totH) / 2;

            uint32_t mot = min(dur, (uint32_t)1000UL), mel = min(el, mot);
            float eased = bezierEase01((float)mel / mot, 0.30f, 0.80f);
            uint32_t p = (uint32_t)(eased * 255.0f);

            int16_t off = _render.screenHeight - (int16_t)((_render.screenHeight - targetY) * p / 255);
            if (off < targetY)
                off = targetY;

            int16_t saveY = _boot.y;
            _boot.y = off;
            draw(_boot.fgColor);
            _boot.y = saveY;
            break;
        }
        }

        if (done)
        {
            _flags.bootActive = 0;
            _flags.needRedraw = 1;
            if (_boot.anim == LightFade && _disp.backlightCb)
                _disp.backlightCb(_disp.brightnessMax);
            // Reset font back to default after boot
            (void)setFont(WixMadeForDisplay);
        }
    }
}

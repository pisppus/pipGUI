#include <pipGUI/core/api/pipGUI.hpp>
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

    void GUI::showLogo(const String &t, const String &s, BootAnimation a, uint16_t fg, uint16_t bg, uint32_t dur, int16_t x, int16_t y)
    {
        _flags.bootActive = 1;
        _bootAnim = a;
        _bootTitle = t;
        _bootSubtitle = s;
        _bootFgColor = fg;
        _bootBgColor = bg;
        _bootDurationMs = dur;
        _bootX = x;
        _bootY = y;
        _bootStartMs = nowMs();
    }

    void GUI::renderBootFrame(uint32_t now)
    {
        if (!_flags.bootActive)
            return;
        uint32_t el = now - _bootStartMs, dur = _bootDurationMs ? _bootDurationMs : 1;
        bool done = (el >= dur);

        auto draw = [&](uint16_t fg)
        {
            if (_flags.spriteEnabled)
            {
                bool prevRender = _flags.renderToSprite;
                pipcore::Sprite *prevActive = _activeSprite;

                _activeSprite = &_sprite;
                _flags.renderToSprite = 1;
                drawCenteredTitle(_bootTitle, _bootSubtitle, fg, _bootBgColor);

                _flags.renderToSprite = prevRender;
                _activeSprite = prevActive;
                if (_display && _flags.spriteEnabled)
                    _sprite.writeToDisplay(*_display, 0, 0, (int16_t)_screenWidth, (int16_t)_screenHeight);
            }
            else
            {
                drawCenteredTitle(_bootTitle, _bootSubtitle, fg, _bootBgColor);
            }
        }; 

        switch (_bootAnim)
        {
        case BootAnimNone:
            draw(_bootFgColor);
            done = true;
            break;

        case LightFade:
        {
            uint32_t p = done ? _brightnessMax : (el * (uint32_t)_brightnessMax) / dur;
            if (_backlightCb)
                _backlightCb(min((uint32_t)_brightnessMax, p));
            draw(_bootFgColor);
            break;
        }

        case FadeIn:
        {
            uint8_t p = done ? 255 : (el * 255U) / dur;
            uint16_t c = rgb(p, p, p);
            draw(c);
            break;
        }

        case ZoomIn:
        {
            float tn = (float)((el > dur) ? dur : el) / (float)dur;
            float zoomEase = bezierEase01(tn, 0.30f, 0.80f);
            float fadeEase = bezierEase01(tn, 0.20f, 0.90f);

            bool useSprite = _flags.spriteEnabled;
            bool prevRender = _flags.renderToSprite;
            pipcore::Sprite *prevActive = _activeSprite;

            if (useSprite)
            {
                _activeSprite = &_sprite;
                _flags.renderToSprite = 1;
            }

            _sprite.fillScreen(_bootBgColor);

            uint8_t alpha = (uint8_t)(fadeEase * 255.0f + 0.5f);
            uint16_t fgBlend = detail::blend565(_bootBgColor, _bootFgColor, alpha);

            uint16_t baseTitlePx = _logoTitleSizePx ? _logoTitleSizePx : _textStyleH1Px;
            uint16_t baseSubPx = _logoSubtitleSizePx ? _logoSubtitleSizePx : (uint16_t)((baseTitlePx * 3U) / 4U);

            float scale = 0.50f + 0.50f * zoomEase;

            uint16_t titlePx = (uint16_t)max(4, (int)(baseTitlePx * scale + 0.5f));
            uint16_t subPx = (uint16_t)max(4, (int)(baseSubPx * scale + 0.5f));

            int16_t titleW = 0;
            int16_t titleH = 0;
            setPSDFFontSize(titlePx);
            psdfMeasureText(_bootTitle, titleW, titleH);

            bool hasSub = _bootSubtitle.length() > 0;
            int16_t subW = 0;
            int16_t subH = 0;

            int16_t spacing = (int16_t)((float)baseTitlePx * 0.12f);
            if (spacing < 4)
                spacing = 4;

            if (hasSub)
            {
                setPSDFFontSize(subPx);
                psdfMeasureText(_bootSubtitle, subW, subH);
            }

            int16_t totalH = hasSub ? (titleH + spacing + subH) : titleH;
            int16_t topY = (_bootY != -1) ? _bootY : (int16_t)((_screenHeight - totalH) / 2);
            int16_t cx = (_bootX != -1) ? _bootX : (int16_t)(_screenWidth / 2);

            setPSDFFontSize(titlePx);
            psdfDrawTextInternal(_bootTitle, cx, topY, fgBlend, _bootBgColor, AlignCenter);

            if (hasSub)
            {
                int16_t subY = topY + titleH + spacing;
                setPSDFFontSize(subPx);
                psdfDrawTextInternal(_bootSubtitle, cx, subY, fgBlend, _bootBgColor, AlignCenter);
            }

            if (useSprite)
            {
                _flags.renderToSprite = prevRender;
                _activeSprite = prevActive;
                if (_display && _flags.spriteEnabled)
                    _sprite.writeToDisplay(*_display, 0, 0, (int16_t)_screenWidth, (int16_t)_screenHeight);
            }
            break;
        }

        case SlideUp:
        {
            auto t = getDrawTarget();

            int16_t th = 0, sh = 0, sp = 4;
            bool sub = _bootSubtitle.length() > 0;

            uint16_t titlePx = _logoTitleSizePx ? _logoTitleSizePx : _textStyleH1Px;
            uint16_t subPx = _logoSubtitleSizePx ? _logoSubtitleSizePx : (uint16_t)((titlePx * 3U) / 4U);

            int16_t tmpW = 0;
            setPSDFFontSize(titlePx);
            psdfMeasureText(_bootTitle, tmpW, th);

            if (sub)
            {
                setPSDFFontSize(subPx);
                psdfMeasureText(_bootSubtitle, tmpW, sh);
            }

            int16_t totH = sub ? (th + sp + sh) : th;
            int16_t targetY = (_bootY != -1) ? _bootY : (_screenHeight - totH) / 2;

            uint32_t mot = min(dur, (uint32_t)1000UL), mel = min(el, mot);
            float eased = bezierEase01((float)mel / mot, 0.30f, 0.80f);
            uint32_t p = (uint32_t)(eased * 255.0f);

            int16_t off = _screenHeight - (int16_t)((_screenHeight - targetY) * p / 255);
            if (off < targetY)
                off = targetY;

            int16_t saveY = _bootY;
            _bootY = off;
            draw(_bootFgColor);
            _bootY = saveY;
            break;
        }
        }

        if (done)
        {
            _flags.bootActive = 0;
            _flags.needRedraw = 1;
            if (_bootAnim == LightFade && _backlightCb)
                _backlightCb(_brightnessMax);
        }
    }
}
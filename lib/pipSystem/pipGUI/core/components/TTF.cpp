#include <pipGUI/core/api/pipGUI.h>
#include <pipGUI/icons/error.h>
#include <pipGUI/fonts/KronaOne.h>
#include <pipGUI/fonts/WixMadeforDisplay-Bold.h>
#include <pipGUI/fonts/WixMadeforDisplay-Medium.h>
#include <pipGUI/fonts/WixMadeforDisplay-Regular.h>
#include <pipGUI/fonts/WixMadeforDisplay-SemiBold.h>

namespace pipgui
{

    bool GUI::loadTTF(const unsigned char *data, size_t size, uint8_t faceIndex)
    {
        if (!data || size == 0)
        {
            _flags.ttfLoaded = 0;
            return false;
        }

        if (platform())
            platform()->configureOpenFontRender();

        FT_Error err = _ttf.loadFont(data, size, faceIndex);

        if (!err)
        {
            _ttf.setBackgroundFillMethod(BgFillMethod::None);
            _flags.ttfLoaded = 1;
        }
        else
        {
            _flags.ttfLoaded = 0;
        }
        return _flags.ttfLoaded;
    }

    bool GUI::loadBuiltinTTF(BuiltinTTF font)
    {
        switch (font)
        {
        case KronaOne:
            return loadTTF(gBuiltinKronaOneTTF, sizeof(gBuiltinKronaOneTTF));
        case WixRegular:
            return loadTTF(gBuiltinWixRegularTTF, sizeof(gBuiltinWixRegularTTF));
        case WixMedium:
            return loadTTF(gBuiltinWixMediumTTF, sizeof(gBuiltinWixMediumTTF));
        case WixSemiBold:
            return loadTTF(gBuiltinWixSemiBoldTTF, sizeof(gBuiltinWixSemiBoldTTF));
        case WixBold:
            return loadTTF(gBuiltinWixBoldTTF, sizeof(gBuiltinWixBoldTTF));
        default:
            break;
        }
        return false;
    }

    bool GUI::ttfFontLoaded() const
    {
        return _flags.ttfLoaded;
    }

    void GUI::setTTFFontSize(uint16_t px)
    {
        if (!_flags.ttfLoaded)
            return;
        _ttf.setFontSize(px);
    }

    void GUI::setTTFFontColor(uint16_t fg, uint16_t bg)
    {
        if (!_flags.ttfLoaded)
            return;
        _ttf.setFontColor(fg, bg);
    }

    void GUI::configureTextStyles(uint16_t h1Px, uint16_t h2Px, uint16_t bodyPx, uint16_t captionPx)
    {
        _textStyleH1Px = h1Px;
        _textStyleH2Px = h2Px;
        _textStyleBodyPx = bodyPx;
        _textStyleCaptionPx = captionPx;
    }

    void GUI::setTextStyle(TextStyle style)
    {
        if (!_flags.ttfLoaded)
            return;

        uint16_t px = _textStyleBodyPx;
        switch (style)
        {
        case H1:
            px = _textStyleH1Px;
            break;
        case H2:
            px = _textStyleH2Px;
            break;
        case Body:
            px = _textStyleBodyPx;
            break;
        case Caption:
            px = _textStyleCaptionPx;
            break;
        default:
            break;
        }

        setTTFFontSize(px);
    }

    void GUI::drawTTFText(const String &text, int16_t x, int16_t y, uint16_t color, uint16_t bgColor, TextAlign align)
    {
        if (!_flags.ttfLoaded)
            return;
        auto target = getDrawTarget();
        _ttf.setDrawer(*target);
        _ttf.setFontColor(color, bgColor);

        uint32_t tw = _ttf.getTextWidth("%s", text.c_str());
        uint32_t th = _ttf.getTextHeight("%s", text.c_str());

        if (x == -1)
            x = AutoX((int32_t)tw);

        else if (align == Center)
            x -= tw / 2;
        else if (align == Right)
            x -= tw;
        if (y == -1)
            y = AutoY((int32_t)th);

        _ttf.drawString(text.c_str(), x, y, color, bgColor, Layout::Horizontal);
    }

    void GUI::drawTTFText(const String &text, int16_t x, int16_t y, uint16_t color, TextAlign align)
    {
        drawTTFText(text, x, y, color, _bgColor, align);
    }

    void GUI::updateTTFText(const String &text, int16_t x, int16_t y, uint16_t color, uint16_t bgColor, TextAlign align)
    {
        if (!_flags.ttfLoaded)
            return;

        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;

            _flags.renderToSprite = 0;
            drawTTFText(text, x, y, color, bgColor, align);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        _ttf.setFontColor(color, bgColor);

        uint32_t tw = _ttf.getTextWidth("%s", text.c_str());
        uint32_t th = _ttf.getTextHeight("%s", text.c_str());
        if (tw == 0 || th == 0)
            return;

        int16_t rx = x;
        int16_t ry = y;

        if (rx == -1)
            rx = AutoX((int32_t)tw);
        else if (align == Center)
            rx -= (int16_t)(tw / 2);
        else if (align == Right)
            rx -= (int16_t)tw;

        if (ry == -1)
            ry = AutoY((int32_t)th);

        int16_t pad = 4;
        int16_t rw = (int16_t)tw;
        int16_t rh = (int16_t)th + 2;

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;

        _sprite.fillRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(rw + pad * 2), (int16_t)(rh + pad * 2), bgColor);

        _ttf.setDrawer(_sprite);
        _ttf.setFontColor(color, bgColor);
        _ttf.drawString(text.c_str(), rx, ry, color, bgColor, Layout::Horizontal);

        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        invalidateRect((int16_t)(rx - pad), (int16_t)(ry - pad), (int16_t)(rw + pad * 2), (int16_t)(rh + pad * 2));
        flushDirty();
    }

    void GUI::drawTTFTextStyle(const String &text, TextStyle style, int16_t x, int16_t y, uint16_t color, TextAlign align, int32_t bgColor)
    {
        if (!_flags.ttfLoaded)
            return;

        setTextStyle(style);
        uint16_t bg = (bgColor >= 0) ? static_cast<uint16_t>(bgColor) : _bgColor;
        drawTTFText(text, x, y, color, bg, align);
    }

    static OpenFontRender g_errorEmojiFont;
    static bool g_errorEmojiFontInited = false;

    static void ensureErrorEmojiFont()
    {
        if (g_errorEmojiFontInited)
            return;

        pipcore::GuiPlatform *p = pipgui::GUI::sharedPlatform();
        if (p)
            p->configureOpenFontRender();

        FT_Error e = g_errorEmojiFont.loadFont(gErrorEmojiTTF, sizeof(gErrorEmojiTTF), 0);
        if (!e)
        {
            g_errorEmojiFont.setBackgroundFillMethod(BgFillMethod::None);
            g_errorEmojiFontInited = true;
        }
    }

    void GUI::drawEmojiIconTTF(const String &emoji,
                               int16_t x,
                               int16_t y,
                               uint16_t sizePx,
                               uint16_t color,
                               int32_t bgColor)
    {
        if (!emoji.length() || sizePx == 0)
            return;

        ensureErrorEmojiFont();
        if (!g_errorEmojiFontInited)
            return;

        auto target = getDrawTarget();
        g_errorEmojiFont.setDrawer(*target);

        g_errorEmojiFont.setFontSize(sizePx);

        uint16_t bg = (bgColor >= 0) ? static_cast<uint16_t>(bgColor) : _bgColor;
        g_errorEmojiFont.setFontColor(color, bg);

        uint32_t tw = g_errorEmojiFont.getTextWidth("%s", emoji.c_str());
        uint32_t th = g_errorEmojiFont.getTextHeight("%s", emoji.c_str());

        if (tw == 0 || th == 0)
        {
            int16_t cx = x;
            int16_t cy = y;
            int16_t r = (int16_t)(sizePx / 2);
            if (r < 8)
                r = 8;

            target->fillSmoothCircle(cx, cy, r, color);

            int16_t triH = (int16_t)(r * 1.1f);
            if (triH < 6)
                triH = 6;
            int16_t triW = (int16_t)(r * 0.7f);
            if (triW < 4)
                triW = 4;

            int16_t tipX = cx;
            int16_t tipY = cy - triH / 3;
            int16_t baseY = tipY + triH;

            target->fillTriangle(tipX,
                                 tipY,
                                 tipX - triW,
                                 baseY,
                                 tipX + triW,
                                 baseY,
                                 0x0000);
            return;
        }

        int16_t drawX = x - static_cast<int16_t>(tw) / 2;
        int16_t drawY = y - static_cast<int16_t>(th) / 2;

        g_errorEmojiFont.drawString(emoji.c_str(), drawX, drawY, color, bg, Layout::Horizontal);
    }

}

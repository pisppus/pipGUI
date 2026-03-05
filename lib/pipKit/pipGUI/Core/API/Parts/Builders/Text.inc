#if !defined(PIPGUI_FLUENT_BUILDERS_INC)
 #error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
 #endif
 
struct DrawTextFluent
{
    GUI *_gui;
    int16_t _x, _y;
    FontId _fontId;
    uint16_t _sizePx;
    String _text;
    uint16_t _fg565;
    uint16_t _bg565;
    TextAlign _align;
    bool _consumed;

    DrawTextFluent(GUI *g)
        : _gui(g),
          _x(-1), _y(-1),
          _fontId((FontId)0),
          _sizePx(0),
          _text(),
          _fg565(0xFFFF),
          _bg565(0x0000),
          _align(AlignLeft),
          _consumed(false)
    {
    }

    DrawTextFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _x = x;
        _y = y;
        return *this;
    }

    DrawTextFluent &font(FontId fontId)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _fontId = fontId;
        return *this;
    }

    DrawTextFluent &size(uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _sizePx = sizePx;
        return *this;
    }

    DrawTextFluent &text(const String &t)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _text = t;
        return *this;
    }

    DrawTextFluent &color(uint16_t fg565)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _fg565 = fg565;
        return *this;
    }

    DrawTextFluent &bgColor(uint16_t bg565)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _bg565 = bg565;
        return *this;
    }

    DrawTextFluent &align(TextAlign a)
    {
        PIPGUI_FLUENT_GUARD(DrawTextFluent);
        _align = a;
        return *this;
    }

    void draw();
};

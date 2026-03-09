#if !defined(PIPGUI_FLUENT_BUILDERS_INC)
 #error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
 #endif
 
template <bool IsUpdate>
struct TextFluentT
{
    GUI *_gui;
    int16_t _x, _y;
    FontId _fontId;
    uint16_t _sizePx;
    uint16_t _weight;
    String _text;
    uint16_t _fg565;
    uint16_t _bg565;
    TextAlign _align;
    bool _consumed;

    TextFluentT(GUI *g)
        : _gui(g),
          _x(-1), _y(-1),
          _fontId((FontId)0),
          _sizePx(0),
          _weight(0),
          _text(),
          _fg565(0xFFFF),
          _bg565(0x0000),
          _align(AlignLeft),
          _consumed(false)
    {
    }

    ~TextFluentT() { draw(); }

    TextFluentT &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _x = x;
        _y = y;
        return *this;
    }

    TextFluentT &font(FontId fontId)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _fontId = fontId;
        return *this;
    }

    TextFluentT &size(uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _sizePx = sizePx;
        return *this;
    }

    TextFluentT &weight(uint16_t weight)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _weight = weight;
        return *this;
    }

    TextFluentT &weight(WeightToken weight)
    {
        return this->weight(weight.value);
    }

    TextFluentT &text(const String &t)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _text = t;
        return *this;
    }

    TextFluentT &color(uint16_t fg565)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _fg565 = fg565;
        return *this;
    }

    TextFluentT &bgColor(uint16_t bg565)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _bg565 = bg565;
        return *this;
    }

    TextFluentT &align(TextAlign a)
    {
        PIPGUI_FLUENT_GUARD(TextFluentT);
        _align = a;
        return *this;
    }

    void draw();
};

using DrawTextFluent = TextFluentT<false>;
using UpdateTextFluent = TextFluentT<true>;

struct DrawTextMarqueeFluent
{
    GUI *_gui;
    int16_t _x, _y, _maxWidth;
    FontId _fontId;
    uint16_t _sizePx;
    uint16_t _weight;
    String _text;
    uint16_t _fg565;
    TextAlign _align;
    MarqueeTextOptions _opts;
    bool _consumed;

    DrawTextMarqueeFluent(GUI *g)
        : _gui(g),
          _x(-1), _y(-1), _maxWidth(0),
          _fontId((FontId)0),
          _sizePx(0),
          _weight(0),
          _text(),
          _fg565(0xFFFF),
          _align(AlignLeft),
          _opts(),
          _consumed(false)
    {
    }

    ~DrawTextMarqueeFluent() { draw(); }

    DrawTextMarqueeFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _x = x;
        _y = y;
        return *this;
    }

    DrawTextMarqueeFluent &maxWidth(int16_t width)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _maxWidth = width;
        return *this;
    }

    DrawTextMarqueeFluent &font(FontId fontId)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _fontId = fontId;
        return *this;
    }

    DrawTextMarqueeFluent &size(uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _sizePx = sizePx;
        return *this;
    }

    DrawTextMarqueeFluent &weight(uint16_t weight)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _weight = weight;
        return *this;
    }

    DrawTextMarqueeFluent &weight(WeightToken weight)
    {
        return this->weight(weight.value);
    }

    DrawTextMarqueeFluent &text(const String &t)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _text = t;
        return *this;
    }

    DrawTextMarqueeFluent &color(uint16_t fg565)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _fg565 = fg565;
        return *this;
    }

    DrawTextMarqueeFluent &align(TextAlign a)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _align = a;
        return *this;
    }

    DrawTextMarqueeFluent &options(const MarqueeTextOptions &opts)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _opts = opts;
        return *this;
    }

    DrawTextMarqueeFluent &speed(uint16_t pxPerSec)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _opts.speedPxPerSec = pxPerSec;
        return *this;
    }

    DrawTextMarqueeFluent &holdStart(uint16_t ms)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _opts.holdStartMs = ms;
        return *this;
    }

    DrawTextMarqueeFluent &phaseStart(uint32_t ms)
    {
        PIPGUI_FLUENT_GUARD(DrawTextMarqueeFluent);
        _opts.phaseStartMs = ms;
        return *this;
    }

    void draw();
};

struct DrawTextEllipsizedFluent
{
    GUI *_gui;
    int16_t _x, _y, _maxWidth;
    FontId _fontId;
    uint16_t _sizePx;
    uint16_t _weight;
    String _text;
    uint16_t _fg565;
    TextAlign _align;
    bool _consumed;

    DrawTextEllipsizedFluent(GUI *g)
        : _gui(g),
          _x(-1), _y(-1), _maxWidth(0),
          _fontId((FontId)0),
          _sizePx(0),
          _weight(0),
          _text(),
          _fg565(0xFFFF),
          _align(AlignLeft),
          _consumed(false)
    {
    }

    ~DrawTextEllipsizedFluent() { draw(); }

    DrawTextEllipsizedFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _x = x;
        _y = y;
        return *this;
    }

    DrawTextEllipsizedFluent &maxWidth(int16_t width)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _maxWidth = width;
        return *this;
    }

    DrawTextEllipsizedFluent &font(FontId fontId)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _fontId = fontId;
        return *this;
    }

    DrawTextEllipsizedFluent &size(uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _sizePx = sizePx;
        return *this;
    }

    DrawTextEllipsizedFluent &weight(uint16_t weight)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _weight = weight;
        return *this;
    }

    DrawTextEllipsizedFluent &weight(WeightToken weight)
    {
        return this->weight(weight.value);
    }

    DrawTextEllipsizedFluent &text(const String &t)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _text = t;
        return *this;
    }

    DrawTextEllipsizedFluent &color(uint16_t fg565)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _fg565 = fg565;
        return *this;
    }

    DrawTextEllipsizedFluent &align(TextAlign a)
    {
        PIPGUI_FLUENT_GUARD(DrawTextEllipsizedFluent);
        _align = a;
        return *this;
    }

    void draw();
};

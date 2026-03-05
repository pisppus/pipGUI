#if !defined(PIPGUI_FLUENT_BUILDERS_INC)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

template <bool IsUpdate>
struct ToggleSwitchFluentT
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    ToggleSwitchState *_state;
    uint16_t _activeColor;
    int32_t _inactiveColor;
    int32_t _knobColor;
    bool _consumed;

    ToggleSwitchFluentT(GUI *g)
        : _gui(g),
          _x(0), _y(0), _w(0), _h(0),
          _state(nullptr),
          _activeColor(0),
          _inactiveColor(-1),
          _knobColor(-1),
          _consumed(false)
    {
    }

    ToggleSwitchFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }

    ToggleSwitchFluentT &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _w = w;
        _h = h;
        return *this;
    }

    ToggleSwitchFluentT &state(ToggleSwitchState &s)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _state = &s;
        return *this;
    }

    ToggleSwitchFluentT &activeColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _activeColor = c;
        return *this;
    }

    ToggleSwitchFluentT &inactiveColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _inactiveColor = c;
        return *this;
    }

    ToggleSwitchFluentT &knobColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(ToggleSwitchFluentT<IsUpdate>);
        _knobColor = c;
        return *this;
    }

    void draw();
};

using DrawToggleSwitchFluent = ToggleSwitchFluentT<false>;
using UpdateToggleSwitchFluent = ToggleSwitchFluentT<true>;

template <bool IsUpdate>
struct ButtonFluentT
{
    GUI *_gui;
    String _label;
    int16_t _x, _y, _w, _h;
    uint16_t _baseColor;
    uint8_t _radius;
    const ButtonVisualState *_state;
    bool _consumed;

    ButtonFluentT(GUI *g)
        : _gui(g),
          _label(),
          _x(0), _y(0), _w(0), _h(0),
          _baseColor(0),
          _radius(6),
          _state(nullptr),
          _consumed(false)
    {
    }

    ButtonFluentT &label(const String &t)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _label = t;
        return *this;
    }

    ButtonFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }

    ButtonFluentT &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _w = w;
        _h = h;
        return *this;
    }

    ButtonFluentT &baseColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _baseColor = c;
        return *this;
    }

    ButtonFluentT &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _radius = r;
        return *this;
    }

    ButtonFluentT &state(const ButtonVisualState &s)
    {
        PIPGUI_FLUENT_GUARD(ButtonFluentT<IsUpdate>);
        _state = &s;
        return *this;
    }

    void draw();
};

using DrawButtonFluent = ButtonFluentT<false>;
using UpdateButtonFluent = ButtonFluentT<true>;

template <bool IsUpdate>
struct ProgressBarFluentT
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint8_t _value;
    uint16_t _baseColor;
    uint16_t _fillColor;
    uint8_t _radius;
    ProgressAnim _anim;
    bool _doFlush;
    bool _consumed;

    ProgressBarFluentT(GUI *g)
        : _gui(g),
          _x(0), _y(0), _w(0), _h(0),
          _value(0),
          _baseColor(0),
          _fillColor(0),
          _radius(6),
          _anim(None),
          _doFlush(true),
          _consumed(false)
    {
    }

    ProgressBarFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }

    ProgressBarFluentT &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _w = w;
        _h = h;
        return *this;
    }

    ProgressBarFluentT &value(uint8_t v)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _value = v;
        return *this;
    }

    ProgressBarFluentT &baseColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _baseColor = c;
        return *this;
    }

    ProgressBarFluentT &fillColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _fillColor = c;
        return *this;
    }

    ProgressBarFluentT &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _radius = r;
        return *this;
    }

    ProgressBarFluentT &anim(ProgressAnim a)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _anim = a;
        return *this;
    }

    ProgressBarFluentT &doFlush(bool v)
    {
        PIPGUI_FLUENT_GUARD(ProgressBarFluentT<IsUpdate>);
        _doFlush = v;
        return *this;
    }

    void draw();
};

using DrawProgressBarFluent = ProgressBarFluentT<false>;
using UpdateProgressBarFluent = ProgressBarFluentT<true>;

template <bool IsUpdate>
struct CircularProgressBarFluentT
{
    GUI *_gui;
    int16_t _x, _y;
    int16_t _r;
    uint8_t _thickness;
    uint8_t _value;
    uint16_t _baseColor;
    uint16_t _fillColor;
    ProgressAnim _anim;
    bool _doFlush;
    bool _consumed;

    CircularProgressBarFluentT(GUI *g)
        : _gui(g),
          _x(0), _y(0),
          _r(0),
          _thickness(1),
          _value(0),
          _baseColor(0),
          _fillColor(0),
          _anim(None),
          _doFlush(true),
          _consumed(false)
    {
    }

    CircularProgressBarFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }

    CircularProgressBarFluentT &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _r = r;
        return *this;
    }

    CircularProgressBarFluentT &thickness(uint8_t t)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _thickness = t;
        return *this;
    }

    CircularProgressBarFluentT &value(uint8_t v)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _value = v;
        return *this;
    }

    CircularProgressBarFluentT &baseColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _baseColor = c;
        return *this;
    }

    CircularProgressBarFluentT &fillColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _fillColor = c;
        return *this;
    }

    CircularProgressBarFluentT &anim(ProgressAnim a)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _anim = a;
        return *this;
    }

    CircularProgressBarFluentT &doFlush(bool v)
    {
        PIPGUI_FLUENT_GUARD(CircularProgressBarFluentT<IsUpdate>);
        _doFlush = v;
        return *this;
    }

    void draw();
};

using DrawCircularProgressBarFluent = CircularProgressBarFluentT<false>;
using UpdateCircularProgressBarFluent = CircularProgressBarFluentT<true>;

template <bool IsUpdate>
struct ScrollDotsFluentT
{
    GUI *_gui;
    int16_t _x, _y;
    uint8_t _count, _activeIndex, _prevIndex;
    float _animProgress;
    bool _animate;
    int8_t _animDirection;
    uint16_t _activeColor, _inactiveColor;
    uint8_t _dotRadius, _spacing, _activeWidth;
    bool _consumed;
    ScrollDotsFluentT(GUI *g)
        : _gui(g), _x(0), _y(0), _count(0), _activeIndex(0), _prevIndex(0), _animProgress(0), _animate(false), _animDirection(0),
          _activeColor(0xFFFF), _inactiveColor(0x7BEF), _dotRadius(3), _spacing(14), _activeWidth(18), _consumed(false) {}
    ScrollDotsFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }
    ScrollDotsFluentT &count(uint8_t n)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _count = n;
        return *this;
    }
    ScrollDotsFluentT &activeIndex(uint8_t i)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _activeIndex = i;
        return *this;
    }
    ScrollDotsFluentT &prevIndex(uint8_t i)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _prevIndex = i;
        return *this;
    }
    ScrollDotsFluentT &animProgress(float p)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _animProgress = p;
        return *this;
    }
    ScrollDotsFluentT &animate(bool a)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _animate = a;
        return *this;
    }
    ScrollDotsFluentT &animDirection(int8_t d)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _animDirection = d;
        return *this;
    }
    ScrollDotsFluentT &activeColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _activeColor = c;
        return *this;
    }
    ScrollDotsFluentT &inactiveColor(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _inactiveColor = c;
        return *this;
    }
    ScrollDotsFluentT &dotRadius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _dotRadius = r;
        return *this;
    }
    ScrollDotsFluentT &spacing(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _spacing = s;
        return *this;
    }
    ScrollDotsFluentT &activeWidth(uint8_t w)
    {
        PIPGUI_FLUENT_GUARD(ScrollDotsFluentT<IsUpdate>);
        _activeWidth = w;
        return *this;
    }
    void draw();
};
using DrawScrollDotsFluent = ScrollDotsFluentT<false>;
using UpdateScrollDotsFluent = ScrollDotsFluentT<true>;

struct ToastFluent
{
    GUI *_gui;
    String _text;
    uint32_t _durationMs;
    bool _fromTop;
    IconId _iconId;
    uint16_t _iconSizePx;
    bool _consumed;

    ToastFluent(GUI *g)
        : _gui(g),
          _text(),
          _durationMs(2500),
          _fromTop(false),
          _iconId(WarningLayer0),
          _iconSizePx(20),
          _consumed(false) {}

    ToastFluent &text(const String &t)
    {
        PIPGUI_FLUENT_GUARD(ToastFluent);
        _text = t;
        return *this;
    }

    ToastFluent &duration(uint32_t ms)
    {
        PIPGUI_FLUENT_GUARD(ToastFluent);
        _durationMs = ms;
        return *this;
    }

    ToastFluent &fromTop(bool v = true)
    {
        PIPGUI_FLUENT_GUARD(ToastFluent);
        _fromTop = v;
        return *this;
    }

    ToastFluent &icon(IconId id, uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(ToastFluent);
        _iconId = id;
        _iconSizePx = sizePx;
        return *this;
    }

    void show();
};

struct DrawIconFluent
{
    GUI *_gui;
    int16_t _x, _y;
    uint16_t _sizePx;
    uint16_t _iconId;
    uint16_t _fg565;
    uint16_t _bg565;
    bool _consumed;

    DrawIconFluent(GUI *g)
        : _gui(g),
          _x(-1), _y(-1),
          _sizePx(0),
          _iconId(0),
          _fg565(0xFFFF),
          _bg565(0x0000),
          _consumed(false)
    {
    }

    DrawIconFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawIconFluent);
        _x = x;
        _y = y;
        return *this;
    }

    DrawIconFluent &size(uint16_t sizePx)
    {
        PIPGUI_FLUENT_GUARD(DrawIconFluent);
        _sizePx = sizePx;
        return *this;
    }

    DrawIconFluent &icon(uint16_t iconId)
    {
        PIPGUI_FLUENT_GUARD(DrawIconFluent);
        _iconId = iconId;
        return *this;
    }

    DrawIconFluent &color(uint16_t fg565)
    {
        PIPGUI_FLUENT_GUARD(DrawIconFluent);
        _fg565 = fg565;
        return *this;
    }

    DrawIconFluent &bgColor(uint16_t bg565)
    {
        PIPGUI_FLUENT_GUARD(DrawIconFluent);
        _bg565 = bg565;
        return *this;
    }

    void draw();
};

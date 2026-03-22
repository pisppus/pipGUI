#pragma once

namespace pipgui
{

    template <bool IsUpdate>
    struct ToggleSwitchFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ToggleSwitchFluentT);
        int16_t _x, _y, _w, _h;
        bool *_value;
        bool *_changed;
        bool _pressed;
        bool _enabled;
        bool _pressedSet;
        bool _enabledSet;
        uint16_t _activeColor;
        std::optional<uint16_t> _inactiveColor;
        std::optional<uint16_t> _knobColor;
        ToggleSwitchFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _value(nullptr),
              _changed(nullptr),
              _pressed(false),
              _enabled(true),
              _pressedSet(false),
              _enabledSet(false),
              _activeColor(0),
              _inactiveColor(std::nullopt),
              _knobColor(std::nullopt)
        {
        }

        ~ToggleSwitchFluentT() { draw(); }

        ToggleSwitchFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        ToggleSwitchFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        ToggleSwitchFluentT &value(bool &v)
        {
            if (!canMutate())
                return *this;
            _value = &v;
            return *this;
        }

        ToggleSwitchFluentT &pressed(bool v)
        {
            if (!canMutate())
                return *this;
            _pressed = v;
            _pressedSet = true;
            return *this;
        }

        ToggleSwitchFluentT &enabled(bool v = true)
        {
            if (!canMutate())
                return *this;
            _enabled = v;
            _enabledSet = true;
            return *this;
        }

        ToggleSwitchFluentT &changed(bool &v)
        {
            if (!canMutate())
                return *this;
            _changed = &v;
            return *this;
        }

        ToggleSwitchFluentT &activeColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _activeColor = c;
            return *this;
        }

        ToggleSwitchFluentT &inactiveColor(int32_t c)
        {
            if (!canMutate())
                return *this;
            detail::assignOptionalColor(_inactiveColor, c);
            return *this;
        }

        ToggleSwitchFluentT &knobColor(int32_t c)
        {
            if (!canMutate())
                return *this;
            detail::assignOptionalColor(_knobColor, c);
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct ButtonFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ButtonFluentT);
        String _label;
        int16_t _x, _y, _w, _h;
        uint16_t _baseColor;
        uint8_t _radius;
        IconId _iconId;
        bool _enabled;
        bool _loading;
        bool _down;
        bool _modeSet;
        bool _downSet;
        ButtonFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _label(),
              _x(0), _y(0), _w(0), _h(0),
              _baseColor(0),
              _radius(6),
              _iconId(static_cast<IconId>(0xFFFF)),
              _enabled(true),
              _loading(false),
              _down(false),
              _modeSet(false),
              _downSet(false)
        {
        }

        ~ButtonFluentT() { draw(); }

        ButtonFluentT &label(const String &t)
        {
            if (!canMutate())
                return *this;
            _label = t;
            return *this;
        }

        ButtonFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        ButtonFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        ButtonFluentT &baseColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _baseColor = c;
            return *this;
        }

        ButtonFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }

        ButtonFluentT &icon(IconId id)
        {
            if (!canMutate())
                return *this;
            _iconId = id;
            return *this;
        }

        ButtonFluentT &mode(bool enabled = true, bool loading = false)
        {
            if (!canMutate())
                return *this;
            _enabled = enabled;
            _loading = loading;
            _modeSet = true;
            return *this;
        }

        ButtonFluentT &down(bool v)
        {
            if (!canMutate())
                return *this;
            _down = v;
            _downSet = true;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct ProgressFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ProgressFluentT);
        int16_t _x, _y, _w, _h;
        uint8_t _value;
        uint16_t _baseColor;
        uint16_t _fillColor;
        uint8_t _radius;
        ProgressAnim _anim;
        String _label;
        bool _showLabel;
        uint16_t _labelColor;
        TextAlign _labelAlign;
        uint16_t _labelFontPx;
        bool _showPercent;
        uint16_t _percentColor;
        TextAlign _percentAlign;
        uint16_t _percentFontPx;

        ProgressFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _value(0),
              _baseColor(0),
              _fillColor(0),
              _radius(6),
              _anim(None),
              _label(),
              _showLabel(false),
              _labelColor(0xFFFF),
              _labelAlign(Left),
              _labelFontPx(0),
              _showPercent(false),
              _percentColor(0xFFFF),
              _percentAlign(Right),
              _percentFontPx(0)
        {
        }

        ~ProgressFluentT() { draw(); }

        ProgressFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        ProgressFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        ProgressFluentT &value(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _value = v;
            return *this;
        }

        ProgressFluentT &baseColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _baseColor = c;
            return *this;
        }

        ProgressFluentT &fillColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _fillColor = c;
            return *this;
        }

        ProgressFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }

        ProgressFluentT &anim(ProgressAnim a)
        {
            if (!canMutate())
                return *this;
            _anim = a;
            return *this;
        }

        ProgressFluentT &label(const String &text)
        {
            if (!canMutate())
                return *this;
            _label = text;
            _showLabel = text.length() > 0;
            return *this;
        }

        ProgressFluentT &labelColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _labelColor = c;
            return *this;
        }

        ProgressFluentT &labelAlign(TextAlign a)
        {
            if (!canMutate())
                return *this;
            _labelAlign = a;
            return *this;
        }

        ProgressFluentT &labelFont(uint16_t px)
        {
            if (!canMutate())
                return *this;
            _labelFontPx = px;
            return *this;
        }

        ProgressFluentT &percent(bool enabled = true)
        {
            if (!canMutate())
                return *this;
            _showPercent = enabled;
            return *this;
        }

        ProgressFluentT &percentColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _percentColor = c;
            return *this;
        }

        ProgressFluentT &percentAlign(TextAlign a)
        {
            if (!canMutate())
                return *this;
            _percentAlign = a;
            return *this;
        }

        ProgressFluentT &percentFont(uint16_t px)
        {
            if (!canMutate())
                return *this;
            _percentFontPx = px;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct CircleProgressFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(CircleProgressFluentT);
        int16_t _x, _y;
        int16_t _r;
        uint8_t _thickness;
        uint8_t _value;
        uint16_t _baseColor;
        uint16_t _fillColor;
        ProgressAnim _anim;

        CircleProgressFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0),
              _r(0),
              _thickness(1),
              _value(0),
              _baseColor(0),
              _fillColor(0),
              _anim(None)
        {
        }

        ~CircleProgressFluentT() { draw(); }

        CircleProgressFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        CircleProgressFluentT &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _r = r;
            return *this;
        }

        CircleProgressFluentT &thickness(uint8_t t)
        {
            if (!canMutate())
                return *this;
            _thickness = t;
            return *this;
        }

        CircleProgressFluentT &value(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _value = v;
            return *this;
        }

        CircleProgressFluentT &baseColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _baseColor = c;
            return *this;
        }

        CircleProgressFluentT &fillColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _fillColor = c;
            return *this;
        }

        CircleProgressFluentT &anim(ProgressAnim a)
        {
            if (!canMutate())
                return *this;
            _anim = a;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct ScrollDotsFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ScrollDotsFluentT);
        int16_t _x, _y;
        uint8_t _count, _activeIndex, _prevIndex;
        float _animProgress;
        bool _animate;
        int8_t _animDirection;
        uint16_t _activeColor, _inactiveColor;
        uint8_t _dotRadius, _spacing, _activeWidth;
        ScrollDotsFluentT(GUI *g)
            : detail::FluentLifetime(g), _x(0), _y(0), _count(0), _activeIndex(0), _prevIndex(0), _animProgress(0), _animate(false), _animDirection(0),
              _activeColor(0xFFFF), _inactiveColor(0x7BEF), _dotRadius(3), _spacing(14), _activeWidth(18) {}

        ~ScrollDotsFluentT() { draw(); }
        ScrollDotsFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        ScrollDotsFluentT &count(uint8_t n)
        {
            if (!canMutate())
                return *this;
            _count = n;
            return *this;
        }
        ScrollDotsFluentT &activeIndex(uint8_t i)
        {
            if (!canMutate())
                return *this;
            _activeIndex = i;
            return *this;
        }
        ScrollDotsFluentT &prevIndex(uint8_t i)
        {
            if (!canMutate())
                return *this;
            _prevIndex = i;
            return *this;
        }
        ScrollDotsFluentT &animProgress(float p)
        {
            if (!canMutate())
                return *this;
            _animProgress = p;
            return *this;
        }
        ScrollDotsFluentT &animate(bool a)
        {
            if (!canMutate())
                return *this;
            _animate = a;
            return *this;
        }
        ScrollDotsFluentT &animDirection(int8_t d)
        {
            if (!canMutate())
                return *this;
            _animDirection = d;
            return *this;
        }
        ScrollDotsFluentT &activeColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _activeColor = c;
            return *this;
        }
        ScrollDotsFluentT &inactiveColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _inactiveColor = c;
            return *this;
        }
        ScrollDotsFluentT &dotRadius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _dotRadius = r;
            return *this;
        }
        ScrollDotsFluentT &spacing(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _spacing = s;
            return *this;
        }
        ScrollDotsFluentT &activeWidth(uint8_t w)
        {
            if (!canMutate())
                return *this;
            _activeWidth = w;
            return *this;
        }
        void draw();
    };

    template <bool IsUpdate>
    struct GraphGridFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GraphGridFluentT);
        int16_t _x, _y, _w, _h;
        uint8_t _radius;
        GraphDirection _dir;
        uint32_t _bgColor;
        float _speed;

        GraphGridFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _radius(0),
              _dir(LeftToRight),
              _bgColor(0),
              _speed(1.0f)
        {
        }

        ~GraphGridFluentT() { draw(); }

        GraphGridFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        GraphGridFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        GraphGridFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }

        GraphGridFluentT &direction(GraphDirection dir)
        {
            if (!canMutate())
                return *this;
            _dir = dir;
            return *this;
        }

        GraphGridFluentT &bgColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = c;
            return *this;
        }

        GraphGridFluentT &bgColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = detail::color565To888(c);
            return *this;
        }

        GraphGridFluentT &bgColor565(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = detail::color565To888(c);
            return *this;
        }

        GraphGridFluentT &speed(float v)
        {
            if (!canMutate())
                return *this;
            _speed = v;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct GraphLineFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GraphLineFluentT);
        uint8_t _lineIndex;
        int16_t _value;
        uint32_t _color;
        int16_t _valueMin;
        int16_t _valueMax;

        GraphLineFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _lineIndex(0),
              _value(0),
              _color(0),
              _valueMin(0),
              _valueMax(1)
        {
        }

        ~GraphLineFluentT() { draw(); }

        GraphLineFluentT &line(uint8_t idx)
        {
            if (!canMutate())
                return *this;
            _lineIndex = idx;
            return *this;
        }

        GraphLineFluentT &value(int16_t v)
        {
            if (!canMutate())
                return *this;
            _value = v;
            return *this;
        }

        GraphLineFluentT &color(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }

        GraphLineFluentT &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = detail::color565To888(c);
            return *this;
        }

        GraphLineFluentT &color565(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = detail::color565To888(c);
            return *this;
        }

        GraphLineFluentT &range(int16_t vmin, int16_t vmax)
        {
            if (!canMutate())
                return *this;
            _valueMin = vmin;
            _valueMax = vmax;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct GraphSamplesFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GraphSamplesFluentT);
        uint8_t _lineIndex;
        const int16_t *_samples;
        uint16_t _sampleCount;
        uint32_t _color;
        int16_t _valueMin;
        int16_t _valueMax;

        GraphSamplesFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _lineIndex(0),
              _samples(nullptr),
              _sampleCount(0),
              _color(0),
              _valueMin(0),
              _valueMax(1)
        {
        }

        ~GraphSamplesFluentT() { draw(); }

        GraphSamplesFluentT &line(uint8_t idx)
        {
            if (!canMutate())
                return *this;
            _lineIndex = idx;
            return *this;
        }

        GraphSamplesFluentT &samples(const int16_t *samples, uint16_t count)
        {
            if (!canMutate())
                return *this;
            _samples = samples;
            _sampleCount = count;
            return *this;
        }

        GraphSamplesFluentT &color(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }

        GraphSamplesFluentT &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = detail::color565To888(c);
            return *this;
        }

        GraphSamplesFluentT &color565(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = detail::color565To888(c);
            return *this;
        }

        GraphSamplesFluentT &range(int16_t vmin, int16_t vmax)
        {
            if (!canMutate())
                return *this;
            _valueMin = vmin;
            _valueMax = vmax;
            return *this;
        }

        void draw();
    };

    struct ToastFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ToastFluent);
        String _text;
        uint32_t _durationMs;
        bool _fromTop;
        IconId _iconId;
        uint16_t _iconSizePx;

        ToastFluent(GUI *g)
            : detail::FluentLifetime(g),
              _text(),
              _durationMs(2500),
              _fromTop(false),
              _iconId(warning),
              _iconSizePx(20) {}

        ~ToastFluent() { show(); }

        ToastFluent &text(const String &t)
        {
            if (!canMutate())
                return *this;
            _text = t;
            return *this;
        }

        ToastFluent &duration(uint32_t ms)
        {
            if (!canMutate())
                return *this;
            _durationMs = ms;
            return *this;
        }

        ToastFluent &fromTop(bool v = true)
        {
            if (!canMutate())
                return *this;
            _fromTop = v;
            return *this;
        }

        ToastFluent &icon(IconId id, uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _iconId = id;
            _iconSizePx = sizePx;
            return *this;
        }

        void show();
    };

    struct NotificationFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(NotificationFluent);
        String _title;
        String _message;
        String _buttonText;
        uint16_t _delaySeconds;
        NotificationType _type;
        std::optional<IconId> _iconId;

        NotificationFluent(GUI *g)
            : detail::FluentLifetime(g),
              _title(),
              _message(),
              _buttonText("OK"),
              _delaySeconds(0),
              _type(NotificationType::Normal),
              _iconId(std::nullopt) {}

        ~NotificationFluent() { show(); }

        NotificationFluent &title(const String &t)
        {
            if (!canMutate())
                return *this;
            _title = t;
            return *this;
        }

        NotificationFluent &message(const String &m)
        {
            if (!canMutate())
                return *this;
            _message = m;
            return *this;
        }

        NotificationFluent &button(const String &label)
        {
            if (!canMutate())
                return *this;
            _buttonText = label;
            return *this;
        }

        NotificationFluent &delay(uint16_t seconds)
        {
            if (!canMutate())
                return *this;
            _delaySeconds = seconds;
            return *this;
        }

        NotificationFluent &type(NotificationType v)
        {
            if (!canMutate())
                return *this;
            _type = v;
            return *this;
        }

        NotificationFluent &icon(IconId id)
        {
            if (!canMutate())
                return *this;
            _iconId = id;
            return *this;
        }

        void show();
    };

    struct ShowErrorFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ShowErrorFluent);
        String _message;
        String _code;
        String _buttonText;
        ErrorType _type;

        explicit ShowErrorFluent(GUI *g)
            : detail::FluentLifetime(g),
              _message(),
              _code(),
              _buttonText("OK"),
              _type(ErrorTypeWarning) {}

        ~ShowErrorFluent() { show(); }

        ShowErrorFluent &message(const String &value)
        {
            if (!canMutate())
                return *this;
            _message = value;
            return *this;
        }

        ShowErrorFluent &code(const String &value)
        {
            if (!canMutate())
                return *this;
            _code = value;
            return *this;
        }

        ShowErrorFluent &button(const String &value)
        {
            if (!canMutate())
                return *this;
            _buttonText = value;
            return *this;
        }

        ShowErrorFluent &type(ErrorType value)
        {
            if (!canMutate())
                return *this;
            _type = value;
            return *this;
        }

        void show();
    };

    struct PopupMenuFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(PopupMenuFluent);
        PopupMenuItemFn _itemFn;
        void *_itemUser;
        uint8_t _count;
        uint8_t _selectedIndex;
        uint8_t _maxVisible;
        int16_t _x;
        int16_t _y;
        int16_t _w;

        PopupMenuFluent(GUI *g)
            : detail::FluentLifetime(g),
              _itemFn(nullptr),
              _itemUser(nullptr),
              _count(0),
              _selectedIndex(0),
              _maxVisible(6),
              _x(0),
              _y(0),
              _w(0)
        {
        }

        ~PopupMenuFluent() { show(); }

        static const char *arrayItem(void *user, uint8_t idx)
        {
            const char *const *items = static_cast<const char *const *>(user);
            if (!items)
                return "";
            const char *s = items[idx];
            return s ? s : "";
        }

        PopupMenuFluent &items(const char *const *items, uint8_t count)
        {
            if (!canMutate())
                return *this;
            _itemFn = &arrayItem;
            _itemUser = const_cast<void *>(reinterpret_cast<const void *>(items));
            _count = count;
            return *this;
        }

        PopupMenuFluent &items(PopupMenuItemFn fn, void *user, uint8_t count)
        {
            if (!canMutate())
                return *this;
            _itemFn = fn;
            _itemUser = user;
            _count = count;
            return *this;
        }

        PopupMenuFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        PopupMenuFluent &width(int16_t w)
        {
            if (!canMutate())
                return *this;
            _w = w;
            return *this;
        }

        PopupMenuFluent &selected(uint8_t idx)
        {
            if (!canMutate())
                return *this;
            _selectedIndex = idx;
            return *this;
        }

        PopupMenuFluent &maxVisible(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _maxVisible = v;
            return *this;
        }

        void show();
    };

    struct DrawIconFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawIconFluent);
        int16_t _x, _y;
        uint16_t _sizePx;
        uint16_t _iconId;
        uint16_t _fg565;
        uint16_t _bg565;

        DrawIconFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(-1), _y(-1),
              _sizePx(0),
              _iconId(0),
              _fg565(0xFFFF),
              _bg565(0x0000)
        {
        }

        ~DrawIconFluent() { draw(); }

        DrawIconFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        DrawIconFluent &size(uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _sizePx = sizePx;
            return *this;
        }

        DrawIconFluent &icon(uint16_t iconId)
        {
            if (!canMutate())
                return *this;
            _iconId = iconId;
            return *this;
        }

        DrawIconFluent &color(uint16_t fg565)
        {
            if (!canMutate())
                return *this;
            _fg565 = fg565;
            return *this;
        }

        DrawIconFluent &bgColor(uint16_t bg565)
        {
            if (!canMutate())
                return *this;
            _bg565 = bg565;
            return *this;
        }

        void draw();
    };

    struct DrawScreenshotFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawScreenshotFluent);
        int16_t _x, _y, _w, _h;
        uint16_t _padding;
        uint8_t _cols, _rows;

        DrawScreenshotFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(-1), _h(-1),
              _padding(0),
              _cols(0), _rows(0)
        {
        }

        ~DrawScreenshotFluent() { draw(); }

        DrawScreenshotFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        DrawScreenshotFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        DrawScreenshotFluent &grid(uint8_t cols, uint8_t rows)
        {
            if (!canMutate())
                return *this;
            _cols = cols;
            _rows = rows;
            return *this;
        }

        DrawScreenshotFluent &padding(uint16_t px)
        {
            if (!canMutate())
                return *this;
            _padding = px;
            return *this;
        }

        void draw();
    };

}

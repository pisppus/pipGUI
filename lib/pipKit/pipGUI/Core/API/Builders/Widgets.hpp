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
        uint16_t _fillColor;
        uint8_t _radius;
        IconId _iconId;
        bool _enabled;
        bool _loading;
        bool _down;
        bool _showProgress;
        bool _modeSet;
        bool _downSet;
        uint8_t _progressValue;
        ButtonFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _label(),
              _x(0), _y(0), _w(0), _h(0),
              _baseColor(0),
              _fillColor(0xFFFF),
              _radius(6),
              _iconId(static_cast<IconId>(0xFFFF)),
              _enabled(true),
              _loading(false),
              _down(false),
              _showProgress(false),
              _modeSet(false),
              _downSet(false),
              _progressValue(0)
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

        ButtonFluentT &value(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _progressValue = v;
            _showProgress = true;
            return *this;
        }

        ButtonFluentT &fillColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _fillColor = c;
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
    struct SliderFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(SliderFluentT);
        int16_t _x, _y, _w, _h;
        int16_t *_value;
        bool _enabled;
        bool _enabledSet;
        uint16_t _activeColor;
        std::optional<uint16_t> _inactiveColor;
        std::optional<uint16_t> _thumbColor;

        SliderFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _value(nullptr),
              _enabled(true),
              _enabledSet(false),
              _activeColor(0xFFFF),
              _inactiveColor(std::nullopt),
              _thumbColor(std::nullopt)
        {
        }

        ~SliderFluentT() { draw(); }

        SliderFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        SliderFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        SliderFluentT &bind(int16_t &v)
        {
            if (!canMutate())
                return *this;
            _value = &v;
            return *this;
        }

        SliderFluentT &enabled(bool v = true)
        {
            if (!canMutate())
                return *this;
            _enabled = v;
            _enabledSet = true;
            return *this;
        }

        SliderFluentT &activeColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _activeColor = c;
            return *this;
        }

        SliderFluentT &inactiveColor(int32_t c)
        {
            if (!canMutate())
                return *this;
            detail::assignOptionalColor(_inactiveColor, c);
            return *this;
        }

        SliderFluentT &thumbColor(int32_t c)
        {
            if (!canMutate())
                return *this;
            detail::assignOptionalColor(_thumbColor, c);
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

        ProgressFluentT &label(const String &text, TextAlign align = Left)
        {
            if (!canMutate())
                return *this;
            _label = text;
            _labelAlign = align;
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

        ProgressFluentT &labelFont(uint16_t px)
        {
            if (!canMutate())
                return *this;
            _labelFontPx = px;
            return *this;
        }

        ProgressFluentT &percent(TextAlign align = Right)
        {
            if (!canMutate())
                return *this;
            _showPercent = true;
            _percentAlign = align;
            return *this;
        }

        ProgressFluentT &percentColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _percentColor = c;
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

    struct DrawDrumRollFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawDrumRollFluent);
        static constexpr uint8_t INLINE_OPTIONS_MAX = 8;
        int16_t _x, _y, _w, _h;
        const String *_options;
        String _inlineOptions[INLINE_OPTIONS_MAX];
        uint8_t _count;
        uint8_t _selectedIndex;
        uint32_t _fgColor;
        uint32_t _bgColor;
        uint16_t _fontPx;
        bool _vertical;

        DrawDrumRollFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _options(nullptr),
              _inlineOptions(),
              _count(0),
              _selectedIndex(0),
              _fgColor(0xFFFFFF),
              _bgColor(0),
              _fontPx(0),
              _vertical(false)
        {
        }

        ~DrawDrumRollFluent() { draw(); }

        DrawDrumRollFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        DrawDrumRollFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }

        template <size_t N>
        DrawDrumRollFluent &options(uint16_t fontPx, const String (&items)[N])
        {
            if (!canMutate())
                return *this;
            _fontPx = fontPx;
            _options = items;
            _count = static_cast<uint8_t>(N);
            return *this;
        }

        template <size_t N>
        DrawDrumRollFluent &options(uint16_t fontPx, const char *const (&items)[N])
        {
            if (!canMutate())
                return *this;
            static_assert(N <= INLINE_OPTIONS_MAX, "Too many drum roll options");
            _fontPx = fontPx;
            for (size_t i = 0; i < N; ++i)
                _inlineOptions[i] = String(items[i]);
            _options = _inlineOptions;
            _count = static_cast<uint8_t>(N);
            return *this;
        }

        template <typename... Args>
        DrawDrumRollFluent &options(uint16_t fontPx, Args &&...items)
        {
            if (!canMutate())
                return *this;
            static_assert(sizeof...(Args) > 0, "DrumRoll options require at least one item");
            static_assert(sizeof...(Args) <= INLINE_OPTIONS_MAX, "Too many drum roll options");
            _fontPx = fontPx;
            size_t index = 0;
            ((void)(_inlineOptions[index++] = String(items)), ...);
            _options = _inlineOptions;
            _count = static_cast<uint8_t>(sizeof...(Args));
            return *this;
        }

        DrawDrumRollFluent &selected(uint8_t index)
        {
            if (!canMutate())
                return *this;
            _selectedIndex = index;
            return *this;
        }

        DrawDrumRollFluent &fgColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _fgColor = color;
            return *this;
        }

        DrawDrumRollFluent &fgColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _fgColor = detail::color565To888(color565);
            return *this;
        }

        DrawDrumRollFluent &bgColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _bgColor = color;
            return *this;
        }

        DrawDrumRollFluent &bgColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _bgColor = detail::color565To888(color565);
            return *this;
        }

        DrawDrumRollFluent &vertical(bool enabled = true)
        {
            if (!canMutate())
                return *this;
            _vertical = enabled;
            return *this;
        }

        void draw();
    };

    template <bool IsUpdate>
    struct ScrollDotsFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ScrollDotsFluentT);
        int16_t _x, _y;
        uint8_t _count, _activeIndex;
        uint16_t _activeColor, _inactiveColor;
        uint8_t _radius, _spacing;
        ScrollDotsFluentT(GUI *g)
            : detail::FluentLifetime(g), _x(0), _y(0), _count(0), _activeIndex(0),
              _activeColor(0xFFFF), _inactiveColor(0x7BEF), _radius(3), _spacing(14) {}

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
        ScrollDotsFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }
        ScrollDotsFluentT &spacing(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _spacing = s;
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
        bool _autoScale;
        uint16_t _scopeRateHz;
        uint16_t _scopeTimebaseMs;
        uint16_t _scopeVisibleSamples;

        GraphGridFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _radius(0),
              _dir(LeftToRight),
              _bgColor(0),
              _speed(1.0f),
              _autoScale(false),
              _scopeRateHz(0),
              _scopeTimebaseMs(0),
              _scopeVisibleSamples(0)
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

        GraphGridFluentT &scale(bool enabled = true)
        {
            if (!canMutate())
                return *this;
            _autoScale = enabled;
            return *this;
        }

        GraphGridFluentT &scope(uint16_t rateHz, uint16_t timebaseMs)
        {
            if (!canMutate())
                return *this;
            _scopeRateHz = rateHz;
            _scopeTimebaseMs = timebaseMs;
            return *this;
        }

        GraphGridFluentT &visible(uint16_t samples)
        {
            if (!canMutate())
                return *this;
            _scopeVisibleSamples = samples;
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
        uint8_t _thickness;

        GraphLineFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _lineIndex(0),
              _value(0),
              _color(0),
              _valueMin(0),
              _valueMax(1),
              _thickness(1)
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

        GraphLineFluentT &thickness(uint8_t t)
        {
            if (!canMutate())
                return *this;
            _thickness = t;
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
        uint8_t _thickness;

        GraphSamplesFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _lineIndex(0),
              _samples(nullptr),
              _sampleCount(0),
              _color(0),
              _valueMin(0),
              _valueMax(1),
              _thickness(1)
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

        GraphSamplesFluentT &thickness(uint8_t t)
        {
            if (!canMutate())
                return *this;
            _thickness = t;
            return *this;
        }

        void draw();
    };

    struct ToastFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ToastFluent);
        String _text;
        ToastPos _pos;
        IconId _iconId;

        ToastFluent(GUI *g)
            : detail::FluentLifetime(g),
              _text(),
              _pos(ToastPos::Down),
              _iconId(static_cast<IconId>(0xFFFF)) {}

        ~ToastFluent() { commit(); }

        ToastFluent &text(const String &t)
        {
            if (!canMutate())
                return *this;
            _text = t;
            return *this;
        }

        ToastFluent &pos(ToastPos p)
        {
            if (!canMutate())
                return *this;
            _pos = p;
            return *this;
        }

        ToastFluent &icon(IconId id)
        {
            if (!canMutate())
                return *this;
            _iconId = id;
            return *this;
        }

    private:
        void commit();
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

        NotificationFluent &text(const String &title, const String &message = "")
        {
            if (!canMutate())
                return *this;
            _title = title;
            _message = message;
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
        const char *const *_items;
        uint8_t _count;
        uint8_t _selectedIndex;
        int16_t _w;
        int16_t _anchorX;
        int16_t _anchorY;
        int16_t _anchorW;
        int16_t _anchorH;

        PopupMenuFluent(GUI *g)
            : detail::FluentLifetime(g),
              _items(nullptr),
              _count(0),
              _selectedIndex(0xFF),
              _w(0),
              _anchorX(0),
              _anchorY(0),
              _anchorW(0),
              _anchorH(0)
        {
        }

        ~PopupMenuFluent() { show(); }

        template <size_t N>
        PopupMenuFluent &items(const char *const (&items)[N])
        {
            if (!canMutate())
                return *this;
            _items = items;
            _count = static_cast<uint8_t>(N);
            return *this;
        }

        PopupMenuFluent &items(const char *const *items, uint8_t count)
        {
            if (!canMutate())
                return *this;
            _items = items;
            _count = count;
            return *this;
        }

        template <typename AnchorFluent>
        PopupMenuFluent &anchor(const AnchorFluent &component)
        {
            if (!canMutate())
                return *this;
            _anchorX = component._x;
            _anchorY = component._y;
            _anchorW = component._w;
            _anchorH = component._h;
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

    template <bool IsUpdate>
    struct AnimIconFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(AnimIconFluentT);
        int16_t _x, _y;
        uint16_t _sizePx;
        uint16_t _iconId;
        uint16_t _fg565;
        uint16_t _bg565;

        AnimIconFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(-1), _y(-1),
              _sizePx(0),
              _iconId(0),
              _fg565(0xFFFF),
              _bg565(0x0000)
        {
        }

        ~AnimIconFluentT() { draw(); }

        AnimIconFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        AnimIconFluentT &size(uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _sizePx = sizePx;
            return *this;
        }

        AnimIconFluentT &icon(uint16_t iconId)
        {
            if (!canMutate())
                return *this;
            _iconId = iconId;
            return *this;
        }

        AnimIconFluentT &color(uint16_t fg565)
        {
            if (!canMutate())
                return *this;
            _fg565 = fg565;
            return *this;
        }

        AnimIconFluentT &bgColor(uint16_t bg565)
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

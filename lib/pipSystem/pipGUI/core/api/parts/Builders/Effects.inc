 #if !defined(PIPGUI_FLUENT_BUILDERS_INC)
 #error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
 #endif
 
 template <bool IsUpdate>
struct BlurStripFluentT
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint8_t _radius;
    BlurDirection _dir;
    bool _gradient;
    uint8_t _materialStrength;
    uint8_t _noiseAmount;
    int32_t _materialColor;
    bool _consumed;
    BlurStripFluentT(GUI *g)
        : _gui(g), _x(0), _y(0), _w(0), _h(0), _radius(0), _dir(TopDown), _gradient(true), _materialStrength(32), _noiseAmount(8), _materialColor(-1), _consumed(false) {}
    BlurStripFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }
    BlurStripFluentT &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _w = w;
        _h = h;
        return *this;
    }
    BlurStripFluentT &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _radius = r;
        return *this;
    }
    BlurStripFluentT &direction(BlurDirection d)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _dir = d;
        return *this;
    }
    BlurStripFluentT &gradient(bool g)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _gradient = g;
        return *this;
    }
    BlurStripFluentT &materialStrength(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _materialStrength = s;
        return *this;
    }
    BlurStripFluentT &noiseAmount(uint8_t n)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _noiseAmount = n;
        return *this;
    }
    BlurStripFluentT &materialColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(BlurStripFluentT<IsUpdate>);
        _materialColor = c;
        return *this;
    }
    void draw();
};
using DrawBlurStripFluent = BlurStripFluentT<false>;
using UpdateBlurStripFluent = BlurStripFluentT<true>;

template <bool IsUpdate>
struct GlowCircleFluentT
{
    GUI *_gui;
    int16_t _x, _y;
    int16_t _r;
    uint32_t _fillColor;
    int32_t _bgColor, _glowColor;
    uint8_t _glowSize, _glowStrength;
    GlowAnim _anim;
    uint16_t _pulsePeriodMs;
    bool _consumed;
    GlowCircleFluentT(GUI *g)
        : _gui(g), _x(0), _y(0), _r(0), _fillColor(0), _bgColor(-1), _glowColor(-1), _glowSize(12), _glowStrength(220), _anim(None), _pulsePeriodMs(1000), _consumed(false) {}
    GlowCircleFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }
    GlowCircleFluentT &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _r = r;
        return *this;
    }
    GlowCircleFluentT &fillColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _fillColor = c;
        return *this;
    }
    GlowCircleFluentT &bgColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _bgColor = c;
        return *this;
    }
    GlowCircleFluentT &glowColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _glowColor = c;
        return *this;
    }
    GlowCircleFluentT &glowSize(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _glowSize = s;
        return *this;
    }
    GlowCircleFluentT &glowStrength(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _glowStrength = s;
        return *this;
    }
    GlowCircleFluentT &anim(GlowAnim a)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _anim = a;
        return *this;
    }
    GlowCircleFluentT &pulsePeriodMs(uint16_t ms)
    {
        PIPGUI_FLUENT_GUARD(GlowCircleFluentT<IsUpdate>);
        _pulsePeriodMs = ms;
        return *this;
    }
    void draw();
};
using DrawGlowCircleFluent = GlowCircleFluentT<false>;
using UpdateGlowCircleFluent = GlowCircleFluentT<true>;

template <bool IsUpdate>
struct GlowRoundRectFluentT
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint8_t _radius;
    uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
    uint32_t _fillColor;
    int32_t _bgColor, _glowColor;
    uint8_t _glowSize, _glowStrength;
    GlowAnim _anim;
    uint16_t _pulsePeriodMs;
    bool _perCorner;
    bool _consumed;
    GlowRoundRectFluentT(GUI *g)
        : _gui(g), _x(0), _y(0), _w(0), _h(0), _radius(0),
          _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
          _fillColor(0), _bgColor(-1), _glowColor(-1),
          _glowSize(12), _glowStrength(220), _anim(None), _pulsePeriodMs(1000),
          _perCorner(false), _consumed(false) {}
    GlowRoundRectFluentT &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _x = x;
        _y = y;
        return *this;
    }
    GlowRoundRectFluentT &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _w = w;
        _h = h;
        return *this;
    }
    GlowRoundRectFluentT &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _radius = r;
        _perCorner = false;
        return *this;
    }
    GlowRoundRectFluentT &radius(uint8_t tl, uint8_t tr, uint8_t br, uint8_t bl)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _radiusTL = tl;
        _radiusTR = tr;
        _radiusBR = br;
        _radiusBL = bl;
        _perCorner = true;
        return *this;
    }
    GlowRoundRectFluentT &fillColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _fillColor = c;
        return *this;
    }
    GlowRoundRectFluentT &bgColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _bgColor = c;
        return *this;
    }
    GlowRoundRectFluentT &glowColor(int32_t c)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _glowColor = c;
        return *this;
    }
    GlowRoundRectFluentT &glowSize(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _glowSize = s;
        return *this;
    }
    GlowRoundRectFluentT &glowStrength(uint8_t s)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _glowStrength = s;
        return *this;
    }
    GlowRoundRectFluentT &anim(GlowAnim a)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _anim = a;
        return *this;
    }
    GlowRoundRectFluentT &pulsePeriodMs(uint16_t ms)
    {
        PIPGUI_FLUENT_GUARD(GlowRoundRectFluentT<IsUpdate>);
        _pulsePeriodMs = ms;
        return *this;
    }
    void draw();
};
using DrawGlowRoundRectFluent = GlowRoundRectFluentT<false>;
using UpdateGlowRoundRectFluent = GlowRoundRectFluentT<true>;

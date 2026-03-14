#pragma once

namespace pipgui
{

    template <bool IsUpdate>
    struct BlurRegionFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(BlurRegionFluentT);
        int16_t _x, _y, _w, _h;
        uint8_t _radius;
        BlurDirection _dir;
        bool _gradient;
        uint8_t _materialStrength;
        uint8_t _noiseAmount;
        int32_t _materialColor;
        BlurRegionFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0),
              _y(0),
              _w(0),
              _h(0),
              _radius(0),
              _dir(TopDown),
              _gradient(true),
              _materialStrength(32),
              _noiseAmount(8),
              _materialColor(-1)
        {
        }
        ~BlurRegionFluentT() { draw(); }
        BlurRegionFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        BlurRegionFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        BlurRegionFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }
        BlurRegionFluentT &direction(BlurDirection d)
        {
            if (!canMutate())
                return *this;
            _dir = d;
            return *this;
        }
        BlurRegionFluentT &gradient(bool g)
        {
            if (!canMutate())
                return *this;
            _gradient = g;
            return *this;
        }
        BlurRegionFluentT &materialStrength(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _materialStrength = s;
            return *this;
        }
        BlurRegionFluentT &noiseAmount(uint8_t n)
        {
            if (!canMutate())
                return *this;
            _noiseAmount = n;
            return *this;
        }
        BlurRegionFluentT &materialColor(int32_t c)
        {
            if (!canMutate())
                return *this;
            _materialColor = c;
            return *this;
        }
        void draw();
    };

    template <bool IsUpdate>
    struct GlowCircleFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GlowCircleFluentT);
        int16_t _x, _y;
        int16_t _r;
        uint16_t _fillColor;
        int16_t _bgColor, _glowColor;
        uint8_t _glowSize, _glowStrength;
        GlowAnim _anim;
        uint16_t _pulsePeriodMs;
        GlowCircleFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(0),
              _y(0),
              _r(0),
              _fillColor(0),
              _bgColor(-1),
              _glowColor(-1),
              _glowSize(12),
              _glowStrength(220),
              _anim(None),
              _pulsePeriodMs(1000)
        {
        }
        ~GlowCircleFluentT() { draw(); }
        GlowCircleFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GlowCircleFluentT &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _r = r;
            return *this;
        }
        GlowCircleFluentT &fillColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _fillColor = c;
            return *this;
        }
        GlowCircleFluentT &bgColor(int16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = c;
            return *this;
        }
        GlowCircleFluentT &bgColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = (int16_t)c;
            return *this;
        }
        GlowCircleFluentT &glowColor(int16_t c)
        {
            if (!canMutate())
                return *this;
            _glowColor = c;
            return *this;
        }
        GlowCircleFluentT &glowColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _glowColor = (int16_t)c;
            return *this;
        }
        GlowCircleFluentT &glowSize(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _glowSize = s;
            return *this;
        }
        GlowCircleFluentT &glowStrength(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _glowStrength = s;
            return *this;
        }
        GlowCircleFluentT &anim(GlowAnim a)
        {
            if (!canMutate())
                return *this;
            _anim = a;
            return *this;
        }
        GlowCircleFluentT &pulsePeriodMs(uint16_t ms)
        {
            if (!canMutate())
                return *this;
            _pulsePeriodMs = ms;
            return *this;
        }
        void draw();
    };

    template <bool IsUpdate>
    struct GlowRectFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GlowRectFluentT);
        int16_t _x, _y, _w, _h;
        uint8_t _radius;
        uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
        uint16_t _fillColor;
        int16_t _bgColor, _glowColor;
        uint8_t _glowSize, _glowStrength;
        GlowAnim _anim;
        uint16_t _pulsePeriodMs;
        bool _perCorner;
        GlowRectFluentT(GUI *g)
            : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _radius(0),
              _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
              _fillColor(0), _bgColor(-1), _glowColor(-1),
              _glowSize(12), _glowStrength(220), _anim(None), _pulsePeriodMs(1000),
              _perCorner(false) {}
        ~GlowRectFluentT()
        {
            if (!_consumed)
                draw();
        }
        GlowRectFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GlowRectFluentT &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GlowRectFluentT &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            _perCorner = false;
            return *this;
        }
        GlowRectFluentT &radius(uint8_t tl, uint8_t tr, uint8_t br, uint8_t bl)
        {
            if (!canMutate())
                return *this;
            _radiusTL = tl;
            _radiusTR = tr;
            _radiusBR = br;
            _radiusBL = bl;
            _perCorner = true;
            return *this;
        }
        GlowRectFluentT &fillColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _fillColor = c;
            return *this;
        }
        GlowRectFluentT &bgColor(int16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = c;
            return *this;
        }
        GlowRectFluentT &bgColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _bgColor = (int16_t)c;
            return *this;
        }
        GlowRectFluentT &glowColor(int16_t c)
        {
            if (!canMutate())
                return *this;
            _glowColor = c;
            return *this;
        }
        GlowRectFluentT &glowColor(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _glowColor = (int16_t)c;
            return *this;
        }
        GlowRectFluentT &glowSize(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _glowSize = s;
            return *this;
        }
        GlowRectFluentT &glowStrength(uint8_t s)
        {
            if (!canMutate())
                return *this;
            _glowStrength = s;
            return *this;
        }
        GlowRectFluentT &anim(GlowAnim a)
        {
            if (!canMutate())
                return *this;
            _anim = a;
            return *this;
        }
        GlowRectFluentT &pulsePeriodMs(uint16_t ms)
        {
            if (!canMutate())
                return *this;
            _pulsePeriodMs = ms;
            return *this;
        }
        void draw();
    };

}

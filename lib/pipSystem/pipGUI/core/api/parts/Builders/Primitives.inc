 #if !defined(PIPGUI_FLUENT_BUILDERS_INC)
 #error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
 #endif
 
 struct FillRectFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _color;
    uint8_t _radius;
    uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
    bool _perCorner;
    bool _consumed;
    FillRectFluent(GUI *g)
        : _gui(g),
          _x(0), _y(0), _w(0), _h(0),
          _color(0),
          _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
          _perCorner(false), _consumed(false) {}
    FillRectFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _color = c;
        return *this;
    }
    FillRectFluent &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _radius = r;
        _perCorner = false;
        return *this;
    }
    FillRectFluent &radius(uint8_t tl, uint8_t tr, uint8_t br, uint8_t bl)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _radiusTL = tl;
        _radiusTR = tr;
        _radiusBR = br;
        _radiusBL = bl;
        _perCorner = true;
        return *this;
    }
    void draw();
};

struct FillRectAlphaFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _color;
    uint8_t _alpha;
    bool _consumed;
    FillRectAlphaFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _color(0), _alpha(255), _consumed(false) {}
    FillRectAlphaFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectAlphaFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectAlphaFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectAlphaFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectAlphaFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectAlphaFluent);
        _color = c;
        return *this;
    }
    FillRectAlphaFluent &alpha(uint8_t a)
    {
        PIPGUI_FLUENT_GUARD(FillRectAlphaFluent);
        _alpha = a;
        return *this;
    }
    void draw();
};

struct FillRectGradientVerticalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _topColor, _bottomColor;
    bool _consumed;
    FillRectGradientVerticalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _topColor(0), _bottomColor(0), _consumed(false) {}
    FillRectGradientVerticalFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientVerticalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectGradientVerticalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientVerticalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectGradientVerticalFluent &topColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientVerticalFluent);
        _topColor = c;
        return *this;
    }
    FillRectGradientVerticalFluent &bottomColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientVerticalFluent);
        _bottomColor = c;
        return *this;
    }
    void draw();
};

struct FillRectGradientHorizontalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _leftColor, _rightColor;
    bool _consumed;
    FillRectGradientHorizontalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _leftColor(0), _rightColor(0), _consumed(false) {}
    FillRectGradientHorizontalFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientHorizontalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectGradientHorizontalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientHorizontalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectGradientHorizontalFluent &leftColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientHorizontalFluent);
        _leftColor = c;
        return *this;
    }
    FillRectGradientHorizontalFluent &rightColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientHorizontalFluent);
        _rightColor = c;
        return *this;
    }
    void draw();
};

struct FillRectGradient4Fluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _c00, _c10, _c01, _c11;
    bool _consumed;
    FillRectGradient4Fluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _c00(0), _c10(0), _c01(0), _c11(0), _consumed(false) {}
    FillRectGradient4Fluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectGradient4Fluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectGradient4Fluent &topLeftColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _c00 = color;
        return *this;
    }
    FillRectGradient4Fluent &topRightColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _c10 = color;
        return *this;
    }
    FillRectGradient4Fluent &bottomLeftColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _c01 = color;
        return *this;
    }
    FillRectGradient4Fluent &bottomRightColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradient4Fluent);
        _c11 = color;
        return *this;
    }
    void draw();
};

struct FillRectGradientDiagonalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _tlColor, _brColor;
    bool _consumed;
    FillRectGradientDiagonalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _tlColor(0), _brColor(0), _consumed(false) {}
    FillRectGradientDiagonalFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientDiagonalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectGradientDiagonalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientDiagonalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectGradientDiagonalFluent &topLeftColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientDiagonalFluent);
        _tlColor = c;
        return *this;
    }
    FillRectGradientDiagonalFluent &bottomRightColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientDiagonalFluent);
        _brColor = c;
        return *this;
    }
    void draw();
};

struct FillRectGradientRadialFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    int16_t _cx, _cy;
    int16_t _radius;
    uint32_t _innerColor, _outerColor;
    bool _consumed;
    FillRectGradientRadialFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _cx(0), _cy(0), _radius(0), _innerColor(0), _outerColor(0), _consumed(false) {}
    FillRectGradientRadialFluent &at(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _x = x;
        _y = y;
        return *this;
    }
    FillRectGradientRadialFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _w = w;
        _h = h;
        return *this;
    }
    FillRectGradientRadialFluent &center(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    FillRectGradientRadialFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _radius = r;
        return *this;
    }
    FillRectGradientRadialFluent &innerColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _innerColor = c;
        return *this;
    }
    FillRectGradientRadialFluent &outerColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectGradientRadialFluent);
        _outerColor = c;
        return *this;
    }
    void draw();
};

struct DrawLineFluent
{
    GUI *_gui;
    int16_t _x0, _y0, _x1, _y1;
    uint32_t _color;
    bool _consumed;
    DrawLineFluent(GUI *g) : _gui(g), _x0(0), _y0(0), _x1(0), _y1(0), _color(0), _consumed(false) {}
    DrawLineFluent &from(int16_t x0, int16_t y0)
    {
        PIPGUI_FLUENT_GUARD(DrawLineFluent);
        _x0 = x0;
        _y0 = y0;
        return *this;
    }
    DrawLineFluent &to(int16_t x1, int16_t y1)
    {
        PIPGUI_FLUENT_GUARD(DrawLineFluent);
        _x1 = x1;
        _y1 = y1;
        return *this;
    }
    DrawLineFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawLineFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct DrawCircleFluent
{
    GUI *_gui;
    int16_t _cx, _cy;
    int16_t _r;
    uint32_t _color;
    bool _consumed;
    DrawCircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    DrawCircleFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(DrawCircleFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    DrawCircleFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(DrawCircleFluent);
        _r = r;
        return *this;
    }
    DrawCircleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawCircleFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct FillCircleFluent
{
    GUI *_gui;
    int16_t _cx, _cy;
    int16_t _r;
    uint32_t _color;
    bool _consumed;
    FillCircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    FillCircleFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(FillCircleFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    FillCircleFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(FillCircleFluent);
        _r = r;
        return *this;
    }
    FillCircleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillCircleFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct DrawArcFluent
{
    GUI *_gui;
    int16_t _cx, _cy;
    int16_t _r;
    float _startDeg, _endDeg;
    uint32_t _color;
    bool _consumed;
    DrawArcFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _startDeg(0), _endDeg(360), _color(0), _consumed(false) {}
    DrawArcFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(DrawArcFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    DrawArcFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(DrawArcFluent);
        _r = r;
        return *this;
    }
    DrawArcFluent &startDeg(float d)
    {
        PIPGUI_FLUENT_GUARD(DrawArcFluent);
        _startDeg = d;
        return *this;
    }
    DrawArcFluent &endDeg(float d)
    {
        PIPGUI_FLUENT_GUARD(DrawArcFluent);
        _endDeg = d;
        return *this;
    }
    DrawArcFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawArcFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct DrawEllipseFluent
{
    GUI *_gui;
    int16_t _cx, _cy, _rx, _ry;
    uint32_t _color;
    bool _consumed;
    DrawEllipseFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0), _consumed(false) {}
    DrawEllipseFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(DrawEllipseFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    DrawEllipseFluent &radiusX(int16_t rx)
    {
        PIPGUI_FLUENT_GUARD(DrawEllipseFluent);
        _rx = rx;
        return *this;
    }
    DrawEllipseFluent &radiusY(int16_t ry)
    {
        PIPGUI_FLUENT_GUARD(DrawEllipseFluent);
        _ry = ry;
        return *this;
    }
    DrawEllipseFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawEllipseFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct FillEllipseFluent
{
    GUI *_gui;
    int16_t _cx, _cy, _rx, _ry;
    uint32_t _color;
    bool _consumed;
    FillEllipseFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0), _consumed(false) {}
    FillEllipseFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(FillEllipseFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    FillEllipseFluent &radiusX(int16_t rx)
    {
        PIPGUI_FLUENT_GUARD(FillEllipseFluent);
        _rx = rx;
        return *this;
    }
    FillEllipseFluent &radiusY(int16_t ry)
    {
        PIPGUI_FLUENT_GUARD(FillEllipseFluent);
        _ry = ry;
        return *this;
    }
    FillEllipseFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillEllipseFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct DrawTriangleFluent
{
    GUI *_gui;
    int16_t _x0, _y0, _x1, _y1, _x2, _y2;
    uint32_t _color;
    uint8_t _radius;
    bool _consumed;
    DrawTriangleFluent(GUI *g) : _gui(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0), _consumed(false) {}
    DrawTriangleFluent &point0(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTriangleFluent);
        _x0 = x;
        _y0 = y;
        return *this;
    }
    DrawTriangleFluent &point1(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTriangleFluent);
        _x1 = x;
        _y1 = y;
        return *this;
    }
    DrawTriangleFluent &point2(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawTriangleFluent);
        _x2 = x;
        _y2 = y;
        return *this;
    }
    DrawTriangleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawTriangleFluent);
        _color = c;
        return *this;
    }
    DrawTriangleFluent &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(DrawTriangleFluent);
        _radius = r;
        return *this;
    }
    void draw();
};

struct FillTriangleFluent
{
    GUI *_gui;
    int16_t _x0, _y0, _x1, _y1, _x2, _y2;
    uint32_t _color;
    uint8_t _radius;
    bool _consumed;
    FillTriangleFluent(GUI *g) : _gui(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0), _consumed(false) {}
    FillTriangleFluent &point0(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillTriangleFluent);
        _x0 = x;
        _y0 = y;
        return *this;
    }
    FillTriangleFluent &point1(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillTriangleFluent);
        _x1 = x;
        _y1 = y;
        return *this;
    }
    FillTriangleFluent &point2(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(FillTriangleFluent);
        _x2 = x;
        _y2 = y;
        return *this;
    }
    FillTriangleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillTriangleFluent);
        _color = c;
        return *this;
    }
    FillTriangleFluent &radius(uint8_t r)
    {
        PIPGUI_FLUENT_GUARD(FillTriangleFluent);
        _radius = r;
        return *this;
    }
    void draw();
};

struct DrawSquircleFluent
{
    GUI *_gui;
    int16_t _cx, _cy;
    int16_t _r;
    uint32_t _color;
    bool _consumed;
    DrawSquircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    DrawSquircleFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(DrawSquircleFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    DrawSquircleFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(DrawSquircleFluent);
        _r = r;
        return *this;
    }
    DrawSquircleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawSquircleFluent);
        _color = c;
        return *this;
    }
    void draw();
};

struct FillSquircleFluent
{
    GUI *_gui;
    int16_t _cx, _cy;
    int16_t _r;
    uint32_t _color;
    bool _consumed;
    FillSquircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    FillSquircleFluent &at(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(FillSquircleFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    FillSquircleFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(FillSquircleFluent);
        _r = r;
        return *this;
    }
    FillSquircleFluent &color(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(FillSquircleFluent);
        _color = c;
        return *this;
    }
    void draw();
};

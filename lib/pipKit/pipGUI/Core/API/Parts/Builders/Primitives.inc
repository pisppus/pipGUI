 #if !defined(PIPGUI_FLUENT_BUILDERS_INC)
 #error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
 #endif
 
 struct FillRectFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint16_t _color;
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
    ~FillRectFluent() { draw(); }
    FillRectFluent &pos(int16_t x, int16_t y)
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
    FillRectFluent &color(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        _color = c;
        return *this;
    }
    FillRectFluent &radius(std::initializer_list<uint8_t> radii)
    {
        PIPGUI_FLUENT_GUARD(FillRectFluent);
        if (radii.size() == 1)
        {
            _radius = *radii.begin();
            _perCorner = false;
        }
        else if (radii.size() == 4)
        {
            auto it = radii.begin();
            _radiusTL = *it++;
            _radiusTR = *it++;
            _radiusBR = *it++;
            _radiusBL = *it++;
            _perCorner = true;
        }
        return *this;
    }
    void draw();
};

struct DrawRectFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint16_t _color;
    uint8_t _radius;
    uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
    bool _perCorner;
    bool _consumed;
    DrawRectFluent(GUI *g)
        : _gui(g),
          _x(0), _y(0), _w(0), _h(0),
          _color(0),
          _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
          _perCorner(false), _consumed(false) {}
    ~DrawRectFluent() { draw(); }
    DrawRectFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(DrawRectFluent);
        _x = x;
        _y = y;
        return *this;
    }
    DrawRectFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(DrawRectFluent);
        _w = w;
        _h = h;
        return *this;
    }
    DrawRectFluent &color(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(DrawRectFluent);
        _color = c;
        return *this;
    }
    DrawRectFluent &radius(std::initializer_list<uint8_t> radii)
    {
        PIPGUI_FLUENT_GUARD(DrawRectFluent);
        if (radii.size() == 1)
        {
            _radius = *radii.begin();
            _perCorner = false;
        }
        else if (radii.size() == 4)
        {
            auto it = radii.begin();
            _radiusTL = *it++;
            _radiusTR = *it++;
            _radiusBR = *it++;
            _radiusBL = *it++;
            _perCorner = true;
        }
        return *this;
    }
    void draw();
};

struct GradientVerticalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _topColor, _bottomColor;
    bool _consumed;
    GradientVerticalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _topColor(0), _bottomColor(0), _consumed(false) {}
    ~GradientVerticalFluent() { draw(); }
    GradientVerticalFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    GradientVerticalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    GradientVerticalFluent &topColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _topColor = c;
        return *this;
    }
    GradientVerticalFluent &topColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _topColor = detail::color565To888(c565);
        return *this;
    }
    GradientVerticalFluent &bottomColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _bottomColor = c;
        return *this;
    }
    GradientVerticalFluent &bottomColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientVerticalFluent);
        _bottomColor = detail::color565To888(c565);
        return *this;
    }
    void draw();
};

struct GradientHorizontalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _leftColor, _rightColor;
    bool _consumed;
    GradientHorizontalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _leftColor(0), _rightColor(0), _consumed(false) {}
    ~GradientHorizontalFluent() { draw(); }
    GradientHorizontalFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    GradientHorizontalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    GradientHorizontalFluent &leftColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _leftColor = c;
        return *this;
    }
    GradientHorizontalFluent &leftColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _leftColor = detail::color565To888(c565);
        return *this;
    }
    GradientHorizontalFluent &rightColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _rightColor = c;
        return *this;
    }
    GradientHorizontalFluent &rightColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientHorizontalFluent);
        _rightColor = detail::color565To888(c565);
        return *this;
    }
    void draw();
};

struct GradientCornersFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _c00, _c10, _c01, _c11;
    bool _consumed;
    GradientCornersFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _c00(0), _c10(0), _c01(0), _c11(0), _consumed(false) {}
    ~GradientCornersFluent() { draw(); }
    GradientCornersFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _x = x;
        _y = y;
        return *this;
    }
    GradientCornersFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _w = w;
        _h = h;
        return *this;
    }
    GradientCornersFluent &topLeftColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c00 = color;
        return *this;
    }
    GradientCornersFluent &topLeftColor(uint16_t color565)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c00 = detail::color565To888(color565);
        return *this;
    }
    GradientCornersFluent &topRightColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c10 = color;
        return *this;
    }
    GradientCornersFluent &topRightColor(uint16_t color565)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c10 = detail::color565To888(color565);
        return *this;
    }
    GradientCornersFluent &bottomLeftColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c01 = color;
        return *this;
    }
    GradientCornersFluent &bottomLeftColor(uint16_t color565)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c01 = detail::color565To888(color565);
        return *this;
    }
    GradientCornersFluent &bottomRightColor(uint32_t color)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c11 = color;
        return *this;
    }
    GradientCornersFluent &bottomRightColor(uint16_t color565)
    {
        PIPGUI_FLUENT_GUARD(GradientCornersFluent);
        _c11 = detail::color565To888(color565);
        return *this;
    }
    void draw();
};

struct GradientDiagonalFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    uint32_t _tlColor, _brColor;
    bool _consumed;
    GradientDiagonalFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _tlColor(0), _brColor(0), _consumed(false) {}
    ~GradientDiagonalFluent() { draw(); }
    GradientDiagonalFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _x = x;
        _y = y;
        return *this;
    }
    GradientDiagonalFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _w = w;
        _h = h;
        return *this;
    }
    GradientDiagonalFluent &topLeftColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _tlColor = c;
        return *this;
    }
    GradientDiagonalFluent &topLeftColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _tlColor = detail::color565To888(c565);
        return *this;
    }
    GradientDiagonalFluent &bottomRightColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _brColor = c;
        return *this;
    }
    GradientDiagonalFluent &bottomRightColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientDiagonalFluent);
        _brColor = detail::color565To888(c565);
        return *this;
    }
    void draw();
};

struct GradientRadialFluent
{
    GUI *_gui;
    int16_t _x, _y, _w, _h;
    int16_t _cx, _cy;
    int16_t _radius;
    uint32_t _innerColor, _outerColor;
    bool _consumed;
    GradientRadialFluent(GUI *g) : _gui(g), _x(0), _y(0), _w(0), _h(0), _cx(0), _cy(0), _radius(0), _innerColor(0), _outerColor(0), _consumed(false) {}
    ~GradientRadialFluent() { draw(); }
    GradientRadialFluent &pos(int16_t x, int16_t y)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _x = x;
        _y = y;
        return *this;
    }
    GradientRadialFluent &size(int16_t w, int16_t h)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _w = w;
        _h = h;
        return *this;
    }
    GradientRadialFluent &center(int16_t cx, int16_t cy)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _cx = cx;
        _cy = cy;
        return *this;
    }
    GradientRadialFluent &radius(int16_t r)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _radius = r;
        return *this;
    }
    GradientRadialFluent &innerColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _innerColor = c;
        return *this;
    }
    GradientRadialFluent &innerColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _innerColor = detail::color565To888(c565);
        return *this;
    }
    GradientRadialFluent &outerColor(uint32_t c)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _outerColor = c;
        return *this;
    }
    GradientRadialFluent &outerColor(uint16_t c565)
    {
        PIPGUI_FLUENT_GUARD(GradientRadialFluent);
        _outerColor = detail::color565To888(c565);
        return *this;
    }
    void draw();
};

struct DrawLineFluent
{
    GUI *_gui;
    int16_t _x0, _y0, _x1, _y1;
    uint16_t _color;
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
    DrawLineFluent &color(uint16_t c)
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
    uint16_t _color;
    bool _consumed;
    DrawCircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    ~DrawCircleFluent() { draw(); }
    DrawCircleFluent &pos(int16_t cx, int16_t cy)
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
    DrawCircleFluent &color(uint16_t c)
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
    uint16_t _color;
    bool _consumed;
    FillCircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    ~FillCircleFluent() { draw(); }
    FillCircleFluent &pos(int16_t cx, int16_t cy)
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
    FillCircleFluent &color(uint16_t c)
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
    uint16_t _color;
    bool _consumed;
    DrawArcFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _startDeg(0), _endDeg(360), _color(0), _consumed(false) {}
    ~DrawArcFluent() { draw(); }
    DrawArcFluent &pos(int16_t cx, int16_t cy)
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
    DrawArcFluent &color(uint16_t c)
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
    int16_t _cx, _cy;
    int16_t _rx, _ry;
    uint16_t _color;
    bool _consumed;
    DrawEllipseFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0), _consumed(false) {}
    ~DrawEllipseFluent() { draw(); }
    DrawEllipseFluent &pos(int16_t cx, int16_t cy)
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
    DrawEllipseFluent &color(uint16_t c)
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
    int16_t _cx, _cy;
    int16_t _rx, _ry;
    uint16_t _color;
    bool _consumed;
    FillEllipseFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0), _consumed(false) {}
    ~FillEllipseFluent() { draw(); }
    FillEllipseFluent &pos(int16_t cx, int16_t cy)
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
    FillEllipseFluent &color(uint16_t c)
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
    uint16_t _color;
    uint8_t _radius;
    bool _consumed;
    DrawTriangleFluent(GUI *g) : _gui(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0), _consumed(false) {}
    ~DrawTriangleFluent() { draw(); }
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
    DrawTriangleFluent &color(uint16_t c)
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
    uint16_t _color;
    uint8_t _radius;
    bool _consumed;
    FillTriangleFluent(GUI *g) : _gui(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0), _consumed(false) {}
    ~FillTriangleFluent() { draw(); }
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
    FillTriangleFluent &color(uint16_t c)
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
    uint16_t _color;
    bool _consumed;
    DrawSquircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    ~DrawSquircleFluent() { draw(); }
    DrawSquircleFluent &pos(int16_t cx, int16_t cy)
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
    DrawSquircleFluent &color(uint16_t c)
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
    uint16_t _color;
    bool _consumed;
    FillSquircleFluent(GUI *g) : _gui(g), _cx(0), _cy(0), _r(0), _color(0), _consumed(false) {}
    ~FillSquircleFluent() { draw(); }
    FillSquircleFluent &pos(int16_t cx, int16_t cy)
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
    FillSquircleFluent &color(uint16_t c)
    {
        PIPGUI_FLUENT_GUARD(FillSquircleFluent);
        _color = c;
        return *this;
    }
    void draw();
};

#pragma once

namespace pipgui
{

    struct FillRectFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(FillRectFluent);
        int16_t _x, _y, _w, _h;
        uint16_t _color;
        uint8_t _radius;
        uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
        bool _perCorner;
        FillRectFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _color(0),
              _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
              _perCorner(false) {}
        ~FillRectFluent() { draw(); }
        FillRectFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        FillRectFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        FillRectFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        FillRectFluent &radius(std::initializer_list<uint8_t> radii)
        {
            if (!canMutate())
                return *this;
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

    struct DrawRectFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawRectFluent);
        int16_t _x, _y, _w, _h;
        uint16_t _color;
        uint8_t _radius;
        uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
        bool _perCorner;
        DrawRectFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _color(0),
              _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
              _perCorner(false) {}
        ~DrawRectFluent() { draw(); }
        DrawRectFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        DrawRectFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        DrawRectFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        DrawRectFluent &radius(std::initializer_list<uint8_t> radii)
        {
            if (!canMutate())
                return *this;
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

    struct GradientVerticalFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GradientVerticalFluent);
        int16_t _x, _y, _w, _h;
        uint32_t _topColor, _bottomColor;
        GradientVerticalFluent(GUI *g) : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _topColor(0), _bottomColor(0) {}
        ~GradientVerticalFluent() { draw(); }
        GradientVerticalFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GradientVerticalFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GradientVerticalFluent &topColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _topColor = c;
            return *this;
        }
        GradientVerticalFluent &topColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _topColor = detail::color565To888(c565);
            return *this;
        }
        GradientVerticalFluent &bottomColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _bottomColor = c;
            return *this;
        }
        GradientVerticalFluent &bottomColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _bottomColor = detail::color565To888(c565);
            return *this;
        }
        void draw();
    };

    struct GradientHorizontalFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GradientHorizontalFluent);
        int16_t _x, _y, _w, _h;
        uint32_t _leftColor, _rightColor;
        GradientHorizontalFluent(GUI *g) : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _leftColor(0), _rightColor(0) {}
        ~GradientHorizontalFluent() { draw(); }
        GradientHorizontalFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GradientHorizontalFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GradientHorizontalFluent &leftColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _leftColor = c;
            return *this;
        }
        GradientHorizontalFluent &leftColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _leftColor = detail::color565To888(c565);
            return *this;
        }
        GradientHorizontalFluent &rightColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _rightColor = c;
            return *this;
        }
        GradientHorizontalFluent &rightColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _rightColor = detail::color565To888(c565);
            return *this;
        }
        void draw();
    };

    struct GradientCornersFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GradientCornersFluent);
        int16_t _x, _y, _w, _h;
        uint32_t _c00, _c10, _c01, _c11;
        GradientCornersFluent(GUI *g) : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _c00(0), _c10(0), _c01(0), _c11(0) {}
        ~GradientCornersFluent() { draw(); }
        GradientCornersFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GradientCornersFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GradientCornersFluent &topLeftColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _c00 = color;
            return *this;
        }
        GradientCornersFluent &topLeftColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _c00 = detail::color565To888(color565);
            return *this;
        }
        GradientCornersFluent &topRightColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _c10 = color;
            return *this;
        }
        GradientCornersFluent &topRightColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _c10 = detail::color565To888(color565);
            return *this;
        }
        GradientCornersFluent &bottomLeftColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _c01 = color;
            return *this;
        }
        GradientCornersFluent &bottomLeftColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _c01 = detail::color565To888(color565);
            return *this;
        }
        GradientCornersFluent &bottomRightColor(uint32_t color)
        {
            if (!canMutate())
                return *this;
            _c11 = color;
            return *this;
        }
        GradientCornersFluent &bottomRightColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _c11 = detail::color565To888(color565);
            return *this;
        }
        void draw();
    };

    struct GradientDiagonalFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GradientDiagonalFluent);
        int16_t _x, _y, _w, _h;
        uint32_t _tlColor, _brColor;
        GradientDiagonalFluent(GUI *g) : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _tlColor(0), _brColor(0) {}
        ~GradientDiagonalFluent() { draw(); }
        GradientDiagonalFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GradientDiagonalFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GradientDiagonalFluent &topLeftColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _tlColor = c;
            return *this;
        }
        GradientDiagonalFluent &topLeftColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _tlColor = detail::color565To888(c565);
            return *this;
        }
        GradientDiagonalFluent &bottomRightColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _brColor = c;
            return *this;
        }
        GradientDiagonalFluent &bottomRightColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _brColor = detail::color565To888(c565);
            return *this;
        }
        void draw();
    };

    struct GradientRadialFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(GradientRadialFluent);
        int16_t _x, _y, _w, _h;
        int16_t _cx, _cy;
        int16_t _radius;
        uint32_t _innerColor, _outerColor;
        GradientRadialFluent(GUI *g) : detail::FluentLifetime(g), _x(0), _y(0), _w(0), _h(0), _cx(0), _cy(0), _radius(0), _innerColor(0), _outerColor(0) {}
        ~GradientRadialFluent() { draw(); }
        GradientRadialFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        GradientRadialFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        GradientRadialFluent &center(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        GradientRadialFluent &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }
        GradientRadialFluent &innerColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _innerColor = c;
            return *this;
        }
        GradientRadialFluent &innerColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _innerColor = detail::color565To888(c565);
            return *this;
        }
        GradientRadialFluent &outerColor(uint32_t c)
        {
            if (!canMutate())
                return *this;
            _outerColor = c;
            return *this;
        }
        GradientRadialFluent &outerColor(uint16_t c565)
        {
            if (!canMutate())
                return *this;
            _outerColor = detail::color565To888(c565);
            return *this;
        }
        void draw();
    };

    struct DrawLineFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawLineFluent);
        int16_t _x0, _y0, _x1, _y1;
        uint8_t _thickness;
        uint16_t _color;
        DrawLineFluent(GUI *g) : detail::FluentLifetime(g), _x0(0), _y0(0), _x1(0), _y1(0), _thickness(1), _color(0) {}
        ~DrawLineFluent() { draw(); }
        DrawLineFluent &from(int16_t x0, int16_t y0)
        {
            if (!canMutate())
                return *this;
            _x0 = x0;
            _y0 = y0;
            return *this;
        }
        DrawLineFluent &to(int16_t x1, int16_t y1)
        {
            if (!canMutate())
                return *this;
            _x1 = x1;
            _y1 = y1;
            return *this;
        }
        DrawLineFluent &thickness(uint8_t t)
        {
            if (!canMutate())
                return *this;
            _thickness = t;
            return *this;
        }
        DrawLineFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct DrawCircleFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawCircleFluent);
        int16_t _cx, _cy;
        int16_t _r;
        uint16_t _color;
        DrawCircleFluent(GUI *g) : detail::FluentLifetime(g), _cx(0), _cy(0), _r(0), _color(0) {}
        ~DrawCircleFluent() { draw(); }
        DrawCircleFluent &pos(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        DrawCircleFluent &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _r = r;
            return *this;
        }
        DrawCircleFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct FillCircleFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(FillCircleFluent);
        int16_t _cx, _cy;
        int16_t _r;
        uint16_t _color;
        FillCircleFluent(GUI *g) : detail::FluentLifetime(g), _cx(0), _cy(0), _r(0), _color(0) {}
        ~FillCircleFluent() { draw(); }
        FillCircleFluent &pos(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        FillCircleFluent &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _r = r;
            return *this;
        }
        FillCircleFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct DrawArcFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawArcFluent);
        int16_t _cx, _cy;
        int16_t _r;
        uint8_t _thickness;
        float _startDeg, _endDeg;
        uint16_t _color;
        DrawArcFluent(GUI *g) : detail::FluentLifetime(g), _cx(0), _cy(0), _r(0), _thickness(1), _startDeg(0), _endDeg(360), _color(0) {}
        ~DrawArcFluent() { draw(); }
        DrawArcFluent &pos(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        DrawArcFluent &radius(int16_t r)
        {
            if (!canMutate())
                return *this;
            _r = r;
            return *this;
        }
        DrawArcFluent &thickness(uint8_t t)
        {
            if (!canMutate())
                return *this;
            _thickness = t;
            return *this;
        }
        DrawArcFluent &startDeg(float d)
        {
            if (!canMutate())
                return *this;
            _startDeg = d;
            return *this;
        }
        DrawArcFluent &endDeg(float d)
        {
            if (!canMutate())
                return *this;
            _endDeg = d;
            return *this;
        }
        DrawArcFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct DrawEllipseFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawEllipseFluent);
        int16_t _cx, _cy;
        int16_t _rx, _ry;
        uint16_t _color;
        DrawEllipseFluent(GUI *g) : detail::FluentLifetime(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0) {}
        ~DrawEllipseFluent() { draw(); }
        DrawEllipseFluent &pos(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        DrawEllipseFluent &radiusX(int16_t rx)
        {
            if (!canMutate())
                return *this;
            _rx = rx;
            return *this;
        }
        DrawEllipseFluent &radiusY(int16_t ry)
        {
            if (!canMutate())
                return *this;
            _ry = ry;
            return *this;
        }
        DrawEllipseFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct FillEllipseFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(FillEllipseFluent);
        int16_t _cx, _cy;
        int16_t _rx, _ry;
        uint16_t _color;
        FillEllipseFluent(GUI *g) : detail::FluentLifetime(g), _cx(0), _cy(0), _rx(0), _ry(0), _color(0) {}
        ~FillEllipseFluent() { draw(); }
        FillEllipseFluent &pos(int16_t cx, int16_t cy)
        {
            if (!canMutate())
                return *this;
            _cx = cx;
            _cy = cy;
            return *this;
        }
        FillEllipseFluent &radiusX(int16_t rx)
        {
            if (!canMutate())
                return *this;
            _rx = rx;
            return *this;
        }
        FillEllipseFluent &radiusY(int16_t ry)
        {
            if (!canMutate())
                return *this;
            _ry = ry;
            return *this;
        }
        FillEllipseFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct DrawTriangleFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawTriangleFluent);
        int16_t _x0, _y0, _x1, _y1, _x2, _y2;
        uint16_t _color;
        uint8_t _radius;
        DrawTriangleFluent(GUI *g) : detail::FluentLifetime(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0) {}
        ~DrawTriangleFluent() { draw(); }
        DrawTriangleFluent &point0(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x0 = x;
            _y0 = y;
            return *this;
        }
        DrawTriangleFluent &point1(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x1 = x;
            _y1 = y;
            return *this;
        }
        DrawTriangleFluent &point2(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x2 = x;
            _y2 = y;
            return *this;
        }
        DrawTriangleFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        DrawTriangleFluent &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }
        void draw();
    };

    struct FillTriangleFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(FillTriangleFluent);
        int16_t _x0, _y0, _x1, _y1, _x2, _y2;
        uint16_t _color;
        uint8_t _radius;
        FillTriangleFluent(GUI *g) : detail::FluentLifetime(g), _x0(0), _y0(0), _x1(0), _y1(0), _x2(0), _y2(0), _color(0), _radius(0) {}
        ~FillTriangleFluent() { draw(); }
        FillTriangleFluent &point0(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x0 = x;
            _y0 = y;
            return *this;
        }
        FillTriangleFluent &point1(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x1 = x;
            _y1 = y;
            return *this;
        }
        FillTriangleFluent &point2(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x2 = x;
            _y2 = y;
            return *this;
        }
        FillTriangleFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        FillTriangleFluent &radius(uint8_t r)
        {
            if (!canMutate())
                return *this;
            _radius = r;
            return *this;
        }
        void draw();
    };

    struct DrawSquircleRectFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawSquircleRectFluent);
        int16_t _x, _y, _w, _h;
        uint8_t _radius;
        uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
        bool _perCorner;
        uint16_t _color;
        DrawSquircleRectFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
              _perCorner(false), _color(0) {}
        ~DrawSquircleRectFluent() { draw(); }
        DrawSquircleRectFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        DrawSquircleRectFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        DrawSquircleRectFluent &radius(std::initializer_list<uint8_t> radii)
        {
            if (!canMutate())
                return *this;
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
        DrawSquircleRectFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

    struct FillSquircleRectFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(FillSquircleRectFluent);
        int16_t _x, _y, _w, _h;
        uint8_t _radius;
        uint8_t _radiusTL, _radiusTR, _radiusBR, _radiusBL;
        bool _perCorner;
        uint16_t _color;
        FillSquircleRectFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(0), _y(0), _w(0), _h(0),
              _radius(0), _radiusTL(0), _radiusTR(0), _radiusBR(0), _radiusBL(0),
              _perCorner(false), _color(0) {}
        ~FillSquircleRectFluent() { draw(); }
        FillSquircleRectFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }
        FillSquircleRectFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            return *this;
        }
        FillSquircleRectFluent &radius(std::initializer_list<uint8_t> radii)
        {
            if (!canMutate())
                return *this;
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
        FillSquircleRectFluent &color(uint16_t c)
        {
            if (!canMutate())
                return *this;
            _color = c;
            return *this;
        }
        void draw();
    };

}

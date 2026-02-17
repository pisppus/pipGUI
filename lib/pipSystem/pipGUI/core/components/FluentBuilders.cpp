#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{

void FillRectFluent::draw()
{
    if (_consumed || !_gui)
        return;
    if (_perCorner)
        _gui->fillRoundRectCornersFrc(_x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
    else if (_radius > 0)
        _gui->fillRoundRectFrc(_x, _y, _w, _h, _radius, _color);
    else
        _gui->fillRectImpl(_x, _y, _w, _h, _color);
    _consumed = true;
}

void FillRectAlphaFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectAlphaImpl(_x, _y, _w, _h, _color, _alpha);
    _consumed = true;
}

void FillRectGradientVerticalFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectGradientVerticalImpl(_x, _y, _w, _h, _topColor, _bottomColor);
    _consumed = true;
}

void FillRectGradientHorizontalFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectGradientHorizontalImpl(_x, _y, _w, _h, _leftColor, _rightColor);
    _consumed = true;
}

void FillRectGradient4Fluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectGradient4Impl(_x, _y, _w, _h, _c00, _c10, _c01, _c11);
    _consumed = true;
}

void FillRectGradientDiagonalFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectGradientDiagonalImpl(_x, _y, _w, _h, _tlColor, _brColor);
    _consumed = true;
}

void FillRectGradientRadialFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillRectGradientRadialImpl(_x, _y, _w, _h, _cx, _cy, _radius, _innerColor, _outerColor);
    _consumed = true;
}

void DrawLineFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->drawLineImpl(_x0, _y0, _x1, _y1, _color);
    _consumed = true;
}

void DrawCircleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->drawCircleImpl(_cx, _cy, _r, _color);
    _consumed = true;
}

void FillCircleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillCircleImpl(_cx, _cy, _r, _color);
    _consumed = true;
}

void DrawArcFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->drawArcImpl(_cx, _cy, _r, _startDeg, _endDeg, _color);
    _consumed = true;
}

void DrawEllipseFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->drawEllipseImpl(_cx, _cy, _rx, _ry, _color);
    _consumed = true;
}

void FillEllipseFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillEllipseImpl(_cx, _cy, _rx, _ry, _color);
    _consumed = true;
}

void DrawTriangleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    if (_radius > 0)
        _gui->drawRoundTriangleImpl(_x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
    else
        _gui->drawTriangleImpl(_x0, _y0, _x1, _y1, _x2, _y2, _color);
    _consumed = true;
}

void FillTriangleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    if (_radius > 0)
        _gui->fillRoundTriangleImpl(_x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
    else
        _gui->fillTriangleImpl(_x0, _y0, _x1, _y1, _x2, _y2, _color);
    _consumed = true;
}

void DrawSquircleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->drawSquircleImpl(_cx, _cy, _r, _color);
    _consumed = true;
}

void FillSquircleFluent::draw()
{
    if (_consumed || !_gui)
        return;
    _gui->fillSquircleImpl(_cx, _cy, _r, _color);
    _consumed = true;
}

template <bool IsUpdate>
void BlurStripFluentT<IsUpdate>::draw()
{
    if (_consumed || !_gui)
        return;
    if (IsUpdate)
        _gui->updateBlurStripImpl(_x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, _noiseAmount, _materialColor);
    else
        _gui->drawBlurStripImpl(_x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, _noiseAmount, _materialColor);
    _consumed = true;
}
template void BlurStripFluentT<false>::draw();
template void BlurStripFluentT<true>::draw();

template <bool IsUpdate>
void ScrollDotsFluentT<IsUpdate>::draw()
{
    if (_consumed || !_gui)
        return;
    if (IsUpdate)
        _gui->updateScrollDotsImpl(_x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth);
    else
        _gui->drawScrollDotsImpl(_x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth);
    _consumed = true;
}
template void ScrollDotsFluentT<false>::draw();
template void ScrollDotsFluentT<true>::draw();

template <bool IsUpdate>
void GlowCircleFluentT<IsUpdate>::draw()
{
    if (_consumed || !_gui)
        return;
    if (IsUpdate)
        _gui->updateGlowCircleImpl(_x, _y, _r, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
    else
        _gui->drawGlowCircleImpl(_x, _y, _r, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
    _consumed = true;
}
template void GlowCircleFluentT<false>::draw();
template void GlowCircleFluentT<true>::draw();

template <bool IsUpdate>
void GlowRoundRectFluentT<IsUpdate>::draw()
{
    if (_consumed || !_gui)
        return;
    if (_perCorner)
    {
        if (IsUpdate)
        {
            _gui->updateGlowRoundRectCornersImpl(_x, _y, _w, _h,
                                                 _radiusTL, _radiusTR, _radiusBR, _radiusBL,
                                                 _fillColor, _bgColor, _glowColor,
                                                 _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        }
        else
        {
            _gui->drawGlowRoundRectCornersImpl(_x, _y, _w, _h,
                                               _radiusTL, _radiusTR, _radiusBR, _radiusBL,
                                               _fillColor, _bgColor, _glowColor,
                                               _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        }
    }
    else
    {
        if (IsUpdate)
            _gui->updateGlowRoundRectImpl(_x, _y, _w, _h, _radius, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        else
            _gui->drawGlowRoundRectImpl(_x, _y, _w, _h, _radius, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
    }
    _consumed = true;
}
template void GlowRoundRectFluentT<false>::draw();
template void GlowRoundRectFluentT<true>::draw();

void ToastFluent::show()
{
    if (_consumed || !_gui)
        return;
    _gui->showToastInternal(_text, _durationMs, _fromTop, _iconId, _iconSizePx);
    _consumed = true;
}

}

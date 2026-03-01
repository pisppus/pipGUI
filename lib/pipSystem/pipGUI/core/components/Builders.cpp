#include <pipGUI/core/api/pipGUI.hpp>

namespace pipgui
{

    void FillRectFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        if (_perCorner)
            _gui->fillRoundRect(_x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        else if (_radius > 0)
            _gui->fillRoundRect(_x, _y, _w, _h, _radius, _color);
        else
            _gui->fillRect(_x, _y, _w, _h, _color);
        _consumed = true;
    }

    void DrawRectFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        if (_perCorner)
        {
            _gui->drawRoundRect(_x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        }
        else if (_radius > 0)
        {
            _gui->drawRoundRect(_x, _y, _w, _h, _radius, _color);
        }
        else
        {
            const int16_t x0 = _x;
            const int16_t y0 = _y;
            const int16_t x1 = (int16_t)(_x + _w - 1);
            const int16_t y1 = (int16_t)(_y + _h - 1);
            _gui->drawLine(x0, y0, x1, y0, _color);
            _gui->drawLine(x1, y0, x1, y1, _color);
            _gui->drawLine(x1, y1, x0, y1, _color);
            _gui->drawLine(x0, y1, x0, y0, _color);
        }
        _consumed = true;
    }

    void FillRectAlphaFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectAlpha(_x, _y, _w, _h, _color, _alpha);
        _consumed = true;
    }

    void FillRectGradientVerticalFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectGradientVertical(_x, _y, _w, _h, _topColor, _bottomColor);
        _consumed = true;
    }

    void FillRectGradientHorizontalFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectGradientHorizontal(_x, _y, _w, _h, _leftColor, _rightColor);
        _consumed = true;
    }

    void FillRectGradient4Fluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectGradient4(_x, _y, _w, _h, _c00, _c10, _c01, _c11);
        _consumed = true;
    }

    void FillRectGradientDiagonalFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectGradientDiagonal(_x, _y, _w, _h, _tlColor, _brColor);
        _consumed = true;
    }

    void FillRectGradientRadialFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillRectGradientRadial(_x, _y, _w, _h, _cx, _cy, _radius, _innerColor, _outerColor);
        _consumed = true;
    }

    void DrawLineFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->drawLine(_x0, _y0, _x1, _y1, _color);
        _consumed = true;
    }

    void DrawCircleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->drawCircle(_cx, _cy, _r, _color);
        _consumed = true;
    }

    void FillCircleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillCircle(_cx, _cy, _r, _color);
        _consumed = true;
    }

    void DrawArcFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->drawArc(_cx, _cy, _r, _startDeg, _endDeg, _color);
        _consumed = true;
    }

    void DrawEllipseFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->drawEllipse(_cx, _cy, _rx, _ry, _color);
        _consumed = true;
    }

    void FillEllipseFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillEllipse(_cx, _cy, _rx, _ry, _color);
        _consumed = true;
    }

    void DrawTriangleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        if (_radius > 0)
            _gui->drawRoundTriangle(_x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            _gui->drawTriangle(_x0, _y0, _x1, _y1, _x2, _y2, _color);
        _consumed = true;
    }

    void FillTriangleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        if (_radius > 0)
            _gui->fillRoundTriangle(_x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            _gui->fillTriangle(_x0, _y0, _x1, _y1, _x2, _y2, _color);
        _consumed = true;
    }

    void DrawSquircleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->drawSquircle(_cx, _cy, _r, _color);
        _consumed = true;
    }

    void FillSquircleFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _gui->fillSquircle(_cx, _cy, _r, _color);
        _consumed = true;
    }

    template <bool IsUpdate>
    void BlurRegionFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui)
            return;
        if (IsUpdate)
            _gui->updateBlurRegion(_x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, _noiseAmount, _materialColor);
        else
            _gui->drawBlurRegion(_x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, _noiseAmount, _materialColor);
        _consumed = true;
    }
    template void BlurRegionFluentT<false>::draw();
    template void BlurRegionFluentT<true>::draw();

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
            _gui->updateGlowCircle(_x, _y, _r, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        else
            _gui->drawGlowCircle(_x, _y, _r, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        _consumed = true;
    }
    template void GlowCircleFluentT<false>::draw();
    template void GlowCircleFluentT<true>::draw();

    template <bool IsUpdate>
    void GlowRectFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui)
            return;
        if (_perCorner)
        {
            if (IsUpdate)
            {
                _gui->updateGlowRect(_x, _y, _w, _h,
                                     _radiusTL, _radiusTR, _radiusBR, _radiusBL,
                                     _fillColor, _bgColor, _glowColor,
                                     _glowSize, _glowStrength, _anim, _pulsePeriodMs);
            }
            else
            {
                _gui->drawGlowRect(_x, _y, _w, _h,
                                   _radiusTL, _radiusTR, _radiusBR, _radiusBL,
                                   _fillColor, _bgColor, _glowColor,
                                   _glowSize, _glowStrength, _anim, _pulsePeriodMs);
            }
        }
        else
        {
            if (IsUpdate)
                _gui->updateGlowRect(_x, _y, _w, _h, _radius, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
            else
                _gui->drawGlowRect(_x, _y, _w, _h, _radius, _fillColor, _bgColor, _glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs);
        }
        _consumed = true;
    }
    template void GlowRectFluentT<false>::draw();
    template void GlowRectFluentT<true>::draw();

    void ToastFluent::show()
    {
        if (_consumed || !_gui)
            return;
        _gui->showToastInternal(_text, _durationMs, _fromTop, _iconId, _iconSizePx);
        _consumed = true;
    }

    template <bool IsUpdate>
    void ToggleSwitchFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui || !_state)
            return;
        if (IsUpdate)
            _gui->updateToggleSwitch(_x, _y, _w, _h, *_state, _activeColor, _inactiveColor, _knobColor);
        else
            _gui->drawToggleSwitch(_x, _y, _w, _h, *_state, _activeColor, _inactiveColor, _knobColor);
        _consumed = true;
    }
    template void ToggleSwitchFluentT<false>::draw();
    template void ToggleSwitchFluentT<true>::draw();

    template <bool IsUpdate>
    void ButtonFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui || !_state)
            return;
        if (IsUpdate)
            _gui->updateButton(_label, _x, _y, _w, _h, _baseColor, _radius, *_state);
        else
            _gui->drawButton(_label, _x, _y, _w, _h, _baseColor, _radius, *_state);
        _consumed = true;
    }
    template void ButtonFluentT<false>::draw();
    template void ButtonFluentT<true>::draw();

    template <bool IsUpdate>
    void ProgressBarFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui)
            return;
        if (IsUpdate)
            _gui->updateProgressBar(_x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim, _doFlush);
        else
            _gui->drawProgressBar(_x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim);
        _consumed = true;
    }
    template void ProgressBarFluentT<false>::draw();
    template void ProgressBarFluentT<true>::draw();

    template <bool IsUpdate>
    void CircularProgressBarFluentT<IsUpdate>::draw()
    {
        if (_consumed || !_gui)
            return;
        if (IsUpdate)
            _gui->updateCircularProgressBar(_x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim, _doFlush);
        else
            _gui->drawCircularProgressBar(_x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim);
        _consumed = true;
    }
    template void CircularProgressBarFluentT<false>::draw();
    template void CircularProgressBarFluentT<true>::draw();

}

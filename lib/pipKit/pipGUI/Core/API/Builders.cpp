#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>

namespace pipgui
{

    void FillRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
        {
            detail::GuiAccess::fillRoundRect(
                *_gui,
                _x,
                _y,
                _w,
                _h,
                _radiusTL,
                _radiusTR,
                _radiusBR,
                _radiusBL,
                _color);
        }
        else if (_radius > 0)
        {
            detail::GuiAccess::fillRoundRect(*_gui, _x, _y, _w, _h, _radius, _color);
        }
        else
        {
            detail::GuiAccess::fillRect(*_gui, _x, _y, _w, _h, _color);
        }
    }

    void DrawRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
        {
            detail::GuiAccess::drawRoundRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        }
        else if (_radius > 0)
        {
            detail::GuiAccess::drawRoundRect(*_gui, _x, _y, _w, _h, _radius, _color);
        }
        else
        {
            const int16_t x0 = _x;
            const int16_t y0 = _y;
            const int16_t x1 = (int16_t)(_x + _w - 1);
            const int16_t y1 = (int16_t)(_y + _h - 1);
            detail::GuiAccess::drawLine(*_gui, x0, y0, x1, y0, _color);
            detail::GuiAccess::drawLine(*_gui, x1, y0, x1, y1, _color);
            detail::GuiAccess::drawLine(*_gui, x1, y1, x0, y1, _color);
            detail::GuiAccess::drawLine(*_gui, x0, y1, x0, y0, _color);
        }
    }

    void GradientVerticalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillRectGradientVertical(*_gui, _x, _y, _w, _h, _topColor, _bottomColor);
    }

    void GradientHorizontalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillRectGradientHorizontal(*_gui, _x, _y, _w, _h, _leftColor, _rightColor);
    }

    void GradientCornersFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillRectGradientCorners(*_gui, _x, _y, _w, _h, _c00, _c10, _c01, _c11);
    }

    void GradientDiagonalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillRectGradientDiagonal(*_gui, _x, _y, _w, _h, _tlColor, _brColor);
    }

    void GradientRadialFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillRectGradientRadial(*_gui, _x, _y, _w, _h, _cx, _cy, _radius, _innerColor, _outerColor);
    }

    void DrawLineFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::drawLine(*_gui, _x0, _y0, _x1, _y1, _color);
    }

    void DrawCircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::drawCircle(*_gui, _cx, _cy, _r, _color);
    }

    void FillCircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillCircle(*_gui, _cx, _cy, _r, _color);
    }

    void DrawArcFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::drawArc(*_gui, _cx, _cy, _r, _startDeg, _endDeg, _color);
    }

    void DrawEllipseFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::drawEllipse(*_gui, _cx, _cy, _rx, _ry, _color);
    }

    void FillEllipseFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::fillEllipse(*_gui, _cx, _cy, _rx, _ry, _color);
    }

    void DrawTriangleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_radius > 0)
            detail::GuiAccess::drawRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            detail::GuiAccess::drawTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _color);
    }

    void FillTriangleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_radius > 0)
            detail::GuiAccess::fillRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            detail::GuiAccess::fillTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _color);
    }

    void DrawSquircleRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
            detail::GuiAccess::drawSquircleRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        else
            detail::GuiAccess::drawSquircleRect(*_gui, _x, _y, _w, _h, _radius, _color);
    }

    void FillSquircleRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
            detail::GuiAccess::fillSquircleRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        else
            detail::GuiAccess::fillSquircleRect(*_gui, _x, _y, _w, _h, _radius, _color);
    }

    template <bool IsUpdate>
    void BlurRegionFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        const int32_t materialColor = detail::optionalColor32(_materialColor);
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateBlurRegion(*_gui, _x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, materialColor); },
            [&] { detail::GuiAccess::drawBlurRegion(*_gui, _x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, materialColor); });
    }
    template void BlurRegionFluentT<false>::draw();
    template void BlurRegionFluentT<true>::draw();

    template <bool IsUpdate>
    void ScrollDotsFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateScrollDots(*_gui, _x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth); },
            [&] { detail::GuiAccess::drawScrollDots(*_gui, _x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth); });
    }
    template void ScrollDotsFluentT<false>::draw();
    template void ScrollDotsFluentT<true>::draw();

    template <bool IsUpdate>
    void GraphGridFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateGraphGrid(*_gui, _x, _y, _w, _h, _radius, _dir, _bgColor, _speed); },
            [&] { detail::GuiAccess::drawGraphGrid(*_gui, _x, _y, _w, _h, _radius, _dir, _bgColor, _speed); });
    }
    template void GraphGridFluentT<false>::draw();
    template void GraphGridFluentT<true>::draw();

    template <bool IsUpdate>
    void GraphLineFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateGraphLine(*_gui, _lineIndex, _value, _color, _valueMin, _valueMax); },
            [&] { detail::GuiAccess::drawGraphLine(*_gui, _lineIndex, _value, _color, _valueMin, _valueMax); });
    }
    template void GraphLineFluentT<false>::draw();
    template void GraphLineFluentT<true>::draw();

    template <bool IsUpdate>
    void GraphSamplesFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateGraphSamples(*_gui, _lineIndex, _samples, _sampleCount, _color, _valueMin, _valueMax); },
            [&] { detail::GuiAccess::drawGraphSamples(*_gui, _lineIndex, _samples, _sampleCount, _color, _valueMin, _valueMax); });
    }
    template void GraphSamplesFluentT<false>::draw();
    template void GraphSamplesFluentT<true>::draw();

    template <bool IsUpdate>
    void GlowCircleFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        const int16_t bgColor = detail::optionalColor16(_bgColor);
        const int16_t glowColor = detail::optionalColor16(_glowColor);
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateGlowCircle(*_gui, _x, _y, _r, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
            [&] { detail::GuiAccess::drawGlowCircle(*_gui, _x, _y, _r, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
    }
    template void GlowCircleFluentT<false>::draw();
    template void GlowCircleFluentT<true>::draw();

    template <bool IsUpdate>
    void GlowRectFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        const int16_t bgColor = detail::optionalColor16(_bgColor);
        const int16_t glowColor = detail::optionalColor16(_glowColor);
        if (_perCorner)
        {
            detail::callByMode<IsUpdate>(
                [&] { detail::GuiAccess::updateGlowRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
                [&] { detail::GuiAccess::drawGlowRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
        }
        else
        {
            detail::callByMode<IsUpdate>(
                [&] { detail::GuiAccess::updateGlowRect(*_gui, _x, _y, _w, _h, _radius, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
                [&] { detail::GuiAccess::drawGlowRect(*_gui, _x, _y, _w, _h, _radius, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
        }
    }
    template void GlowRectFluentT<false>::draw();
    template void GlowRectFluentT<true>::draw();

    void ToastFluent::show()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::showToast(*_gui, _text, _durationMs, _fromTop, _iconId, _iconSizePx);
    }

    void NotificationFluent::show()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::showNotification(*_gui, _title, _message, _buttonText, _delaySeconds, _type, detail::valueOr(_iconId, static_cast<IconId>(0xFFFF)));
    }

    void ShowErrorFluent::show()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::showError(*_gui, _message, _code, _type, _buttonText);
    }

    void PopupMenuFluent::show()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::showPopupMenu(*_gui, _itemFn, _itemUser, _count, _selectedIndex, _x, _y, _w, _maxVisible);
    }

    template <bool IsUpdate>
    void ToggleSwitchFluentT<IsUpdate>::draw()
    {
        if (!_value || !beginCommit())
            return;
        const int32_t inactiveColor = detail::optionalColor32(_inactiveColor);
        const int32_t knobColor = detail::optionalColor32(_knobColor);
        detail::ToggleState &state = detail::GuiAccess::resolveToggleState(*_gui, _x, _y, _w, _h, _activeColor, inactiveColor, knobColor);
        state.enabled = _enabledSet ? _enabled : true;
        const bool changed = detail::GuiAccess::stepToggleState(*_gui, state, *_value, _pressedSet ? _pressed : false);
        if (_changed)
            *_changed = changed;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateToggleSwitch(*_gui, _x, _y, _w, _h, state, _activeColor, inactiveColor, knobColor); },
            [&] { detail::GuiAccess::drawToggleSwitch(*_gui, _x, _y, _w, _h, state, _activeColor, inactiveColor, knobColor); });
    }
    template void ToggleSwitchFluentT<false>::draw();
    template void ToggleSwitchFluentT<true>::draw();

    template <bool IsUpdate>
    void ButtonFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::ButtonState &state = detail::GuiAccess::resolveButtonState(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, _iconId);
        state.enabled = _modeSet ? _enabled : true;
        state.loading = _modeSet ? _loading : false;
        detail::GuiAccess::stepButtonState(*_gui, state, _downSet ? _down : false);
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, _iconId, state); },
            [&] { detail::GuiAccess::drawButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, _iconId, state); });
    }
    template void ButtonFluentT<false>::draw();
    template void ButtonFluentT<true>::draw();

    template <bool IsUpdate>
    void ProgressBarFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateProgressBar(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim, _doFlush); },
            [&] { detail::GuiAccess::drawProgressBar(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim); });
    }
    template void ProgressBarFluentT<false>::draw();
    template void ProgressBarFluentT<true>::draw();

    template <bool IsUpdate>
    void CircularProgressBarFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateCircularProgressBar(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim, _doFlush); },
            [&] { detail::GuiAccess::drawCircularProgressBar(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim); });
    }
    template void CircularProgressBarFluentT<false>::draw();
    template void CircularProgressBarFluentT<true>::draw();

    void DrawScreenshotFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::renderScreenshotGallery(*_gui, _x, _y, _w, _h, _cols, _rows, _padding);
    }

}



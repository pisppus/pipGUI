#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>

namespace pipgui
{
    namespace
    {
        inline uint8_t shrinkRadius(uint8_t radius, uint8_t amount) noexcept
        {
            return (radius > amount) ? static_cast<uint8_t>(radius - amount) : 0;
        }
    }

    void DrawRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_hasFill)
        {
            if (_perCorner)
            {
                detail::GuiAccess::fillRoundRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor);
            }
            else if (_radius > 0)
            {
                detail::GuiAccess::fillRoundRect(*_gui, _x, _y, _w, _h, _radius, _fillColor);
            }
            else
            {
                detail::GuiAccess::fillRect(*_gui, _x, _y, _w, _h, _fillColor);
            }
        }

        if (_borderWidth == 0)
            return;

        for (uint8_t i = 0; i < _borderWidth; ++i)
        {
            const int16_t x = (int16_t)(_x + i);
            const int16_t y = (int16_t)(_y + i);
            const int16_t w = (int16_t)(_w - i * 2);
            const int16_t h = (int16_t)(_h - i * 2);
            if (w <= 0 || h <= 0)
                break;

            if (_perCorner)
            {
                detail::GuiAccess::drawRoundRect(*_gui, x, y, w, h,
                                                shrinkRadius(_radiusTL, i),
                                                shrinkRadius(_radiusTR, i),
                                                shrinkRadius(_radiusBR, i),
                                                shrinkRadius(_radiusBL, i),
                                                _borderColor);
            }
            else if (_radius > 0)
            {
                detail::GuiAccess::drawRoundRect(*_gui, x, y, w, h, shrinkRadius(_radius, i), _borderColor);
            }
            else
            {
                const int16_t x0 = x;
                const int16_t y0 = y;
                const int16_t x1 = (int16_t)(x + w - 1);
                const int16_t y1 = (int16_t)(y + h - 1);
                detail::GuiAccess::drawLine(*_gui, x0, y0, x1, y0, 1, _borderColor);
                detail::GuiAccess::drawLine(*_gui, x1, y0, x1, y1, 1, _borderColor);
                detail::GuiAccess::drawLine(*_gui, x1, y1, x0, y1, 1, _borderColor);
                detail::GuiAccess::drawLine(*_gui, x0, y1, x0, y0, 1, _borderColor);
            }
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
        detail::GuiAccess::drawLine(*_gui, _x0, _y0, _x1, _y1, _thickness, _color);
    }

    void DrawCircleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_hasFill)
            detail::GuiAccess::fillCircle(*_gui, _cx, _cy, _r, _fillColor);
        for (uint8_t i = 0; i < _borderWidth; ++i)
        {
            const int16_t r = (int16_t)(_r - i);
            if (r < 0)
                break;
            detail::GuiAccess::drawCircle(*_gui, _cx, _cy, r, _borderColor);
        }
    }

    void DrawArcFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::drawArc(*_gui, _cx, _cy, _r, _thickness, _startDeg, _endDeg, _color);
    }

    void DrawEllipseFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_hasFill)
            detail::GuiAccess::fillEllipse(*_gui, _cx, _cy, _rx, _ry, _fillColor);
        for (uint8_t i = 0; i < _borderWidth; ++i)
        {
            const int16_t rx = (int16_t)(_rx - i);
            const int16_t ry = (int16_t)(_ry - i);
            if (rx < 0 || ry < 0)
                break;
            detail::GuiAccess::drawEllipse(*_gui, _cx, _cy, rx, ry, _borderColor);
        }
    }

    void DrawTriangleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_hasFill)
        {
            if (_radius > 0)
                detail::GuiAccess::fillRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _radius, _fillColor);
            else
                detail::GuiAccess::fillTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _fillColor);
        }
        for (uint8_t i = 0; i < _borderWidth; ++i)
        {
            if (_radius > 0)
                detail::GuiAccess::drawRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, shrinkRadius(_radius, i), _borderColor);
            else
                detail::GuiAccess::drawTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _borderColor);
        }
    }

    void DrawSquircleRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_hasFill)
        {
            if (_perCorner)
                detail::GuiAccess::fillSquircleRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor);
            else
                detail::GuiAccess::fillSquircleRect(*_gui, _x, _y, _w, _h, _radius, _fillColor);
        }
        for (uint8_t i = 0; i < _borderWidth; ++i)
        {
            const int16_t x = (int16_t)(_x + i);
            const int16_t y = (int16_t)(_y + i);
            const int16_t w = (int16_t)(_w - i * 2);
            const int16_t h = (int16_t)(_h - i * 2);
            if (w <= 0 || h <= 0)
                break;

            if (_perCorner)
                detail::GuiAccess::drawSquircleRect(*_gui, x, y, w, h,
                                                    shrinkRadius(_radiusTL, i),
                                                    shrinkRadius(_radiusTR, i),
                                                    shrinkRadius(_radiusBR, i),
                                                    shrinkRadius(_radiusBL, i),
                                                    _borderColor);
            else
                detail::GuiAccess::drawSquircleRect(*_gui, x, y, w, h, shrinkRadius(_radius, i), _borderColor);
        }
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
            [&] { detail::GuiAccess::updateScrollDots(*_gui, _x, _y, _count, _activeIndex, _activeColor, _inactiveColor, _radius, _spacing); },
            [&] { detail::GuiAccess::drawScrollDots(*_gui, _x, _y, _count, _activeIndex, _activeColor, _inactiveColor, _radius, _spacing); });
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
            [&] { detail::GuiAccess::updateGraphLine(*_gui, _lineIndex, _value, _color, _valueMin, _valueMax, _thickness); },
            [&] { detail::GuiAccess::drawGraphLine(*_gui, _lineIndex, _value, _color, _valueMin, _valueMax, _thickness); });
    }
    template void GraphLineFluentT<false>::draw();
    template void GraphLineFluentT<true>::draw();

    template <bool IsUpdate>
    void GraphSamplesFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateGraphSamples(*_gui, _lineIndex, _samples, _sampleCount, _color, _valueMin, _valueMax, _thickness); },
            [&] { detail::GuiAccess::drawGraphSamples(*_gui, _lineIndex, _samples, _sampleCount, _color, _valueMin, _valueMax, _thickness); });
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

    void ToastFluent::commit()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::showToast(*_gui, _text, _pos == ToastPos::Top, _iconId);
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
        detail::GuiAccess::showPopupMenu(*_gui, _itemFn, _itemUser, _count, _selectedIndex, _w, _maxVisible, _anchorX, _anchorY, _anchorW, _anchorH);
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
        state.progressEnabled = _showProgress;
        state.progressFillColor = _fillColor;
        state.progressTarget = _progressValue > 100 ? 100 : _progressValue;
        state.loading = _showProgress ? false : (_modeSet ? _loading : false);
        detail::GuiAccess::stepButtonState(*_gui, state, _downSet ? _down : false);
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, _iconId, state); },
            [&] { detail::GuiAccess::drawButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, _iconId, state); });
    }
    template void ButtonFluentT<false>::draw();
    template void ButtonFluentT<true>::draw();

    template <bool IsUpdate>
    void SliderFluentT<IsUpdate>::draw()
    {
        if (!_value || !beginCommit())
            return;
        const int32_t inactiveColor = detail::optionalColor32(_inactiveColor);
        const int32_t thumbColor = detail::optionalColor32(_thumbColor);
        detail::SliderState &state = detail::GuiAccess::resolveSliderState(*_gui, _x, _y, _w, _h, _minValue, _maxValue, _step, _activeColor, inactiveColor, thumbColor);
        state.enabled = _enabledSet ? _enabled : true;
        const bool changed = detail::GuiAccess::stepSliderState(*_gui, state, *_value, _inputSet ? _nextDown : false, _inputSet ? _prevDown : false);
        if (_changed)
            *_changed = changed;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateSlider(*_gui, _x, _y, _w, _h, state, _activeColor, inactiveColor, thumbColor); },
            [&] { detail::GuiAccess::drawSlider(*_gui, _x, _y, _w, _h, state, _activeColor, inactiveColor, thumbColor); });
    }
    template void SliderFluentT<false>::draw();
    template void SliderFluentT<true>::draw();

    template <bool IsUpdate>
    void ProgressFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateProgressDecorated(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim, _showLabel ? &_label : nullptr, _labelColor, _labelAlign, _labelFontPx, _showPercent, _percentColor, _percentAlign, _percentFontPx); },
            [&] { detail::GuiAccess::drawProgressDecorated(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim, _showLabel ? &_label : nullptr, _labelColor, _labelAlign, _labelFontPx, _showPercent, _percentColor, _percentAlign, _percentFontPx); });
    }
    template void ProgressFluentT<false>::draw();
    template void ProgressFluentT<true>::draw();

    template <bool IsUpdate>
    void CircleProgressFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::GuiAccess::updateCircleProgress(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim); },
            [&] { detail::GuiAccess::drawCircleProgress(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim); });
    }
    template void CircleProgressFluentT<false>::draw();
    template void CircleProgressFluentT<true>::draw();

    void DrawScreenshotFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::GuiAccess::renderScreenshotGallery(*_gui, _x, _y, _w, _h, _cols, _rows, _padding);
    }

}



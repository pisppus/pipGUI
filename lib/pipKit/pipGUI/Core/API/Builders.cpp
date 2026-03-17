#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/BuilderAccess.hpp>

namespace pipgui
{

    void FillRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
        {
            detail::BuilderAccess::fillRoundRect(
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
            detail::BuilderAccess::fillRoundRect(*_gui, _x, _y, _w, _h, _radius, _color);
        }
        else
        {
            detail::BuilderAccess::fillRect(*_gui, _x, _y, _w, _h, _color);
        }
    }

    void DrawRectFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_perCorner)
        {
            detail::BuilderAccess::drawRoundRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _color);
        }
        else if (_radius > 0)
        {
            detail::BuilderAccess::drawRoundRect(*_gui, _x, _y, _w, _h, _radius, _color);
        }
        else
        {
            const int16_t x0 = _x;
            const int16_t y0 = _y;
            const int16_t x1 = (int16_t)(_x + _w - 1);
            const int16_t y1 = (int16_t)(_y + _h - 1);
            detail::BuilderAccess::drawLine(*_gui, x0, y0, x1, y0, _color);
            detail::BuilderAccess::drawLine(*_gui, x1, y0, x1, y1, _color);
            detail::BuilderAccess::drawLine(*_gui, x1, y1, x0, y1, _color);
            detail::BuilderAccess::drawLine(*_gui, x0, y1, x0, y0, _color);
        }
    }

    void GradientVerticalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillRectGradientVertical(*_gui, _x, _y, _w, _h, _topColor, _bottomColor);
    }

    void GradientHorizontalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillRectGradientHorizontal(*_gui, _x, _y, _w, _h, _leftColor, _rightColor);
    }

    void GradientCornersFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillRectGradientCorners(*_gui, _x, _y, _w, _h, _c00, _c10, _c01, _c11);
    }

    void GradientDiagonalFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillRectGradientDiagonal(*_gui, _x, _y, _w, _h, _tlColor, _brColor);
    }

    void GradientRadialFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillRectGradientRadial(*_gui, _x, _y, _w, _h, _cx, _cy, _radius, _innerColor, _outerColor);
    }

    void DrawLineFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::drawLine(*_gui, _x0, _y0, _x1, _y1, _color);
    }

    void DrawCircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::drawCircle(*_gui, _cx, _cy, _r, _color);
    }

    void FillCircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillCircle(*_gui, _cx, _cy, _r, _color);
    }

    void DrawArcFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::drawArc(*_gui, _cx, _cy, _r, _startDeg, _endDeg, _color);
    }

    void DrawEllipseFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::drawEllipse(*_gui, _cx, _cy, _rx, _ry, _color);
    }

    void FillEllipseFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillEllipse(*_gui, _cx, _cy, _rx, _ry, _color);
    }

    void DrawTriangleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_radius > 0)
            detail::BuilderAccess::drawRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            detail::BuilderAccess::drawTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _color);
    }

    void FillTriangleFluent::draw()
    {
        if (!beginCommit())
            return;
        if (_radius > 0)
            detail::BuilderAccess::fillRoundTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _radius, _color);
        else
            detail::BuilderAccess::fillTriangle(*_gui, _x0, _y0, _x1, _y1, _x2, _y2, _color);
    }

    void DrawSquircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::drawSquircle(*_gui, _cx, _cy, _r, _color);
    }

    void FillSquircleFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::fillSquircle(*_gui, _cx, _cy, _r, _color);
    }

    template <bool IsUpdate>
    void BlurRegionFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        const int32_t materialColor = detail::optionalColor32(_materialColor);
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateBlurRegion(*_gui, _x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, materialColor); },
            [&] { detail::BuilderAccess::drawBlurRegion(*_gui, _x, _y, _w, _h, _radius, _dir, _gradient, _materialStrength, materialColor); });
    }
    template void BlurRegionFluentT<false>::draw();
    template void BlurRegionFluentT<true>::draw();

    template <bool IsUpdate>
    void ScrollDotsFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateScrollDots(*_gui, _x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth); },
            [&] { detail::BuilderAccess::drawScrollDots(*_gui, _x, _y, _count, _activeIndex, _prevIndex, _animProgress, _animate, _animDirection, _activeColor, _inactiveColor, _dotRadius, _spacing, _activeWidth); });
    }
    template void ScrollDotsFluentT<false>::draw();
    template void ScrollDotsFluentT<true>::draw();

    template <bool IsUpdate>
    void GlowCircleFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        const int16_t bgColor = detail::optionalColor16(_bgColor);
        const int16_t glowColor = detail::optionalColor16(_glowColor);
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateGlowCircle(*_gui, _x, _y, _r, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
            [&] { detail::BuilderAccess::drawGlowCircle(*_gui, _x, _y, _r, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
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
                [&] { detail::BuilderAccess::updateGlowRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
                [&] { detail::BuilderAccess::drawGlowRect(*_gui, _x, _y, _w, _h, _radiusTL, _radiusTR, _radiusBR, _radiusBL, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
        }
        else
        {
            detail::callByMode<IsUpdate>(
                [&] { detail::BuilderAccess::updateGlowRect(*_gui, _x, _y, _w, _h, _radius, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); },
                [&] { detail::BuilderAccess::drawGlowRect(*_gui, _x, _y, _w, _h, _radius, _fillColor, bgColor, glowColor, _glowSize, _glowStrength, _anim, _pulsePeriodMs); });
        }
    }
    template void GlowRectFluentT<false>::draw();
    template void GlowRectFluentT<true>::draw();

    void ToastFluent::show()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::showToast(*_gui, _text, _durationMs, _fromTop, _iconId, _iconSizePx);
    }

    void NotificationFluent::show()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::showNotification(*_gui, _title, _message, _buttonText, _delaySeconds, _type, detail::valueOr(_iconId, static_cast<IconId>(0xFFFF)));
    }

    template <bool IsUpdate>
    void ToggleSwitchFluentT<IsUpdate>::draw()
    {
        if (!_state || !beginCommit())
            return;
        const int32_t inactiveColor = detail::optionalColor32(_inactiveColor);
        const int32_t knobColor = detail::optionalColor32(_knobColor);
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateToggleSwitch(*_gui, _x, _y, _w, _h, *_state, _activeColor, inactiveColor, knobColor); },
            [&] { detail::BuilderAccess::drawToggleSwitch(*_gui, _x, _y, _w, _h, *_state, _activeColor, inactiveColor, knobColor); });
    }
    template void ToggleSwitchFluentT<false>::draw();
    template void ToggleSwitchFluentT<true>::draw();

    template <bool IsUpdate>
    void ButtonFluentT<IsUpdate>::draw()
    {
        if (!_state || !beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, *_state); },
            [&] { detail::BuilderAccess::drawButton(*_gui, _label, _x, _y, _w, _h, _baseColor, _radius, *_state); });
    }
    template void ButtonFluentT<false>::draw();
    template void ButtonFluentT<true>::draw();

    template <bool IsUpdate>
    void ProgressBarFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateProgressBar(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim, _doFlush); },
            [&] { detail::BuilderAccess::drawProgressBar(*_gui, _x, _y, _w, _h, _value, _baseColor, _fillColor, _radius, _anim); });
    }
    template void ProgressBarFluentT<false>::draw();
    template void ProgressBarFluentT<true>::draw();

    template <bool IsUpdate>
    void CircularProgressBarFluentT<IsUpdate>::draw()
    {
        if (!beginCommit())
            return;
        detail::callByMode<IsUpdate>(
            [&] { detail::BuilderAccess::updateCircularProgressBar(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim, _doFlush); },
            [&] { detail::BuilderAccess::drawCircularProgressBar(*_gui, _x, _y, _r, _thickness, _value, _baseColor, _fillColor, _anim); });
    }
    template void CircularProgressBarFluentT<false>::draw();
    template void CircularProgressBarFluentT<true>::draw();

    void DrawScreenshotFluent::draw()
    {
        if (!beginCommit())
            return;
        detail::BuilderAccess::renderScreenshotGallery(*_gui, _x, _y, _w, _h, _cols, _rows, _padding);
    }

}

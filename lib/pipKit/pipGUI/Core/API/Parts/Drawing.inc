#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

pipcore::GuiDisplay &display()
{
    assert(_disp.display != nullptr);
    return *_disp.display;
}

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) const;

void clear(uint16_t color = 0x0000);

FillRectFluent fillRect()
{
    return FillRectFluent(this);
}

DrawRectFluent drawRect()
{
    return DrawRectFluent(this);
}
GradientVerticalFluent gradientVertical()
{
    return GradientVerticalFluent(this);
}
GradientHorizontalFluent gradientHorizontal()
{
    return GradientHorizontalFluent(this);
}
GradientCornersFluent gradientCorners()
{
    return GradientCornersFluent(this);
}
GradientDiagonalFluent gradientDiagonal()
{
    return GradientDiagonalFluent(this);
}
GradientRadialFluent gradientRadial()
{
    return GradientRadialFluent(this);
}



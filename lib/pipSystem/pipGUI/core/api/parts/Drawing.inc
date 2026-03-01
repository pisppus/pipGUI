#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

pipcore::GuiDisplay &display()
{
    assert(_disp.display != nullptr);
    return *_disp.display;
}

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) const;
uint16_t screenWidth() const;
uint16_t screenHeight() const;

void clear(uint16_t color = 0x0000);

FillRectFluent fillRect()
{
    return FillRectFluent(this);
}

DrawRectFluent drawRect()
{
    return DrawRectFluent(this);
}
FillRectAlphaFluent fillRectAlpha()
{
    return FillRectAlphaFluent(this);
}
FillRectGradientVerticalFluent fillRectGradientVertical()
{
    return FillRectGradientVerticalFluent(this);
}
FillRectGradientHorizontalFluent fillRectGradientHorizontal()
{
    return FillRectGradientHorizontalFluent(this);
}
FillRectGradient4Fluent fillRectGradient4()
{
    return FillRectGradient4Fluent(this);
}
FillRectGradientDiagonalFluent fillRectGradientDiagonal()
{
    return FillRectGradientDiagonalFluent(this);
}
FillRectGradientRadialFluent fillRectGradientRadial()
{
    return FillRectGradientRadialFluent(this);
}



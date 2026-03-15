#pragma once

#include <pipGUI/Core/API/GUI.hpp>

namespace pipgui
{
    inline FillRectFluent GUI::fillRect() { return FillRectFluent(this); }
    inline DrawRectFluent GUI::drawRect() { return DrawRectFluent(this); }
    inline GradientVerticalFluent GUI::gradientVertical() { return GradientVerticalFluent(this); }
    inline GradientHorizontalFluent GUI::gradientHorizontal() { return GradientHorizontalFluent(this); }
    inline GradientCornersFluent GUI::gradientCorners() { return GradientCornersFluent(this); }
    inline GradientDiagonalFluent GUI::gradientDiagonal() { return GradientDiagonalFluent(this); }
    inline GradientRadialFluent GUI::gradientRadial() { return GradientRadialFluent(this); }

    inline DrawLineFluent GUI::drawLine() { return DrawLineFluent(this); }
    inline DrawCircleFluent GUI::drawCircle() { return DrawCircleFluent(this); }
    inline FillCircleFluent GUI::fillCircle() { return FillCircleFluent(this); }
    inline DrawArcFluent GUI::drawArc() { return DrawArcFluent(this); }
    inline DrawEllipseFluent GUI::drawEllipse() { return DrawEllipseFluent(this); }
    inline FillEllipseFluent GUI::fillEllipse() { return FillEllipseFluent(this); }
    inline DrawTriangleFluent GUI::drawTriangle() { return DrawTriangleFluent(this); }
    inline FillTriangleFluent GUI::fillTriangle() { return FillTriangleFluent(this); }
    inline DrawSquircleFluent GUI::drawSquircle() { return DrawSquircleFluent(this); }
    inline FillSquircleFluent GUI::fillSquircle() { return FillSquircleFluent(this); }

    inline DrawBlurFluent GUI::drawBlur() { return DrawBlurFluent(this); }
    inline UpdateBlurFluent GUI::updateBlur() { return UpdateBlurFluent(this); }

    inline DrawGlowCircleFluent GUI::drawGlowCircle() { return DrawGlowCircleFluent(this); }
    inline UpdateGlowCircleFluent GUI::updateGlowCircle() { return UpdateGlowCircleFluent(this); }
    inline DrawGlowRectFluent GUI::drawGlowRect() { return DrawGlowRectFluent(this); }
    inline UpdateGlowRectFluent GUI::updateGlowRect() { return UpdateGlowRectFluent(this); }

    inline DrawScrollDotsFluent GUI::drawScrollDots() { return DrawScrollDotsFluent(this); }
    inline UpdateScrollDotsFluent GUI::updateScrollDots() { return UpdateScrollDotsFluent(this); }

    inline DrawToggleSwitchFluent GUI::drawToggleSwitch() { return DrawToggleSwitchFluent(this); }
    inline UpdateToggleSwitchFluent GUI::updateToggleSwitch() { return UpdateToggleSwitchFluent(this); }

    inline DrawButtonFluent GUI::drawButton() { return DrawButtonFluent(this); }
    inline UpdateButtonFluent GUI::updateButton() { return UpdateButtonFluent(this); }

    inline DrawProgressBarFluent GUI::drawProgressBar() { return DrawProgressBarFluent(this); }
    inline UpdateProgressBarFluent GUI::updateProgressBar() { return UpdateProgressBarFluent(this); }

    inline DrawCircularProgressBarFluent GUI::drawCircularProgressBar() { return DrawCircularProgressBarFluent(this); }
    inline UpdateCircularProgressBarFluent GUI::updateCircularProgressBar() { return UpdateCircularProgressBarFluent(this); }

    inline ToastFluent GUI::showToast() { return ToastFluent(this); }
    inline NotificationFluent GUI::showNotification() { return NotificationFluent(this); }

    inline DrawIconFluent GUI::drawIcon() { return DrawIconFluent(this); }
    inline DrawScreenshotFluent GUI::drawScreenshot() { return DrawScreenshotFluent(this); }
    inline DrawTextFluent GUI::drawText() { return DrawTextFluent(this); }
    inline UpdateTextFluent GUI::updateText() { return UpdateTextFluent(this); }
    inline DrawTextMarqueeFluent GUI::drawTextMarquee() { return DrawTextMarqueeFluent(this); }
    inline DrawTextEllipsizedFluent GUI::drawTextEllipsized() { return DrawTextEllipsizedFluent(this); }

    inline ConfigureListFluent GUI::configureList() { return ConfigureListFluent(this); }
    inline ListInputFluent GUI::listInput(uint8_t screenId) { return ListInputFluent(this, screenId); }

    inline ConfigureTileFluent GUI::configureTile() { return ConfigureTileFluent(this); }
    inline TileInputFluent GUI::tileInput(uint8_t screenId) { return TileInputFluent(this, screenId); }
}

#pragma once

#include <pipGUI/Core/GUI.hpp>

namespace pipgui
{
    inline DrawRectFluent GUI::drawRect() { return DrawRectFluent(this); }
    inline GradientVerticalFluent GUI::gradientVertical() { return GradientVerticalFluent(this); }
    inline GradientHorizontalFluent GUI::gradientHorizontal() { return GradientHorizontalFluent(this); }
    inline GradientCornersFluent GUI::gradientCorners() { return GradientCornersFluent(this); }
    inline GradientDiagonalFluent GUI::gradientDiagonal() { return GradientDiagonalFluent(this); }
    inline GradientRadialFluent GUI::gradientRadial() { return GradientRadialFluent(this); }

    inline DrawLineFluent GUI::drawLine() { return DrawLineFluent(this); }
    inline DrawCircleFluent GUI::drawCircle() { return DrawCircleFluent(this); }
    inline DrawArcFluent GUI::drawArc() { return DrawArcFluent(this); }
    inline DrawEllipseFluent GUI::drawEllipse() { return DrawEllipseFluent(this); }
    inline DrawTriangleFluent GUI::drawTriangle() { return DrawTriangleFluent(this); }
    inline DrawSquircleRectFluent GUI::drawSquircleRect() { return DrawSquircleRectFluent(this); }

    inline DrawBlurFluent GUI::drawBlur() { return DrawBlurFluent(this); }
    inline UpdateBlurFluent GUI::updateBlur() { return UpdateBlurFluent(this); }

    inline DrawGlowCircleFluent GUI::drawGlowCircle() { return DrawGlowCircleFluent(this); }
    inline UpdateGlowCircleFluent GUI::updateGlowCircle() { return UpdateGlowCircleFluent(this); }

    inline DrawScrollDotsFluent GUI::drawScrollDots() { return DrawScrollDotsFluent(this); }
    inline UpdateScrollDotsFluent GUI::updateScrollDots() { return UpdateScrollDotsFluent(this); }

    inline DrawGraphGridFluent GUI::drawGraphGrid() { return DrawGraphGridFluent(this); }
    inline UpdateGraphGridFluent GUI::updateGraphGrid() { return UpdateGraphGridFluent(this); }
    inline DrawGraphLineFluent GUI::drawGraphLine() { return DrawGraphLineFluent(this); }
    inline UpdateGraphLineFluent GUI::updateGraphLine() { return UpdateGraphLineFluent(this); }
    inline DrawGraphSamplesFluent GUI::drawGraphSamples() { return DrawGraphSamplesFluent(this); }
    inline UpdateGraphSamplesFluent GUI::updateGraphSamples() { return UpdateGraphSamplesFluent(this); }

    inline DrawToggleSwitchFluent GUI::drawToggleSwitch() { return DrawToggleSwitchFluent(this); }
    inline UpdateToggleSwitchFluent GUI::updateToggleSwitch() { return UpdateToggleSwitchFluent(this); }

    inline DrawButtonFluent GUI::drawButton() { return DrawButtonFluent(this); }
    inline UpdateButtonFluent GUI::updateButton() { return UpdateButtonFluent(this); }
    inline DrawSliderFluent GUI::drawSlider() { return DrawSliderFluent(this); }
    inline UpdateSliderFluent GUI::updateSlider() { return UpdateSliderFluent(this); }

    inline DrawProgressFluent GUI::drawProgress() { return DrawProgressFluent(this); }
    inline UpdateProgressFluent GUI::updateProgress() { return UpdateProgressFluent(this); }

    inline DrawCircleProgressFluent GUI::drawCircleProgress() { return DrawCircleProgressFluent(this); }
    inline UpdateCircleProgressFluent GUI::updateCircleProgress() { return UpdateCircleProgressFluent(this); }
    inline DrawDrumRollFluent GUI::drawDrumRoll() { return DrawDrumRollFluent(this); }

    inline ToastFluent GUI::showToast() { return ToastFluent(this); }
    inline NotificationFluent GUI::showNotification() { return NotificationFluent(this); }
    inline ShowErrorFluent GUI::showError() { return ShowErrorFluent(this); }
    inline PopupMenuFluent GUI::showPopupMenu() { return PopupMenuFluent(this); }
    inline PopupMenuInputFluent GUI::popupMenuInput() { return PopupMenuInputFluent(this); }

    inline DrawIconFluent GUI::drawIcon() { return DrawIconFluent(this); }
    inline DrawAnimIconFluent GUI::drawAnimIcon() { return DrawAnimIconFluent(this); }
    inline UpdateAnimIconFluent GUI::updateAnimIcon() { return UpdateAnimIconFluent(this); }
    inline DrawScreenshotFluent GUI::drawScreenshot() { return DrawScreenshotFluent(this); }
    inline DrawTextFluent GUI::drawText() { return DrawTextFluent(this); }
    inline UpdateTextFluent GUI::updateText() { return UpdateTextFluent(this); }
    inline DrawTextMarqueeFluent GUI::drawTextMarquee() { return DrawTextMarqueeFluent(this); }
    inline DrawTextEllipsizedFluent GUI::drawTextEllipsized() { return DrawTextEllipsizedFluent(this); }

    inline ConfigStatusBarFluent GUI::configStatusBar() { return ConfigStatusBarFluent(this); }
    inline SetStatusBarTextFluent GUI::setStatusBarText() { return SetStatusBarTextFluent(this); }
    inline SetStatusBarIconFluent GUI::setStatusBarIcon() { return SetStatusBarIconFluent(this); }

    inline UpdateListFluent GUI::updateList() { return UpdateListFluent(this); }
    inline ListInputFluent GUI::listInput() { return ListInputFluent(this); }

    inline UpdateTileFluent GUI::updateTile() { return UpdateTileFluent(this); }
    inline TileInputFluent GUI::tileInput() { return TileInputFluent(this); }
    inline ConfigureBacklightFluent GUI::setBacklight() { return ConfigureBacklightFluent(this); }
    inline SetClipFluent GUI::setClip() { return SetClipFluent(this); }
    inline ShowLogoFluent GUI::showLogo() { return ShowLogoFluent(this); }
}

#pragma once

#include <pipGUI/Core/GUI.hpp>

namespace pipgui
{
    namespace detail
    {
        struct GuiAccess
        {
            static void configureDisplay(GUI &gui, const pipcore::DisplayConfig &cfg)
            {
                gui.configureDisplay(cfg);
            }

            static void setClip(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h)
            {
                gui.applyClip(x, y, w, h);
            }

            static void startLogo(GUI &gui,
                                  const String &title,
                                  const String &subtitle,
                                  BootAnimation anim,
                                  uint32_t fg,
                                  uint32_t bg,
                                  uint32_t dur,
                                  int16_t x,
                                  int16_t y)
            {
                gui.startLogo(title, subtitle, anim, fg, bg, dur, x, y);
            }

            static void configureBacklight(GUI &gui,
                                           uint8_t pin,
                                           uint8_t channel,
                                           uint32_t freqHz,
                                           uint8_t resolutionBits)
            {
                gui.configureBacklight(pin, channel, freqHz, resolutionBits);
            }

            static pipcore::Platform *platform(GUI &gui)
            {
                return gui.platform();
            }

            static void renderScreenshotGallery(GUI &gui,
                                                int16_t x,
                                                int16_t y,
                                                int16_t w,
                                                int16_t h,
                                                uint8_t cols,
                                                uint8_t rows,
                                                uint16_t padding)
            {
                gui.renderScreenshotGallery(x, y, w, h, cols, rows, padding);
            }

            static ListState *ensureList(GUI &gui, uint8_t screenId)
            {
                return gui.ensureList(screenId);
            }

            static TileState *getTile(GUI &gui, uint8_t screenId)
            {
                return gui.getTile(screenId);
            }

            static void handleListInput(GUI &gui, uint8_t screenId, bool nextDown, bool prevDown)
            {
                gui.handleListInput(screenId, nextDown, prevDown);
            }

            static void handleTileInput(GUI &gui, uint8_t screenId, bool nextDown, bool prevDown)
            {
                gui.handleTileInput(screenId, nextDown, prevDown);
            }

            static void handlePopupMenuInput(GUI &gui, bool nextDown, bool prevDown)
            {
                gui.handlePopupMenuInput(nextDown, prevDown);
            }

            static void configGraphScope(GUI &gui,
                                         uint8_t screenId,
                                         uint16_t sampleRateHz,
                                         uint16_t timebaseMs,
                                         uint16_t visibleSamples)
            {
                gui.configGraphScope(screenId, sampleRateHz, timebaseMs, visibleSamples);
            }

            static void configureTile(GUI &gui,
                                      uint8_t screenId,
                                      const TileItemDef *items,
                                      uint8_t itemCount,
                                      const TileStyle &style)
            {
                gui.configureTile(screenId, items, itemCount, style);
            }

            static void fillRect(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
            {
                gui.fillRect(x, y, w, h, color);
            }

            static void fillRoundRect(GUI &gui,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h,
                                      uint8_t radius,
                                      uint16_t color)
            {
                gui.fillRoundRect(x, y, w, h, radius, color);
            }

            static void fillRoundRect(GUI &gui,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h,
                                      uint8_t tl,
                                      uint8_t tr,
                                      uint8_t br,
                                      uint8_t bl,
                                      uint16_t color)
            {
                gui.fillRoundRect(x, y, w, h, tl, tr, br, bl, color);
            }

            static void drawRoundRect(GUI &gui,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h,
                                      uint8_t radius,
                                      uint16_t color)
            {
                gui.drawRoundRect(x, y, w, h, radius, color);
            }

            static void drawRoundRect(GUI &gui,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h,
                                      uint8_t tl,
                                      uint8_t tr,
                                      uint8_t br,
                                      uint8_t bl,
                                      uint16_t color)
            {
                gui.drawRoundRect(x, y, w, h, tl, tr, br, bl, color);
            }

            static void drawLine(GUI &gui,
                                 int16_t x0,
                                 int16_t y0,
                                 int16_t x1,
                                 int16_t y1,
                                 uint8_t thickness,
                                 uint16_t color)
            {
                gui.drawLine(x0, y0, x1, y1, thickness, color);
            }

            static void drawLineSegment(GUI &gui,
                                        int16_t x0,
                                        int16_t y0,
                                        int16_t x1,
                                        int16_t y1,
                                        uint8_t thickness,
                                        uint16_t color,
                                        bool roundStart,
                                        bool roundEnd)
            {
                gui.drawLineCore(x0, y0, x1, y1, thickness, color, roundStart, roundEnd, false);
            }

            static void fillRectGradientVertical(GUI &gui,
                                                 int16_t x,
                                                 int16_t y,
                                                 int16_t w,
                                                 int16_t h,
                                                 uint32_t topColor,
                                                 uint32_t bottomColor)
            {
                gui.fillRectGradientVertical(x, y, w, h, topColor, bottomColor);
            }

            static void fillRectGradientHorizontal(GUI &gui,
                                                   int16_t x,
                                                   int16_t y,
                                                   int16_t w,
                                                   int16_t h,
                                                   uint32_t leftColor,
                                                   uint32_t rightColor)
            {
                gui.fillRectGradientHorizontal(x, y, w, h, leftColor, rightColor);
            }

            static void fillRectGradientCorners(GUI &gui,
                                                int16_t x,
                                                int16_t y,
                                                int16_t w,
                                                int16_t h,
                                                uint32_t c00,
                                                uint32_t c10,
                                                uint32_t c01,
                                                uint32_t c11)
            {
                gui.fillRectGradientCorners(x, y, w, h, c00, c10, c01, c11);
            }

            static void fillRectGradientDiagonal(GUI &gui,
                                                 int16_t x,
                                                 int16_t y,
                                                 int16_t w,
                                                 int16_t h,
                                                 uint32_t tlColor,
                                                 uint32_t brColor)
            {
                gui.fillRectGradientDiagonal(x, y, w, h, tlColor, brColor);
            }

            static void fillRectGradientRadial(GUI &gui,
                                               int16_t x,
                                               int16_t y,
                                               int16_t w,
                                               int16_t h,
                                               int16_t cx,
                                               int16_t cy,
                                               int16_t radius,
                                               uint32_t innerColor,
                                               uint32_t outerColor)
            {
                gui.fillRectGradientRadial(x, y, w, h, cx, cy, radius, innerColor, outerColor);
            }

            static void drawCircle(GUI &gui, int16_t cx, int16_t cy, int16_t r, uint16_t color)
            {
                gui.drawCircle(cx, cy, r, color);
            }

            static void fillCircle(GUI &gui, int16_t cx, int16_t cy, int16_t r, uint16_t color)
            {
                gui.fillCircle(cx, cy, r, color);
            }

            static void drawArc(GUI &gui,
                                int16_t cx,
                                int16_t cy,
                                int16_t r,
                                uint8_t thickness,
                                float startDeg,
                                float endDeg,
                                uint16_t color)
            {
                gui.drawArc(cx, cy, r, thickness, startDeg, endDeg, color);
            }

            static void drawEllipse(GUI &gui,
                                    int16_t cx,
                                    int16_t cy,
                                    int16_t rx,
                                    int16_t ry,
                                    uint16_t color)
            {
                gui.drawEllipse(cx, cy, rx, ry, color);
            }

            static void fillEllipse(GUI &gui,
                                    int16_t cx,
                                    int16_t cy,
                                    int16_t rx,
                                    int16_t ry,
                                    uint16_t color)
            {
                gui.fillEllipse(cx, cy, rx, ry, color);
            }

            static void drawTriangle(GUI &gui,
                                     int16_t x0,
                                     int16_t y0,
                                     int16_t x1,
                                     int16_t y1,
                                     int16_t x2,
                                     int16_t y2,
                                     uint16_t color)
            {
                gui.drawTriangle(x0, y0, x1, y1, x2, y2, color);
            }

            static void fillTriangle(GUI &gui,
                                     int16_t x0,
                                     int16_t y0,
                                     int16_t x1,
                                     int16_t y1,
                                     int16_t x2,
                                     int16_t y2,
                                     uint16_t color)
            {
                gui.fillTriangle(x0, y0, x1, y1, x2, y2, color);
            }

            static void drawRoundTriangle(GUI &gui,
                                          int16_t x0,
                                          int16_t y0,
                                          int16_t x1,
                                          int16_t y1,
                                          int16_t x2,
                                          int16_t y2,
                                          uint8_t radius,
                                          uint16_t color)
            {
                gui.drawRoundTriangle(x0, y0, x1, y1, x2, y2, radius, color);
            }

            static void fillRoundTriangle(GUI &gui,
                                          int16_t x0,
                                          int16_t y0,
                                          int16_t x1,
                                          int16_t y1,
                                          int16_t x2,
                                          int16_t y2,
                                          uint8_t radius,
                                          uint16_t color)
            {
                gui.fillRoundTriangle(x0, y0, x1, y1, x2, y2, radius, color);
            }

            static void drawSquircleRect(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color)
            {
                gui.drawSquircleRect(x, y, w, h, radius, color);
            }

            static void drawSquircleRect(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h,
                                         uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                                         uint16_t color)
            {
                gui.drawSquircleRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL, color);
            }

            static void fillSquircleRect(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, uint16_t color)
            {
                gui.fillSquircleRect(x, y, w, h, radius, color);
            }

            static void fillSquircleRect(GUI &gui, int16_t x, int16_t y, int16_t w, int16_t h,
                                         uint8_t radiusTL, uint8_t radiusTR, uint8_t radiusBR, uint8_t radiusBL,
                                         uint16_t color)
            {
                gui.fillSquircleRect(x, y, w, h, radiusTL, radiusTR, radiusBR, radiusBL, color);
            }

            static void drawBlurRegion(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       int16_t w,
                                       int16_t h,
                                       uint8_t radius,
                                       BlurDirection dir,
                                       bool gradient,
                                       uint8_t materialStrength,
                                       int32_t materialColor)
            {
                gui.drawBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, materialColor);
            }

            static void updateBlurRegion(GUI &gui,
                                         int16_t x,
                                         int16_t y,
                                         int16_t w,
                                         int16_t h,
                                         uint8_t radius,
                                         BlurDirection dir,
                                         bool gradient,
                                         uint8_t materialStrength,
                                         int32_t materialColor)
            {
                gui.updateBlurRegion(x, y, w, h, radius, dir, gradient, materialStrength, materialColor);
            }

            static void drawScrollDots(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       uint8_t count,
                                       uint8_t activeIndex,
                                       uint16_t activeColor,
                                       uint16_t inactiveColor,
                                       uint8_t radius,
                                       uint8_t spacing)
            {
                gui.drawScrollDotsImpl(
                    x,
                    y,
                    count,
                    activeIndex,
                    activeColor,
                    inactiveColor,
                    radius,
                    spacing);
            }

            static void updateScrollDots(GUI &gui,
                                         int16_t x,
                                         int16_t y,
                                         uint8_t count,
                                         uint8_t activeIndex,
                                         uint16_t activeColor,
                                         uint16_t inactiveColor,
                                         uint8_t radius,
                                         uint8_t spacing)
            {
                gui.updateScrollDotsImpl(
                    x,
                    y,
                    count,
                    activeIndex,
                    activeColor,
                    inactiveColor,
                    radius,
                    spacing);
            }

            static void drawGraphGrid(GUI &gui,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h,
                                      uint8_t radius,
                                      GraphDirection dir,
                                      uint32_t bgColor,
                                      float speed)
            {
                gui.drawGraphGrid(x, y, w, h, radius, dir, bgColor, speed);
            }

            static void updateGraphGrid(GUI &gui,
                                        int16_t x,
                                        int16_t y,
                                        int16_t w,
                                        int16_t h,
                                        uint8_t radius,
                                        GraphDirection dir,
                                        uint32_t bgColor,
                                        float speed)
            {
                gui.updateGraphGrid(x, y, w, h, radius, dir, bgColor, speed);
            }

            static void drawGraphLine(GUI &gui,
                                      uint8_t lineIndex,
                                      int16_t value,
                                      uint32_t color,
                                      int16_t valueMin,
                                      int16_t valueMax,
                                      uint8_t thickness)
            {
                gui.drawGraphLine(lineIndex, value, color, valueMin, valueMax, thickness);
            }

            static void updateGraphLine(GUI &gui,
                                        uint8_t lineIndex,
                                        int16_t value,
                                        uint32_t color,
                                        int16_t valueMin,
                                        int16_t valueMax,
                                        uint8_t thickness)
            {
                gui.updateGraphLine(lineIndex, value, color, valueMin, valueMax, thickness);
            }

            static void drawGraphSamples(GUI &gui,
                                         uint8_t lineIndex,
                                         const int16_t *samples,
                                         uint16_t sampleCount,
                                         uint32_t color,
                                         int16_t valueMin,
                                         int16_t valueMax,
                                         uint8_t thickness)
            {
                gui.drawGraphSamples(lineIndex, samples, sampleCount, color, valueMin, valueMax, thickness);
            }

            static void updateGraphSamples(GUI &gui,
                                           uint8_t lineIndex,
                                           const int16_t *samples,
                                           uint16_t sampleCount,
                                           uint32_t color,
                                           int16_t valueMin,
                                           int16_t valueMax,
                                           uint8_t thickness)
            {
                gui.updateGraphSamples(lineIndex, samples, sampleCount, color, valueMin, valueMax, thickness);
            }

            static void drawGlowCircle(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       int16_t r,
                                       uint16_t fillColor,
                                       int16_t bgColor,
                                       int16_t glowColor,
                                       uint8_t glowSize,
                                       uint8_t glowStrength,
                                       GlowAnim anim,
                                       uint16_t pulsePeriodMs)
            {
                gui.drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void updateGlowCircle(GUI &gui,
                                         int16_t x,
                                         int16_t y,
                                         int16_t r,
                                         uint16_t fillColor,
                                         int16_t bgColor,
                                         int16_t glowColor,
                                         uint8_t glowSize,
                                         uint8_t glowStrength,
                                         GlowAnim anim,
                                         uint16_t pulsePeriodMs)
            {
                gui.updateGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void drawGlowRect(GUI &gui,
                                     int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     uint8_t radius,
                                     uint16_t fillColor,
                                     int16_t bgColor,
                                     int16_t glowColor,
                                     uint8_t glowSize,
                                     uint8_t glowStrength,
                                     GlowAnim anim,
                                     uint16_t pulsePeriodMs)
            {
                gui.drawGlowRect(x, y, w, h, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void updateGlowRect(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       int16_t w,
                                       int16_t h,
                                       uint8_t radius,
                                       uint16_t fillColor,
                                       int16_t bgColor,
                                       int16_t glowColor,
                                       uint8_t glowSize,
                                       uint8_t glowStrength,
                                       GlowAnim anim,
                                       uint16_t pulsePeriodMs)
            {
                gui.updateGlowRect(x, y, w, h, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void drawGlowRect(GUI &gui,
                                     int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     uint8_t tl,
                                     uint8_t tr,
                                     uint8_t br,
                                     uint8_t bl,
                                     uint16_t fillColor,
                                     int16_t bgColor,
                                     int16_t glowColor,
                                     uint8_t glowSize,
                                     uint8_t glowStrength,
                                     GlowAnim anim,
                                     uint16_t pulsePeriodMs)
            {
                gui.drawGlowRect(x, y, w, h, tl, tr, br, bl, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void updateGlowRect(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       int16_t w,
                                       int16_t h,
                                       uint8_t tl,
                                       uint8_t tr,
                                       uint8_t br,
                                       uint8_t bl,
                                       uint16_t fillColor,
                                       int16_t bgColor,
                                       int16_t glowColor,
                                       uint8_t glowSize,
                                       uint8_t glowStrength,
                                       GlowAnim anim,
                                       uint16_t pulsePeriodMs)
            {
                gui.updateGlowRect(x, y, w, h, tl, tr, br, bl, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
            }

            static void showToast(GUI &gui,
                                  const String &text,
                                  bool fromTop,
                                  IconId iconId)
            {
                gui.showToastInternal(text, fromTop, iconId);
            }

            static void showNotification(GUI &gui,
                                         const String &title,
                                         const String &message,
                                         const String &buttonText,
                                         uint16_t delaySeconds,
                                         NotificationType type,
                                         IconId iconId)
            {
                gui.showNotificationInternal(title, message, buttonText, delaySeconds, type, iconId);
            }

            static void showError(GUI &gui,
                                  const String &message,
                                  const String &code,
                                  ErrorType type,
                                  const String &buttonText)
            {
                gui.startError(message, code, type, buttonText);
            }

            static void showPopupMenu(GUI &gui,
                                      PopupMenuItemFn itemFn,
                                      void *itemUser,
                                      uint8_t count,
                                      uint8_t selectedIndex,
                                      int16_t w,
                                      uint8_t maxVisible,
                                      int16_t anchorX,
                                      int16_t anchorY,
                                      int16_t anchorW,
                                      int16_t anchorH)
            {
                gui.showPopupMenuInternal(itemFn, itemUser, count, selectedIndex, w, maxVisible, anchorX, anchorY, anchorW, anchorH);
            }

            static void drawToggleSwitch(GUI &gui,
                                         int16_t x,
                                         int16_t y,
                                         int16_t w,
                                         int16_t h,
                                         ToggleState &state,
                                         uint16_t activeColor,
                                         int32_t inactiveColor,
                                         int32_t knobColor)
            {
                gui.drawToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            }

            static void updateToggleSwitch(GUI &gui,
                                           int16_t x,
                                           int16_t y,
                                           int16_t w,
                                           int16_t h,
                                           ToggleState &state,
                                           uint16_t activeColor,
                                           int32_t inactiveColor,
                                           int32_t knobColor)
            {
                gui.updateToggleSwitch(x, y, w, h, state, activeColor, inactiveColor, knobColor);
            }

            static ToggleState &resolveToggleState(GUI &gui,
                                                   int16_t x,
                                                   int16_t y,
                                                   int16_t w,
                                                   int16_t h,
                                                   uint16_t activeColor,
                                                   int32_t inactiveColor,
                                                   int32_t knobColor)
            {
                return gui.resolveToggleState(x, y, w, h, activeColor, inactiveColor, knobColor);
            }

            static bool stepToggleState(GUI &gui, ToggleState &state, bool &value, bool pressed)
            {
                return gui.stepToggleState(state, value, pressed);
            }

            static void drawButton(GUI &gui,
                                   const String &label,
                                   int16_t x,
                                   int16_t y,
                                   int16_t w,
                                   int16_t h,
                                   uint16_t baseColor,
                                   uint8_t radius,
                                   IconId iconId,
                                   ButtonState &state)
            {
                gui.drawButton(label, x, y, w, h, baseColor, radius, iconId, state);
            }

            static void updateButton(GUI &gui,
                                     const String &label,
                                     int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     uint16_t baseColor,
                                     uint8_t radius,
                                     IconId iconId,
                                     ButtonState &state)
            {
                gui.updateButton(label, x, y, w, h, baseColor, radius, iconId, state);
            }

            static ButtonState &resolveButtonState(GUI &gui,
                                                   const String &label,
                                                   int16_t x,
                                                   int16_t y,
                                                   int16_t w,
                                                   int16_t h,
                                                   uint16_t baseColor,
                                                   uint8_t radius,
                                                   IconId iconId)
            {
                return gui.resolveButtonState(label, x, y, w, h, baseColor, radius, iconId);
            }

            static void stepButtonState(GUI &gui, ButtonState &state, bool isDown)
            {
                gui.stepButtonState(state, isDown);
            }

            static void drawSlider(GUI &gui,
                                   int16_t x,
                                   int16_t y,
                                   int16_t w,
                                   int16_t h,
                                   SliderState &state,
                                   uint16_t activeColor,
                                   int32_t inactiveColor,
                                   int32_t thumbColor)
            {
                gui.drawSlider(x, y, w, h, state, activeColor, inactiveColor, thumbColor);
            }

            static void updateSlider(GUI &gui,
                                     int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     SliderState &state,
                                     uint16_t activeColor,
                                     int32_t inactiveColor,
                                     int32_t thumbColor)
            {
                gui.updateSlider(x, y, w, h, state, activeColor, inactiveColor, thumbColor);
            }

            static SliderState &resolveSliderState(GUI &gui,
                                                   int16_t x,
                                                   int16_t y,
                                                   int16_t w,
                                                   int16_t h,
                                                   int16_t minValue,
                                                   int16_t maxValue,
                                                   int16_t step,
                                                   uint16_t activeColor,
                                                   int32_t inactiveColor,
                                                   int32_t thumbColor)
            {
                return gui.resolveSliderState(x, y, w, h, minValue, maxValue, step, activeColor, inactiveColor, thumbColor);
            }

            static bool stepSliderState(GUI &gui, SliderState &state, int16_t &value, bool nextDown, bool prevDown)
            {
                return gui.stepSliderState(state, value, nextDown, prevDown);
            }

            static void drawProgress(GUI &gui,
                                     int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     uint8_t value,
                                     uint16_t baseColor,
                                     uint16_t fillColor,
                                     uint8_t radius,
                                     ProgressAnim anim)
            {
                gui.drawProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            }

            static void updateProgress(GUI &gui,
                                       int16_t x,
                                       int16_t y,
                                       int16_t w,
                                       int16_t h,
                                       uint8_t value,
                                       uint16_t baseColor,
                                       uint16_t fillColor,
                                       uint8_t radius,
                                       ProgressAnim anim)
            {
                gui.updateProgress(x, y, w, h, value, baseColor, fillColor, radius, anim);
            }

            static void drawProgressDecorated(GUI &gui,
                                              int16_t x,
                                              int16_t y,
                                              int16_t w,
                                              int16_t h,
                                              uint8_t value,
                                              uint16_t baseColor,
                                              uint16_t fillColor,
                                              uint8_t radius,
                                              ProgressAnim anim,
                                              const String *label,
                                              uint16_t labelColor,
                                              TextAlign labelAlign,
                                              uint16_t labelFontPx,
                                              bool showPercent,
                                              uint16_t percentColor,
                                              TextAlign percentAlign,
                                              uint16_t percentFontPx)
            {
                gui.drawProgressDecorated(x, y, w, h, value, baseColor, fillColor, radius, anim,
                                          label, labelColor, labelAlign, labelFontPx,
                                          showPercent, percentColor, percentAlign, percentFontPx);
            }

            static void updateProgressDecorated(GUI &gui,
                                                int16_t x,
                                                int16_t y,
                                                int16_t w,
                                                int16_t h,
                                                uint8_t value,
                                                uint16_t baseColor,
                                                uint16_t fillColor,
                                                uint8_t radius,
                                                ProgressAnim anim,
                                                const String *label,
                                                uint16_t labelColor,
                                                TextAlign labelAlign,
                                                uint16_t labelFontPx,
                                                bool showPercent,
                                                uint16_t percentColor,
                                                TextAlign percentAlign,
                                                uint16_t percentFontPx)
            {
                gui.updateProgressDecorated(x, y, w, h, value, baseColor, fillColor, radius, anim,
                                            label, labelColor, labelAlign, labelFontPx,
                                            showPercent, percentColor, percentAlign, percentFontPx);
            }

            static void drawCircleProgress(GUI &gui,
                                           int16_t x,
                                           int16_t y,
                                           int16_t r,
                                           uint8_t thickness,
                                           uint8_t value,
                                           uint16_t baseColor,
                                           uint16_t fillColor,
                                           ProgressAnim anim)
            {
                gui.drawCircleProgress(x, y, r, thickness, value, baseColor, fillColor, anim);
            }

            static void updateCircleProgress(GUI &gui,
                                             int16_t x,
                                             int16_t y,
                                             int16_t r,
                                             uint8_t thickness,
                                             uint8_t value,
                                             uint16_t baseColor,
                                             uint16_t fillColor,
                                             ProgressAnim anim)
            {
                gui.updateCircleProgress(x, y, r, thickness, value, baseColor, fillColor, anim);
            }

            static void drawText(GUI &gui,
                                 const String &text,
                                 int16_t x,
                                 int16_t y,
                                 uint16_t fg565,
                                 uint16_t bg565,
                                 TextAlign align)
            {
                gui.drawText(text, x, y, fg565, bg565, align);
            }

            static void updateText(GUI &gui,
                                   const String &text,
                                   int16_t x,
                                   int16_t y,
                                   uint16_t fg565,
                                   uint16_t bg565,
                                   TextAlign align)
            {
                gui.updateText(text, x, y, fg565, bg565, align);
            }

            static bool drawTextMarquee(GUI &gui,
                                        const String &text,
                                        int16_t x,
                                        int16_t y,
                                        int16_t maxWidth,
                                        uint16_t fg565,
                                        TextAlign align,
                                        const MarqueeTextOptions &opts)
            {
                return gui.drawTextMarquee(text, x, y, maxWidth, fg565, align, opts);
            }

            static bool drawTextEllipsized(GUI &gui,
                                           const String &text,
                                           int16_t x,
                                           int16_t y,
                                           int16_t maxWidth,
                                           uint16_t fg565,
                                           TextAlign align)
            {
                return gui.drawTextEllipsized(text, x, y, maxWidth, fg565, align);
            }

            static void drawIcon(GUI &gui,
                                 uint16_t iconId,
                                 int16_t x,
                                 int16_t y,
                                 uint16_t sizePx,
                                 uint16_t fg565,
                                 uint16_t bg565)
            {
                if (gui._flags.spriteEnabled && gui._disp.display && !gui._flags.inSpritePass)
                    gui.updateIconInternal(iconId, x, y, sizePx, fg565, bg565);
                else
                    gui.drawIconInternal(iconId, x, y, sizePx, fg565);
            }

            static void configureStatusBar(GUI &gui, bool enabled, uint32_t bgColor, uint8_t height, StatusBarPosition pos)
            {
                gui.configureStatusBar(enabled, bgColor, height, pos);
            }

            static void setStatusBarText(GUI &gui, const String &left, const String &center, const String &right)
            {
                gui.setStatusBarText(left, center, right);
            }

            static void setStatusBarIcon(GUI &gui, TextAlign side, IconId iconId, int32_t color, uint16_t sizePx)
            {
                gui.setStatusBarIcon(side, iconId, color, sizePx);
            }
        };
    }
}

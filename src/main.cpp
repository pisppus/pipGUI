#include <Arduino.h>
#include <SPIFFS.h>
#include <math.h>
#include <pipKit.hpp>

using namespace pipgui;

namespace
{
  constexpr int16_t kStatusBarHeight = 20;
  constexpr uint32_t kToastIntervalMs = 12000;
  constexpr uint32_t kBatteryStepMs = 250;
  constexpr uint32_t kFrameStepMs = 30;
  constexpr uint32_t kSettingsLoadingDurationMs = 2400;
  constexpr uint8_t kBlurRectCount = 3;

  struct BlurRect
  {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint32_t color;
  };

  struct BlurTrailState
  {
    bool initialized = false;
    BlurRect rects[kBlurRectCount] = {};
  };

  GUI ui;

  Button btnNext(2, Pullup);
  Button btnPrev(4, Pullup);

  ButtonVisualState settingsBtnState{true, 0, 255, true, false, 0};
  ToggleSwitchState g_toggleState{false, 0, 0};

  float g_graphPhase = 0.0f;
  uint32_t g_lastGraphUpdateMs = 0;
  uint8_t g_batteryLevel = 100;
  bool g_batteryDirDown = true;
  uint8_t g_progressValue = 0;
  bool g_progressDirDown = false;
  uint32_t g_lastProgressUpdateMs = 0;

  int16_t g_graphV1 = 0;
  int16_t g_graphV2 = 0;
  int16_t g_graphV3 = 0;

  uint32_t g_settingsLoadingUntil = 0;
  bool g_ditherCompareFrozen = false;
  uint8_t g_ditherCompareFrozenR = 0;
  uint8_t g_ditherCompareFrozenG = 0;
  uint8_t g_ditherCompareFrozenB = 0;

  constexpr uint8_t g_dotsCount = 15;
  uint8_t g_dotsActive = 0;
  uint8_t g_dotsPrev = 0;
  bool g_dotsAnimate = false;
  int8_t g_dotsDir = 0;
  uint32_t g_dotsAnimStartMs = 0;
  constexpr uint32_t g_dotsAnimDurMs = 240;

  const char *const g_drumOptionsH[] = {"Off", "5 min", "10 min", "30 min", "1 hr"};
  constexpr uint8_t g_drumCountH = 5;
  const char *const g_drumOptionsV[] = {"Small", "Medium", "Large"};
  constexpr uint8_t g_drumCountV = 3;

  uint8_t g_drumSelectedH = 0;
  uint8_t g_drumPrevH = 0;
  bool g_drumAnimateH = false;
  uint32_t g_drumAnimStartMs = 0;
  constexpr uint32_t g_drumAnimDurMs = 280;

  uint8_t g_drumSelectedV = 0;
  uint8_t g_drumPrevV = 0;
  bool g_drumAnimateV = false;

  float g_blurPhase = 0.0f;
  uint32_t g_blurLastUpdateMs = 0;
  BlurTrailState g_blurRow2Trail;

  uint32_t color565To888(uint16_t c)
  {
    const uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
    const uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
    const uint8_t b5 = (uint8_t)(c & 0x1F);
    const uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
    const uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
    const uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
    return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
  }

  String settingsButtonLabel(uint32_t nowMs)
  {
    if (!settingsBtnState.loading || g_settingsLoadingUntil == 0 || nowMs >= g_settingsLoadingUntil)
      return String("Show modal");

    const uint32_t remainingMs = g_settingsLoadingUntil - nowMs;
    char buf[20];
    snprintf(buf, sizeof(buf), "Saving %lu.%lus",
             (unsigned long)(remainingMs / 1000U),
             (unsigned long)((remainingMs % 1000U) / 100U));
    return String(buf);
  }

  void drawTextAt(const char *text, int16_t x, int16_t y, uint32_t fg, uint16_t bg, TextAlign align = AlignLeft)
  {
    ui.drawText()
        .text(text)
        .pos(x, y)
        .color(fg)
        .bgColor(bg)
        .align(align);
  }

  void drawTextAt(const String &text, int16_t x, int16_t y, uint32_t fg, uint16_t bg, TextAlign align = AlignLeft)
  {
    ui.drawText()
        .text(text)
        .pos(x, y)
        .color(fg)
        .bgColor(bg)
        .align(align);
  }

  void drawCenteredText(const char *text, int16_t y, uint32_t fg, uint16_t bg)
  {
    drawTextAt(text, -1, y, fg, bg, AlignCenter);
  }

  void drawCenteredText(const String &text, int16_t y, uint32_t fg, uint16_t bg)
  {
    drawTextAt(text, -1, y, fg, bg, AlignCenter);
  }

  void updateTextAt(const char *text, int16_t x, int16_t y, uint32_t fg, uint16_t bg, TextAlign align = AlignLeft)
  {
    ui.updateText()
        .text(text)
        .pos(x, y)
        .color(fg)
        .bgColor(bg)
        .align(align);
  }

  void updateTextAt(const String &text, int16_t x, int16_t y, uint32_t fg, uint16_t bg, TextAlign align = AlignLeft)
  {
    ui.updateText()
        .text(text)
        .pos(x, y)
        .color(fg)
        .bgColor(bg)
        .align(align);
  }

  void updateCenteredText(const char *text, int16_t y, uint32_t fg, uint16_t bg)
  {
    updateTextAt(text, -1, y, fg, bg, AlignCenter);
  }

  void updateCenteredText(const String &text, int16_t y, uint32_t fg, uint16_t bg)
  {
    updateTextAt(text, -1, y, fg, bg, AlignCenter);
  }

  void stepPingPong(uint8_t &value, bool &dirDown)
  {
    if (dirDown)
    {
      if (value > 0)
        --value;
      else
        dirDown = false;
    }
    else
    {
      if (value < 100)
        ++value;
      else
        dirDown = true;
    }
  }

  float animProgress(uint32_t now, uint32_t startMs, uint32_t durationMs)
  {
    if (durationMs == 0)
      return 1.0f;
    const uint32_t elapsed = now - startMs;
    return (elapsed >= durationMs) ? 1.0f : (float)elapsed / (float)durationMs;
  }

  uint32_t blurRectColor(uint8_t index)
  {
    switch (index)
    {
    case 0:
      return ui.rgb(255, 80, 80);
    case 1:
      return ui.rgb(80, 255, 120);
    default:
      return ui.rgb(80, 255, 140);
    }
  }

  void buildBlurRow(float phase, int16_t screenWidth, int16_t baseY, int16_t yStep, int16_t rectHeight, BlurRect *rects)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      const float tt = phase * (0.8f + 0.3f * i);
      const int16_t cx = (int16_t)(screenWidth / 2 + sinf(tt + i) * (float)(screenWidth / 2 - 30));
      const int16_t cy = (int16_t)(baseY + i * yStep + cosf(tt * 1.3f) * 8.0f);
      rects[i] = {
          (int16_t)(cx - (60 + i * 20) / 2),
          (int16_t)(cy - rectHeight / 2),
          (int16_t)(60 + i * 20),
          rectHeight,
          blurRectColor(i)};
    }
  }

  void drawBlurRow(GUI &ui, const BlurRect *rects)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      ui.fillRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .color(rects[i].color);
    }
  }

  void clearBlurRow(GUI &ui, const BlurRect *rects, uint16_t bg565)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      ui.fillRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .color(bg565);
    }
  }

  void runBootAnimation(GUI &ui, BootAnimation anim, uint32_t durationMs, const String &title, const String &subtitle)
  {
    ui.showLogo(title, subtitle, anim, 0xFFFF, 0x0000, durationMs);
    const uint32_t start = millis();
    while ((millis() - start) < durationMs)
    {
      ui.loop();
    }
  }
}

SCREEN(screenMain, 0)
{
  const uint16_t bg565 = ui.rgb(0, 0, 0);
  ui.clear(bg565);

  ui.setTextStyle(H1);
  drawCenteredText("Main menu", -1, ui.rgb(255, 255, 255), bg565);

  const int16_t ix = 12;
  const int16_t iy = 12;
  const uint16_t is = 48;
  const uint8_t level = g_batteryLevel;
  const uint16_t fill565 = (level <= 20) ? ui.rgb(255, 40, 40) : ui.rgb(80, 255, 120);

  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer2)
      .color(fill565)
      .bgColor(bg565);

  const int16_t cutX = (int16_t)(ix + (int16_t)((uint32_t)is * (uint32_t)level) / 100u);
  ui.fillRect()
      .pos(cutX, iy)
      .size((int16_t)(ix + (int16_t)is - cutX), (int16_t)is)
      .color(bg565);

  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer0)
      .color(0xFFFF)
      .bgColor(bg565);

  ui.drawIcon()
      .pos(ix, iy)
      .size(is)
      .icon(battery_layer1)
      .color(0xFFFF)
      .bgColor(bg565);

  ui.drawIcon()
      .pos(70, 12)
      .size(48)
      .icon(warning_layer0)
      .color(0xFF00)
      .bgColor(bg565);
}

SCREEN(screenSettings, 1)
{
  const uint16_t bg565 = ui.rgb(0, 31, 31);
  ui.clear(bg565);

  ui.setTextStyle(H1);
  drawCenteredText("Settings menu", 80, color565To888(0xFFFF), bg565);

  const String label = settingsButtonLabel(millis());

  ui.drawButton()
      .label(label)
      .pos(60, 20)
      .size(120, 44)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(10)
      .state(settingsBtnState);
}

SCREEN(screenGlowDemo, 2)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.drawGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg565)
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900);

  ui.drawGlowCircle()
      .pos(70, 175)
      .radius(22)
      .fillColor(ui.rgb(80, 255, 120))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(200);

  ui.drawGlowRect()
      .pos(center, 70)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400);

  ui.drawGlowRect()
      .pos(140, 175)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg565)
      .glowSize(16)
      .glowStrength(180);

  ui.setTextStyle(H2);
  drawCenteredText("Glow demo", 22, color565To888(0xFFFF), bg565);
  ui.setTextStyle(Body);
  drawCenteredText("REC / shapes", 44, ui.rgb(200, 200, 200), bg565);
}

SCREEN(screenBlurDemo, 17)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const uint16_t w = ui.screenWidth();
  const int16_t bandY = kStatusBarHeight;
  const int16_t bandH = 80;
  const float t = g_blurPhase;
  BlurRect row[kBlurRectCount];
  buildBlurRow(t, (int16_t)w, bandY + 15, 22, 18, row);
  drawBlurRow(ui, row);

  ui.drawBlur()
      .pos(0, bandY)
      .size((int16_t)w, bandH)
      .radius(10)
      .direction(TopDown)
      .gradient(false)
      .materialStrength(160)
      .noiseAmount(40)
      .materialColor(ui.rgb(10, 10, 10));

  buildBlurRow(t, (int16_t)w, bandY + bandH + 10, 18, 16, row);
  drawBlurRow(ui, row);

  ui.setTextStyle(H2);
  drawCenteredText("Blur strip", (int16_t)(bandY + bandH + 60), color565To888(0xFFFF), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("Next: change screen", (int16_t)(bandY + bandH + 80), ui.rgb(160, 160, 160), bg565);
  drawCenteredText("Prev: back / OK", (int16_t)(bandY + bandH + 96), ui.rgb(160, 160, 160), bg565);
}

SCREEN(screenGraph, 3)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 280, 170, 13, LeftToRight, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

SCREEN(screenGraphSmall, 4)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 160, 80, 10, LeftToRight, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(true);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

SCREEN(screenGraphTall, 5)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 160, 180, 10, RightToLeft, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(true);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

SCREEN(screenGraphOsc, 9)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 200, 170, 13, Oscilloscope, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(false);

  float tBase = g_graphPhase;
  float slow = sinf(tBase * 0.05f);
  float offset = slow * 40.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(tBase * 0.3f) * 20.0f;
  float t = tBase;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  ui.drawGraphLine(1, (int16_t)v2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, (int16_t)v3, ui.rgb(100, 160, 255), -110, 110);
}

SCREEN(screenProgressDemo, 6)
{
  ui.clear(0x000000);

  ui.drawProgressBar()
      .pos(center, 60)
      .size(200, 10)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .anim(Indeterminate);

  ui.drawProgressBar()
      .pos(center, 74)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .anim(Indeterminate);

  ui.drawProgressBar()
      .pos(center, 88)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .radius(6)
      .anim(None);

  ui.drawCircularProgressBar()
      .pos(50, 165)
      .radius(22)
      .thickness(8)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .anim(Indeterminate);

  ui.drawCircularProgressBar()
      .pos(105, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  ui.drawCircularProgressBar()
      .pos(160, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .anim(Shimmer);

  ui.drawCircularProgressBar()
      .pos(215, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 200, 120))
      .anim(Shimmer);
}

SCREEN(screenProgressTextDemo, 28)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Progress with text", 24, ui.rgb(220, 220, 220), bg565);

  const uint32_t base = ui.rgb(20, 20, 20);
  const uint32_t fill1 = ui.rgb(0, 122, 255);
  const uint32_t fill2 = ui.rgb(255, 59, 48);
  ui.drawProgressBar()
      .pos(center, 70)
      .size(200, 14)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill1)
      .radius(7)
      .anim(None);

  ui.drawProgressPercent(center, 70, 200, 14, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), base, 0);

  ui.drawProgressBar()
      .pos(center, 92)
      .size(200, 14)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill2)
      .radius(7)
      .anim(Shimmer);

  ui.drawProgressText(center, 92, 200, 14, String("Uploading"), AlignLeft, ui.rgb(255, 255, 255), base, 0);
  ui.drawProgressPercent(center, 92, 200, 14, g_progressValue, AlignRight, ui.rgb(200, 200, 200), 0x000000, 0);

  ui.drawCircularProgressBar()
      .pos(center, 160)
      .radius(30)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill1)
      .anim(None);

  ui.drawProgressPercent(
      (int16_t)(center - 30), 130, 60, 60, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), bg565, 0);
}

SCREEN(screenPrimitivesDemo, 23)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  const uint32_t w = ui.screenWidth();

  ui.setTextStyle(Caption);
  drawCenteredText("Primitives", 8, ui.rgb(220, 220, 220), bg565);

  ui.fillCircle()
      .pos(50, 55)
      .radius(18)
      .color(ui.rgb(0, 87, 250));

  ui.drawCircle()
      .pos(50, 55)
      .radius(18)
      .color(ui.rgb(255, 255, 255));

  ui.fillEllipse()
      .pos(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 0, 72));

  ui.drawEllipse()
      .pos(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 255, 255));

  ui.fillTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(0, 200, 120));

  ui.drawTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(255, 255, 255));

  ui.fillSquircle()
      .pos(70, 135)
      .radius(26)
      .color(ui.rgb(255, 128, 0));

  ui.drawSquircle()
      .pos(70, 135)
      .radius(26)
      .color(ui.rgb(255, 255, 255));

  ui.fillRect()
      .pos(210, 165)
      .size(80, 40)
      .color(bg565);

  ui.drawGlowRect()
      .pos(250, 185)
      .size(80, 40)
      .radius(12)
      .fillColor(ui.rgb(40, 120, 255))
      .bgColor(bg565)
      .glowSize(10)
      .glowStrength(180);

  ui.drawCircle()
      .pos(170, 135)
      .radius(28)
      .color(ui.rgb(60, 60, 60));

  ui.drawArc()
      .pos(170, 135)
      .radius(28)
      .startDeg(-90.0f)
      .endDeg(90.0f)
      .color(ui.rgb(80, 255, 120));

  ui.drawArc()
      .pos(170, 135)
      .radius(22)
      .startDeg(90.0f)
      .endDeg(270.0f)
      .color(ui.rgb(100, 160, 255));

  ui.drawLine()
      .from(12, 90)
      .to(118, 160)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(12, 160)
      .to(118, 90)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(12, 125)
      .to(118, 126)
      .color(ui.rgb(255, 255, 255));

  ui.drawArc()
      .pos(250, 135)
      .radius(14)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255));

  ui.drawArc()
      .pos(250, 135)
      .radius(10)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(80, 255, 120));

  ui.fillRect()
      .pos(10, 180)
      .size(80, 40)
      .color(ui.rgb(30, 30, 30))
      .radius({16, 4, 16, 4});

  ui.drawLine()
      .from(10, 225)
      .to((int16_t)(w - 10), 225)
      .color(ui.rgb(200, 200, 200));

  drawCenteredText("Next: change screen", 235, ui.rgb(160, 160, 160), bg565);
}

SCREEN(screenGradientsDemo, 25)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("Gradients / Alpha", 8, ui.rgb(220, 220, 220), bg565);

  char buf[64];
  uint32_t tStart, tElapsed;

  tStart = micros();
  ui.gradientVertical()
      .pos(10, 30)
      .size(140, 60)
      .topColor(ui.rgb(255, 0, 72))
      .bottomColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "V:%luus", tElapsed);
  drawTextAt(buf, 80, 65, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientHorizontal()
      .pos(170, 30)
      .size(140, 60)
      .leftColor(ui.rgb(255, 128, 0))
      .rightColor(ui.rgb(80, 255, 120));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "H:%luus", tElapsed);
  drawTextAt(buf, 240, 65, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientCorners()
      .pos(10, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 0, 72))
      .topRightColor(ui.rgb(0, 87, 250))
      .bottomLeftColor(ui.rgb(80, 255, 120))
      .bottomRightColor(ui.rgb(255, 128, 0));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "C4:%luus", tElapsed);
  drawTextAt(buf, 80, 135, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientDiagonal()
      .pos(170, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 255, 255))
      .bottomRightColor(ui.rgb(40, 40, 40));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "D:%luus", tElapsed);
  drawTextAt(buf, 240, 135, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientRadial()
      .pos(10, 170)
      .size(140, 60)
      .center(80, 200)
      .radius(48)
      .innerColor(ui.rgb(255, 255, 255))
      .outerColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "R:%luus", tElapsed);
  drawTextAt(buf, 80, 205, ui.rgb(255, 255, 255), bg565, AlignCenter);

  tStart = micros();
  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .topColor(ui.rgb(20, 20, 20))
      .bottomColor(ui.rgb(20, 20, 20));
  tElapsed = micros() - tStart;

  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .topColor(ui.rgb(20, 20, 20))
      .bottomColor(ui.rgb(60, 60, 60));
  snprintf(buf, sizeof(buf), "V2:%luus", micros() - tStart);
  drawTextAt(buf, 240, 205, ui.rgb(255, 255, 255), bg565, AlignCenter);
}

SCREEN(screenListMenuDemo, 7)
{
  ui.clear(0x0000);
  ui.updateList(screenListMenuDemo);
}

SCREEN(screenListMenuPlainDemo, 10)
{
  ui.clear(0x0000);
  ui.updateList(screenListMenuPlainDemo);
}

SCREEN(screenTileMenuDemo, 8)
{
  ui.renderTile(screenTileMenuDemo);
}

SCREEN(screenTileMenuLayoutDemo, 11)
{
  ui.renderTile(screenTileMenuLayoutDemo);
}

SCREEN(screenTileMenu4ColsDemo, 12)
{
  ui.renderTile(screenTileMenu4ColsDemo);
}

SCREEN(screenToggleSwitchDemo, 13)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .state(g_toggleState)
      .activeColor(ui.rgb(21, 180, 110));

  ui.setTextStyle(H2);
  drawCenteredText("ToggleSwitch", 24, ui.rgb(220, 220, 220), bg565);
}

SCREEN(screenScrollDotsDemo, 14)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  drawCenteredText("Scroll dots", 24, ui.rgb(220, 220, 220), bg565);
  drawCenteredText("15 dots (tapering)", 48, ui.rgb(120, 120, 120), bg565);
}

SCREEN(screenDrumRollDemo, 27)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  drawCenteredText("Drum roll", 24, ui.rgb(220, 220, 220), bg565);
  drawCenteredText("Next: H  Prev: V", 48, ui.rgb(120, 120, 120), bg565);

  uint32_t now = millis();
  if (g_drumAnimateH && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateH = false;
  if (g_drumAnimateV && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateV = false;

  const float progressH = g_drumAnimateH ? animProgress(now, g_drumAnimStartMs, g_drumAnimDurMs) : 1.0f;
  const float progressV = g_drumAnimateV ? animProgress(now, g_drumAnimStartMs, g_drumAnimDurMs) : 1.0f;

  String optsH[g_drumCountH] = {
      String(g_drumOptionsH[0]),
      String(g_drumOptionsH[1]),
      String(g_drumOptionsH[2]),
      String(g_drumOptionsH[3]),
      String(g_drumOptionsH[4]),
  };
  ui.drawDrumRollHorizontal(
      0,
      75,
      ui.screenWidth(),
      28,
      optsH,
      g_drumCountH,
      g_drumSelectedH,
      g_drumPrevH,
      progressH,
      ui.rgb(255, 255, 255),
      ui.rgb(8, 8, 8),
      18);

  String optsV[g_drumCountV] = {
      String(g_drumOptionsV[0]),
      String(g_drumOptionsV[1]),
      String(g_drumOptionsV[2]),
  };
  ui.drawDrumRollVertical(
      ui.screenWidth() - 80,
      120,
      70,
      90,
      optsV,
      g_drumCountV,
      g_drumSelectedV,
      g_drumPrevV,
      progressV,
      ui.rgb(200, 200, 200),
      ui.rgb(8, 8, 8),
      14);
}

SCREEN(screenErrorOverlayDemo, 15)
{
  ui.clear(ui.rgb(8, 8, 8));
  ui.showError("Error demo", "Something went wrong", Crash, "OK");
}

SCREEN(screenWarningOverlayDemo, 16)
{
  ui.clear(ui.rgb(8, 8, 8));
  ui.showError("Warning demo", "Check settings", Warning, "OK");
}

SCREEN(screenDitherDemo, 18)
{
  ui.clear(ui.rgb(8, 8, 8));
}

SCREEN(screenDitherBlueDemo, 19)
{
  ui.clear(ui.rgb(8, 8, 20));
}

SCREEN(screenDitherTemporalDemo, 20)
{
  ui.clear(ui.rgb(20, 8, 8));
}

SCREEN(screenDitherCubesDemo, 21)
{
  ui.clear(ui.rgb(8, 20, 8));
}

SCREEN(screenDitherCompareGradDemo, 22)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();
  const int16_t h = (int16_t)ui.screenHeight();
  int16_t cw = (int16_t)(w / 2 - 24);
  int16_t ch = (int16_t)(h - 100);

  if (cw < 32)
    cw = 32;
  if (ch < 32)
    ch = 32;

  const int16_t leftX = 12;
  const int16_t rightX = (int16_t)(w / 2 + 12);
  const int16_t topY = 44;
  const uint32_t now = millis();

  uint8_t r, g, b;
  if (g_ditherCompareFrozen)
  {
    r = g_ditherCompareFrozenR;
    g = g_ditherCompareFrozenG;
    b = g_ditherCompareFrozenB;
  }
  else
  {
    float t = (float)now / 1000.0f;
    r = (uint8_t)((sinf(t * 0.9f) * 0.5f + 0.5f) * 255.0f);
    g = (uint8_t)((sinf(t * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);
    b = (uint8_t)((sinf(t * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
  }

  ui.fillRect()
      .pos(leftX, topY)
      .size(cw, ch)
      .color(ui.rgb(r, g, b));

  ui.fillRect()
      .pos(rightX, topY)
      .size(cw, ch)
      .color(ui.rgb(r, g, b));

  const int16_t sampleX = (int16_t)(w - 64);
  const int16_t sampleY = (int16_t)(topY + 4);

  ui.fillRect()
      .pos(sampleX - 1, sampleY - 1)
      .size(50, 50)
      .color(ui.rgb(40, 40, 40));

  ui.fillRect()
      .pos(sampleX, sampleY)
      .size(48, 48)
      .color(ui.rgb(r, g, b));

  ui.setTextStyle(Caption);
  drawTextAt(g_ditherCompareFrozen ? "Frozen (PREV)" : "Live (PREV to freeze)",
             (int16_t)(sampleX + 24),
             (int16_t)(sampleY + 56),
             ui.rgb(200, 200, 200),
             bg565,
             AlignCenter);
  drawTextAt("16-bit: RGB565", leftX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg565, AlignCenter);
  drawTextAt("24-bit: FRC BlueNoise+gamma", rightX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg565, AlignCenter);

  char buf[64];
  snprintf(buf, sizeof(buf), "HEX: #%02X%02X%02X", r, g, b);
  drawCenteredText(buf, (int16_t)(topY + ch + 8), ui.rgb(160, 160, 160), bg565);
  drawCenteredText(
      "RGB565 steps per channel: R=32 G=64 B=32",
      (int16_t)(topY + ch + 26),
      ui.rgb(160, 160, 160),
      bg565);
  drawCenteredText(
      "24-bit per channel: 256 (visualized via FRC)",
      (int16_t)(topY + ch + 44),
      ui.rgb(160, 160, 160),
      bg565);

  const uint8_t r5 = (uint8_t)(r >> 3);
  const uint8_t g6 = (uint8_t)(g >> 2);
  const uint8_t b5 = (uint8_t)(b >> 3);
  snprintf(buf, sizeof(buf), "RGB565: R5=%02u G6=%02u B5=%02u", r5, g6, b5);
  drawCenteredText(buf, (int16_t)(topY + ch + 62), ui.rgb(160, 160, 160), bg565);
}

void updateDitherCompareDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (prevPressed)
  {
    g_ditherCompareFrozen = !g_ditherCompareFrozen;
    if (g_ditherCompareFrozen)
    {
      float tcap = (float)nowMs / 1000.0f;
      g_ditherCompareFrozenR = (uint8_t)((sinf(tcap * 0.9f) * 0.5f + 0.5f) * 255.0f);
      g_ditherCompareFrozenG = (uint8_t)((sinf(tcap * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);
      g_ditherCompareFrozenB = (uint8_t)((sinf(tcap * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
    }
    ui.requestRedraw();
  }

  if (nextPressed)
    ui.nextScreen();
}

void updateGraphDemo()
{
  float t = g_graphPhase;
  float slow = sinf(t * 0.05f);
  float offset = slow * 40.0f;
  float amp1 = 40.0f + slow * 20.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(t * 0.3f) * 20.0f;

  float v1 = offset + sinf(t) * amp1;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  g_graphV1 = (int16_t)v1;
  g_graphV2 = (int16_t)v2;
  g_graphV3 = (int16_t)v3;

  g_graphPhase += 0.12f;
  if (g_graphPhase > 1000.0f)
    g_graphPhase -= 1000.0f;
}

SCREEN(screenFontCompareDemo, 24)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();

  ui.setTextStyle(H2);
  drawTextAt("PSDF font demo", (int16_t)(w / 2), 20, ui.rgb(220, 220, 220), bg565, AlignCenter);

  const String sample = "The quick brown fox";
  const uint16_t sizes[4] = {18, 24, 36, 48};
  const int16_t y0 = 50;
  const int16_t spacing = 42;

  for (int i = 0; i < 4; ++i)
  {
    ui.setFontSize(sizes[i]);
    const int16_t ty = (int16_t)(y0 + i * spacing);
    drawTextAt(String("PSDF ") + String(sizes[i]) + "px", 8, ty, ui.rgb(255, 255, 255), bg565);
    drawTextAt(sample, 8, (int16_t)(ty + 18), ui.rgb(200, 200, 200), bg565);
  }
}

SCREEN(screenFontWeightDemo, 26)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();

  ui.setTextStyle(H2);
  drawTextAt("Font Weight Test", (int16_t)(w / 2), 10, ui.rgb(255, 255, 0), bg565, AlignCenter);

  const String sample = "The quick brown fox";
  const uint16_t weights[] = {100, 300, 400, 500, 600, 700, 900};
  const char *labels[] = {
      "Thin 100",
      "Light 300",
      "Regular 400",
      "Medium 500",
      "SemiBold 600",
      "Bold 700",
      "Black 900",
  };

  int16_t y = 35;
  const int16_t spacing = 38;

  for (int i = 0; i < 7; ++i)
  {
    ui.drawText()
        .text(labels[i])
        .pos(8, y)
        .size(12)
        .weight(Regular)
        .color(ui.rgb(150, 150, 150))
        .bgColor(bg565);

    ui.drawText()
        .text(sample)
        .pos(8, (int16_t)(y + 14))
        .size(20)
        .weight(weights[i])
        .color(ui.rgb(255, 255, 255))
        .bgColor(bg565);

    y += spacing;
  }
}

SCREEN(screenTestCircles, 30)
{
  const uint16_t bg565 = ui.rgb(20, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Circle Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fillCircle + drawCircle (AA)", 28, ui.rgb(180, 180, 200), bg565);
  drawTextAt("r=1-4", 25, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(25, 58).radius(1).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 58).radius(1).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 65).radius(2).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 65).radius(2).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 75).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 75).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 88).radius(4).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 88).radius(4).color(ui.rgb(255, 255, 255));
  drawTextAt("r=6-15", 80, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(65, 70).radius(6).color(ui.rgb(255, 0, 72));
  ui.drawCircle().pos(65, 70).radius(6).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(95, 70).radius(9).color(ui.rgb(80, 255, 120));
  ui.drawCircle().pos(95, 70).radius(9).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 105).radius(12).color(ui.rgb(255, 128, 0));
  ui.drawCircle().pos(80, 105).radius(12).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 145).radius(15).color(ui.rgb(180, 80, 255));
  ui.drawCircle().pos(80, 145).radius(15).color(ui.rgb(255, 255, 255));
  drawTextAt("r=18-35", 155, 45, ui.rgb(160, 160, 180), bg565, AlignCenter);

  ui.fillCircle().pos(145, 75).radius(18).color(ui.rgb(0, 200, 200));
  ui.drawCircle().pos(145, 75).radius(18).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(195, 75).radius(22).color(ui.rgb(200, 200, 80));
  ui.drawCircle().pos(195, 75).radius(22).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(145, 130).radius(25).color(ui.rgb(255, 100, 100));
  ui.drawCircle().pos(145, 130).radius(25).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(195, 135).radius(30).color(ui.rgb(100, 150, 255));
  ui.drawCircle().pos(195, 135).radius(30).color(ui.rgb(255, 255, 255));
  ui.fillCircle().pos(40, 185).radius(18).color(ui.rgb(0, 87, 250));
  ui.fillCircle().pos(60, 195).radius(18).color(ui.rgb(255, 0, 72));
  ui.fillCircle().pos(50, 175).radius(18).color(ui.rgb(80, 255, 120));
  ui.drawCircle().pos(110, 185).radius(15).color(ui.rgb(255, 255, 255));
  ui.drawCircle().pos(110, 185).radius(12).color(ui.rgb(200, 200, 200));
  ui.drawCircle().pos(110, 185).radius(8).color(ui.rgb(150, 150, 150));

  ui.drawCircle().pos(160, 195).radius(20).color(ui.rgb(255, 128, 0));
  ui.drawCircle().pos(210, 185).radius(25).color(ui.rgb(0, 200, 200));

  drawCenteredText("Check AA edges on overlaps!", 240, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestRoundRects, 31)
{
  const uint16_t bg565 = ui.rgb(20, 28, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("RoundRect Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fill/draw RoundRect (1 & 4 radius)", 28, ui.rgb(180, 200, 180), bg565);
  drawTextAt("Single radius:", 60, 45, ui.rgb(180, 220, 180), bg565, AlignCenter);

  ui.fillRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(0, 87, 250));
  ui.drawRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 0, 72));
  ui.drawRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(80, 255, 120));
  ui.drawRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 128, 0));
  ui.drawRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 255, 255));
  drawTextAt("4-corner radii:", 60, 110, ui.rgb(220, 180, 180), bg565, AlignCenter);
  ui.fillRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(180, 80, 255));
  ui.drawRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(255, 255, 255));
  ui.fillRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(0, 200, 200));
  ui.drawRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(255, 255, 255));
  ui.fillRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(200, 200, 80));
  ui.drawRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(255, 255, 255));
  drawTextAt("drawRoundRect only:", 70, 185, ui.rgb(200, 200, 200), bg565, AlignCenter);

  ui.drawRect().pos(20, 200).size(45, 35).radius({6}).color(ui.rgb(255, 255, 255));
  ui.drawRect().pos(75, 205).size(50, 40).radius({10}).color(ui.rgb(0, 255, 255));
  ui.drawRect().pos(135, 200).size(55, 45).radius({5, 15, 5, 15}).color(ui.rgb(255, 200, 0));
  ui.drawRect().pos(200, 205).size(50, 40).radius({15, 5, 15, 5}).color(ui.rgb(255, 100, 200));

  drawCenteredText("Check corner AA quality!", 260, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestEllipses, 32)
{
  const uint16_t bg565 = ui.rgb(28, 20, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Ellipse Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fillEllipse + drawEllipse (Wu-style AA)", 28, ui.rgb(200, 180, 180), bg565);
  drawTextAt("rx > ry (wide):", 70, 45, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(40, 70).radiusX(15).radiusY(8).color(ui.rgb(0, 87, 250));
  ui.drawEllipse().pos(40, 70).radiusX(15).radiusY(8).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(100, 70).radiusX(25).radiusY(10).color(ui.rgb(255, 0, 72));
  ui.drawEllipse().pos(100, 70).radiusX(25).radiusY(10).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(170, 70).radiusX(35).radiusY(12).color(ui.rgb(80, 255, 120));
  ui.drawEllipse().pos(170, 70).radiusX(35).radiusY(12).color(ui.rgb(255, 255, 255));

  drawTextAt("rx < ry (tall):", 70, 95, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(40, 120).radiusX(8).radiusY(15).color(ui.rgb(255, 128, 0));
  ui.drawEllipse().pos(40, 120).radiusX(8).radiusY(15).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(100, 120).radiusX(10).radiusY(25).color(ui.rgb(180, 80, 255));
  ui.drawEllipse().pos(100, 120).radiusX(10).radiusY(25).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(170, 120).radiusX(12).radiusY(35).color(ui.rgb(0, 200, 200));
  ui.drawEllipse().pos(170, 120).radiusX(12).radiusY(35).color(ui.rgb(255, 255, 255));
  drawTextAt("Near-circular:", 70, 165, ui.rgb(200, 180, 180), bg565, AlignCenter);

  ui.fillEllipse().pos(50, 190).radiusX(18).radiusY(16).color(ui.rgb(200, 200, 80));
  ui.drawEllipse().pos(50, 190).radiusX(18).radiusY(16).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(120, 190).radiusX(22).radiusY(20).color(ui.rgb(100, 255, 100));
  ui.drawEllipse().pos(120, 190).radiusX(22).radiusY(20).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(190, 190).radiusX(28).radiusY(25).color(ui.rgb(255, 100, 100));
  ui.drawEllipse().pos(190, 190).radiusX(28).radiusY(25).color(ui.rgb(255, 255, 255));
  ui.fillEllipse().pos(30, 235).radiusX(3).radiusY(5).color(ui.rgb(255, 200, 0));
  ui.drawEllipse().pos(30, 235).radiusX(3).radiusY(5).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(50, 235).radiusX(5).radiusY(3).color(ui.rgb(0, 255, 200));
  ui.drawEllipse().pos(50, 235).radiusX(5).radiusY(3).color(ui.rgb(255, 255, 255));

  ui.fillEllipse().pos(70, 235).radiusX(4).radiusY(4).color(ui.rgb(255, 100, 200));
  ui.drawEllipse().pos(70, 235).radiusX(4).radiusY(4).color(ui.rgb(255, 255, 255));

  drawCenteredText("Check Wu-style AA on edges!", 260, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestTriangles, 33)
{
  const uint16_t bg565 = ui.rgb(20, 24, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Triangle Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fillTriangle (4x subpixel AA)", 28, ui.rgb(180, 200, 220), bg565);
  drawTextAt("Small (5-15px):", 70, 45, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(255, 255, 255));
  drawTextAt("Medium (20-40px):", 80, 95, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(255, 255, 255));
  drawTextAt("Flat/thin (AA stress):", 90, 170, ui.rgb(220, 180, 180), bg565, AlignCenter);
  ui.fillTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(30, 235).point1(60, 275).point2(0, 275).radius(0).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 250).point1(75, 290).point2(15, 290).radius(0).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 250).point1(45, 290).point2(-15, 290).radius(0).color(ui.rgb(80, 255, 120));

  drawCenteredText("Check coverage-based AA edges!", 305, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestRoundTriangles, 36)
{
  const uint16_t bg565 = ui.rgb(24, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("RoundTriangle Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fill/draw RoundTriangle (SDF AA)", 28, ui.rgb(200, 180, 220), bg565);
  drawTextAt("Small (r=3-6):", 70, 45, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(255, 255, 255));
  drawTextAt("Medium (r=8-12):", 80, 95, ui.rgb(180, 200, 220), bg565, AlignCenter);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(255, 255, 255));
  drawTextAt("Various shapes (r=10):", 90, 170, ui.rgb(220, 180, 180), bg565, AlignCenter);
  ui.fillTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(30, 270).point1(60, 310).point2(0, 310).radius(8).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 280).point1(75, 320).point2(15, 320).radius(8).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 280).point1(45, 320).point2(-15, 320).radius(8).color(ui.rgb(80, 255, 120));

  drawCenteredText("Check SDF AA on rounded corners!", 335, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestSquircles, 37)
{
  const uint16_t bg565 = ui.rgb(18, 18, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Squircle Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fillSquircle + drawSquircle (AA + aaGamma)", 28, ui.rgb(200, 200, 220), bg565);

  ui.fillSquircle().pos(50, 80).radius(18).color(ui.rgb(0, 87, 250));
  ui.drawSquircle().pos(50, 80).radius(18).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 0, 72));
  ui.drawSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(200, 80).radius(34).color(ui.rgb(80, 255, 120));
  ui.drawSquircle().pos(200, 80).radius(34).color(ui.rgb(255, 255, 255));

  drawTextAt("Small r=6..10 (edge case):", 95, 128, ui.rgb(200, 180, 160), bg565, AlignCenter);
  ui.fillSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 128, 0));
  ui.drawSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(55, 150).radius(8).color(ui.rgb(180, 80, 255));
  ui.drawSquircle().pos(55, 150).radius(8).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(85, 150).radius(10).color(ui.rgb(0, 200, 200));
  ui.drawSquircle().pos(85, 150).radius(10).color(ui.rgb(255, 255, 255));

  drawTextAt("Overlay check (blending):", 170, 128, ui.rgb(200, 180, 160), bg565, AlignCenter);
  ui.fillSquircle().pos(160, 160).radius(24).color(ui.rgb(0, 87, 250));
  ui.fillSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 0, 72));
  ui.fillSquircle().pos(140, 170).radius(24).color(ui.rgb(80, 255, 120));

  ui.drawSquircle().pos(160, 160).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(140, 170).radius(24).color(ui.rgb(255, 255, 255));

  drawCenteredText("Check AA edges (no dark halos)", 305, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestArcsAndLines, 34)
{
  const uint16_t bg565 = ui.rgb(28, 24, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Arc & Line Test", 6, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("drawArc + drawLine (AA)", 28, ui.rgb(220, 200, 180), bg565);
  drawTextAt("Arcs (various angles):", 85, 45, ui.rgb(220, 200, 180), bg565, AlignCenter);
  ui.drawArc().pos(40, 75).radius(18).startDeg(-90).endDeg(0).color(ui.rgb(0, 87, 250));
  ui.drawArc().pos(80, 75).radius(18).startDeg(0).endDeg(90).color(ui.rgb(255, 0, 72));
  ui.drawArc().pos(120, 75).radius(18).startDeg(90).endDeg(180).color(ui.rgb(80, 255, 120));
  ui.drawArc().pos(160, 75).radius(18).startDeg(180).endDeg(270).color(ui.rgb(255, 128, 0));
  ui.drawArc().pos(70, 125).radius(22).startDeg(-90).endDeg(90).color(ui.rgb(180, 80, 255));
  ui.drawArc().pos(130, 125).radius(22).startDeg(90).endDeg(270).color(ui.rgb(0, 200, 200));
  ui.drawArc().pos(180, 125).radius(20).startDeg(0).endDeg(360).color(ui.rgb(200, 200, 80));
  ui.drawArc().pos(200, 75).radius(8).startDeg(45).endDeg(135).color(ui.rgb(255, 100, 100));
  ui.drawArc().pos(215, 75).radius(6).startDeg(200).endDeg(340).color(ui.rgb(100, 255, 200));
  drawTextAt("Lines (AA quality):", 80, 155, ui.rgb(220, 200, 180), bg565, AlignCenter);
  ui.drawLine().from(20, 175).to(80, 175).color(ui.rgb(255, 255, 255));
  ui.drawLine().from(50, 165).to(50, 185).color(ui.rgb(200, 200, 200));
  ui.drawLine().from(100, 170).to(140, 210).color(ui.rgb(255, 100, 100));
  ui.drawLine().from(140, 170).to(100, 210).color(ui.rgb(100, 255, 100));
  ui.drawLine().from(160, 170).to(165, 220).color(ui.rgb(100, 100, 255));
  ui.drawLine().from(180, 170).to(200, 190).color(ui.rgb(255, 200, 100));
  ui.drawLine().from(20, 230).to(100, 280).color(ui.rgb(0, 200, 255));
  ui.drawLine().from(140, 240).to(220, 300).color(ui.rgb(255, 255, 255));
  drawTextAt("Pie arcs:", 200, 155, ui.rgb(220, 200, 180), bg565, AlignCenter);
  ui.drawArc().pos(200, 200).radius(25).startDeg(-30).endDeg(30).color(ui.rgb(255, 128, 0));
  ui.drawArc().pos(200, 200).radius(20).startDeg(60).endDeg(120).color(ui.rgb(0, 255, 128));
  ui.drawArc().pos(200, 200).radius(15).startDeg(150).endDeg(210).color(ui.rgb(255, 0, 255));

  drawCenteredText("Check sqrt_fraction AA!", 305, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenTestAllPrimitivesGrid, 35)
{
  const uint16_t bg565 = ui.rgb(16, 16, 16);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("All Primitives Grid", 4, ui.rgb(255, 255, 255), bg565);
  const int16_t cols = 4;
  const int16_t rows = 5;
  const int16_t cellW = 60;
  const int16_t cellH = 58;
  const int16_t startX = 0;
  const int16_t startY = 28;

  const uint16_t colors[] = {
      ui.rgb(0, 87, 250),
      ui.rgb(255, 0, 72),
      ui.rgb(80, 255, 120),
      ui.rgb(255, 128, 0),
      ui.rgb(180, 80, 255),
      ui.rgb(0, 200, 200),
      ui.rgb(200, 200, 80),
      ui.rgb(255, 100, 100),
  };

  int colorIdx = 0;

  for (int row = 0; row < rows; row++)
  {
    for (int col = 0; col < cols; col++)
    {
      int16_t x = startX + col * cellW + cellW / 2;
      int16_t y = startY + row * cellH + cellH / 2;
      const uint16_t c = colors[colorIdx % 8];
      const uint16_t white = ui.rgb(255, 255, 255);
      colorIdx++;

      const int type = (row * cols + col) % 8;

      if (type == 0)
      {
        ui.fillCircle().pos(x, y).radius(14).color(c);
        ui.drawCircle().pos(x, y).radius(14).color(white);
      }
      else if (type == 1)
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(white);
      }
      else if (type == 2)
      {
        ui.fillEllipse().pos(x, y).radiusX(16).radiusY(10).color(c);
        ui.drawEllipse().pos(x, y).radiusX(16).radiusY(10).color(white);
      }
      else if (type == 3)
      {
        ui.fillTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(c);
        ui.drawTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(white);
      }
      else if (type == 4)
      {
        ui.drawArc().pos(x, y).radius(14).startDeg(0).endDeg(270).color(c);
      }
      else if (type == 5)
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(white);
      }
      else if (type == 6)
      {
        ui.fillEllipse().pos(x, y).radiusX(8).radiusY(16).color(c);
        ui.drawEllipse().pos(x, y).radiusX(8).radiusY(16).color(white);
      }
      else if (type == 7)
      {
        ui.fillTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(c);
        ui.drawTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(white);
      }
    }
  }

  drawCenteredText("All primitives with AA comparison", 318, ui.rgb(200, 200, 100), bg565);
}

SCREEN(screenCircleDemo, 29)
{
  const uint16_t bg565 = ui.rgb(16, 16, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Circle / RoundRect", 8, ui.rgb(255, 255, 255), bg565);
  ui.setTextStyle(Caption);
  drawCenteredText("fillCircle + fillRoundRect", 28, ui.rgb(160, 160, 200), bg565);

  ui.fillCircle()
      .pos(48, 78)
      .radius(22)
      .color(ui.rgb(0, 87, 250));

  ui.drawCircle()
      .pos(48, 78)
      .radius(22)
      .color(ui.rgb(255, 255, 255));

  ui.fillRect()
      .pos(88, 52)
      .size(140, 44)
      .color(ui.rgb(80, 255, 120))
      .radius({12});

  ui.fillRect()
      .pos(88, 104)
      .size(140, 44)
      .color(ui.rgb(255, 128, 0))
      .radius({20, 6, 20, 6});

  ui.drawRect()
      .pos(12, 160)
      .size(100, 44)
      .color(ui.rgb(255, 255, 255))
      .radius({12});

  ui.drawRect()
      .pos(12, 214)
      .size(100, 44)
      .color(ui.rgb(255, 255, 255))
      .radius({20, 6, 20, 6});

  ui.fillEllipse()
      .pos(175, 190)
      .radiusX(46)
      .radiusY(20)
      .color(ui.rgb(255, 0, 72));

  ui.drawArc()
      .pos(175, 250)
      .radius(24)
      .startDeg(-90.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(120, 270)
      .to(230, 300)
      .color(ui.rgb(255, 255, 255));

  ui.setTextStyle(Caption);
  drawTextAt("fillCircle", 48, 108, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("fillRoundRect r=12", 158, 46, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("fillRoundRect {20,6,20,6}", 158, 98, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("drawRoundRect r=12", 62, 154, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("drawRoundRect {20,6,20,6}", 62, 208, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("fillEllipse", 175, 216, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("drawArc", 175, 276, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawTextAt("drawLine", 175, 304, ui.rgb(180, 180, 200), bg565, AlignCenter);
  drawCenteredText("Next/Prev: change screen", 235, ui.rgb(120, 120, 140), bg565);
}

SCREEN(screenAutoTextColorDemo, 38)
{
  const uint16_t bg565 = ui.rgb(32, 32, 32);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  drawCenteredText("Auto Text Color Test", 4, ui.rgb(255, 255, 255), bg565);
  const uint16_t testColors[] = {
      ui.rgb(255, 255, 255),
      ui.rgb(200, 200, 200),
      ui.rgb(128, 128, 128),
      ui.rgb(64, 64, 64),
      ui.rgb(0, 0, 0),
      ui.rgb(255, 0, 0),
      ui.rgb(0, 255, 0),
      ui.rgb(0, 0, 255),
      ui.rgb(255, 255, 0),
      ui.rgb(0, 255, 255),
      ui.rgb(255, 0, 255),
      ui.rgb(255, 128, 0),
  };

  const char *labels[] = {
      "White", "Light", "Mid", "Dark", "Black",
      "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "Orange"};

  const int cols = 3;
  const int16_t cellW = 76;
  const int16_t cellH = 70;
  const int16_t startX = 6;
  const int16_t startY = 32;

  for (int i = 0; i < 12; i++)
  {
    const int col = i % cols;
    const int row = i / cols;
    const int16_t x = startX + col * cellW;
    const int16_t y = startY + row * cellH;

    const uint16_t bg = testColors[i];
    const uint16_t fg = detail::autoTextColor(bg, 128);
    ui.fillRect().pos(x, y).size(cellW - 4, cellH - 4).color(bg);
    ui.setTextStyle(Body);
    drawTextAt(labels[i], x + (cellW - 4) / 2, y + (cellH - 4) / 2 - 6, fg, bg, AlignCenter);
  }

  ui.setTextStyle(Caption);
  drawCenteredText("Text color auto-selected based on bg", 310, ui.rgb(180, 180, 180), bg565);
}

void updateClockDisplay(uint32_t nowMs)
{
  static uint32_t lastMinute = 0xFFFFFFFF;
  const uint32_t minute = (nowMs / 1000U) / 60U;
  if (minute == lastMinute)
    return;

  lastMinute = minute;
  const uint8_t m = minute % 60U;
  const uint8_t h = (minute / 60U) % 24U;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)h, (unsigned)m);
  ui.setStatusBarText("pipGUI", String(buf), "");
  if (!ui.notificationActive())
    ui.updateStatusBar();
}

void maybeShowToast(uint32_t nowMs)
{
  static uint32_t lastToastMs = 0;
  if (ui.toastActive() || ui.notificationActive() || nowMs - lastToastMs < kToastIntervalMs)
    return;

  lastToastMs = nowMs;
  static const char *const toastMessages[] = {
      "Settings saved",
      "Connected",
      "Sync complete",
      "Ready",
      "pipGUI",
  };
  constexpr uint8_t toastCount = sizeof(toastMessages) / sizeof(toastMessages[0]);
  const uint8_t idx = (uint8_t)((nowMs / kToastIntervalMs) % toastCount);
  const bool fromTop = (idx & 1u) != 0;

  ui.showToast()
      .text(String(toastMessages[idx]))
      .duration(2500)
      .fromTop(fromTop)
      .icon(fromTop ? warning_layer0 : error_layer0, 20)
      .show();
}

void updateBatteryStatus(uint32_t nowMs)
{
  static uint32_t lastBatteryMs = 0;
  if (nowMs - lastBatteryMs < kBatteryStepMs)
    return;

  lastBatteryMs = nowMs;
  stepPingPong(g_batteryLevel, g_batteryDirDown);
  ui.setStatusBarBattery(g_batteryLevel, Bar);
  if (!ui.notificationActive())
    ui.updateStatusBar();
}

bool isListMenuScreen(uint8_t screenId)
{
  return screenId == screenListMenuDemo || screenId == screenListMenuPlainDemo;
}

bool isTileMenuScreen(uint8_t screenId)
{
  return screenId == screenTileMenuDemo || screenId == screenTileMenuLayoutDemo || screenId == screenTileMenu4ColsDemo;
}

void configureListMenus()
{
  ui.configureList()
      .screen(screenListMenuDemo)
      .items({
          {warning_layer0, "Main screen with widgets", "Simple home screen with status bar, overlays, and transitions", screenMain},
          {error_layer0, "Settings and alerts preview", "Buttons, notifications, warning states, and input handling", screenSettings},
          {warning_layer0, "Glow demo with blur and bloom", "Bloom, soft edges, layered primitives, and bright composition", screenGlowDemo},
          {error_layer0, "Graph demo with multiple traces", "Grid, labels, and several moving lines in one viewport", screenGraph},
          {error_layer0, "Graph small", "Compact graph with automatic scale fitting", screenGraphSmall},
          {warning_layer0, "Graph tall", "Tall graph layout with right-side emphasis", screenGraphTall},
          {warning_layer0, "Graph osc", "Digital oscilloscope style waveform preview", screenGraphOsc},
          {error_layer0, "Progress demo", "Progress bars with animated fill and labels", screenProgressDemo},
          {warning_layer0, "Progress + text", "Fresh progress styles with text overlays", screenProgressTextDemo},
          {error_layer0, "Tile menu with icon cards", "Grid menu with icons, title, subtitle, and active marquee", screenTileMenuDemo},
          {warning_layer0, "Tile layout", "Custom tile layout with merged cells", screenTileMenuLayoutDemo},
          {error_layer0, "Tile 4 cols", "Four compact tiles in a single row", screenTileMenu4ColsDemo},
          {warning_layer0, "List menu 2", "Text-focused list with active highlight panel", screenListMenuPlainDemo},
          {warning_layer0, "ToggleSwitch", "Toggle switch interaction and state preview", screenToggleSwitchDemo},
          {error_layer0, "Scroll dots", "Page indicator dots with animated transitions", screenScrollDotsDemo},
          {error_layer0, "Error", "Full-screen error overlay presentation", screenErrorOverlayDemo},
          {warning_layer0, "Warning", "Full-screen warning overlay presentation", screenWarningOverlayDemo},
          {error_layer0, "Blur strip", "Material-style blur strip with moving content", screenBlurDemo},
          {warning_layer0, "Dither demo", "Gamma + BlueNoise", screenDitherDemo},
          {error_layer0, "Dither Blue", "BlueNoise only", screenDitherBlueDemo},
          {warning_layer0, "Dither Temporal", "Temporal FRC", screenDitherTemporalDemo},
          {error_layer0, "Dither Cubes", "Two cubes + avg (16/24)", screenDitherCubesDemo},
          {warning_layer0, "Dither Compare", "Compare 16-bit vs 24-bit", screenDitherCompareGradDemo},
          {error_layer0, "Primitives", "Circle / Arc / Ellipse / Triangle / Squircle", screenPrimitivesDemo},
          {warning_layer0, "Gradients", "All gradient types", screenGradientsDemo},
          {error_layer0, "Font compare", "PSDF", screenFontCompareDemo},
          {warning_layer0, "Font weights", "Test all weights", screenFontWeightDemo},
          {error_layer0, "Drum roll", "Horizontal + vertical picker", screenDrumRollDemo},
          {warning_layer0, "Circle AA", "Gupta-Sproull optimized", screenCircleDemo},
          {error_layer0, "Test: Circles", "fillCircle + drawCircle", screenTestCircles},
          {warning_layer0, "Test: RoundRects", "1 & 4 radius variants", screenTestRoundRects},
          {error_layer0, "Test: Ellipses", "Wu-style AA ellipses", screenTestEllipses},
          {warning_layer0, "Test: Triangles", "4x subpixel AA triangles", screenTestTriangles},
          {error_layer0, "Test: Arcs+Lines", "sqrt_fraction AA", screenTestArcsAndLines},
          {warning_layer0, "Test: All Grid", "All primitives comparison", screenTestAllPrimitivesGrid},
          {error_layer0, "Test: RoundTriangles", "SDF AA rounded triangles", screenTestRoundTriangles},
          {warning_layer0, "Test: Squircles", "fill/draw squircle AA", screenTestSquircles},
          {error_layer0, "Auto text color", "BT.709 luminance test", screenAutoTextColorDemo},
      })
      .parent(screenMain)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .cardSize(310, 50);

  ui.configureList()
      .screen(screenListMenuPlainDemo)
      .items({
          {warning_layer0, "Back", "Return to the card-style list menu", screenListMenuDemo},
          {error_layer0, "Main screen with widgets and overlays", "Simple home screen with status bar, overlays, and transitions", screenMain},
          {warning_layer0, "Settings, alerts, and input handling", "Buttons, notifications, warning states, and input handling", screenSettings},
          {warning_layer0, "Glow demo with blur, bloom, and soft shapes", "Bloom, soft edges, layered primitives, and bright composition", screenGlowDemo},
          {error_layer0, "Graph osc", "Digital oscilloscope style waveform preview", screenGraphOsc},
          {error_layer0, "Tile menu", "Grid menu with icons, title, and subtitle", screenTileMenuDemo},
          {warning_layer0, "ToggleSwitch", "Toggle switch interaction and state preview", screenToggleSwitchDemo},
          {error_layer0, "Dither demo", "Gamma + BlueNoise", screenDitherDemo},
          {warning_layer0, "Dither Cubes", "Two cubes + avg (16/24)", screenDitherCubesDemo},
          {error_layer0, "Dither Compare", "Compare 16-bit vs 24-bit", screenDitherCompareGradDemo},
          {warning_layer0, "Font compare", "PSDF", screenFontCompareDemo},
      })
      .parent(screenListMenuDemo)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(12)
      .cardSize(310, 34)
      .mode(Plain);
}
void configureTileMenus()
{
  ui.configureTile()
      .screen(screenTileMenuDemo)
      .items({
          {warning_layer0, "Main", "Home screen", screenMain},
          {error_layer0, "Settings", "Alerts and controls", screenSettings},
          {battery_layer0, "Glow demo", "Glow shapes", screenGlowDemo},
          {battery_layer1, "Graph", "Live graphs", screenGraph},
          {battery_layer2, "Toggle", "Switch", screenToggleSwitchDemo},
      })
      .parent(screenMain)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .columns(2)
      .tileSize(100, 70)
      .mode(TextSubtitle);
  ui.configureTile()
      .screen(screenTileMenuLayoutDemo)
      .items({
          {warning_layer0, "Back", "", screenListMenuDemo},
          {battery_layer0, "Small 1", "", screenMain},
          {battery_layer1, "Small 2", "", screenSettings},
          {error_layer0, "Big", "Main", screenGlowDemo},
      })
      .parent(screenListMenuDemo)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .columns(2)
      .layout({
          "AA",
          "BC",
          "BD",
      })
      .mode(TextOnly);
  ui.configureTile()
      .screen(screenTileMenu4ColsDemo)
      .items({
          {warning_layer0, "Back", "", screenListMenuDemo},
          {battery_layer0, "1", "", screenMain},
          {battery_layer1, "2", "", screenSettings},
          {battery_layer2, "3", "", screenGlowDemo},
          {error_layer0, "4", "", screenGraph},
          {warning_layer0, "5", "", screenGraphSmall},
          {battery_layer0, "6", "", screenGraphTall},
          {battery_layer1, "7", "", screenProgressDemo},
          {error_layer0, "8", "", screenGraphOsc},
      })
      .parent(screenListMenuDemo)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(8)
      .columns(4)
      .mode(TextOnly);
}
void updateGlowDemoFrame(uint32_t nowMs)
{
  static uint32_t lastGlowRedrawMs = 0;
  if (nowMs - lastGlowRedrawMs < kFrameStepMs)
    return;

  lastGlowRedrawMs = nowMs;
  const uint16_t bg565 = ui.rgb(10, 10, 10);

  ui.updateGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg565)
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900);

  ui.updateGlowRect()
      .pos(center, 70)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400);

  ui.updateGlowRect()
      .pos(140, 175)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg565)
      .glowSize(16)
      .glowStrength(180);
}

void updateBlurDemoFrame(uint32_t nowMs)
{
  if (nowMs - g_blurLastUpdateMs < 1)
    return;

  g_blurLastUpdateMs = nowMs;
  g_blurPhase += 0.10f;
  if (g_blurPhase > 1000.0f)
    g_blurPhase -= 1000.0f;

  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const int16_t w = (int16_t)ui.screenWidth();
  const int16_t bandY = kStatusBarHeight;
  const int16_t bandH = 80;
  BlurRect row1[kBlurRectCount];
  BlurRect row2[kBlurRectCount];

  ui.fillRect()
      .pos(0, bandY)
      .size(w, bandH)
      .color(bg565);

  buildBlurRow(g_blurPhase, w, bandY + 15, 22, 18, row1);
  drawBlurRow(ui, row1);

  if (g_blurRow2Trail.initialized)
    clearBlurRow(ui, g_blurRow2Trail.rects, bg565);

  buildBlurRow(g_blurPhase, w, bandY + bandH + 10, 18, 16, row2);
  drawBlurRow(ui, row2);
  for (uint8_t i = 0; i < kBlurRectCount; ++i)
    g_blurRow2Trail.rects[i] = row2[i];
  g_blurRow2Trail.initialized = true;

  ui.updateBlur()
      .pos(0, bandY)
      .size(w, bandH)
      .radius(10)
      .direction(TopDown)
      .gradient(false)
      .materialStrength(160)
      .noiseAmount(40)
      .materialColor(ui.rgb(10, 10, 10));
}

void updateGraphScreen(uint8_t screenId, uint32_t nowMs)
{
  if (nowMs - g_lastGraphUpdateMs < kFrameStepMs)
    return;

  g_lastGraphUpdateMs = nowMs;
  updateGraphDemo();

  if (screenId != screenGraphOsc)
    ui.updateGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);

  ui.updateGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.updateGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void updateProgressDemoFrame(uint32_t nowMs)
{
  if (nowMs - g_lastProgressUpdateMs < kFrameStepMs)
    return;

  g_lastProgressUpdateMs = nowMs;
  stepPingPong(g_progressValue, g_progressDirDown);

  ui.updateProgressBar()
      .pos(center, 60)
      .size(200, 10)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .anim(Indeterminate)
      .doFlush(false);

  ui.updateProgressBar()
      .pos(center, 74)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .radius(6)
      .anim(None)
      .doFlush(false);

  ui.updateProgressBar()
      .pos(center, 88)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .radius(6)
      .anim(Shimmer)
      .doFlush(false);

  ui.updateCircularProgressBar()
      .pos(50, 165)
      .radius(22)
      .thickness(8)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .anim(Indeterminate)
      .doFlush(false);

  ui.updateCircularProgressBar()
      .pos(105, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None)
      .doFlush(false);

  ui.updateCircularProgressBar()
      .pos(160, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .anim(Shimmer)
      .doFlush(false);

  ui.updateCircularProgressBar()
      .pos(215, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 200, 120))
      .anim(Shimmer)
      .doFlush(true);
}

void updateSettingsDemoFrame(uint32_t nowMs, bool prevPressed, bool prevDown)
{
  ui.updateButtonPress(settingsBtnState, prevDown);

  if (settingsBtnState.loading && g_settingsLoadingUntil != 0 && nowMs >= g_settingsLoadingUntil)
  {
    settingsBtnState.loading = false;
    g_settingsLoadingUntil = 0;
  }

  if (prevPressed && !settingsBtnState.loading)
  {
    settingsBtnState.loading = true;
    g_settingsLoadingUntil = nowMs + kSettingsLoadingDurationMs;
    ui.showNotification()
        .title("Sync paused")
        .message("Cloud sync is paused.\nConfirm changes to continue.")
        .button("OK")
        .delay(2)
        .type(Error);
  }

  const String label = settingsButtonLabel(nowMs);
  ui.updateButton()
      .label(label)
      .pos(60, 20)
      .size(120, 44)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(10)
      .state(settingsBtnState);
}

void updateToggleDemo(bool nextPressed, bool prevPressed)
{
  const bool prevVal = g_toggleState.value;
  const bool changed = ui.updateToggleSwitch(g_toggleState, nextPressed);

  if (prevPressed)
    ui.setScreen(screenListMenuDemo);

  if (!changed)
    return;

  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const uint16_t active = ui.rgb(21, 180, 110);

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .state(g_toggleState)
      .activeColor(active);

  if (prevVal != g_toggleState.value)
  {
    ui.setTextStyle(Body);
    updateCenteredText(g_toggleState.value ? "ON" : "OFF", 105, ui.rgb(200, 200, 200), bg565);
  }
}

void updateScrollDotsDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (nextPressed || prevPressed)
  {
    const uint8_t prev = g_dotsActive;
    if (nextPressed)
    {
      g_dotsActive = (uint8_t)((g_dotsActive + 1) % g_dotsCount);
      g_dotsDir = 1;
    }
    else
    {
      g_dotsActive = (uint8_t)((g_dotsActive + g_dotsCount - 1) % g_dotsCount);
      g_dotsDir = -1;
    }
    g_dotsPrev = prev;
    g_dotsAnimate = true;
    g_dotsAnimStartMs = nowMs;
  }

  if (!g_dotsAnimate)
    return;

  if (nowMs - g_dotsAnimStartMs >= g_dotsAnimDurMs)
    g_dotsAnimate = false;

  const float p = animProgress(nowMs, g_dotsAnimStartMs, g_dotsAnimDurMs);

  ui.updateScrollDots()
      .pos(center, 150)
      .count(g_dotsCount)
      .activeIndex(g_dotsActive)
      .prevIndex(g_dotsPrev)
      .animProgress(p)
      .animate(g_dotsAnimate)
      .animDirection(g_dotsDir)
      .activeColor(ui.rgb(0, 87, 250))
      .inactiveColor(ui.rgb(60, 60, 60))
      .dotRadius(3)
      .spacing(14)
      .activeWidth(18);
}

void updateDrumRollDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (nextPressed)
  {
    g_drumPrevH = g_drumSelectedH;
    g_drumSelectedH = (uint8_t)((g_drumSelectedH + 1) % g_drumCountH);
    g_drumAnimateH = true;
    g_drumAnimStartMs = nowMs;
  }
  else if (prevPressed)
  {
    g_drumPrevH = g_drumSelectedH;
    g_drumSelectedH = (uint8_t)((g_drumSelectedH + g_drumCountH - 1) % g_drumCountH);
    g_drumAnimateH = true;
    g_drumAnimStartMs = nowMs;
  }

  if (prevPressed)
  {
    g_drumPrevV = g_drumSelectedV;
    g_drumSelectedV = (uint8_t)((g_drumSelectedV + 1) % g_drumCountV);
    g_drumAnimateV = true;
    g_drumAnimStartMs = nowMs;
  }

  ui.requestRedraw();
}

void handleErrorDemo(uint8_t screenId, bool nextPressed, bool prevPressed)
{
  if (ui.errorActive())
    return;

  if (prevPressed)
  {
    ui.setScreen(screenListMenuDemo);
    return;
  }

  if (!nextPressed)
    return;

  if (screenId == screenErrorOverlayDemo)
  {
    ui.showError("Failed to create screen sprite", "Code: 0xFAISPRT", Crash, "OK");
    ui.showError("LittleFS mount failed!", "Code: 0xLFS_MNT", Crash, "OK");
    ui.showError("Sprite creation failed!", "Code: 0xSPR_MEM", Crash, "OK");
    return;
  }

  ui.showError("No profile found!", "Code:0xNOPROF", Warning, "OK");
  ui.showError("Network is unstable", "Code:0xNETWARN", Warning, "OK");
  ui.showError("Low battery warning", "Code:0xLOWBATT", Warning, "OK");
}

void setup()
{
  Serial.begin(115200);
  SPIFFS.begin(true);

  ui.configureDisplay()
      .pins({11, 12, 10, 9, 14})
      .hz(80000000)
      .size(240, 320)
      .apply();

  ui.begin(3, 0);
  ui.setScreenAnimation(SlideX, 320);
  ui.configureStatusBar(false, 0x0000, kStatusBarHeight, Top);
  ui.setStatusBarStyle(StatusBarStyleBlurGradient);
  ui.setStatusBarText("pipGUI", "00:00", "");
  ui.setStatusBarBattery(100, Bar);
  ui.setBacklightPin(4);

  btnNext.begin();
  btnPrev.begin();

  ui.configureTextStyles(24, 18, 14, 12);
  ui.logoSizesPx(36, 24);

  runBootAnimation(ui, FadeIn, 900, "PISPPUS", "Fade in");
  configureListMenus();
  configureTileMenus();

  ui.setScreen(screenListMenuDemo);
}

void loop()
{
  btnNext.update();
  btnPrev.update();

  const uint32_t nowMs = millis();
  const bool nextPressed = btnNext.wasPressed();
  const bool prevPressed = btnPrev.wasPressed();
  const bool nextDown = btnNext.isDown();
  const bool prevDown = btnPrev.isDown();

  if (ui.screenTransitionActive())
  {
    ui.loop();
    return;
  }

  updateClockDisplay(nowMs);
  maybeShowToast(nowMs);
  updateBatteryStatus(nowMs);

  const uint8_t cur = ui.currentScreen();
  const bool notificationActive = ui.notificationActive();

  if (!notificationActive)
  {
    if (isListMenuScreen(cur))
    {
      ui.listInput(cur)
          .nextDown(nextDown)
          .prevDown(prevDown);
    }
    else if (isTileMenuScreen(cur))
    {
      ui.tileInput(cur)
          .nextDown(nextDown)
          .prevDown(prevDown);
    }
    else
    {
      switch (cur)
      {
      case screenGlowDemo:
        updateGlowDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case screenBlurDemo:
        updateBlurDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case screenGraph:
      case screenGraphSmall:
      case screenGraphTall:
      case screenGraphOsc:
        updateGraphScreen(cur, nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case screenProgressDemo:
        updateProgressDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case screenSettings:
        if (nextPressed)
          ui.nextScreen();
        else
          updateSettingsDemoFrame(nowMs, prevPressed, prevDown);
        break;
      case screenToggleSwitchDemo:
        updateToggleDemo(nextPressed, prevPressed);
        break;
      case screenScrollDotsDemo:
        updateScrollDotsDemo(nowMs, nextPressed, prevPressed);
        break;
      case screenDrumRollDemo:
        updateDrumRollDemo(nowMs, nextPressed, prevPressed);
        break;
      case screenErrorOverlayDemo:
      case screenWarningOverlayDemo:
        handleErrorDemo(cur, nextPressed, prevPressed);
        break;
      case screenDitherCompareGradDemo:
        updateDitherCompareDemo(nowMs, nextPressed, prevPressed);
        break;
      default:
        if (nextPressed)
          ui.nextScreen();
        break;
      }
    }
  }

  if (ui.errorActive())
  {
    ui.setErrorButtonDown(prevDown);
  }
  else if (ui.notificationActive())
  {
    ui.setNotificationButtonDown(prevDown);
  }

  ui.loop();
}

#include <Arduino.h>
#include <SPIFFS.h>
#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/icons/metrics.hpp>
#include <pipCore/Platforms/ESP32/GUI.hpp>
#include <pipCore/Displays/ST7789/Display.hpp>
using namespace pipgui;

GUI ui;

static pipcore::ESP32Platform g_platform;

Button btnNext(2);
Button btnPrev(4);

static inline uint32_t rgb565(uint16_t c)
{
  uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
  uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
  uint8_t b5 = (uint8_t)(c & 0x1F);
  uint8_t r8 = (uint8_t)((r5 * 255U) / 31U);
  uint8_t g8 = (uint8_t)((g6 * 255U) / 63U);
  uint8_t b8 = (uint8_t)((b5 * 255U) / 31U);
  return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
}

ButtonVisualState settingsBtnState{true, 0, 255, true, false, 0};

static const uint16_t ICON_DOT_WHITE[4] = {
    0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF};

static const uint16_t ICON_DOT_GRAY[4] = {

    0x8410, 0x8410,

    0x8410, 0x8410};

float g_graphPhase = 0.0f;

uint32_t g_lastGraphUpdateMs = 0;

uint32_t g_lastClockSecond = 0;

uint8_t g_batteryLevel = 100;

bool g_batteryDirDown = true;

uint8_t g_progressValue = 0;

bool g_progressDirDown = false;

uint32_t g_lastProgressUpdateMs = 0;

int16_t g_graphV1 = 0;

int16_t g_graphV2 = 0;

int16_t g_graphV3 = 0;

ToggleSwitchState g_toggleState{false, 0, 0};

bool g_errorDemoActive = false;

uint32_t g_settingsLoadingUntil = 0;

static const uint8_t g_dotsCount = 15;

uint8_t g_dotsActive = 0;

uint8_t g_dotsPrev = 0;

bool g_dotsAnimate = false;

int8_t g_dotsDir = 0;

uint32_t g_dotsAnimStartMs = 0;

static const uint32_t g_dotsAnimDurMs = 240;

static const char* const g_drumOptionsH[] = {"Off", "5 min", "10 min", "30 min", "1 hr"};
static const uint8_t g_drumCountH = 5;
static const char* const g_drumOptionsV[] = {"Small", "Medium", "Large"};
static const uint8_t g_drumCountV = 3;
uint8_t g_drumSelectedH = 0;
uint8_t g_drumPrevH = 0;
bool g_drumAnimateH = false;
uint32_t g_drumAnimStartMs = 0;
static const uint32_t g_drumAnimDurMs = 280;
uint8_t g_drumSelectedV = 0;
uint8_t g_drumPrevV = 0;
bool g_drumAnimateV = false;

float g_blurPhase = 0.0f;

uint32_t g_blurLastUpdateMs = 0;

static void runBootAnimation(GUI &ui,

                             Button &next,

                             Button &prev,

                             BootAnimation anim,

                             uint32_t durationMs,

                             const String &title,

                             const String &subtitle)

{

  (void)next;

  (void)prev;

  ui.showLogo(title, subtitle, anim, 0xFFFF, 0x0000, durationMs);

  const uint32_t start = millis();

  while ((millis() - start) < durationMs)

    ui.loop();
}

void screenPrimitivesDemo(GUI &ui);

void screenGradientsDemo(GUI &ui);

void screenFontWeightDemo(GUI &ui);

void screenMain(GUI &ui)

{

  uint32_t bg = ui.rgb(0, 0, 0);

  ui.clear(bg);

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H1);

    ui.drawPSDFText("Main menu", -1, -1, ui.rgb(255, 255, 255), bg, AlignCenter);
  }

  const int16_t ix = 12;

  const int16_t iy = 12;

  const uint16_t is = 48;

  const uint8_t level = g_batteryLevel;

  const uint32_t fillCol = (level <= 20) ? ui.rgb(255, 40, 40) : ui.rgb(80, 255, 120);

  ui.drawIcon(pipgui::battery_layer2, ix, iy, is, fillCol, bg);

  const int16_t cutX = (int16_t)(ix + (int16_t)((uint32_t)is * (uint32_t)level) / 100u);

  ui.fillRect()
      .at(cutX, iy)
      .size((int16_t)(ix + (int16_t)is - cutX), (int16_t)is)
      .color(bg)
      .draw();

  ui.drawIcon(pipgui::battery_layer0, ix, iy, is, ui.rgb(255, 255, 255), bg);

  ui.drawIcon(pipgui::battery_layer1, ix, iy, is, ui.rgb(255, 255, 255), bg);

  ui.drawIcon(pipgui::warning_layer0, 70, 12, 48, ui.rgb(255, 200, 0), bg);
}

void screenSettings(GUI &ui)

{

  uint32_t bg = ui.rgb(0, 31, 31);

  ui.clear(bg);

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H1);

    ui.drawPSDFText("Settings menu", -1, 80, rgb565(0xFFFF), bg, AlignCenter);
  }

  const String label = settingsBtnState.loading ? String("Saving") : String("Show modal");

  ui.drawButton(label,

                ICON_DOT_WHITE, 2, 2,

                60, 20, 200, 30,

                ui.rgb(80, 150, 255),

                7, settingsBtnState);
}

void screenGlowDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  ui.drawGlowCircle()
      .at(60, 90)
      .radius(16)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg)
      .glowSize(14)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900)
      .draw();
  ui.drawGlowCircle()
      .at(60, 160)
      .radius(12)
      .fillColor(ui.rgb(80, 255, 120))
      .bgColor(bg)
      .glowSize(10)
      .glowStrength(200)
      .draw();
  ui.drawGlowRoundRect()
      .at(center, 70)
      .size(110, 56)
      .radius(14)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg)
      .glowSize(12)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400)
      .draw();
  ui.drawGlowRoundRect()
      .at(110, 150)
      .size(110, 56)
      .radius(14)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg)
      .glowSize(10)
      .glowStrength(180)
      .draw();

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H2);

    ui.drawPSDFText("Glow demo", -1, 22, rgb565(0xFFFF), bg, AlignCenter);

    ui.setTextStyle(Body);

    ui.drawPSDFText("REC / shapes", -1, 44, ui.rgb(200, 200, 200), bg, AlignCenter);
  }
}

void screenBlurDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  uint16_t w = ui.screenWidth();

  int16_t statusH = 20; // как в configureStatusBar

  int16_t bandY = statusH;

  int16_t bandH = 80;

  // Движущиеся прямоугольники под полосой

  float t = g_blurPhase;

  for (int i = 0; i < 3; ++i)

  {

    float tt = t * (0.8f + 0.3f * i);

    int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));

    int16_t cy = (int16_t)(bandY + 15 + i * 22 + cosf(tt * 1.3f) * 8.0f);

    int16_t rw = 60 + i * 20;

    int16_t rh = 18;

    uint32_t col;

    if (i == 0)

      col = ui.rgb(255, 80, 80);

    else if (i == 1)

      col = ui.rgb(80, 255, 120);

    else

      col = ui.rgb(80, 255, 140);

    ui.fillRect()
        .at((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
        .size(rw, rh)
        .color(col)
        .draw();
  }

  // Блюр-полоска сверху движущихся фигур
  ui.drawBlurStrip()
      .at(0, bandY)
      .size((int16_t)w, bandH)
      .radius(10)
      .direction(TopDown)
      .gradient(false)
      .materialStrength(160)
      .noiseAmount(40)
      .materialColor(ui.rgb(10, 10, 10))
      .draw();

  // Вторая строка фигур без блюра — для наглядного сравнения

  for (int i = 0; i < 3; ++i)

  {

    float tt = t * (0.8f + 0.3f * i);

    int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));

    int16_t cy = (int16_t)(bandY + bandH + 10 + i * 18);

    int16_t rw = 60 + i * 20;

    int16_t rh = 16;

    uint32_t col;

    if (i == 0)

      col = ui.rgb(255, 80, 80);

    else if (i == 1)

      col = ui.rgb(80, 255, 120);

    else

      col = ui.rgb(80, 255, 140);

    ui.fillRect()
        .at((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
        .size(rw, rh)
        .color(col)
        .draw();
  }

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H2);

    ui.drawPSDFText("Blur strip", -1, (int16_t)(bandY + bandH + 60), rgb565(0xFFFF), bg, AlignCenter);

    ui.setTextStyle(Caption);

    ui.drawPSDFText("Next: change screen", -1, (int16_t)(bandY + bandH + 80), ui.rgb(160, 160, 160), bg, AlignCenter);

    ui.drawPSDFText("Prev: back / OK", -1, (int16_t)(bandY + bandH + 96), ui.rgb(160, 160, 160), bg, AlignCenter);
  }
}

void screenGraph(GUI &ui)

{

  ui.clear(0x0000);

  ui.drawGraphGrid(center, center,

                   280, 170,

                   13,

                   LeftToRight,

                   ui.rgb(8, 8, 8),

                   ui.rgb(20, 20, 20),

                   1.0f);

  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);

  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);

  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphSmall(GUI &ui)

{

  ui.clear(0x0000);

  ui.drawGraphGrid(center, center,

                   160, 80,

                   10,

                   LeftToRight,

                   ui.rgb(8, 8, 8),

                   ui.rgb(20, 20, 20),

                   1.0f);

  ui.setGraphAutoScale(true);

  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);

  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);

  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphTall(GUI &ui)

{

  ui.clear(0x0000);

  ui.drawGraphGrid(center, center,

                   160, 180,

                   10,

                   RightToLeft,

                   ui.rgb(8, 8, 8),

                   ui.rgb(20, 20, 20),

                   1.0f);

  ui.setGraphAutoScale(true);

  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);

  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);

  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

void screenGraphOsc(GUI &ui)

{

  ui.clear(0x0000);

  ui.drawGraphGrid(center, center,

                   200, 170,

                   13,

                   Oscilloscope,

                   ui.rgb(8, 8, 8),

                   ui.rgb(20, 20, 20),

                   1.0f);

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

void screenProgressDemo(GUI &ui)

{

  ui.clear(0x000000);

  ui.drawProgressBar(center, 60, 200, 10, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), 6, Indeterminate);

  ui.drawProgressBar(center, 74, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), 6, Indeterminate);

  ui.drawProgressBar(center, 88, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), 6, None);

  ui.drawCircularProgressBar(50, 165, 22, 8, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), Indeterminate);

  ui.drawCircularProgressBar(105, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), None);

  ui.drawCircularProgressBar(160, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), Shimmer);

  ui.drawCircularProgressBar(215, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(0, 200, 120), Shimmer);
}

void screenProgressTextDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H2);

    ui.drawPSDFText("Progress with text", -1, 24, ui.rgb(220, 220, 220), bg, AlignCenter);
  }

  uint32_t base = ui.rgb(20, 20, 20);
  uint32_t fill1 = ui.rgb(0, 122, 255);
  uint32_t fill2 = ui.rgb(255, 59, 48);

  // Simple percent centered
  ui.drawProgressBar(center, 70, 200, 14, g_progressValue, base, fill1, 7, None);
  ui.drawProgressPercent(center, 70, 200, 14, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), base, 0);

  // Left custom label + right percent
  ui.drawProgressBar(center, 92, 200, 14, g_progressValue, base, fill2, 7, Shimmer);
  ui.drawProgressText(center, 92, 200, 14, String("Uploading"), AlignLeft, ui.rgb(255, 255, 255), base, 0);
  ui.drawProgressPercent(center, 92, 200, 14, g_progressValue, AlignRight, ui.rgb(200, 200, 200), 0x000000, 0);

  // Circular progress with text in the middle area
  ui.drawCircularProgressBar(center, 160, 30, 8, g_progressValue, base, fill1, None);
  ui.drawProgressPercent((int16_t)(center - 30), 130, 60, 60, g_progressValue, AlignCenter, ui.rgb(255, 255, 255), bg, 0);
}

void screenPrimitivesDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  uint32_t w = ui.screenWidth();

  ui.setTextStyle(Caption);

  ui.drawPSDFText("Primitives", -1, 8, ui.rgb(220, 220, 220), bg, AlignCenter);

  ui.fillCircle()
      .at(50, 55)
      .radius(18)
      .color(ui.rgb(0, 87, 250))
      .draw();
  ui.drawCircle()
      .at(50, 55)
      .radius(18)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.fillEllipse()
      .at(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 0, 72))
      .draw();
  ui.drawEllipse()
      .at(140, 55)
      .radiusX(28)
      .radiusY(16)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.fillTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(0, 200, 120))
      .draw();
  ui.drawTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.fillSquircle()
      .at(70, 135)
      .radius(26)
      .color(ui.rgb(255, 128, 0))
      .draw();
  ui.drawSquircle()
      .at(70, 135)
      .radius(26)
      .color(ui.rgb(255, 255, 255))
      .draw();
  // Прямоугольник с разными радиусами углов для демонстрации per-corner скругления
  ui.fillRect()
      .at(210, 165)
      .size(80, 40)
      .color(bg)
      .draw();
  ui.drawGlowRoundRect()
      .at(250, 185)
      .size(80, 40)
      .radius(12)
      .fillColor(ui.rgb(40, 120, 255))
      .bgColor(bg)
      .glowSize(10)
      .glowStrength(180)
      .draw();
  ui.drawCircle()
      .at(170, 135)
      .radius(28)
      .color(ui.rgb(60, 60, 60))
      .draw();
  ui.drawArc()
      .at(170, 135)
      .radius(28)
      .startDeg(-90.0f)
      .endDeg(90.0f)
      .color(ui.rgb(80, 255, 120))
      .draw();
  ui.drawArc()
      .at(170, 135)
      .radius(22)
      .startDeg(90.0f)
      .endDeg(270.0f)
      .color(ui.rgb(100, 160, 255))
      .draw();
  ui.drawLine()
      .from(12, 90)
      .to(118, 160)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.drawLine()
      .from(12, 160)
      .to(118, 90)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.drawLine()
      .from(12, 125)
      .to(118, 126)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.drawArc()
      .at(250, 135)
      .radius(14)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255))
      .draw();
  ui.drawArc()
      .at(250, 135)
      .radius(10)
      .startDeg(-180.0f)
      .endDeg(180.0f)
      .color(ui.rgb(80, 255, 120))
      .draw();
  // Прямоугольник с разными радиусами углов для демонстрации per-corner скругления
  ui.fillRect()
      .at(10, 180)
      .size(80, 40)
      .color(ui.rgb(30, 30, 30))
      .radius(16, 4, 16, 4)
      .draw();

  ui.drawLine()
      .from(10, 225)
      .to((int16_t)(w - 10), 225)
      .color(ui.rgb(200, 200, 200))
      .draw();

  ui.drawPSDFText("Next: change screen", -1, 235, ui.rgb(160, 160, 160), bg, AlignCenter);
}

void screenGradientsDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  ui.setTextStyle(Caption);

  ui.drawPSDFText("Gradients / Alpha", -1, 8, ui.rgb(220, 220, 220), bg, AlignCenter);

  ui.fillRectGradientVertical()
      .at(10, 30)
      .size(140, 60)
      .topColor(ui.rgb(255, 0, 72))
      .bottomColor(ui.rgb(0, 87, 250))
      .draw();
  ui.fillRectGradientHorizontal()
      .at(170, 30)
      .size(140, 60)
      .leftColor(ui.rgb(255, 128, 0))
      .rightColor(ui.rgb(80, 255, 120))
      .draw();
  ui.fillRectGradient4()
      .at(10, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 0, 72))
      .topRightColor(ui.rgb(0, 87, 250))
      .bottomLeftColor(ui.rgb(80, 255, 120))
      .bottomRightColor(ui.rgb(255, 128, 0))
      .draw();
  ui.fillRectGradientDiagonal()
      .at(170, 100)
      .size(140, 60)
      .topLeftColor(ui.rgb(255, 255, 255))
      .bottomRightColor(ui.rgb(40, 40, 40))
      .draw();
  ui.fillRectGradientRadial()
      .at(10, 170)
      .size(140, 60)
      .center(80, 200)
      .radius(48)
      .innerColor(ui.rgb(255, 255, 255))
      .outerColor(ui.rgb(0, 87, 250))
      .draw();
  ui.fillRectGradientVertical()
      .at(170, 170)
      .size(140, 60)
      .topColor(ui.rgb(20, 20, 20))
      .bottomColor(ui.rgb(20, 20, 20))
      .draw();
  ui.fillRectAlpha()
      .at(170, 170)
      .size(140, 60)
      .color(ui.rgb(0, 0, 0))
      .alpha(140)
      .draw();

  ui.drawPSDFText("fillRectAlpha", 240, 192, ui.rgb(220, 220, 220), bg, AlignCenter);
}

void screenListMenuDemo(GUI &ui)

{

  ui.clear(0x0000);

  ui.renderListMenu(7);
}

void screenListMenuPlainDemo(GUI &ui)

{

  ui.clear(0x0000);

  ui.renderListMenu(10);
}

void screenTileMenuDemo(GUI &ui)

{

  ui.renderTileMenu(8);
}

void screenTileMenuLayoutDemo(GUI &ui)

{

  ui.renderTileMenu(11);
}

void screenTileMenu4ColsDemo(GUI &ui)

{

  ui.renderTileMenu(12);
}

void screenToggleSwitchDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  ui.updateToggleSwitch(center, 150, 78, 36, g_toggleState, ui.rgb(21, 180, 110), -1, -1);

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H2);

    ui.drawPSDFText("ToggleSwitch", -1, 24, ui.rgb(220, 220, 220), bg, AlignCenter);
  }
}

void screenScrollDotsDemo(GUI &ui)
{
  uint32_t bg = ui.rgb(8, 8, 8);
  ui.clear(bg);
  if (ui.psdfFontLoaded())
  {
    ui.setTextStyle(H2);
    ui.drawPSDFText("Scroll dots", -1, 24, ui.rgb(220, 220, 220), bg, AlignCenter);
    ui.drawPSDFText("15 dots (tapering)", -1, 48, ui.rgb(120, 120, 120), bg, AlignCenter);
  }
}

void screenDrumRollDemo(GUI &ui)
{
  uint32_t bg = ui.rgb(8, 8, 8);
  ui.clear(bg);
  if (ui.psdfFontLoaded())
  {
    ui.setTextStyle(H2);
    ui.drawPSDFText("Drum roll", -1, 24, ui.rgb(220, 220, 220), bg, AlignCenter);
    ui.drawPSDFText("Next: H  Prev: V", -1, 48, ui.rgb(120, 120, 120), bg, AlignCenter);
  }
  uint32_t now = millis();
  if (g_drumAnimateH && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateH = false;
  if (g_drumAnimateV && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateV = false;
  float progressH = 1.0f;
  if (g_drumAnimateH && g_drumAnimDurMs > 0)
  {
    uint32_t el = now - g_drumAnimStartMs;
    progressH = (el >= g_drumAnimDurMs) ? 1.0f : (float)el / (float)g_drumAnimDurMs;
  }
  float progressV = 1.0f;
  if (g_drumAnimateV && g_drumAnimDurMs > 0)
  {
    uint32_t el = now - g_drumAnimStartMs;
    progressV = (el >= g_drumAnimDurMs) ? 1.0f : (float)el / (float)g_drumAnimDurMs;
  }
  String optsH[5] = {String(g_drumOptionsH[0]), String(g_drumOptionsH[1]), String(g_drumOptionsH[2]), String(g_drumOptionsH[3]), String(g_drumOptionsH[4])};
  ui.drawDrumRollHorizontal(0, 75, ui.screenWidth(), 28, optsH, g_drumCountH,
                            g_drumSelectedH, g_drumPrevH, progressH,
                            ui.rgb(255, 255, 255), ui.rgb(8, 8, 8), 18);
  String optsV[3] = {String(g_drumOptionsV[0]), String(g_drumOptionsV[1]), String(g_drumOptionsV[2])};
  ui.drawDrumRollVertical(ui.screenWidth() - 80, 120, 70, 90, optsV, g_drumCountV,
                          g_drumSelectedV, g_drumPrevV, progressV,
                          ui.rgb(200, 200, 200), ui.rgb(8, 8, 8), 14);
}

void screenErrorOverlayDemo(GUI &ui)

{

  ui.clear(ui.rgb(8, 8, 8));

  ui.showError("Error demo", "Something went wrong", Crash, "OK");
}

void screenWarningOverlayDemo(GUI &ui)

{

  ui.clear(ui.rgb(8, 8, 8));

  ui.showError("Warning demo", "Check settings", Warning, "OK");
}

void screenDitherDemo(GUI &ui)

{

  ui.clear(ui.rgb(8, 8, 8));
}

void screenDitherBlueDemo(GUI &ui)

{

  ui.clear(ui.rgb(8, 8, 20));
}

void screenDitherTemporalDemo(GUI &ui)

{

  ui.clear(ui.rgb(20, 8, 8));
}

void screenDitherCubesDemo(GUI &ui)

{

  ui.clear(ui.rgb(8, 20, 8));
}

void screenDitherCompareGradDemo(GUI &ui)

{

  // Left: direct 16-bit (RGB565). Right: 24-bit approximated with FRC

  ui.setFrcProfile(FrcProfile::BlueNoise);

  ui.setFrcTemporalPeriodMs(0); // keep static spatial FRC for this comparison

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  int16_t w = (int16_t)ui.screenWidth();

  int16_t h = (int16_t)ui.screenHeight();

  int16_t cw = (int16_t)(w / 2 - 24);

  int16_t ch = (int16_t)(h - 100);

  if (cw < 32)

    cw = 32;

  if (ch < 32)

    ch = 32;

  int16_t leftX = 12;

  int16_t rightX = (int16_t)(w / 2 + 12);

  int16_t topY = 44;

  uint32_t now = millis();

  // smooth per-channel sinusoidal animation (avoids hue-discontinuities)

  static bool frozen = false;

  static uint8_t frozen_r = 0, frozen_g = 0, frozen_b = 0;

  if (btnPrev.wasPressed())

  {

    frozen = !frozen; // toggle freeze state on PREV press

    if (frozen)

    {

      // capture current color into the frozen sample

      float tcap = (float)now / 1000.0f;

      frozen_r = (uint8_t)((sinf(tcap * 0.9f) * 0.5f + 0.5f) * 255.0f);

      frozen_g = (uint8_t)((sinf(tcap * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);

      frozen_b = (uint8_t)((sinf(tcap * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
    }
  }

  uint8_t r, g, b;

  if (frozen)

  {

    r = frozen_r;

    g = frozen_g;

    b = frozen_b;
  }

  else

  {

    float t = (float)now / 1000.0f;

    r = (uint8_t)((sinf(t * 0.9f) * 0.5f + 0.5f) * 255.0f);

    g = (uint8_t)((sinf(t * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);

    b = (uint8_t)((sinf(t * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
  }

  // Draw left: 16-bit direct

  ui.fillRect()
      .at(leftX, topY)
      .size(cw, ch)
      .color(ui.rgb(r, g, b))
      .draw();

  // Draw right: 24-bit approximated via FRC

  ui.fillRect888(rightX, topY, cw, ch, r, g, b, FrcProfile::BlueNoise);

  // Static sample square (for quick inspection) + status

  int16_t sampleX = (int16_t)(w - 64);

  int16_t sampleY = (int16_t)(topY + 4);

  ui.fillRect()
      .at(sampleX - 1, sampleY - 1)
      .size(50, 50)
      .color(ui.rgb(40, 40, 40))
      .draw(); // border
  ui.fillRect()
      .at(sampleX, sampleY)
      .size(48, 48)
      .color(ui.rgb(r, g, b))
      .draw();

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(Caption);

    ui.drawPSDFText(String(frozen ? "Frozen (PREV)" : "Live (PREV to freeze)"), (int16_t)(sampleX + 24), (int16_t)(sampleY + 56), ui.rgb(200, 200, 200), bg, AlignCenter);

    ui.drawPSDFText("16-bit: RGB565", leftX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg, AlignCenter);

    ui.drawPSDFText("24-bit: FRC BlueNoise+gamma", rightX + cw / 2, topY - 8, ui.rgb(200, 200, 200), bg, AlignCenter);

    char buf[64];

    snprintf(buf, sizeof(buf), "HEX: #%02X%02X%02X", r, g, b);

    ui.drawPSDFText(String(buf), (int16_t)(w / 2), (int16_t)(topY + ch + 8), ui.rgb(160, 160, 160), bg, AlignCenter);

    ui.drawPSDFText("RGB565 steps per channel: R=32 G=64 B=32", (int16_t)(w / 2), (int16_t)(topY + ch + 26), ui.rgb(160, 160, 160), bg, AlignCenter);

    ui.drawPSDFText("24-bit per channel: 256 (visualized via FRC)", (int16_t)(w / 2), (int16_t)(topY + ch + 44), ui.rgb(160, 160, 160), bg, AlignCenter);

    uint8_t r5 = (uint8_t)(r >> 3);

    uint8_t g6 = (uint8_t)(g >> 2);

    uint8_t b5 = (uint8_t)(b >> 3);

    snprintf(buf, sizeof(buf), "RGB565: R5=%02u G6=%02u B5=%02u", r5, g6, b5);

    ui.drawPSDFText(String(buf), (int16_t)(w / 2), (int16_t)(topY + ch + 62), ui.rgb(160, 160, 160), bg, AlignCenter);
  }
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

void screenFontCompareDemo(GUI &ui)

{

  uint32_t bg = ui.rgb(10, 10, 10);

  ui.clear(bg);

  int16_t w = (int16_t)ui.screenWidth();

  int16_t h = (int16_t)ui.screenHeight();

  if (ui.psdfFontLoaded())

  {

    ui.setTextStyle(H2);

    ui.drawPSDFText("PSDF font demo", (int16_t)(w / 2), 20, ui.rgb(220, 220, 220), bg, AlignCenter);
  }

  const String sample = "The quick brown fox";

  // Left: PSDF (size + weight variations)

  if (ui.loadBuiltinPSDF())

  {

    ui.enablePSDF(true);

    const uint16_t sizes[4] = {18, 24, 36, 48};

    const uint16_t weights[4] = {400, 500, 600, 700};

    int16_t y0 = 50;

    int16_t spacing = 42;

    for (int i = 0; i < 4; ++i)

    {

      ui.setPSDFFontSize(sizes[i]);

      int16_t ty = (int16_t)(y0 + i * spacing);

      // label + sample

      ui.drawPSDFText(String("PSDF ") + String(sizes[i]) + "px", 8, ty, 0xFFFFFF, bg, AlignLeft);

      ui.drawPSDFText(sample, 8, (int16_t)(ty + 18), ui.rgb(200, 200, 200), bg, AlignLeft);
    }
  }
}

void screenFontWeightDemo(GUI &ui)
{
  uint32_t bg = ui.rgb(10, 10, 10);
  ui.clear(bg);

  int16_t w = (int16_t)ui.screenWidth();

  if (ui.psdfFontLoaded())
  {
    ui.enablePSDF(true);
    
    uint16_t prevWeight = ui.psdfWeight();
    
    ui.setPSDFFontSize(16);
    ui.drawPSDFText("Font Weight Test", (int16_t)(w / 2), 10, ui.rgb(255, 255, 0), bg, AlignCenter);

    const String sample = "The quick brown fox";
    const uint16_t weights[] = {100, 300, 400, 500, 600, 700, 900};
    const char* labels[] = {"Thin 100", "Light 300", "Regular 400", "Medium 500", "SemiBold 600", "Bold 700", "Black 900"};
    
    int16_t y = 35;
    int16_t spacing = 38;

    for (int i = 0; i < 7; ++i)
    {
      ui.setPSDFFontSize(12);
      ui.setPSDFWeight(400);
      ui.drawPSDFText(String(labels[i]), 8, y, ui.rgb(150, 150, 150), bg, AlignLeft);
      
      ui.setPSDFFontSize(20);
      ui.setPSDFWeight(weights[i]);
      ui.drawPSDFText(sample, 8, (int16_t)(y + 14), ui.rgb(255, 255, 255), bg, AlignLeft);
      
      y += spacing;
    }
    
    ui.setPSDFWeight(prevWeight);
  }
}

void setup()

{

  Serial.begin(115200);

  SPIFFS.begin(true);

  ui.setPlatform(&g_platform);


  ui.configureDisplay()
      .pins({11, 12, 10, 9, 14})
      .hz(80000000)
      .size(240, 320)
      .apply();

  ui.begin(3, 0);

  ui.setDebugShowDirtyRects(true);

  ui.configureStatusBar(true, 0x0000, 20, Top);
  ui.setStatusBarStyle(StatusBarStyleBlurGradient);

  ui.setStatusBarText("pipGUI", "00:00", "");

  ui.setStatusBarBattery(100, Bar);

  ui.clearStatusBarIcons();

  ui.addStatusBarIcon(Left, ICON_DOT_WHITE, 2, 2);

  ui.addStatusBarIcon(Center, ICON_DOT_GRAY, 2, 2);

  ui.addStatusBarIcon(Right, ICON_DOT_WHITE, 2, 2);

  ui.setBacklightPin(4);

  btnNext.begin();

  btnPrev.begin();

  ui.enableRecovery(&btnPrev, &btnNext, &btnPrev, 900);

  ui.setRecoveryMenu({

      {"Reboot", "Demo action", +[](GUI &ui)

                                { ui.showNotification("Recovery", "Reboot pressed"); }},

      {"Reset settings", "Demo action", +[](GUI &ui)

                                        { ui.showNotification("Recovery", "Reset pressed"); }},

      {"Exit", "Long press PREV", +[](GUI &ui)

                                  { ui.showNotification("Recovery", "Use long press PREV to exit"); }},

  });

  ui.configureTextStyles(24, 18, 14, 12);

  ui.logoSizesPx(36, 24);

  runBootAnimation(ui, btnNext, btnPrev, FadeIn, 900, "PISPPUS", "Fade in");

  // Enable PSDF as primary font if available

  ui.loadBuiltinPSDF();

  ui.enablePSDF(true);

  if (ui.recoveryActive())

    return;

  ui.regScreen(0, screenMain);

  ui.regScreen(1, screenSettings);

  ui.regScreen(2, screenGlowDemo);

  ui.regScreen(3, screenGraph);

  ui.regScreen(4, screenGraphSmall);

  ui.regScreen(5, screenGraphTall);

  ui.regScreen(6, screenProgressDemo);
  ui.regScreen(28, screenProgressTextDemo);

  ui.regScreen(7, screenListMenuDemo);

  ui.regScreen(8, screenTileMenuDemo);

  ui.regScreen(9, screenGraphOsc);

  ui.regScreen(10, screenListMenuPlainDemo);

  ui.regScreen(11, screenTileMenuLayoutDemo);

  ui.regScreen(12, screenTileMenu4ColsDemo);

  ui.regScreen(13, screenToggleSwitchDemo);

  ui.regScreen(14, screenScrollDotsDemo);

  ui.regScreen(15, screenErrorOverlayDemo);

  ui.regScreen(16, screenWarningOverlayDemo);

  ui.regScreen(17, screenBlurDemo);

  ui.regScreen(18, screenDitherDemo);

  ui.regScreen(19, screenDitherBlueDemo);

  ui.regScreen(20, screenDitherTemporalDemo);

  ui.regScreen(21, screenDitherCubesDemo);

  ui.regScreen(22, screenDitherCompareGradDemo);

  ui.regScreen(23, screenPrimitivesDemo);

  ui.regScreen(24, screenFontCompareDemo);

  ui.regScreen(25, screenGradientsDemo);

  ui.regScreen(26, screenFontWeightDemo);
  ui.regScreen(27, screenDrumRollDemo);

  ui.configureListMenu(

      7,

      0,

      {

          {"Main screen", "Простой экран", 0},

          {"Settings", "Кнопка и уведомление", 1},

          {"Glow demo", "Фигуры со свечением", 2},

          {"Graph demo", "Сетка и три линии", 3},

          {"Graph small", "Маленький авто-масштаб", 4},

          {"Graph tall", "Высокий график справа", 5},

          {"Graph osc", "Цифровой осциллограф", 9},

          {"Progress demo", "Прогресс-бар с анимациями", 6},
          {"Progress + text", "Новые стили прогресса", 28},

          {"Tile menu", "Плиточное меню (grid)", 8},

          {"Tile layout", "Кастомная раскладка", 11},

          {"Tile 4 cols", "4 плитки в ряд", 12},

          {"List menu 2", "Текст + активная плашка", 10},

          {"ToggleSwitch", "Переключатель", 13},

          {"Scroll dots", "Индикатор страниц", 14},

          {"Error", "Экран ошибки", 15},

          {"Warning", "Экран предупреждения", 16},

          {"Blur strip", "Блюр-полоска", 17},

          {"Dither demo", "Gamma + BlueNoise", 18},

          {"Dither Blue", "BlueNoise only", 19},

          {"Dither Temporal", "Temporal FRC", 20},

          {"Dither Cubes", "Two cubes + avg (16/24)", 21},

          {"Dither Compare", "Compare 16-bit vs 24-bit", 22},

          {"Primitives", "Circle / Arc / Ellipse / Triangle / Squircle", 23},

          {"Gradients", "Gradients + fillRectAlpha", 25},

          {"Font compare", "PSDF", 24},

          {"Font weights", "Test all weights", 26},

          {"Drum roll", "Horizontal + vertical picker", 27},

      });

  ui.setListMenuStyle(

      7, ui.rgb(8, 8, 8),

      ui.rgb(21, 54, 140),

      5, 13, 310,

      50, 0, 0, 0);

  ui.configureListMenu(

      10,

      7,

      {

          {"Back", "К списку карточек", 7},

          {"Main screen", "Простой экран", 0},

          {"Settings", "Кнопка и уведомление", 1},

          {"Glow demo", "Фигуры со свечением", 2},

          {"Graph osc", "Цифровой осциллограф", 9},

          {"Tile menu", "Плиточное меню (grid)", 8},

          {"ToggleSwitch", "Переключатель", 13},

          {"Dither demo", "Gamma + BlueNoise", 18},

          {"Dither Cubes", "Two cubes + avg (16/24)", 21},

          {"Dither Compare", "Compare 16-bit vs 24-bit", 22},

          {"Font compare", "PSDF", 24},

      });

  ui.setListMenuStyle(

      10, ui.rgb(8, 8, 8),

      ui.rgb(21, 54, 140),

      6, 12, 310,

      34, 18, 0, 0, Plain);

  ui.configureTileMenu(

      8,

      0,

      {

          {"Main", "Главный экран", 0},

          {"Settings", "Настройки", 1},

          {"Glow demo", "Фигуры со свечением", 2},

          {"Graph", "Графики", 3},

          {"Toggle", "Switch", 13},

      });

  ui.setTileMenuStyle(

      8, ui.rgb(8, 8, 8),

      ui.rgb(21, 54, 140),

      13, 10, 2, 100,

      70, 0, 0, 0,

      TextSubtitle);

  ui.configureTileMenu(

      11,

      7,

      {

          {"Back", "", 7},

          {"Small 1", "", 0},

          {"Small 2", "", 1},

          {"Big", "Main", 2},

      });

  ui.setTileMenuStyle(

      11, ui.rgb(8, 8, 8),

      ui.rgb(21, 54, 140),

      13, 10, 2, 0,

      0, 0, 0, 0,

      TextOnly);

  ui.setTileMenuLayout(

      11,

      2, 3,

      {

          {0, 0, 2, 1},

          {0, 1, 1, 1},

          {0, 2, 1, 1},

          {1, 1, 1, 2},

      });

  ui.configureTileMenu(

      12,

      7,

      {

          {"Back", "", 7},

          {"1", "", 0},

          {"2", "", 1},

          {"3", "", 2},

          {"4", "", 3},

          {"5", "", 4},

          {"6", "", 5},

          {"7", "", 6},

          {"8", "", 9},

      });

  ui.setTileMenuStyle(

      12, ui.rgb(8, 8, 8),

      ui.rgb(21, 54, 140),

      13, 8, 4, 0,

      0, 0, 0, 0,

      TextOnly);

  // Invalidate menu caches to apply new font weights
  ui.invalidateListMenuCache(7);
  ui.invalidateListMenuCache(10);

  ui.setScreen(7);
}

void loop()

{

  ui.loopWithInput(btnNext, btnPrev);

  if (ui.recoveryActive())

    return;

  uint32_t nowMs = millis();

  uint32_t sec = nowMs / 1000U;

  static uint32_t lastMinute = 0xFFFFFFFF;

  uint32_t minute = sec / 60U;

  if (minute != lastMinute)

  {

    lastMinute = minute;

    uint8_t m = minute % 60U;

    uint8_t h = (minute / 60U) % 24U;

    char buf[6];

    snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)h, (unsigned)m);

    ui.setStatusBarText("pipGUI", String(buf), "");

    if (!ui.notificationActive())

      ui.updateStatusBarCenter();
  }

  // Demo: show a toast notification at intervals (cycles through messages, with icon)
  static uint32_t lastToastMs = 0;
  const uint32_t toastIntervalMs = 12000;
  if (!ui.toastActive() && !ui.notificationActive() && nowMs - lastToastMs >= toastIntervalMs)
  {
    lastToastMs = nowMs;
    static const char *const toastMessages[] = {
        "Settings saved",
        "Connected",
        "Sync complete",
        "Ready",
        "pipGUI",
    };
    static const uint8_t toastCount = sizeof(toastMessages) / sizeof(toastMessages[0]);
    uint8_t idx = (uint8_t)((nowMs / toastIntervalMs) % (uint32_t)toastCount);
    bool fromTop = (idx & 1u) != 0;
    IconId iconId = fromTop ? WarningLayer0 : ErrorLayer0;
    ui.showToast()
        .text(String(toastMessages[idx]))
        .duration(2500)
        .fromTop(fromTop)
        .icon(iconId, 20)
        .show();
  }

  static uint32_t lastBatteryMs = 0;

  if (nowMs - lastBatteryMs >= 250)

  {

    lastBatteryMs = nowMs;

    if (g_batteryDirDown)

    {

      if (g_batteryLevel > 0)

        g_batteryLevel--;

      else

        g_batteryDirDown = false;
    }

    else

    {

      if (g_batteryLevel < 100)

        g_batteryLevel++;

      else

        g_batteryDirDown = true;
    }

    ui.setStatusBarBattery(g_batteryLevel, Bar);

    if (!ui.notificationActive())

      ui.updateStatusBarBattery();
  }

  bool nextPressed = btnNext.wasPressed();

  bool prevPressed = btnPrev.wasPressed();

  uint8_t cur = ui.currentScreen();

  if (cur == 2 && !ui.notificationActive())

  {

    static uint32_t lastGlowRedrawMs = 0;

    if (nowMs - lastGlowRedrawMs >= 30)

    {

      lastGlowRedrawMs = nowMs;

      uint32_t bg = ui.rgb(10, 10, 10);

      ui.updateGlowCircle()
          .at(60, 90)
          .radius(16)
          .fillColor(ui.rgb(255, 40, 40))
          .bgColor(bg)
          .glowSize(14)
          .glowStrength(240)
          .anim(Pulse)
          .pulsePeriodMs(900)
          .draw();
      ui.updateGlowRoundRect()
          .at(center, 70)
          .size(110, 56)
          .radius(14)
          .fillColor(ui.rgb(80, 150, 255))
          .bgColor(bg)
          .glowSize(12)
          .glowStrength(220)
          .anim(Pulse)
          .pulsePeriodMs(1400)
          .draw();
    }
  }

  if (cur == 17 && !ui.notificationActive())

  {

    uint32_t now = millis();

    if (now - g_blurLastUpdateMs >= 1)

    {

      g_blurLastUpdateMs = now;

      g_blurPhase += 0.10f;

      if (g_blurPhase > 1000.0f)

        g_blurPhase -= 1000.0f;

      uint32_t bg = ui.rgb(10, 10, 10);

      uint32_t w = ui.screenWidth();

      int16_t statusH = 20;

      int16_t bandY = statusH;

      int16_t bandH = 80;

      ui.fillRect()
          .at(0, bandY)
          .size((int16_t)w, bandH)
          .color(bg)
          .draw();

      float t = g_blurPhase;

      for (int i = 0; i < 3; ++i)

      {

        float tt = t * (0.8f + 0.3f * i);

        int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));

        int16_t cy = (int16_t)(bandY + 15 + i * 22 + cosf(tt * 1.3f) * 8.0f);

        int16_t rw = 60 + i * 20;

        int16_t rh = 18;

        uint32_t col;

        if (i == 0)

          col = ui.rgb(255, 80, 80);

        else if (i == 1)

          col = ui.rgb(80, 255, 120);

        else

          col = ui.rgb(80, 255, 140);

        ui.fillRect()
            .at((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2))
            .size(rw, rh)
            .color(col)
            .draw();
      }

      static bool s_row2Inited = false;

      static int16_t s_row2PrevX[3];

      static int16_t s_row2PrevY[3];

      static int16_t s_row2PrevW[3];

      static int16_t s_row2PrevH[3];

      if (s_row2Inited)

      {

        for (int i = 0; i < 3; ++i)

          ui.fillRect()
              .at(s_row2PrevX[i], s_row2PrevY[i])
              .size(s_row2PrevW[i], s_row2PrevH[i])
              .color(bg)
              .draw();
      }

      for (int i = 0; i < 3; ++i)

      {

        float tt = t * (0.8f + 0.3f * i);

        int16_t cx = (int16_t)(w / 2 + sinf(tt + i) * (float)(w / 2 - 30));

        int16_t cy = (int16_t)(bandY + bandH + 10 + i * 18);

        int16_t rw = 60 + i * 20;

        int16_t rh = 16;

        uint32_t col;

        if (i == 0)

          col = ui.rgb(255, 80, 80);

        else if (i == 1)

          col = ui.rgb(80, 255, 120);

        else

          col = ui.rgb(80, 255, 140);

        int16_t x0 = (int16_t)(cx - rw / 2);

        int16_t y0 = (int16_t)(cy - rh / 2);

        s_row2PrevX[i] = x0;

        s_row2PrevY[i] = y0;

        s_row2PrevW[i] = rw;

        s_row2PrevH[i] = rh;

        ui.fillRect()
            .at(x0, y0)
            .size(rw, rh)
            .color(col)
            .draw();
      }

      s_row2Inited = true;

      ui.updateBlurStrip()
          .at(0, bandY)
          .size((int16_t)w, bandH)
          .radius(10)
          .direction(TopDown)
          .gradient(false)
          .materialStrength(160)
          .noiseAmount(40)
          .materialColor(ui.rgb(10, 10, 10))
          .draw();
    }
  }

  if ((cur == 3 || cur == 4 || cur == 5 || cur == 9) && !ui.notificationActive())

  {

    uint32_t now = millis();

    if (now - g_lastGraphUpdateMs >= 30)

    {

      g_lastGraphUpdateMs = now;

      updateGraphDemo();

      if (cur == 3 || cur == 4 || cur == 5)

      {

        ui.updateGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);

        ui.updateGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);

        ui.updateGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
      }

      else

      {

        ui.updateGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);

        ui.updateGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
      }
    }
  }

  if (cur == 6 && !ui.notificationActive())

  {

    uint32_t now = millis();

    if (now - g_lastProgressUpdateMs >= 30)

    {

      g_lastProgressUpdateMs = now;

      if (g_progressDirDown)

      {

        if (g_progressValue > 0)

          g_progressValue--;

        else

          g_progressDirDown = false;
      }

      else

      {

        if (g_progressValue < 100)

          g_progressValue++;

        else

          g_progressDirDown = true;
      }

      ui.updateProgressBar(center, 60, 200, 10, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), 6, Indeterminate, false);

      ui.updateProgressBar(center, 74, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), 6, None, false);

      ui.updateProgressBar(center, 88, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), 6, Shimmer, false);

      ui.updateCircularProgressBar(50, 165, 22, 8, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), Indeterminate, false);

      ui.updateCircularProgressBar(105, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), None, false);

      ui.updateCircularProgressBar(160, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), Shimmer, false);

      ui.updateCircularProgressBar(215, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(0, 200, 120), Shimmer, true);
    }
  }

  if (cur == 1 && !ui.notificationActive())

  {

    ui.updateButtonPress(settingsBtnState, btnPrev.isDown());

    uint32_t now = millis();

    if (settingsBtnState.loading && g_settingsLoadingUntil != 0 && now >= g_settingsLoadingUntil)

    {

      settingsBtnState.loading = false;

      g_settingsLoadingUntil = 0;
    }

    if (prevPressed)

    {

      if (!settingsBtnState.loading)

      {

        settingsBtnState.loading = true;

        g_settingsLoadingUntil = now + 1500U;

        ui.showNotification("Settings", "Saving settings...", "OK", 0, Normal);
      }
    }

    const String label = settingsBtnState.loading ? String("Saving") : String("Show modal");

    ui.updateButton(label,

                    ICON_DOT_WHITE, 2, 2,

                    60, 20, 200, 30,

                    ui.rgb(80, 150, 255),

                    7, settingsBtnState);
  }

  if ((cur == 7 || cur == 10) && !ui.notificationActive())

  {

    ui.handleListMenuInput(

        cur,

        nextPressed,

        prevPressed,

        btnNext.isDown(),

        btnPrev.isDown());
  }

  else if ((cur == 8 || cur == 11 || cur == 12) && !ui.notificationActive())

  {

    ui.handleTileMenuInput(

        cur,

        nextPressed,

        prevPressed,

        btnNext.isDown(),

        btnPrev.isDown());
  }

  else if (cur == 13 && !ui.notificationActive())

  {

    bool prevVal = g_toggleState.value;

    bool changed = ui.updateToggleSwitch(g_toggleState, nextPressed);

    if (prevPressed)

      ui.setScreen(7);

    if (changed)

    {

      uint32_t bg = ui.rgb(10, 10, 10);

      uint32_t active = ui.rgb(21, 180, 110);

      ui.updateToggleSwitch(center, 150, 78, 36, g_toggleState, active, -1, -1);

      if (prevVal != g_toggleState.value && ui.psdfFontLoaded())

      {

        ui.setTextStyle(Body);

        const char *s = g_toggleState.value ? "ON" : "OFF";

        ui.updatePSDFText(String(s), -1, 105, ui.rgb(200, 200, 200), bg, AlignCenter);
      }
    }
  }

  else if (cur == 14 && !ui.notificationActive())

  {

    if (nextPressed || prevPressed)

    {

      uint8_t prev = g_dotsActive;

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

      g_dotsAnimStartMs = millis();
    }

    if (g_dotsAnimate)

    {

      uint32_t now = millis();

      if (now - g_dotsAnimStartMs >= g_dotsAnimDurMs)

        g_dotsAnimate = false;

      float p = 1.0f;

      if (g_dotsAnimDurMs > 0)

      {

        uint32_t el = now - g_dotsAnimStartMs;

        if (el >= g_dotsAnimDurMs)

          p = 1.0f;

        else

          p = (float)el / (float)g_dotsAnimDurMs;
      }

      ui.updateScrollDots()
          .at(center, 150)
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
          .activeWidth(18)
          .draw();
    }
  }

  else if (cur == 27 && !ui.notificationActive())
  {
    if (nextPressed)
    {
      g_drumPrevH = g_drumSelectedH;
      g_drumSelectedH = (uint8_t)((g_drumSelectedH + 1) % g_drumCountH);
      g_drumAnimateH = true;
      g_drumAnimStartMs = millis();
    }
    else if (prevPressed)
    {
      g_drumPrevH = g_drumSelectedH;
      g_drumSelectedH = (uint8_t)((g_drumSelectedH + g_drumCountH - 1) % g_drumCountH);
      g_drumAnimateH = true;
      g_drumAnimStartMs = millis();
    }
    if (prevPressed)
    {
      g_drumPrevV = g_drumSelectedV;
      g_drumSelectedV = (uint8_t)((g_drumSelectedV + 1) % g_drumCountV);
      g_drumAnimateV = true;
      g_drumAnimStartMs = millis();
    }
    ui.requestRedraw();
  }

  else if ((cur == 15 || cur == 16) && !ui.notificationActive())

  {

    if (!ui.errorActive())

    {

      if (prevPressed)

      {

        ui.setScreen(7);
      }

      else if (nextPressed)

      {

        if (cur == 15)

        {

          ui.showError("Failed to create screen sprite", "Code: 0xFAISPRT", Crash, "OK");

          ui.showError("LittleFS mount failed!", "Code: 0xLFS_MNT", Crash, "OK");

          ui.showError("Sprite creation failed!", "Code: 0xSPR_MEM", Crash, "OK");
        }

        else

        {

          ui.showError("No profile found!", "Code:0xNOPROF", Warning, "OK");

          ui.showError("Network is unstable", "Code:0xNETWARN", Warning, "OK");

          ui.showError("Low battery warning", "Code:0xLOWBATT", Warning, "OK");
        }
      }
    }
  }

  else if (!ui.notificationActive())

  {

    if (nextPressed)

      ui.nextScreen();
  }

  if (ui.errorActive())

  {

    ui.setErrorButtonDown(btnPrev.isDown());
  }

  else if (ui.notificationActive())

  {

    ui.setNotificationButtonDown(btnPrev.isDown());
  }
}
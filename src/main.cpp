#include <Arduino.h>







#include <SPIFFS.h>







#include <math.h>







#include <pipGUI/core/api/pipGUI.hpp>







#include <pipGUI/icons/metrics.hpp>







#include <pipCore/Platforms/ESP32/GUI.hpp>







using namespace pipgui;







GUI ui;







static pipcore::Esp32GuiPlatform g_platform;







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







static const uint8_t g_dotsCount = 5;



uint8_t g_dotsActive = 0;



uint8_t g_dotsPrev = 0;



bool g_dotsAnimate = false;



int8_t g_dotsDir = 0;



uint32_t g_dotsAnimStartMs = 0;



static const uint32_t g_dotsAnimDurMs = 240;







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



  ui.fillRect(cutX, iy, (int16_t)(ix + (int16_t)is - cutX), (int16_t)is, bg);







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







  ui.drawGlowCircle(60, 90, 16, ui.rgb(255, 40, 40), bg, -1, 14, 240, Pulse, 900);



  ui.drawGlowCircle(60, 160, 12, ui.rgb(80, 255, 120), bg, -1, 10, 200, None);







  ui.drawGlowRoundRect(center, 70, 110, 56, 14, ui.rgb(80, 150, 255), bg, -1, 12, 220, Pulse, 1400);



  ui.drawGlowRoundRect(110, 150, 110, 56, 14, ui.rgb(255, 180, 0), bg, -1, 10, 180, None);







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







    ui.fillRect(cx - rw / 2, cy - rh / 2, rw, rh, col);



  }







  // Блюр-полоска сверху движущихся фигур



  ui.drawBlurStrip(0, bandY, (int16_t)w, bandH,



                   10,



                   TopDown,



                   false,



                   160,



                   40, ui.rgb(10, 10, 10));







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







    ui.fillRect(cx - rw / 2, cy - rh / 2, rw, rh, col);



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



  ui.drawCircularProgressBar(160, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), Stripes);



  ui.drawCircularProgressBar(215, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(0, 200, 120), Shimmer);



}







void screenPrimitivesDemo(GUI &ui)



{



  uint32_t bg = ui.rgb(10, 10, 10);



  ui.clear(bg);







  uint32_t w = ui.screenWidth();







  ui.setTextStyle(Caption);



  ui.drawPSDFText("Primitives", -1, 8, ui.rgb(220, 220, 220), bg, AlignCenter);







  ui.fillCircle(50, 55, 18, ui.rgb(0, 87, 250));



  ui.drawCircle(50, 55, 18, ui.rgb(255, 255, 255));







  ui.fillEllipse(140, 55, 28, 16, ui.rgb(255, 0, 72));



  ui.drawEllipse(140, 55, 28, 16, ui.rgb(255, 255, 255));







  ui.fillTriangle(220, 72, 250, 40, 280, 72, ui.rgb(0, 200, 120));



  ui.drawTriangle(220, 72, 250, 40, 280, 72, ui.rgb(255, 255, 255));







  ui.fillSquircle(70, 135, 26, ui.rgb(255, 128, 0));



  ui.drawSquircle(70, 135, 26, ui.rgb(255, 255, 255));







  ui.drawCircle(170, 135, 28, ui.rgb(60, 60, 60));



  ui.drawArc(170, 135, 28, -90.0f, 90.0f, ui.rgb(80, 255, 120));



  ui.drawArc(170, 135, 22, 90.0f, 270.0f, ui.rgb(100, 160, 255));







  ui.drawLine(12, 90, 118, 160, ui.rgb(255, 255, 255));



  ui.drawLine(12, 160, 118, 90, ui.rgb(255, 255, 255));



  ui.drawLine(12, 125, 118, 126, ui.rgb(255, 255, 255));







  ui.drawArc(250, 135, 14, -180.0f, 180.0f, ui.rgb(255, 255, 255));



  ui.drawArc(250, 135, 10, -180.0f, 180.0f, ui.rgb(80, 255, 120));







  ui.drawLine(10, 195, (int16_t)(w - 10), 195, ui.rgb(200, 200, 200));



  ui.drawPSDFText("Next: change screen", -1, 205, ui.rgb(160, 160, 160), bg, AlignCenter);



}







void screenGradientsDemo(GUI &ui)



{



  uint32_t bg = ui.rgb(10, 10, 10);



  ui.clear(bg);







  ui.setTextStyle(Caption);



  ui.drawPSDFText("Gradients / Alpha", -1, 8, ui.rgb(220, 220, 220), bg, AlignCenter);







  ui.fillRectGradientVertical(10, 30, 140, 60, ui.rgb(255, 0, 72), ui.rgb(0, 87, 250));



  ui.fillRectGradientHorizontal(170, 30, 140, 60, ui.rgb(255, 128, 0), ui.rgb(80, 255, 120));







  ui.fillRectGradient4(10, 100, 140, 60,



                       ui.rgb(255, 0, 72), ui.rgb(0, 87, 250),



                       ui.rgb(80, 255, 120), ui.rgb(255, 128, 0));







  ui.fillRectGradientDiagonal(170, 100, 140, 60, ui.rgb(255, 255, 255), ui.rgb(40, 40, 40));







  ui.fillRectGradientRadial(10, 170, 140, 60,



                            80, 200, 48,



                            ui.rgb(255, 255, 255), ui.rgb(0, 87, 250));







  ui.fillRectGradientVertical(170, 170, 140, 60, ui.rgb(20, 20, 20), ui.rgb(20, 20, 20));



  ui.fillRectAlpha(170, 170, 140, 60, ui.rgb(0, 0, 0), 140);



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







  }







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







  ui.fillRect(leftX, topY, cw, ch, ui.rgb(r, g, b));















  // Draw right: 24-bit approximated via FRC







  ui.fillRect888(rightX, topY, cw, ch, r, g, b, FrcProfile::BlueNoise);















  // Static sample square (for quick inspection) + status







  int16_t sampleX = (int16_t)(w - 64);







  int16_t sampleY = (int16_t)(topY + 4);







  ui.fillRect(sampleX - 1, sampleY - 1, 50, 50, ui.rgb(40, 40, 40)); // border







  ui.fillRect(sampleX, sampleY, 48, 48, ui.rgb(r, g, b));















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







    const float weights[4] = {-0.25f, 0.0f, 0.18f, 0.45f};







    int16_t y0 = 50;







    int16_t spacing = 42;















    for (int i = 0; i < 4; ++i)







    {







      ui.setPSDFFontSize(sizes[i]);







      ui.setPSDFWeightBias(weights[i]);















      int16_t ty = (int16_t)(y0 + i * spacing);







      // label + sample







      ui.drawPSDFText(String("PSDF " ) + String(sizes[i]) + "px", 8, ty, 0xFFFFFF, bg, AlignLeft);







      ui.drawPSDFText(sample, 8, (int16_t)(ty + 18), ui.rgb(200, 200, 200), bg, AlignLeft);







    }







  }







}















void setup()







{







  Serial.begin(115200);







  SPIFFS.begin(true);















  ui.setPlatform(&g_platform);















  ui.configureDisplay(11, 12, 10, 9, -1, 240, 320);







  ui.begin(3, 0);







  ui.setDebugShowDirtyRects(true);















  ui.configureStatusBar(true, 0x0000, 20, Top);







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







      ui.updateGlowCircle(60, 90, 16, ui.rgb(255, 40, 40), bg, -1, 14, 240, Pulse, 900);







      ui.updateGlowRoundRect(center, 70, 110, 56, 14, ui.rgb(80, 150, 255), bg, -1, 12, 220, Pulse, 1400);







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















      ui.fillRect(0, bandY, (int16_t)w, bandH, bg);















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















        ui.fillRect((int16_t)(cx - rw / 2), (int16_t)(cy - rh / 2), rw, rh, col);







      }















      static bool s_row2Inited = false;







      static int16_t s_row2PrevX[3];







      static int16_t s_row2PrevY[3];







      static int16_t s_row2PrevW[3];







      static int16_t s_row2PrevH[3];















      if (s_row2Inited)







      {







        for (int i = 0; i < 3; ++i)







          ui.fillRect(s_row2PrevX[i], s_row2PrevY[i], s_row2PrevW[i], s_row2PrevH[i], bg);







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















        ui.fillRect(x0, y0, rw, rh, col);







      }







      s_row2Inited = true;















      ui.updateBlurStrip(0, bandY, (int16_t)w, bandH,







                         10,







                         TopDown,







                         false,







                         160,







                         40, ui.rgb(10, 10, 10));







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















      ui.updateProgressBar(center, 60, 200, 10, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), 6, Indeterminate);







      ui.updateProgressBar(center, 74, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), 6, None);







      ui.updateProgressBar(center, 88, 200, 10, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), 6, Stripes);















      ui.updateCircularProgressBar(50, 165, 22, 8, 0, ui.rgb(10, 10, 10), ui.rgb(0, 87, 250), Indeterminate);







      ui.updateCircularProgressBar(105, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 0, 72), None);







      ui.updateCircularProgressBar(160, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(255, 128, 0), Stripes);







      ui.updateCircularProgressBar(215, 165, 22, 8, g_progressValue, ui.rgb(10, 10, 10), ui.rgb(0, 200, 120), Shimmer);







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















      ui.updateScrollDots(center, 150,







                          g_dotsCount,







                          g_dotsActive,







                          g_dotsPrev,







                          p,







                          g_dotsAnimate,







                          g_dotsDir,







                          ui.rgb(0, 87, 250),







                          ui.rgb(60, 60, 60),







                          3, 14, 18);







    }







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
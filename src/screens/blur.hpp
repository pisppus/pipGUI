#pragma once

SCREEN(blur, 17)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();
  const int16_t contentY = kStatusBarHeight;
  const int16_t contentH = (int16_t)ui.screenHeight() - contentY;
  const float t = g_blurPhase;

  BlurRect rowTop[kBlurRectCount];
  BlurRect rowMid[kBlurRectCount];
  BlurRect rowBottom[kBlurRectCount];

  buildBlurRow(t, w, contentY + 16, 24, 18, rowTop);
  buildBlurRow(t + 7.0f, w, contentY + 76, 20, 16, rowMid);
  buildBlurRow(t + 13.0f, w, contentY + 138, 18, 14, rowBottom);

  drawBlurRow(ui, rowTop);
  drawBlurRow(ui, rowMid);
  drawBlurRow(ui, rowBottom);

  ui.drawBlur()
      .pos(14, contentY + 8)
      .size(98, 48)
      .radius(10)
      .direction(TopDown)
      .material(120, -1);

  ui.drawBlur()
      .pos(128, contentY + 8)
      .size(98, 48)
      .radius(10)
      .direction(BottomUp)
      .material(120, -1);

  ui.drawBlur()
      .pos(14, contentY + 70)
      .size(98, 48)
      .radius(10)
      .direction(LeftRight)
      .material(120, -1);

  ui.drawBlur()
      .pos(128, contentY + 70)
      .size(98, 48)
      .radius(10)
      .direction(RightLeft)
      .material(120, -1);

  ui.setTextStyle(Caption);
  ui.drawText().text("TopDown").pos(63, contentY + 61).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.drawText().text("BottomUp").pos(177, contentY + 61).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.drawText().text("LeftRight").pos(63, contentY + 123).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.drawText().text("RightLeft").pos(177, contentY + 123).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.drawText().text("Blur directions").pos(-1, (int16_t)(contentY + contentH - 28)).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.drawText().text("Moving shapes stay visible under each panel").pos(-1, (int16_t)(contentY + contentH - 12)).color(ui.rgb(160, 160, 160)).bgColor(bg565).align(Center);
}

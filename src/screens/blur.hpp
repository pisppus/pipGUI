SCREEN(blur, 17)
{
  ui.clear(ui.rgb(10, 10, 10));

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
      .materialStrength(0)
      .materialColor(ui.rgb(10, 10, 10));

  buildBlurRow(t, (int16_t)w, bandY + bandH + 10, 18, 16, row);
  drawBlurRow(ui, row);

  ui.setTextStyle(H2);
  ui.drawText().text("Blur strip").pos(-1, (int16_t)(bandY + bandH + 60)).color(color565To888(0xFFFF)).bgColor(ui.rgb(10, 10, 10)).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("Next: change screen").pos(-1, (int16_t)(bandY + bandH + 80)).color(ui.rgb(160, 160, 160)).bgColor(ui.rgb(10, 10, 10)).align(Center);
  ui.drawText().text("Prev: back / OK").pos(-1, (int16_t)(bandY + bandH + 96)).color(ui.rgb(160, 160, 160)).bgColor(ui.rgb(10, 10, 10)).align(Center);
}

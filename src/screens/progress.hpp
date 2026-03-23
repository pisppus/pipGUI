SCREEN(progress, 6)
{
  ui.clear(0x000000);
  const String valueRaw = String(g_progressValue);
  const String valuePercent = valueRaw + "%";
  const String valueFraction = String((float)g_progressValue / 100.0f, 2);
  const String valueRange = valueRaw + "/100";

  auto barBase = ui.drawProgress()
                     .pos(center, 60)
                     .size(200, 10)
                     .value(0)
                     .baseColor(ui.rgb(10, 10, 10))
                     .fillColor(ui.rgb(0, 87, 250))
                     .radius(6)
                     .label("Indeterminate")
                     .labelColor(ui.rgb(255, 255, 255))
                     .percent()
                     .percentColor(ui.rgb(200, 200, 200))
                     .anim(Indeterminate);

  barBase.derive()
      .value(g_progressValue)
      .pos(center, 74)
      .label("Shimmer")
      .anim(Shimmer);

  barBase.derive()
      .value(g_progressValue)
      .pos(center, 88)
      .label("Determinate")
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  auto ringBase = ui.drawCircleProgress()
                      .pos(50, 165)
                      .radius(22)
                      .thickness(8)
                      .value(0)
                      .baseColor(ui.rgb(10, 10, 10))
                      .fillColor(ui.rgb(0, 87, 250))
                      .anim(Indeterminate);

  ringBase.derive()
      .pos(105, 165)
      .value(g_progressValue)
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  ringBase.derive()
      .pos(160, 165)
      .value(g_progressValue)
      .fillColor(ui.rgb(255, 128, 0))
      .anim(Shimmer);

  ringBase.derive()
      .pos(215, 165)
      .value(g_progressValue)
      .fillColor(ui.rgb(0, 200, 120))
      .anim(Shimmer);

  ui.drawText()
      .pos(50, 200)
      .text(valueRaw)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .align(Center);

  ui.drawText()
      .pos(105, 200)
      .text(valuePercent)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .align(Center);

  ui.drawText()
      .pos(160, 200)
      .text(valueFraction)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .align(Center);

  ui.drawText()
      .pos(215, 200)
      .text(valueRange)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .align(Center);
}

SCREEN(progressText, 28)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Progress with text").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);

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

  ui.drawProgressPercent(center, 70, 200, 14, g_progressValue, Center, ui.rgb(255, 255, 255), base, 0);

  ui.drawProgressBar()
      .pos(center, 92)
      .size(200, 14)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill2)
      .radius(7)
      .anim(Shimmer);

  ui.drawProgressText(center, 92, 200, 14, String("Uploading"), Left, ui.rgb(255, 255, 255), base, 0);
  ui.drawProgressPercent(center, 92, 200, 14, g_progressValue, Right, ui.rgb(200, 200, 200), 0x000000, 0);

  ui.drawCircularProgressBar()
      .pos(center, 160)
      .radius(30)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(base)
      .fillColor(fill1)
      .anim(None);

  ui.drawProgressPercent(
      (int16_t)(center - 30), 130, 60, 60, g_progressValue, Center, ui.rgb(255, 255, 255), bg565, 0);
}

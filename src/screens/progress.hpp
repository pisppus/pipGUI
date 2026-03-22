SCREEN(progress, 6)
{
  ui.clear(0x000000);

  auto barBase = ui.drawProgressBar()
                     .pos(center, 60)
                     .size(200, 10)
                     .value(0)
                     .baseColor(ui.rgb(10, 10, 10))
                     .fillColor(ui.rgb(0, 87, 250))
                     .radius(6)
                     .anim(Indeterminate);

  barBase.derive()
      .value(g_progressValue)
      .pos(center, 74);

  barBase.derive()
      .value(g_progressValue)
      .pos(center, 88)
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  auto ringBase = ui.drawCircularProgressBar()
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
}

SCREEN(progress, 6)
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

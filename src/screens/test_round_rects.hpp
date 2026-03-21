SCREEN(testRoundRects, 31)
{
  const uint16_t bg565 = ui.rgb(20, 28, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("RoundRect Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fill/draw RoundRect (1 & 4 radius)").pos(-1, 28).color(ui.rgb(180, 200, 180)).bgColor(bg565).align(Center);
  ui.drawText().text("Single radius:").pos(60, 45).color(ui.rgb(180, 220, 180)).bgColor(bg565).align(Center);

  ui.fillRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(0, 87, 250));
  ui.drawRect().pos(15, 58).size(40, 30).radius({4}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 0, 72));
  ui.drawRect().pos(65, 58).size(50, 35).radius({8}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(80, 255, 120));
  ui.drawRect().pos(125, 58).size(60, 40).radius({12}).color(ui.rgb(255, 255, 255));

  ui.fillRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 128, 0));
  ui.drawRect().pos(195, 58).size(50, 45).radius({15}).color(ui.rgb(255, 255, 255));
  ui.drawText().text("4-corner radii:").pos(60, 110).color(ui.rgb(220, 180, 180)).bgColor(bg565).align(Center);
  ui.fillRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(180, 80, 255));
  ui.drawRect().pos(15, 125).size(50, 40).radius({2, 8, 2, 8}).color(ui.rgb(255, 255, 255));
  ui.fillRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(0, 200, 200));
  ui.drawRect().pos(75, 125).size(60, 45).radius({12, 4, 12, 4}).color(ui.rgb(255, 255, 255));
  ui.fillRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(200, 200, 80));
  ui.drawRect().pos(145, 125).size(70, 50).radius({3, 10, 18, 6}).color(ui.rgb(255, 255, 255));
  ui.drawText().text("drawRoundRect only:").pos(70, 185).color(ui.rgb(200, 200, 200)).bgColor(bg565).align(Center);

  ui.drawRect().pos(20, 200).size(45, 35).radius({6}).color(ui.rgb(255, 255, 255));
  ui.drawRect().pos(75, 205).size(50, 40).radius({10}).color(ui.rgb(0, 255, 255));
  ui.drawRect().pos(135, 200).size(55, 45).radius({5, 15, 5, 15}).color(ui.rgb(255, 200, 0));
  ui.drawRect().pos(200, 205).size(50, 40).radius({15, 5, 15, 5}).color(ui.rgb(255, 100, 200));

  ui.drawText().text("Check corner AA quality!").pos(-1, 260).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

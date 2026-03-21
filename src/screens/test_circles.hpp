SCREEN(testCircles, 30)
{
  const uint16_t bg565 = ui.rgb(20, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Circle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fillCircle + drawCircle (AA)").pos(-1, 28).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("r=1-4").pos(25, 45).color(ui.rgb(160, 160, 180)).bgColor(bg565).align(Center);

  ui.fillCircle().pos(25, 58).radius(1).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 58).radius(1).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 65).radius(2).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 65).radius(2).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 75).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 75).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(25, 88).radius(4).color(ui.rgb(0, 87, 250));
  ui.drawCircle().pos(25, 88).radius(4).color(ui.rgb(255, 255, 255));
  ui.drawText().text("r=6-15").pos(80, 45).color(ui.rgb(160, 160, 180)).bgColor(bg565).align(Center);

  ui.fillCircle().pos(65, 70).radius(6).color(ui.rgb(255, 0, 72));
  ui.drawCircle().pos(65, 70).radius(6).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(95, 70).radius(9).color(ui.rgb(80, 255, 120));
  ui.drawCircle().pos(95, 70).radius(9).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 105).radius(12).color(ui.rgb(255, 128, 0));
  ui.drawCircle().pos(80, 105).radius(12).color(ui.rgb(255, 255, 255));

  ui.fillCircle().pos(80, 145).radius(15).color(ui.rgb(180, 80, 255));
  ui.drawCircle().pos(80, 145).radius(15).color(ui.rgb(255, 255, 255));
  ui.drawText().text("r=18-35").pos(155, 45).color(ui.rgb(160, 160, 180)).bgColor(bg565).align(Center);

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

  ui.drawText().text("Check AA edges on overlaps!").pos(-1, 240).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

SCREEN(testTriangles, 33)
{
  const uint16_t bg565 = ui.rgb(20, 24, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Triangle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fillTriangle (4x subpixel AA)").pos(-1, 28).color(ui.rgb(180, 200, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("Small (5-15px):").pos(70, 45).color(ui.rgb(180, 200, 220)).bgColor(bg565).align(Center);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(0).color(ui.rgb(255, 255, 255));
  ui.drawText().text("Medium (20-40px):").pos(80, 95).color(ui.rgb(180, 200, 220)).bgColor(bg565).align(Center);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(0).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(0).color(ui.rgb(255, 255, 255));
  ui.drawText().text("Flat/thin (AA stress):").pos(90, 170).color(ui.rgb(220, 180, 180)).bgColor(bg565).align(Center);
  ui.fillTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(20, 190).point1(100, 195).point2(60, 185).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(120, 180).point1(130, 220).point2(110, 220).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(150, 185).point1(220, 200).point2(150, 205).radius(0).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(30, 235).point1(60, 275).point2(0, 275).radius(0).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 250).point1(75, 290).point2(15, 290).radius(0).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 250).point1(45, 290).point2(-15, 290).radius(0).color(ui.rgb(80, 255, 120));

  ui.drawText().text("Check coverage-based AA edges!").pos(-1, 305).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

SCREEN(testRoundTriangles, 36)
{
  const uint16_t bg565 = ui.rgb(24, 20, 28);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("RoundTriangle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fill/draw RoundTriangle (SDF AA)").pos(-1, 28).color(ui.rgb(200, 180, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("Small (r=3-6):").pos(70, 45).color(ui.rgb(180, 200, 220)).bgColor(bg565).align(Center);

  ui.fillTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(0, 87, 250));
  ui.drawTriangle().point0(25, 50).point1(35, 65).point2(15, 65).radius(3).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 0, 72));
  ui.drawTriangle().point0(55, 55).point1(70, 75).point2(40, 75).radius(4).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(80, 255, 120));
  ui.drawTriangle().point0(95, 50).point1(115, 80).point2(75, 80).radius(6).color(ui.rgb(255, 255, 255));
  ui.drawText().text("Medium (r=8-12):").pos(80, 95).color(ui.rgb(180, 200, 220)).bgColor(bg565).align(Center);

  ui.fillTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 128, 0));
  ui.drawTriangle().point0(35, 110).point1(60, 150).point2(10, 150).radius(8).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(180, 80, 255));
  ui.drawTriangle().point0(90, 105).point1(125, 155).point2(55, 155).radius(10).color(ui.rgb(255, 255, 255));

  ui.fillTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(0, 200, 200));
  ui.drawTriangle().point0(170, 100).point1(210, 160).point2(130, 160).radius(12).color(ui.rgb(255, 255, 255));
  ui.drawText().text("Various shapes (r=10):").pos(90, 170).color(ui.rgb(220, 180, 180)).bgColor(bg565).align(Center);
  ui.fillTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 100, 100));
  ui.drawTriangle().point0(40, 190).point1(70, 240).point2(10, 240).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(100, 255, 100));
  ui.drawTriangle().point0(100, 195).point1(160, 235).point2(40, 235).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(100, 100, 255));
  ui.drawTriangle().point0(190, 185).point1(210, 255).point2(170, 255).radius(10).color(ui.rgb(255, 255, 255));
  ui.fillTriangle().point0(30, 270).point1(60, 310).point2(0, 310).radius(8).color(ui.rgb(0, 87, 250));
  ui.fillTriangle().point0(45, 280).point1(75, 320).point2(15, 320).radius(8).color(ui.rgb(255, 0, 72));
  ui.fillTriangle().point0(15, 280).point1(45, 320).point2(-15, 320).radius(8).color(ui.rgb(80, 255, 120));

  ui.drawText().text("Check SDF AA on rounded corners!").pos(-1, 335).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

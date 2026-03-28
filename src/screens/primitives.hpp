SCREEN(primitives, 23)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  const uint32_t w = ui.screenWidth();

  ui.setTextStyle(Caption);
  ui.drawText().text("Primitives").pos(-1, 8).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);

  ui.drawCircle()
      .pos(50, 55)
      .radius(18)
      .fill(ui.rgb(0, 87, 250))
      .border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse()
      .pos(140, 55)
      .radiusX(28)
      .radiusY(16)
      .fill(ui.rgb(255, 0, 72))
      .border(1, ui.rgb(255, 255, 255));

  ui.drawTriangle()
      .point0(220, 72)
      .point1(250, 40)
      .point2(280, 72)
      .radius(6)
      .fill(ui.rgb(0, 200, 120))
      .border(1, ui.rgb(255, 255, 255));

  ui.drawSquircleRect()
      .pos(44, 109)
      .size(52, 52)
      .radius(26)
      .fill(ui.rgb(255, 128, 0))
      .border(1, ui.rgb(255, 255, 255));

  ui.drawRect()
      .pos(210, 165)
      .size(80, 40)
      .fill(bg565);

  ui.drawGlowCircle()
      .pos(250, 185)
      .radius(20)
      .fillColor(ui.rgb(40, 120, 255))
      .bgColor(bg565)
      .glowSize(10)
      .glowStrength(180);

  ui.drawCircle()
      .pos(170, 135)
      .radius(28)
      .border(1, ui.rgb(60, 60, 60));

  ui.drawArc()
      .pos(170, 135)
      .radius(28)
      .start(-90.0f)
      .end(90.0f)
      .color(ui.rgb(80, 255, 120));

  ui.drawArc()
      .pos(170, 135)
      .radius(22)
      .start(90.0f)
      .end(270.0f)
      .color(ui.rgb(100, 160, 255));

  ui.drawLine()
      .from(12, 90)
      .to(118, 160)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(12, 160)
      .to(118, 90)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(12, 125)
      .to(118, 126)
      .color(ui.rgb(255, 255, 255));

  ui.drawArc()
      .pos(250, 135)
      .radius(14)
      .start(-180.0f)
      .end(180.0f)
      .color(ui.rgb(255, 255, 255));

  ui.drawArc()
      .pos(250, 135)
      .radius(10)
      .start(-180.0f)
      .end(180.0f)
      .color(ui.rgb(80, 255, 120));

  ui.drawRect()
      .pos(10, 180)
      .size(80, 40)
      .fill(ui.rgb(30, 30, 30))
      .radius(16, 4, 16, 4);

  ui.drawLine()
      .from(10, 225)
      .to((int16_t)(w - 10), 225)
      .color(ui.rgb(200, 200, 200));

  ui.drawText().text("Next: change screen").pos(-1, 235).color(ui.rgb(160, 160, 160)).bgColor(bg565).align(Center);
}

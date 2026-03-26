SCREEN(testEllipses, 32)
{
  const uint16_t bg565 = ui.rgb(28, 20, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Ellipse Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("drawEllipse fill / border (Wu-style AA)").pos(-1, 28).color(ui.rgb(200, 180, 180)).bgColor(bg565).align(Center);
  ui.drawText().text("rx > ry (wide):").pos(70, 45).color(ui.rgb(200, 180, 180)).bgColor(bg565).align(Center);

  ui.drawEllipse().pos(40, 70).radiusX(15).radiusY(8).fill(ui.rgb(0, 87, 250)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(100, 70).radiusX(25).radiusY(10).fill(ui.rgb(255, 0, 72)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(170, 70).radiusX(35).radiusY(12).fill(ui.rgb(80, 255, 120)).border(1, ui.rgb(255, 255, 255));

  ui.drawText().text("rx < ry (tall):").pos(70, 95).color(ui.rgb(200, 180, 180)).bgColor(bg565).align(Center);

  ui.drawEllipse().pos(40, 120).radiusX(8).radiusY(15).fill(ui.rgb(255, 128, 0)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(100, 120).radiusX(10).radiusY(25).fill(ui.rgb(180, 80, 255)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(170, 120).radiusX(12).radiusY(35).fill(ui.rgb(0, 200, 200)).border(1, ui.rgb(255, 255, 255));
  ui.drawText().text("Near-circular:").pos(70, 165).color(ui.rgb(200, 180, 180)).bgColor(bg565).align(Center);

  ui.drawEllipse().pos(50, 190).radiusX(18).radiusY(16).fill(ui.rgb(200, 200, 80)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(120, 190).radiusX(22).radiusY(20).fill(ui.rgb(100, 255, 100)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(190, 190).radiusX(28).radiusY(25).fill(ui.rgb(255, 100, 100)).border(1, ui.rgb(255, 255, 255));
  ui.drawEllipse().pos(30, 235).radiusX(3).radiusY(5).fill(ui.rgb(255, 200, 0)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(50, 235).radiusX(5).radiusY(3).fill(ui.rgb(0, 255, 200)).border(1, ui.rgb(255, 255, 255));

  ui.drawEllipse().pos(70, 235).radiusX(4).radiusY(4).fill(ui.rgb(255, 100, 200)).border(1, ui.rgb(255, 255, 255));

  ui.drawText().text("Check Wu-style AA on edges!").pos(-1, 260).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

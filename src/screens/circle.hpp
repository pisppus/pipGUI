SCREEN(circle, 29)
{
  const uint16_t bg565 = ui.rgb(16, 16, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Circle / RoundRect").pos(-1, 8).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("draw shapes with fill / border").pos(-1, 28).color(ui.rgb(160, 160, 200)).bgColor(bg565).align(Center);

  ui.drawCircle()
      .pos(48, 78)
      .radius(22)
      .fill(ui.rgb(0, 87, 250))
      .border(1, ui.rgb(255, 255, 255));

  ui.drawRect()
      .pos(88, 52)
      .size(140, 44)
      .fill(ui.rgb(80, 255, 120))
      .radius({12});

  ui.drawRect()
      .pos(88, 104)
      .size(140, 44)
      .fill(ui.rgb(255, 128, 0))
      .radius({20, 6, 20, 6});

  ui.drawRect()
      .pos(12, 160)
      .size(100, 44)
      .border(1, ui.rgb(255, 255, 255))
      .radius({12});

  ui.drawRect()
      .pos(12, 214)
      .size(100, 44)
      .border(1, ui.rgb(255, 255, 255))
      .radius({20, 6, 20, 6});

  ui.drawEllipse()
      .pos(175, 190)
      .radiusX(46)
      .radiusY(20)
      .fill(ui.rgb(255, 0, 72));

  ui.drawArc()
      .pos(175, 250)
      .radius(24)
      .startDeg(-90.0f)
      .endDeg(180.0f)
      .color(ui.rgb(255, 255, 255));

  ui.drawLine()
      .from(120, 270)
      .to(230, 300)
      .color(ui.rgb(255, 255, 255));

  ui.setTextStyle(Caption);
  ui.drawText().text("circle fill+border").pos(48, 108).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("rect fill r=12").pos(158, 46).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("rect fill {20,6,20,6}").pos(158, 98).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("rect border r=12").pos(62, 154).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("rect border {20,6,20,6}").pos(62, 208).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("ellipse fill").pos(175, 216).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("drawArc").pos(175, 276).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("drawLine").pos(175, 304).color(ui.rgb(180, 180, 200)).bgColor(bg565).align(Center);
  ui.drawText().text("Next/Prev: change screen").pos(-1, 235).color(ui.rgb(120, 120, 140)).bgColor(bg565).align(Center);
}

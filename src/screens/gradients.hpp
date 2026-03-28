SCREEN(gradients, 25)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);
  ui.setTextStyle(Caption);
  ui.drawText().text("Gradients / Alpha").pos(-1, 8).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);

  char buf[64];
  uint32_t tStart, tElapsed;

  tStart = micros();
  ui.gradientVertical()
      .pos(10, 30)
      .size(140, 60)
      .TColor(ui.rgb(255, 0, 72))
      .BColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "V:%luus", tElapsed);
  ui.drawText().text(buf).pos(80, 65).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  tStart = micros();
  ui.gradientHorizontal()
      .pos(170, 30)
      .size(140, 60)
      .LColor(ui.rgb(255, 128, 0))
      .RColor(ui.rgb(80, 255, 120));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "H:%luus", tElapsed);
  ui.drawText().text(buf).pos(240, 65).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  tStart = micros();
  ui.gradientCorners()
      .pos(10, 100)
      .size(140, 60)
      .TLColor(ui.rgb(255, 0, 72))
      .TRColor(ui.rgb(0, 87, 250))
      .BLColor(ui.rgb(80, 255, 120))
      .BRColor(ui.rgb(255, 128, 0));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "C4:%luus", tElapsed);
  ui.drawText().text(buf).pos(80, 135).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  tStart = micros();
  ui.gradientDiagonal()
      .pos(170, 100)
      .size(140, 60)
      .TLColor(ui.rgb(255, 255, 255))
      .BRColor(ui.rgb(40, 40, 40));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "D:%luus", tElapsed);
  ui.drawText().text(buf).pos(240, 135).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  tStart = micros();
  ui.gradientRadial()
      .pos(10, 170)
      .size(140, 60)
      .center(80, 200)
      .radius(48)
      .innerColor(ui.rgb(255, 255, 255))
      .outerColor(ui.rgb(0, 87, 250));
  tElapsed = micros() - tStart;
  snprintf(buf, sizeof(buf), "R:%luus", tElapsed);
  ui.drawText().text(buf).pos(80, 205).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  tStart = micros();
  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .TColor(ui.rgb(20, 20, 20))
      .BColor(ui.rgb(20, 20, 20));
  tElapsed = micros() - tStart;

  ui.gradientVertical()
      .pos(170, 170)
      .size(140, 60)
      .TColor(ui.rgb(20, 20, 20))
      .BColor(ui.rgb(60, 60, 60));
  snprintf(buf, sizeof(buf), "V2:%luus", micros() - tStart);
  ui.drawText().text(buf).pos(240, 205).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
}

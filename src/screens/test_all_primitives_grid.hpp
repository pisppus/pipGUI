SCREEN(testAllPrimitivesGrid, 35)
{
  const uint16_t bg565 = ui.rgb(16, 16, 16);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("All Primitives Grid").pos(-1, 4).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  const int16_t cols = 4;
  const int16_t rows = 5;
  const int16_t cellW = 60;
  const int16_t cellH = 58;
  const int16_t startX = 0;
  const int16_t startY = 28;

  const uint16_t colors[] = {
      ui.rgb(0, 87, 250),
      ui.rgb(255, 0, 72),
      ui.rgb(80, 255, 120),
      ui.rgb(255, 128, 0),
      ui.rgb(180, 80, 255),
      ui.rgb(0, 200, 200),
      ui.rgb(200, 200, 80),
      ui.rgb(255, 100, 100),
  };

  int colorIdx = 0;

  for (int row = 0; row < rows; row++)
  {
    for (int col = 0; col < cols; col++)
    {
      int16_t x = startX + col * cellW + cellW / 2;
      int16_t y = startY + row * cellH + cellH / 2;
      const uint16_t c = colors[colorIdx % 8];
      const uint16_t white = ui.rgb(255, 255, 255);
      colorIdx++;

      const int type = (row * cols + col) % 8;

      if (type == 0)
      {
        ui.fillCircle().pos(x, y).radius(14).color(c);
        ui.drawCircle().pos(x, y).radius(14).color(white);
      }
      else if (type == 1)
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({6}).color(white);
      }
      else if (type == 2)
      {
        ui.fillEllipse().pos(x, y).radiusX(16).radiusY(10).color(c);
        ui.drawEllipse().pos(x, y).radiusX(16).radiusY(10).color(white);
      }
      else if (type == 3)
      {
        ui.fillTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(c);
        ui.drawTriangle().point0(x, y - 12).point1(x + 14, y + 10).point2(x - 14, y + 10).radius(0).color(white);
      }
      else if (type == 4)
      {
        ui.drawArc().pos(x, y).radius(14).startDeg(0).endDeg(270).color(c);
      }
      else if (type == 5)
      {
        ui.fillRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(c);
        ui.drawRect().pos(x - 18, y - 12).size(36, 24).radius({8, 3, 8, 3}).color(white);
      }
      else if (type == 6)
      {
        ui.fillEllipse().pos(x, y).radiusX(8).radiusY(16).color(c);
        ui.drawEllipse().pos(x, y).radiusX(8).radiusY(16).color(white);
      }
      else if (type == 7)
      {
        ui.fillTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(c);
        ui.drawTriangle().point0(x - 16, y - 4).point1(x + 16, y + 4).point2(x - 16, y + 8).radius(0).color(white);
      }
    }
  }

  ui.drawText().text("All primitives with AA comparison").pos(-1, 318).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

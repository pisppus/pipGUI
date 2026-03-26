SCREEN(autoTextColor, 38)
{
  const uint16_t canvas = ui.rgb(24, 24, 24);
  ui.clear(canvas);

  ui.setTextStyle(H2);
  
  ui.drawText().text("Auto Text Color Test")
  .pos(-1, 4)
  .color(ui.rgb(255, 255, 255))
  .bgColor(canvas)
  .align(Center);

  const uint16_t testColors[] = {
      ui.rgb(255, 255, 255),
      ui.rgb(224, 224, 224),
      ui.rgb(192, 192, 192),
      ui.rgb(96, 96, 96),
      ui.rgb(0, 0, 0),
      ui.rgb(255, 0, 0),
      ui.rgb(0, 255, 0),
      ui.rgb(0, 0, 255),
      ui.rgb(255, 255, 0),
      ui.rgb(0, 255, 255),
      ui.rgb(255, 0, 255),
      ui.rgb(255, 128, 0),
      ui.rgb(128, 0, 255),
      ui.rgb(255, 64, 128),
      ui.rgb(0, 160, 120),
      ui.rgb(120, 80, 32),
      ui.rgb(32, 96, 180),
      ui.rgb(220, 40, 120),
      ui.rgb(160, 220, 60),
      ui.rgb(40, 40, 40),
  };

  const char *labels[] = {
      "White", "Silver", "Gray", "Dim", "Black",
      "Red", "Green", "Blue", "Yellow", "Cyan",
      "Pink", "Orange", "Purple", "Rose", "Teal",
      "Brown", "Steel", "Berry", "Lime", "Char"};

  constexpr int count = (int)(sizeof(testColors) / sizeof(testColors[0]));
  constexpr int cols = 4;
  constexpr int rows = (count + cols - 1) / cols;
  const int16_t pad = 4;
  const int16_t startX = 4;
  const int16_t startY = 30;
  const int16_t footerY = 310;
  const int16_t usableH = (int16_t)(footerY - startY - pad);
  const int16_t cellW = (int16_t)((ui.screenWidth() - startX * 2 - pad * (cols - 1)) / cols);
  const int16_t cellH = (int16_t)((usableH - pad * (rows - 1)) / rows);

  for (int i = 0; i < count; ++i)
  {
    const int col = i % cols;
    const int row = i / cols;
    const int16_t x = (int16_t)(startX + col * (cellW + pad));
    const int16_t y = (int16_t)(startY + row * (cellH + pad));
    const uint16_t bg = testColors[i];
    const uint16_t fg = detail::autoTextColor(bg, 128);

    ui.drawRect()
    .pos(x, y)
    .size(cellW, cellH)
    .fill(bg);

    ui.setTextStyle(Body);

    ui.drawText()
    .text(labels[i])
    .pos((int16_t)(x + cellW / 2), (int16_t)(y + cellH / 2 - 6))
    .color(fg)
    .bgColor(bg)
    .align(Center);
  }

  ui.setTextStyle(Caption);
  ui.drawText()
  .text("BT.709 auto text color on 20 backgrounds")
  .pos(-1, footerY)
  .color(ui.rgb(180, 180, 180))
  .bgColor(canvas)
  .align(Center);
}

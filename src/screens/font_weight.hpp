SCREEN(fontWeight, 26)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();

  ui.setTextStyle(H2);
  ui.drawText().text("Font Weight Test").pos((int16_t)(w / 2), 10).color(ui.rgb(255, 255, 0)).bgColor(bg565).align(Center);

  const String sample = "The quick brown fox";
  const uint16_t weights[] = {100, 300, 400, 500, 600, 700, 900};
  const char *labels[] = {
      "Thin 100",
      "Light 300",
      "Regular 400",
      "Medium 500",
      "SemiBold 600",
      "Bold 700",
      "Black 900",
  };

  int16_t y = 35;
  const int16_t spacing = 38;

  for (int i = 0; i < 7; ++i)
  {
    ui.drawText()
        .text(labels[i])
        .pos(8, y)
        .font(ui.fontId(), 12)
        .weight(Regular)
        .color(ui.rgb(150, 150, 150))
        .bgColor(bg565);

    ui.drawText()
        .text(sample)
        .pos(8, (int16_t)(y + 14))
        .font(ui.fontId(), 20)
        .weight(weights[i])
        .color(ui.rgb(255, 255, 255))
        .bgColor(bg565);

    y += spacing;
  }
}

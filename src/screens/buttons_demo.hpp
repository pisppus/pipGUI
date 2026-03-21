SCREEN(buttonsDemo, 42)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText()
      .text("Buttons")
      .pos(-1, 20)
      .color(ui.rgb(220, 220, 220))
      .bgColor(bg565)
      .align(Center);

  ui.setTextStyle(Caption);
  ui.drawText()
      .text("Next: style  Prev: size")
      .pos(-1, 44)
      .color(ui.rgb(150, 150, 150))
      .bgColor(bg565)
      .align(Center);
  ui.drawText()
      .text("Hold both: back")
      .pos(-1, 58)
      .color(ui.rgb(120, 120, 120))
      .bgColor(bg565)
      .align(Center);
}

SCREEN(sliderDemo, 43)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText()
      .text("Slider")
      .pos(-1, 20)
      .color(ui.rgb(220, 220, 220))
      .bgColor(bg565)
      .align(Center);

  ui.setTextStyle(Caption);
  ui.drawText()
      .text("Next/Prev adjust  Hold both: back")
      .pos(-1, 44)
      .color(ui.rgb(130, 130, 130))
      .bgColor(bg565)
      .align(Center);
}

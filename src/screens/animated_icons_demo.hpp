SCREEN(animatedIconsDemo, 44)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText()
      .text("Animated icons")
      .pos(-1, 20)
      .color(ui.rgb(220, 220, 220))
      .bgColor(bg565)
      .align(Center);

  ui.setTextStyle(Caption);
  ui.drawText()
      .text("Single animated icon preview")
      .pos(-1, 46)
      .color(ui.rgb(140, 140, 140))
      .bgColor(bg565)
      .align(Center);
  ui.drawText()
      .text("Hold both buttons: back")
      .pos(-1, 60)
      .color(ui.rgb(110, 110, 110))
      .bgColor(bg565)
      .align(Center);
}

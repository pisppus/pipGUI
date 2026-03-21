SCREEN(scrollDots, 14)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText().text("Scroll dots").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("15 dots (tapering)").pos(-1, 48).color(ui.rgb(120, 120, 120)).bgColor(bg565).align(Center);
}

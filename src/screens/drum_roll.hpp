SCREEN(drumRoll, 27)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText().text("Drum roll").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("Next: H  Prev: V").pos(-1, 48).color(ui.rgb(120, 120, 120)).bgColor(bg565).align(Center);

  ui.drawDrumRoll()
      .pos(0, 75)
      .size(ui.screenWidth(), 28)
      .options(18, g_drumOptionsH)
      .selected(g_drumSelectedH)
      .fgColor(ui.rgb(255, 255, 255))
      .bgColor(ui.rgb(8, 8, 8))
      ;

  ui.drawDrumRoll()
      .pos(ui.screenWidth() - 80, 120)
      .size(70, 90)
      .options(14, g_drumOptionsV)
      .selected(g_drumSelectedV)
      .fgColor(ui.rgb(200, 200, 200))
      .bgColor(ui.rgb(8, 8, 8))
      .vertical();
}

SCREEN(drumRoll, 27)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText().text("Drum roll").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("Next: H  Prev: V").pos(-1, 48).color(ui.rgb(120, 120, 120)).bgColor(bg565).align(Center);

  String optsH[g_drumCountH] = {
      String(g_drumOptionsH[0]),
      String(g_drumOptionsH[1]),
      String(g_drumOptionsH[2]),
      String(g_drumOptionsH[3]),
      String(g_drumOptionsH[4]),
  };
  ui.drawDrumRollHorizontal(
      0,
      75,
      ui.screenWidth(),
      28,
      optsH,
      g_drumCountH,
      g_drumSelectedH,
      ui.rgb(255, 255, 255),
      ui.rgb(8, 8, 8),
      18,
      280);

  String optsV[g_drumCountV] = {
      String(g_drumOptionsV[0]),
      String(g_drumOptionsV[1]),
      String(g_drumOptionsV[2]),
  };
  ui.drawDrumRollVertical(
      ui.screenWidth() - 80,
      120,
      70,
      90,
      optsV,
      g_drumCountV,
      g_drumSelectedV,
      ui.rgb(200, 200, 200),
      ui.rgb(8, 8, 8),
      14,
      280);
}

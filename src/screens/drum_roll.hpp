SCREEN(drumRoll, 27)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);
  ui.setTextStyle(H2);
  ui.drawText().text("Drum roll").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
  ui.drawText().text("Next: H  Prev: V").pos(-1, 48).color(ui.rgb(120, 120, 120)).bgColor(bg565).align(Center);

  uint32_t now = millis();
  if (g_drumAnimateH && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateH = false;
  if (g_drumAnimateV && now - g_drumAnimStartMs >= g_drumAnimDurMs)
    g_drumAnimateV = false;

  const float progressH = g_drumAnimateH ? animProgress(now, g_drumAnimStartMs, g_drumAnimDurMs) : 1.0f;
  const float progressV = g_drumAnimateV ? animProgress(now, g_drumAnimStartMs, g_drumAnimDurMs) : 1.0f;

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
      g_drumPrevH,
      progressH,
      ui.rgb(255, 255, 255),
      ui.rgb(8, 8, 8),
      18);

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
      g_drumPrevV,
      progressV,
      ui.rgb(200, 200, 200),
      ui.rgb(8, 8, 8),
      14);
}

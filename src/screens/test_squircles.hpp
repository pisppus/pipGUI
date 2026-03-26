SCREEN(testSquircles, 37)
{
  const uint16_t bg565 = ui.rgb(18, 18, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Squircle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("drawSquircleRect fill / border").pos(-1, 28).color(ui.rgb(200, 200, 220)).bgColor(bg565).align(Center);

  ui.drawSquircleRect().pos(32, 62).size(36, 36).radius({18}).fill(ui.rgb(0, 87, 250)).border(1, ui.rgb(255, 255, 255));

  ui.drawSquircleRect().pos(94, 54).size(52, 52).radius({26}).fill(ui.rgb(255, 0, 72)).border(1, ui.rgb(255, 255, 255));

  ui.drawSquircleRect().pos(166, 46).size(68, 68).radius({34}).fill(ui.rgb(80, 255, 120)).border(1, ui.rgb(255, 255, 255));

  ui.drawText().text("Small r=6..10 (edge case):").pos(95, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.drawSquircleRect().pos(24, 144).size(12, 12).radius({6}).fill(ui.rgb(255, 128, 0)).border(1, ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(47, 142).size(16, 16).radius({8}).fill(ui.rgb(180, 80, 255)).border(1, ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(75, 140).size(20, 20).radius({10}).fill(ui.rgb(0, 200, 200)).border(1, ui.rgb(255, 255, 255));

  ui.drawText().text("Overlay check (blending):").pos(170, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.drawSquircleRect().pos(136, 136).size(48, 48).radius({24}).fill(ui.rgb(0, 87, 250)).border(1, ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(158, 146).size(48, 48).radius({24}).fill(ui.rgb(255, 0, 72)).border(1, ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(116, 146).size(48, 48).radius({24}).fill(ui.rgb(80, 255, 120)).border(1, ui.rgb(255, 255, 255));

  ui.drawText().text("Check AA edges (no dark halos)").pos(-1, 305).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

SCREEN(testSquircles, 37)
{
  const uint16_t bg565 = ui.rgb(18, 18, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Squircle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fillSquircleRect + drawSquircleRect").pos(-1, 28).color(ui.rgb(200, 200, 220)).bgColor(bg565).align(Center);

  ui.fillSquircleRect().pos(32, 62).size(36, 36).radius({18}).color(ui.rgb(0, 87, 250));
  ui.drawSquircleRect().pos(32, 62).size(36, 36).radius({18}).color(ui.rgb(255, 255, 255));

  ui.fillSquircleRect().pos(94, 54).size(52, 52).radius({26}).color(ui.rgb(255, 0, 72));
  ui.drawSquircleRect().pos(94, 54).size(52, 52).radius({26}).color(ui.rgb(255, 255, 255));

  ui.fillSquircleRect().pos(166, 46).size(68, 68).radius({34}).color(ui.rgb(80, 255, 120));
  ui.drawSquircleRect().pos(166, 46).size(68, 68).radius({34}).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Small r=6..10 (edge case):").pos(95, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.fillSquircleRect().pos(24, 144).size(12, 12).radius({6}).color(ui.rgb(255, 128, 0));
  ui.drawSquircleRect().pos(24, 144).size(12, 12).radius({6}).color(ui.rgb(255, 255, 255));
  ui.fillSquircleRect().pos(47, 142).size(16, 16).radius({8}).color(ui.rgb(180, 80, 255));
  ui.drawSquircleRect().pos(47, 142).size(16, 16).radius({8}).color(ui.rgb(255, 255, 255));
  ui.fillSquircleRect().pos(75, 140).size(20, 20).radius({10}).color(ui.rgb(0, 200, 200));
  ui.drawSquircleRect().pos(75, 140).size(20, 20).radius({10}).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Overlay check (blending):").pos(170, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.fillSquircleRect().pos(136, 136).size(48, 48).radius({24}).color(ui.rgb(0, 87, 250));
  ui.fillSquircleRect().pos(158, 146).size(48, 48).radius({24}).color(ui.rgb(255, 0, 72));
  ui.fillSquircleRect().pos(116, 146).size(48, 48).radius({24}).color(ui.rgb(80, 255, 120));

  ui.drawSquircleRect().pos(136, 136).size(48, 48).radius({24}).color(ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(158, 146).size(48, 48).radius({24}).color(ui.rgb(255, 255, 255));
  ui.drawSquircleRect().pos(116, 146).size(48, 48).radius({24}).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Check AA edges (no dark halos)").pos(-1, 305).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

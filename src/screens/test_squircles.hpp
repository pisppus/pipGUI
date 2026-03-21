SCREEN(testSquircles, 37)
{
  const uint16_t bg565 = ui.rgb(18, 18, 24);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Squircle Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("fillSquircle + drawSquircle (AA + aaGamma)").pos(-1, 28).color(ui.rgb(200, 200, 220)).bgColor(bg565).align(Center);

  ui.fillSquircle().pos(50, 80).radius(18).color(ui.rgb(0, 87, 250));
  ui.drawSquircle().pos(50, 80).radius(18).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 0, 72));
  ui.drawSquircle().pos(120, 80).radius(26).color(ui.rgb(255, 255, 255));

  ui.fillSquircle().pos(200, 80).radius(34).color(ui.rgb(80, 255, 120));
  ui.drawSquircle().pos(200, 80).radius(34).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Small r=6..10 (edge case):").pos(95, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.fillSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 128, 0));
  ui.drawSquircle().pos(30, 150).radius(6).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(55, 150).radius(8).color(ui.rgb(180, 80, 255));
  ui.drawSquircle().pos(55, 150).radius(8).color(ui.rgb(255, 255, 255));
  ui.fillSquircle().pos(85, 150).radius(10).color(ui.rgb(0, 200, 200));
  ui.drawSquircle().pos(85, 150).radius(10).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Overlay check (blending):").pos(170, 128).color(ui.rgb(200, 180, 160)).bgColor(bg565).align(Center);
  ui.fillSquircle().pos(160, 160).radius(24).color(ui.rgb(0, 87, 250));
  ui.fillSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 0, 72));
  ui.fillSquircle().pos(140, 170).radius(24).color(ui.rgb(80, 255, 120));

  ui.drawSquircle().pos(160, 160).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(182, 170).radius(24).color(ui.rgb(255, 255, 255));
  ui.drawSquircle().pos(140, 170).radius(24).color(ui.rgb(255, 255, 255));

  ui.drawText().text("Check AA edges (no dark halos)").pos(-1, 305).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

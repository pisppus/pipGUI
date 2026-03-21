SCREEN(glow, 2)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.drawGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg565)
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900);

  ui.drawGlowCircle()
      .pos(70, 175)
      .radius(22)
      .fillColor(ui.rgb(80, 255, 120))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(200);

  ui.drawGlowRect()
      .pos(center, 70)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400);

  ui.drawGlowRect()
      .pos(140, 175)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg565)
      .glowSize(16)
      .glowStrength(180);

  ui.setTextStyle(H2);
  ui.drawText().text("Glow demo").pos(-1, 22).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.setTextStyle(Body);
  ui.drawText().text("REC / shapes").pos(-1, 44).color(ui.rgb(200, 200, 200)).bgColor(bg565).align(Center);
}

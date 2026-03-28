SCREEN(glow, 2)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.drawGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulseMs(900);

  ui.drawGlowCircle()
      .pos(70, 175)
      .radius(22)
      .fillColor(ui.rgb(80, 255, 120))
      .glowSize(18)
      .glowStrength(200);

  ui.drawGlowCircle()
      .pos(170, 95)
      .radius(34)
      .fillColor(ui.rgb(80, 150, 255))
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulseMs(1400);

  ui.drawGlowCircle()
      .pos(180, 182)
      .radius(28)
      .fillColor(ui.rgb(255, 180, 0))
      .glowSize(16)
      .glowStrength(180);

  ui.setTextStyle(H2);
  ui.drawText().text("Glow demo").pos(-1, 22).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);
  ui.setTextStyle(Body);
  ui.drawText().text("Circles only").pos(-1, 44).color(ui.rgb(200, 200, 200)).bgColor(bg565).align(Center);
}

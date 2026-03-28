SCREEN(testArcsAndLines, 34)
{
  const uint16_t bg565 = ui.rgb(28, 24, 20);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Arc & Line Test").pos(-1, 6).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);
  ui.setTextStyle(Caption);
  ui.drawText().text("drawArc + drawLine (AA)").pos(-1, 28).color(ui.rgb(220, 200, 180)).bgColor(bg565).align(Center);
  ui.drawText().text("Arcs (various angles):").pos(85, 45).color(ui.rgb(220, 200, 180)).bgColor(bg565).align(Center);
  ui.drawArc().pos(40, 75).radius(18).start(-90).end(0).color(ui.rgb(0, 87, 250));
  ui.drawArc().pos(80, 75).radius(18).start(0).end(90).color(ui.rgb(255, 0, 72));
  ui.drawArc().pos(120, 75).radius(18).start(90).end(180).color(ui.rgb(80, 255, 120));
  ui.drawArc().pos(160, 75).radius(18).start(180).end(270).color(ui.rgb(255, 128, 0));
  ui.drawArc().pos(70, 125).radius(22).start(-90).end(90).color(ui.rgb(180, 80, 255));
  ui.drawArc().pos(130, 125).radius(22).start(90).end(270).color(ui.rgb(0, 200, 200));
  ui.drawArc().pos(180, 125).radius(20).start(0).end(360).color(ui.rgb(200, 200, 80));
  ui.drawArc().pos(200, 75).radius(8).start(45).end(135).color(ui.rgb(255, 100, 100));
  ui.drawArc().pos(215, 75).radius(6).start(200).end(340).color(ui.rgb(100, 255, 200));
  ui.drawText().text("Lines (AA quality):").pos(80, 155).color(ui.rgb(220, 200, 180)).bgColor(bg565).align(Center);
  ui.drawLine().from(20, 175).to(80, 175).color(ui.rgb(255, 255, 255));
  ui.drawLine().from(50, 165).to(50, 185).color(ui.rgb(200, 200, 200));
  ui.drawLine().from(100, 170).to(140, 210).color(ui.rgb(255, 100, 100));
  ui.drawLine().from(140, 170).to(100, 210).color(ui.rgb(100, 255, 100));
  ui.drawLine().from(160, 170).to(165, 220).color(ui.rgb(100, 100, 255));
  ui.drawLine().from(180, 170).to(200, 190).color(ui.rgb(255, 200, 100));
  ui.drawLine().from(20, 230).to(100, 280).color(ui.rgb(0, 200, 255));
  ui.drawLine().from(140, 240).to(220, 300).color(ui.rgb(255, 255, 255));
  ui.drawText().text("Pie arcs:").pos(200, 155).color(ui.rgb(220, 200, 180)).bgColor(bg565).align(Center);
  ui.drawArc().pos(200, 200).radius(25).start(-30).end(30).color(ui.rgb(255, 128, 0));
  ui.drawArc().pos(200, 200).radius(20).start(60).end(120).color(ui.rgb(0, 255, 128));
  ui.drawArc().pos(200, 200).radius(15).start(150).end(210).color(ui.rgb(255, 0, 255));

  ui.drawText().text("Check sqrt_fraction AA!").pos(-1, 305).color(ui.rgb(200, 200, 100)).bgColor(bg565).align(Center);
}

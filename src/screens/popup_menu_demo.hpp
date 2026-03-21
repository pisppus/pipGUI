SCREEN(popupMenuDemo, 41)
{
  const uint16_t bg565 = ui.rgb(0, 0, 0);
  const uint16_t fg565 = ui.rgb(255, 255, 255);
  const uint16_t sub565 = ui.rgb(150, 150, 150);
  const uint16_t accent565 = ui.rgb(0, 87, 250);

  ui.clear(bg565);

  ui.setTextStyle(H1);
  ui.drawText()
      .text("Popup Menu")
      .pos(center, 46)
      .color(fg565)
      .bgColor(bg565)
      .align(Center);

  ui.setTextStyle(Body);
  ui.drawText()
      .text("Next: open / down")
      .pos(center, 104)
      .color(sub565)
      .bgColor(bg565)
      .align(Center);

  ui.drawText()
      .text("Prev: up")
      .pos(center, 124)
      .color(sub565)
      .bgColor(bg565)
      .align(Center);

  ui.drawText()
      .text("Hold Next: select")
      .pos(center, 144)
      .color(sub565)
      .bgColor(bg565)
      .align(Center);

  ui.drawText()
      .text("Hold Prev: close")
      .pos(center, 164)
      .color(sub565)
      .bgColor(bg565)
      .align(Center);

  ui.drawButton()
      .label("Open menu")
      .pos(center, 228)
      .size(180, 32)
      .baseColor(accent565)
      .radius(10);
}

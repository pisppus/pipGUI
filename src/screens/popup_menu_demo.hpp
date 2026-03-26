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
      .pos(center, 6)
      .color(fg565)
      .bgColor(bg565)
      .align(Center);

  ui.setTextStyle(Body);
  auto hintText = ui.drawText()
                      .pos(center, 64)
                      .color(sub565)
                      .bgColor(bg565)
                      .align(Center);
  hintText.derive().text("Next: open / down");
  hintText.derive().text("Prev: up").pos(center, 84);
  hintText.derive().text("Hold Next: select").pos(center, 104);
  hintText.derive().text("Hold Prev: close").pos(center, 124);
  hintText.derive().text("4 visible, then scroll").pos(center, 144);

  ui.drawButton()
      .label("Open menu")
      .pos(center, 188)
      .size(180, 32)
      .baseColor(accent565)
      .radius(10);
}

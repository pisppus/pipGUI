SCREEN(errorOverlay, 15)
{
  const uint16_t bg565 = ui.rgb(8, 8, 8);
  ui.clear(bg565);

  ui.setTextStyle(H2);
  ui.drawText().text("Error demo").pos(-1, 58).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Center);

  ui.setTextStyle(Body);
  ui.drawText().text("Next opens 3 critical errors").pos(-1, 102).color(ui.rgb(178, 178, 184)).bgColor(bg565).align(Center);
  ui.drawText().text("Next / Prev browse entries").pos(-1, 128).color(ui.rgb(178, 178, 184)).bgColor(bg565).align(Center);

  ui.setTextStyle(Caption);
  ui.drawText().text("Critical errors cannot be dismissed").pos(-1, 286).color(ui.rgb(118, 118, 124)).bgColor(bg565).align(Center);
}

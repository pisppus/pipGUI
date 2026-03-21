SCREEN(settings, 1)
{
  const uint16_t bg565 = ui.rgb(0, 31, 31);
  ui.clear(bg565);

  ui.setTextStyle(H1);
  ui.drawText().text("Settings menu").pos(-1, 80).color(color565To888(0xFFFF)).bgColor(bg565).align(Center);

  const String label = settingsButtonLabel(millis());

  ui.drawButton()
      .label(label)
      .pos(60, 20)
      .size(120, 44)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(10)
      .state(settingsBtnState);
}

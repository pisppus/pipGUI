SCREEN(toggleSwitch, 13)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .state(g_toggleState)
      .activeColor(ui.rgb(21, 180, 110));

  ui.setTextStyle(H2);
  ui.drawText().text("ToggleSwitch").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
}

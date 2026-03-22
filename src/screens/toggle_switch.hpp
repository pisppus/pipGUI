SCREEN(toggleSwitch, 13)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const bool toggleEnabled = (g_toggleLockedUntil == 0 || millis() >= g_toggleLockedUntil);
  ui.clear(bg565);

  ui.setTextStyle(Body);
  auto statusText = ui.drawText()
                        .pos(-1, 105)
                        .color(ui.rgb(200, 200, 200))
                        .bgColor(bg565)
                        .align(Center);
  statusText.derive().text(g_toggleValue ? (toggleEnabled ? "ON" : "ON...")
                                         : "OFF");

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .value(g_toggleValue)
      .enabled(toggleEnabled)
      .activeColor(ui.rgb(21, 180, 110));

  ui.setTextStyle(H2);
  ui.drawText().text("Toggle switch").pos(-1, 24).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);
}

SCREEN(tileMenu, 8)
{
  ui.updateTile()
      .items(
          tileItem("Main", "Home screen", mainMenu),
          tileItem("Settings", "Alerts and controls", settings),
          tileItem(battery_l0, "Glow demo", "Glow shapes", glow),
          tileItem(battery_l1, "Graph", "Live graphs", graph),
          tileItem(battery_l2, "Toggle", "Switch", toggleSwitch))
      .inactive(ui.rgb(8, 8, 8))
      .active(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .columns(2)
      .tileSize(100, 70)
      .mode(TextSubtitle);
}

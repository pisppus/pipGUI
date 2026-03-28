SCREEN(tileMenu4Cols, 12)
{
  ui.updateTile()
      .items(
          tileItem("Back", "", 7),
          tileItem(battery_l0, "1", "", mainMenu),
          tileItem(battery_l1, "2", "", settings),
          tileItem(battery_l2, "3", "", glow),
          tileItem("4", "", graph),
          tileItem("5", "", graphSmall),
          tileItem(battery_l0, "6", "", graphTall),
          tileItem(battery_l1, "7", "", progress),
          tileItem("8", "", 9))
      .inactive(ui.rgb(8, 8, 8))
      .active(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(8)
      .columns(4)
      .mode(TextOnly);
}

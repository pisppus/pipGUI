SCREEN(tileMenuLayout, 11)
{
  ui.updateTile()
      .grid(2, 3)
      .tile("Back", "", 7)
      .at(0, 0)
      .span(2, 1)
      .tile(battery_l0, "Small 1", "", mainMenu)
      .at(0, 1)
      .tile(battery_l1, "Small 2", "", settings)
      .at(1, 1)
      .tile("Big", "Main", glow)
      .at(1, 2)
      .inactive(ui.rgb(8, 8, 8))
      .active(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .mode(TextOnly);
}

SCREEN(listMenuPlain, 10)
{
  ui.clear(0x0000);
  ui.updateList()
      .items(
          listItem("Back", "Return to the card-style list menu", 7),
          listItem("Main screen with widgets and overlays", "Simple home screen with status bar, overlays, and transitions", mainMenu),
          listItem("Settings, alerts, and input handling", "Buttons, notifications, warning states, and input handling", settings),
          listItem("Glow demo with blur, bloom, and soft shapes", "Bloom, soft edges, layered primitives, and bright composition", glow),
          listItem("Graph osc", "Live full-buffer oscilloscope preview", 9),
          listItem("Tile menu", "Grid menu with icons, title, and subtitle", 8),
          listItem("ToggleSwitch", "Toggle switch interaction and state preview", toggleSwitch),
          listItem("Slider", "Settings-style slider with animated thumb", sliderDemo),
          listItem("Animated icons", "Lottie JSON compiled into scalable animated PSDF icons", animatedIconsDemo),
          listItem("Screenshots", "Captured frames gallery", screenshotGallery),
          listItem("Font compare", "PSDF", fontCompare))
      .inactive(ui.rgb(8, 8, 8))
      .active(ui.rgb(21, 54, 140))
      .cardSize(310, 34)
      .mode(Plain);
}

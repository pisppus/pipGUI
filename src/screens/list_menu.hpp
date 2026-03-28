SCREEN(listMenu, 7)
{
  ui.clear(0x0000);
  ui.updateList()
      .items(
          listItem("Main screen with widgets", "Simple home screen with status bar, overlays, and transitions", mainMenu),
          listItem("Settings and alerts preview", "Buttons, notifications, warning states, and input handling", settings),
          listItem("Glow demo with blur and bloom", "Bloom, soft edges, layered primitives, and bright composition", glow),
          listItem("Graph demo with multiple traces", "Grid, labels, and several moving lines in one viewport", graph),
          listItem("Graph small", "Compact graph with automatic scale fitting", graphSmall),
          listItem("Graph tall", "Tall graph layout with right-side emphasis", graphTall),
          listItem("Graph osc", "Live full-buffer oscilloscope preview", 9),
          listItem("Progress demo", "Progress bars with animated fill and labels", progress),
          listItem("Progress + text", "Fresh progress styles with text overlays", progressText),
          listItem("Popup menu", "Overlay menu with button-driven navigation and selection", popupMenuDemo),
          listItem("Tile menu with icon cards", "Grid menu with icons, title, subtitle, and active marquee", tileMenu),
          listItem("Tile layout", "Custom tile layout with merged cells", 11),
          listItem("Tile 4 cols", "Four compact tiles in a single row", 12),
          listItem("List menu 2", "Text-focused list with active highlight panel", 10),
          listItem("ToggleSwitch", "Toggle switch interaction and state preview", toggleSwitch),
          listItem("Buttons", "Different button forms, sizes and states", buttonsDemo),
          listItem("Slider", "Settings-style slider with animated thumb", sliderDemo),
          listItem("Animated icons", "Lottie JSON compiled into scalable animated PSDF icons", animatedIconsDemo),
          listItem("Scroll dots", "Page indicator dots with animated transitions", scrollDots),
          listItem("Error", "Full-screen error overlay presentation", errorOverlay),
          listItem("Warning", "Full-screen warning overlay presentation", warningOverlay),
          listItem("Blur strip", "Material-style blur strip with moving content", blur),
          listItem("Screenshots", "Captured frames gallery", screenshotGallery),
          listItem("Firmware update", "WiFi OTA updater (signed manifest)", firmwareUpdate),
          listItem("Primitives", "Circle / Arc / Ellipse / Triangle / Squircle", primitives),
          listItem("Gradients", "All gradient types", gradients),
          listItem("Font compare", "PSDF", fontCompare),
          listItem("Font weights", "Test all weights", fontWeight),
          listItem("Drum roll", "Horizontal + vertical picker", drumRoll),
          listItem("Circle AA", "Gupta-Sproull optimized", circle),
          listItem("Test: Circles", "drawCircle fill / border", testCircles),
          listItem("Test: RoundRects", "1 & 4 radius variants", testRoundRects),
          listItem("Test: Ellipses", "Wu-style AA ellipses", testEllipses),
          listItem("Test: Triangles", "4x subpixel AA triangles", testTriangles),
          listItem("Test: Arcs+Lines", "sqrt_fraction AA", testArcsAndLines),
          listItem("Test: All Grid", "All primitives comparison", testAllPrimitivesGrid),
          listItem("Test: RoundTriangles", "SDF AA rounded triangles", testRoundTriangles),
          listItem("Test: Squircles", "fill/draw squircle AA", testSquircles),
          listItem("Auto text color", "BT.709 luminance test", autoTextColor))
      .inactive(ui.rgb(8, 8, 8))
      .active(ui.rgb(21, 54, 140))
      .cardSize(310, 50);
}

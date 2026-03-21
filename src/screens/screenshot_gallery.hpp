SCREEN(screenshotGallery, 39)
{
  ui.clear(ui.rgb(10, 10, 10));
  ui.drawScreenshot()
      .pos(8, 28)
      .size(224, 284)
      .grid(3, 5)
      .padding(8);
}

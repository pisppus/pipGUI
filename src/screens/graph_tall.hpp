SCREEN(graphTall, 5)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 160, 180, 10, RightToLeft, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(true);
  ui.drawGraphLine(0, g_graphV1, ui.rgb(255, 80, 80), -110, 110);
  ui.drawGraphLine(1, g_graphV2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, g_graphV3, ui.rgb(100, 160, 255), -110, 110);
}

SCREEN(graph, 3)
{
  ui.clear(0x0000);
  ui.drawGraphGrid().pos(center, center).size(280, 170).radius(13).direction(LeftToRight).bgColor(ui.rgb(8, 8, 8)).speed(1.0f);
  ui.drawGraphLine().line(0).value(g_graphV1).color(ui.rgb(255, 80, 80)).range(-110, 110);
  ui.drawGraphLine().line(1).value(g_graphV2).color(ui.rgb(80, 255, 120)).range(-110, 110);
  ui.drawGraphLine().line(2).value(g_graphV3).color(ui.rgb(100, 160, 255)).range(-110, 110);
}

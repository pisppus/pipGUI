SCREEN(graphOsc, 9)
{
  ui.clear(0x0000);
  ui.drawGraphGrid(center, center, 200, 170, 13, Oscilloscope, ui.rgb(8, 8, 8), ui.rgb(20, 20, 20), 1.0f);
  ui.setGraphAutoScale(false);

  float tBase = g_graphPhase;
  float slow = sinf(tBase * 0.05f);
  float offset = slow * 40.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(tBase * 0.3f) * 20.0f;
  float t = tBase;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  ui.drawGraphLine(1, (int16_t)v2, ui.rgb(80, 255, 120), -110, 110);
  ui.drawGraphLine(2, (int16_t)v3, ui.rgb(100, 160, 255), -110, 110);
}

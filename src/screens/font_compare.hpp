void updateDitherCompareDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (prevPressed)
  {
    g_ditherCompareFrozen = !g_ditherCompareFrozen;
    if (g_ditherCompareFrozen)
    {
      float tcap = (float)nowMs / 1000.0f;
      g_ditherCompareFrozenR = (uint8_t)((sinf(tcap * 0.9f) * 0.5f + 0.5f) * 255.0f);
      g_ditherCompareFrozenG = (uint8_t)((sinf(tcap * 1.1f + 1.0f) * 0.5f + 0.5f) * 255.0f);
      g_ditherCompareFrozenB = (uint8_t)((sinf(tcap * 1.3f + 2.0f) * 0.5f + 0.5f) * 255.0f);
    }
    ui.requestRedraw();
  }

  if (nextPressed)
    ui.nextScreen();
}

void updateGraphDemo()
{
  float t = g_graphPhase;
  float slow = sinf(t * 0.05f);
  float offset = slow * 40.0f;
  float amp1 = 40.0f + slow * 20.0f;
  float amp2 = 35.0f - slow * 15.0f;
  float amp3 = 50.0f + sinf(t * 0.3f) * 20.0f;

  float v1 = offset + sinf(t) * amp1;
  float v2 = offset + sinf(t * 0.5f + 1.0f) * amp2;
  float v3 = offset + (sinf(t * 1.3f) * 0.6f + sinf(t * 0.2f + 2.0f) * 0.4f) * amp3;

  g_graphV1 = (int16_t)v1;
  g_graphV2 = (int16_t)v2;
  g_graphV3 = (int16_t)v3;

  g_graphPhase += 0.12f;
  if (g_graphPhase > 1000.0f)
    g_graphPhase -= 1000.0f;
}

SCREEN(fontCompare, 24)
{
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  ui.clear(bg565);

  const int16_t w = (int16_t)ui.screenWidth();

  ui.setTextStyle(H2);
  ui.drawText().text("PSDF font demo").pos((int16_t)(w / 2), 20).color(ui.rgb(220, 220, 220)).bgColor(bg565).align(Center);

  const String sample = "The quick brown fox";
  const uint16_t sizes[4] = {18, 24, 36, 48};
  const int16_t y0 = 50;
  const int16_t spacing = 42;

  for (int i = 0; i < 4; ++i)
  {
    ui.setFontSize(sizes[i]);
    const int16_t ty = (int16_t)(y0 + i * spacing);
    ui.drawText().text(String("PSDF ") + String(sizes[i]) + "px").pos(8, ty).color(ui.rgb(255, 255, 255)).bgColor(bg565).align(Left);
    ui.drawText().text(sample).pos(8, (int16_t)(ty + 18)).color(ui.rgb(200, 200, 200)).bgColor(bg565).align(Left);
  }
}

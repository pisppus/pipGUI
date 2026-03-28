SCREEN(graphOsc, 9)
{
  ui.clear(0x0000);
  constexpr uint16_t kOscRateHz = 320;
  constexpr uint16_t kOscTimebaseMs = 700;
  constexpr uint16_t kMaxOscSamples = 256;
  const uint16_t vsRaw = (uint16_t)(((uint32_t)kOscRateHz * (uint32_t)kOscTimebaseMs + 999U) / 1000U);
  const uint16_t sampleCount = (vsRaw < 2) ? 2 : ((vsRaw > kMaxOscSamples) ? kMaxOscSamples : vsRaw);
  ui.setTextStyle(Caption);
  ui.drawText().text("Oscilloscope").pos(-1, 26).color(ui.rgb(180, 220, 180)).bgColor(0x0000).align(Center);
  ui.drawText().text("Live full-buffer mode").pos(-1, 42).color(ui.rgb(100, 120, 100)).bgColor(0x0000).align(Center);
  ui.drawGraphGrid().pos(center, center).size(200, 170).radius(13).direction(Oscilloscope).bgColor(ui.rgb(8, 8, 8)).speed(1.0f).scale(false).scope(kOscRateHz, kOscTimebaseMs).visible(0);

  ui.drawText().text(String(kOscRateHz) + "Hz  " + String(kOscTimebaseMs) + "ms  " + String(sampleCount) + " samples")
      .pos(-1, 58).color(ui.rgb(120, 150, 120)).bgColor(0x0000).align(Center);
  static int16_t greenTrace[kMaxOscSamples];
  static int16_t blueTrace[kMaxOscSamples];
  const float tBase = g_graphPhase;
  const float slow = sinf(tBase * 0.05f);
  const float offset = slow * 40.0f;
  const float amp2 = 35.0f - slow * 15.0f;
  const float amp3 = 50.0f + sinf(tBase * 0.3f) * 20.0f;
  const float dt = (kOscRateHz > 0) ? (1.0f / (float)kOscRateHz) : 0.01f;
  const float baseSec = tBase * 0.016f;

  for (uint16_t i = 0; i < sampleCount; ++i)
  {
    const float sampleT = baseSec - (float)(sampleCount - 1 - i) * dt;
    greenTrace[i] = (int16_t)(offset + sinf(sampleT * 3.2f + 1.0f) * amp2);
    blueTrace[i] = (int16_t)(offset + (sinf(sampleT * 8.3f) * 0.6f + sinf(sampleT * 1.2f + 2.0f) * 0.4f) * amp3);
  }

  ui.drawGraphSamples().line(0).samples(greenTrace, sampleCount).color(ui.rgb(80, 255, 120)).range(-110, 110);
  ui.drawGraphSamples().line(1).samples(blueTrace, sampleCount).color(ui.rgb(100, 160, 255)).range(-110, 110);
}

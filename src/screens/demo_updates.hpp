#pragma once

#include <pipKit.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <pipGUI/Systems/Update/Ota.hpp>

using namespace pipgui;

namespace
{
  constexpr const char *kPopupMenuDemoItems[] = {
      "Open",
      "Rename",
      "Duplicate",
      "Share",
      "Archive",
      "Delete",
  };
  constexpr uint8_t kPopupMenuDemoItemCount = static_cast<uint8_t>(sizeof(kPopupMenuDemoItems) / sizeof(kPopupMenuDemoItems[0]));
}

void updateGlowDemoFrame(uint32_t nowMs)
{
  static uint32_t lastGlowRedrawMs = 0;
  if (nowMs - lastGlowRedrawMs < kFrameStepMs)
    return;

  lastGlowRedrawMs = nowMs;
  const uint16_t bg565 = ui.rgb(10, 10, 10);

  ui.updateGlowCircle()
      .pos(70, 95)
      .radius(28)
      .fillColor(ui.rgb(255, 40, 40))
      .bgColor(bg565)
      .glowSize(20)
      .glowStrength(240)
      .anim(Pulse)
      .pulsePeriodMs(900);

  ui.updateGlowRect()
      .pos(center, 70)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(80, 150, 255))
      .bgColor(bg565)
      .glowSize(18)
      .glowStrength(220)
      .anim(Pulse)
      .pulsePeriodMs(1400);

  ui.updateGlowRect()
      .pos(140, 175)
      .size(150, 78)
      .radius(18)
      .fillColor(ui.rgb(255, 180, 0))
      .bgColor(bg565)
      .glowSize(16)
      .glowStrength(180);
}

void updateBlurDemoFrame(uint32_t nowMs)
{
  if (nowMs - g_blurLastUpdateMs < 1)
    return;

  g_blurLastUpdateMs = nowMs;
  g_blurPhase += 0.10f;
  if (g_blurPhase > 1000.0f)
    g_blurPhase -= 1000.0f;

  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const int16_t w = (int16_t)ui.screenWidth();
  const int16_t bandY = kStatusBarHeight;
  const int16_t bandH = 80;
  BlurRect row1[kBlurRectCount];
  BlurRect row2[kBlurRectCount];

  ui.fillRect()
      .pos(0, bandY)
      .size(w, bandH)
      .color(bg565);

  buildBlurRow(g_blurPhase, w, bandY + 15, 22, 18, row1);
  drawBlurRow(ui, row1);

  if (g_blurRow2Trail.initialized)
    clearBlurRow(ui, g_blurRow2Trail.rects, bg565);

  buildBlurRow(g_blurPhase, w, bandY + bandH + 10, 18, 16, row2);
  drawBlurRow(ui, row2);
  for (uint8_t i = 0; i < kBlurRectCount; ++i)
    g_blurRow2Trail.rects[i] = row2[i];
  g_blurRow2Trail.initialized = true;

  ui.updateBlur()
      .pos(0, bandY)
      .size(w, bandH)
      .radius(10)
      .direction(TopDown)
      .gradient(false)
      .materialStrength(0)
      .materialColor(ui.rgb(10, 10, 10));
}

void updateGraphScreen(uint8_t screenId, uint32_t nowMs)
{
  if (nowMs - g_lastGraphUpdateMs < kFrameStepMs)
    return;

  g_lastGraphUpdateMs = nowMs;
  updateGraphDemo();

  if (screenId != graphOsc)
    ui.updateGraphLine().line(0).value(g_graphV1).color(ui.rgb(255, 80, 80)).range(-110, 110);

  ui.updateGraphLine().line(1).value(g_graphV2).color(ui.rgb(80, 255, 120)).range(-110, 110);
  ui.updateGraphLine().line(2).value(g_graphV3).color(ui.rgb(100, 160, 255)).range(-110, 110);
}

void updateProgressDemoFrame(uint32_t nowMs)
{
  if (nowMs - g_lastProgressUpdateMs < kFrameStepMs)
    return;

  g_lastProgressUpdateMs = nowMs;
  stepPingPong(g_progressValue, g_progressDirDown);
  const uint16_t bg565 = ui.rgb(0, 0, 0);
  const String valueRaw = String(g_progressValue);
  const String valuePercent = valueRaw + "%";
  const String valueFraction = String((float)g_progressValue / 100.0f, 2);
  const String valueRange = valueRaw + "/100";

  ui.updateProgress()
      .pos(center, 60)
      .size(200, 10)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .radius(6)
      .label("Indeterminate")
      .labelColor(ui.rgb(255, 255, 255))
      .percent()
      .percentColor(ui.rgb(200, 200, 200))
      .anim(Indeterminate);

  ui.updateProgress()
      .pos(center, 74)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .radius(6)
      .label("Determinate")
      .labelColor(ui.rgb(255, 255, 255))
      .percent()
      .percentColor(ui.rgb(200, 200, 200))
      .anim(None);

  ui.updateProgress()
      .pos(center, 88)
      .size(200, 10)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .radius(6)
      .label("Shimmer")
      .labelColor(ui.rgb(255, 255, 255))
      .percent()
      .percentColor(ui.rgb(200, 200, 200))
      .anim(Shimmer);

  ui.updateCircleProgress()
      .pos(50, 165)
      .radius(22)
      .thickness(8)
      .value(0)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 87, 250))
      .anim(Indeterminate);

  ui.updateCircleProgress()
      .pos(105, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 0, 72))
      .anim(None);

  ui.updateCircleProgress()
      .pos(160, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(255, 128, 0))
      .anim(Shimmer);

  ui.updateCircleProgress()
      .pos(215, 165)
      .radius(22)
      .thickness(8)
      .value(g_progressValue)
      .baseColor(ui.rgb(10, 10, 10))
      .fillColor(ui.rgb(0, 200, 120))
      .anim(Shimmer);

  ui.updateText()
      .pos(50, 200)
      .text(valueRaw)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .bgColor(bg565)
      .align(Center);

  ui.updateText()
      .pos(105, 200)
      .text(valuePercent)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .bgColor(bg565)
      .align(Center);

  ui.updateText()
      .pos(160, 200)
      .text(valueFraction)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .bgColor(bg565)
      .align(Center);

  ui.updateText()
      .pos(215, 200)
      .text(valueRange)
      .size(12)
      .color(ui.rgb(180, 180, 180))
      .bgColor(bg565)
      .align(Center);
}

void updateSettingsDemoFrame(uint32_t nowMs, bool buttonPressed, bool buttonDown)
{
  static bool settingsAwaitRelease = true;
  if (settingsAwaitRelease)
  {
    if (!buttonDown)
      settingsAwaitRelease = false;
  }

  bool loading = (g_settingsLoadingUntil != 0 && nowMs < g_settingsLoadingUntil);
  if (g_settingsLoadingUntil != 0 && nowMs >= g_settingsLoadingUntil)
    g_settingsLoadingUntil = 0;

  if (!settingsAwaitRelease && buttonPressed && !loading)
  {
    g_settingsLoadingUntil = nowMs + kSettingsLoadingDurationMs;
    loading = true;
    ui.showNotification()
        .title("Sync paused")
        .message("Cloud sync is paused.\nConfirm changes to continue.")
        .button("OK")
        .delay(2)
        .type(Error);
  }

  const String label = settingsButtonLabel(nowMs);
  ui.updateButton()
      .label(label)
      .pos(60, 20)
      .size(120, 44)
      .baseColor(ui.rgb(40, 150, 255))
      .radius(10)
      .mode(true, loading)
      .down(!settingsAwaitRelease && buttonDown);
}

void updateFirmwareUpdateScreen(uint32_t nowMs, bool nextPressed, bool nextDown, bool prevPressed, bool prevDown)
{
  (void)nextPressed;
  static bool rollbackOpenPending = false;

  const FirmwareUpdateLayout l = calcFirmwareUpdateLayout();
  if (ui.wifiState() == net::WifiState::Off)
    ui.requestWiFi(true);

  const OtaStatus &st = ui.otaStatus();
  const bool busy =
      (st.state == OtaState::WifiStarting) ||
      (st.state == OtaState::FetchingManifest) ||
      (st.state == OtaState::Downloading) ||
      (st.state == OtaState::Installing);

  if (ui.popupMenuActive())
  {
    ui.popupMenuInput().nextDown(nextDown).prevDown(prevDown);
    const int16_t picked = ui.popupMenuTakeResult();
    if (picked >= 0 && !busy)
    {
      const char *ver = ui.otaStableListVersion((uint8_t)picked);
      if (ver && ver[0])
        ui.otaRequestInstallStableVersion(ver);
    }
  }
  else
  {
    static uint32_t nextHoldStartMs = 0;
    static bool nextLongFired = false;
    static bool lastNextDown = false;
    constexpr uint32_t kBackHoldMs = 450;
    if (nextDown)
    {
      if (!lastNextDown)
      {
        nextHoldStartMs = nowMs;
        nextLongFired = false;
      }
      else if (!nextLongFired && nextHoldStartMs && (nowMs - nextHoldStartMs) >= kBackHoldMs)
      {
        nextLongFired = true;
        rollbackOpenPending = false;
#if PIPGUI_OTA
        ui.otaCancel();
#endif
        ui.setScreen(listMenu);
        return;
      }
    }
    else
    {
      if (lastNextDown && !nextLongFired && !busy)
      {
        if (ui.otaStableListReady() && ui.otaStableListCount() > 0)
        {
          const uint8_t count = ui.otaStableListCount();
          const uint8_t maxVisible = 7;
          const uint8_t visible = (count < maxVisible) ? count : maxVisible;
          const int16_t menuH = (int16_t)(16 + visible * 28);
          ui.showPopupMenu()
              .items(&otaStableVersionItem, nullptr, count)
              .pos(l.btnX1, (int16_t)(l.btnY - 8 - menuH))
              .width(l.btnW)
              .selected(0)
              .maxVisible(maxVisible);
        }
        else
        {
          rollbackOpenPending = true;
          ui.otaRequestStableList();
        }
      }
      nextHoldStartMs = 0;
      nextLongFired = false;
    }
    lastNextDown = nextDown;

    if (rollbackOpenPending && ui.otaStableListReady())
    {
      rollbackOpenPending = false;
      const uint8_t count = ui.otaStableListCount();
      if (count == 0)
      {
        ui.showToast().text("No previous stable").duration(1800);
      }
      else if (!busy)
      {
        const uint8_t maxVisible = 7;
        const uint8_t visible = (count < maxVisible) ? count : maxVisible;
        const int16_t menuH = (int16_t)(16 + visible * 28);
        ui.showPopupMenu()
            .items(&otaStableVersionItem, nullptr, count)
            .pos(l.btnX1, (int16_t)(l.btnY - 8 - menuH))
            .width(l.btnW)
            .selected(0)
            .maxVisible(maxVisible);
      }
    }

    if (prevPressed)
    {
      if (busy)
      {
        ui.otaCancel();
      }
      else if (st.state == OtaState::UpdateAvailable)
      {
        ui.otaRequestInstall();
      }
      else if (st.state == OtaState::Success)
      {
        ESP.restart();
      }
      else
      {
        ui.otaRequestCheck();
      }
    }
  }

  String wifiText;
  if (ui.wifiConnected())
  {
    wifiText = String("Connected ") + ipV4ToString(ui.wifiLocalIpV4());
  }
  else
  {
    switch (ui.wifiState())
    {
    case net::WifiState::Connecting:
      wifiText = String("Connecting...");
      break;
    case net::WifiState::Failed:
      wifiText = String("Failed (retrying)");
      break;
    case net::WifiState::Unsupported:
      wifiText = String("Unsupported");
      break;
    case net::WifiState::Off:
    default:
      wifiText = String("Not connected");
      break;
    }
  }

  String updateText = otaStateText(st);
  if (st.state == OtaState::UpdateAvailable)
  {
    const char *ch = (st.channel == OtaChannel::Beta) ? "Beta" : "Stable";
    updateText = String(st.manifest.version);
    if (updateText.length() == 0 && st.manifest.verMajor)
    {
      char vb[16];
      snprintf(vb, sizeof(vb), "%u.%u.%u", (unsigned)st.manifest.verMajor, (unsigned)st.manifest.verMinor, (unsigned)st.manifest.verPatch);
      updateText = String(vb);
    }
    updateText += String(" (") + ch + String(")");
    if (st.manifest.title[0] != '\0')
      updateText = String(st.manifest.title) + String(" ") + updateText;
  }
  else if (st.state == OtaState::Error)
  {
    updateText += String(" (") + otaErrorName(st.error) + String(")");
    if (st.httpCode != 0)
      updateText += String(" http=") + String(st.httpCode);
  }

  const uint16_t cardBg = ui.rgb(16, 16, 16);
  const int16_t valX = l.valX;
  const uint32_t valFg = ui.rgb(220, 220, 220);
  const uint32_t valFgDim = ui.rgb(180, 180, 180);

  static const char *kClearWide =
      "                                                                                                    ";

  auto clampText = [](const String &s, size_t maxChars) -> String {
    if (s.length() <= maxChars)
      return s;
    if (maxChars <= 3)
      return String("...");
    return s.substring(0, (unsigned)(maxChars - 3)) + String("...");
  };

  ui.setTextStyle(Body);
  ui.updateText().text(kClearWide).pos(valX, l.rowY0).color(cardBg).bgColor(cardBg);
  ui.updateText().text(clampText(String(PIPGUI_FIRMWARE_TITLE) + " " + fwVersionText(), 40)).pos(valX, l.rowY0).color((uint16_t)valFg).bgColor(cardBg);
  ui.updateText().text(kClearWide).pos(valX, l.rowY1).color(cardBg).bgColor(cardBg);
  ui.updateText().text(clampText(wifiText, 40)).pos(valX, l.rowY1).color((uint16_t)valFg).bgColor(cardBg);
  ui.updateText().text(kClearWide).pos(valX, l.rowY2).color(cardBg).bgColor(cardBg);
  ui.updateText().text(clampText(updateText, 44)).pos(valX, l.rowY2).color((uint16_t)valFgDim).bgColor(cardBg);

  ui.setTextStyle(Caption);
  String notesText;
  if (st.state == OtaState::UpdateAvailable || st.state == OtaState::Downloading || st.state == OtaState::Installing || st.state == OtaState::Success)
  {
    if (st.manifest.desc[0] != '\0')
      notesText = String(st.manifest.desc);
    else
      notesText = String("No description");
  }

  const int16_t notesX = l.cardX + 10;
  ui.updateText().text(kClearWide).pos(notesX, l.notesTextY).color(cardBg).bgColor(cardBg);
  if (notesText.length() > 0)
    ui.updateText().text(clampText(notesText, 56)).pos(notesX, l.notesTextY).color(ui.rgb(200, 200, 200)).bgColor(cardBg);

  uint8_t p = 0;
  if (st.total > 0)
    p = (uint8_t)min<uint32_t>(100u, (st.downloaded * 100u) / st.total);

  ui.updateProgress()
      .pos(l.cardX + 10, l.infoY + l.infoH - 16)
      .size(l.cardW - 20, 8)
      .radius(4)
      .value(p)
      .anim(None)
      .baseColor(ui.rgb(18, 18, 18))
      .fillColor(ui.rgb(40, 150, 255));

  auto otaButton = ui.updateButton()
                       .label(otaButtonLabel(st))
                       .pos(l.btnX0, l.btnY)
                       .size(l.btnW, l.btnH)
                       .baseColor(ui.rgb(40, 150, 255))
                       .radius(11);
  otaButton.derive()
      .mode(true, busy)
      .down(prevDown);
  otaButton.derive()
      .label("Rollback")
      .pos(l.btnX1, l.btnY)
      .mode(!busy, rollbackOpenPending && !ui.otaStableListReady())
      .down(nextDown);
}

void updateToggleDemo(bool nextPressed, bool prevPressed)
{
  const bool prevVal = g_toggleValue;
  bool changed = false;

  if (prevPressed)
    ui.setScreen(listMenu);

  const uint32_t nowMs = millis();
  const bool toggleEnabled = (g_toggleLockedUntil == 0 || nowMs >= g_toggleLockedUntil);
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const uint16_t active = ui.rgb(21, 180, 110);

  ui.updateToggleSwitch()
      .pos(center, 150)
      .size(78, 36)
      .value(g_toggleValue)
      .pressed(nextPressed)
      .changed(changed)
      .enabled(toggleEnabled)
      .activeColor(active);

  if (!changed)
    return;

  if (prevVal != g_toggleValue)
  {
    if (g_toggleValue)
      g_toggleLockedUntil = nowMs + 700U;

    ui.setTextStyle(Body);
    ui.updateText()
        .text(g_toggleValue ? "ON..." : "OFF")
        .pos(-1, 105)
        .color(ui.rgb(200, 200, 200))
        .bgColor(bg565)
        .align(Center);
  }
}

void updateButtonsDemo(uint32_t nowMs, bool nextPressed, bool nextDown, bool prevPressed, bool prevDown, bool comboDown)
{
  struct DemoPreset
  {
    int16_t heroW;
    int16_t heroH;
    int16_t wideW;
    int16_t miniW;
    int16_t miniH;
  };

  constexpr DemoPreset kSizePresets[] = {
      {82, 30, 156, 52, 26},
      {98, 34, 184, 62, 28},
      {112, 38, 212, 74, 32},
  };
  constexpr uint8_t kSizePresetCount = static_cast<uint8_t>(sizeof(kSizePresets) / sizeof(kSizePresets[0]));

  struct DemoStyle
  {
    const char *name;
    uint16_t primary;
    uint16_t secondary;
    uint16_t accent;
    uint8_t heroRadius;
    uint8_t wideRadius;
      uint8_t miniRadius;
  };

  const DemoStyle kStyles[] = {
      {"Rounded", ui.rgb(0, 87, 250), ui.rgb(255, 0, 72), ui.rgb(255, 128, 0), 10, 12, 8},
      {"Pill", ui.rgb(40, 150, 255), ui.rgb(255, 98, 0), ui.rgb(0, 200, 120), 18, 18, 14},
      {"Card", ui.rgb(98, 54, 255), ui.rgb(255, 64, 128), ui.rgb(255, 196, 0), 6, 8, 4},
  };
  constexpr uint8_t kStyleCount = static_cast<uint8_t>(sizeof(kStyles) / sizeof(kStyles[0]));

  static uint32_t comboHoldStartMs = 0;
  if (comboDown)
  {
    if (comboHoldStartMs == 0)
      comboHoldStartMs = nowMs;
    else if ((nowMs - comboHoldStartMs) >= 450U)
    {
      comboHoldStartMs = 0;
      ui.setScreen(listMenu);
      return;
    }
  }
  else
  {
    comboHoldStartMs = 0;
  }

  bool layoutChanged = false;
  if (nextPressed && !prevDown)
  {
    g_buttonsDemoStyle = static_cast<uint8_t>((g_buttonsDemoStyle + 1) % kStyleCount);
    layoutChanged = true;
  }
  if (prevPressed && !nextDown)
  {
    g_buttonsDemoSize = static_cast<uint8_t>((g_buttonsDemoSize + 1) % kSizePresetCount);
    layoutChanged = true;
  }
  if (layoutChanged)
    ui.requestRedraw();

  const DemoPreset &size = kSizePresets[g_buttonsDemoSize];
  const DemoStyle &style = kStyles[g_buttonsDemoStyle];
  const uint16_t bg565 = ui.rgb(10, 10, 10);
  const uint16_t line565 = ui.rgb(145, 145, 145);
  const uint16_t hint565 = ui.rgb(95, 95, 95);
  const int16_t sw = static_cast<int16_t>(ui.screenWidth());

  ui.setTextStyle(Caption);
  ui.updateText().text("                              ").pos(24, 76).color(bg565).bgColor(bg565);
  ui.updateText().text(String("Style: ") + style.name).pos(24, 76).color(line565).bgColor(bg565);
  ui.updateText().text("                              ").pos(138, 76).color(bg565).bgColor(bg565);
  ui.updateText().text(String("Size: ") + String((unsigned)(g_buttonsDemoSize + 1))).pos(138, 76).color(line565).bgColor(bg565);

  auto heroButton = ui.updateButton()
                        .label("NEXT")
                        .pos(18, 98)
                        .size(size.heroW, size.heroH)
                        .baseColor(style.primary)
                        .radius(style.heroRadius)
                        .icon(warning);
  heroButton.derive().down(nextDown);
  heroButton.derive()
      .label("")
      .pos(static_cast<int16_t>(sw - 18 - size.heroW), 98)
      .baseColor(style.secondary)
      .icon(error)
      .down(prevDown);

  auto wideButton = ui.updateButton()
                        .label("Loading")
                        .pos(center, 146)
                        .size(size.wideW, size.heroH)
                        .baseColor(style.accent)
                        .radius(style.wideRadius)
                        .icon(battery_l1);
  wideButton.derive().mode(true, true).down(comboDown);
  wideButton.derive()
      .label("Disabled")
      .pos(center, 192)
      .baseColor(style.primary)
      .icon(battery_l0)
      .mode(false, false);

  ui.updateButton()
      .label("XS")
      .pos(28, 242)
      .size(size.miniW, size.miniH)
      .baseColor(style.secondary)
      .radius(style.miniRadius)
      .icon(warning);

  ui.updateButton()
      .label("")
      .pos(static_cast<int16_t>(sw - 28 - size.wideW), 242)
      .size(size.wideW, size.miniH)
      .baseColor(style.primary)
      .radius(static_cast<uint8_t>(size.miniH / 2))
      .icon(error);

  ui.setTextStyle(Caption);
  ui.updateText().text("Press Next/Prev, tap to switch style/size").pos(center, 286).color(hint565).bgColor(bg565).align(Center);
}

void updateScrollDotsDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (nextPressed || prevPressed)
  {
    const uint8_t prev = g_dotsActive;
    if (nextPressed)
    {
      g_dotsActive = (uint8_t)((g_dotsActive + 1) % g_dotsCount);
      g_dotsDir = 1;
    }
    else
    {
      g_dotsActive = (uint8_t)((g_dotsActive + g_dotsCount - 1) % g_dotsCount);
      g_dotsDir = -1;
    }
    g_dotsPrev = prev;
    g_dotsAnimate = true;
    g_dotsAnimStartMs = nowMs;
  }

  if (!g_dotsAnimate)
    return;

  if (nowMs - g_dotsAnimStartMs >= g_dotsAnimDurMs)
    g_dotsAnimate = false;

  const float p = animProgress(nowMs, g_dotsAnimStartMs, g_dotsAnimDurMs);

  ui.updateScrollDots()
      .pos(center, 150)
      .count(g_dotsCount)
      .activeIndex(g_dotsActive)
      .prevIndex(g_dotsPrev)
      .animProgress(p)
      .animDirection(g_dotsDir)
      .activeColor(ui.rgb(0, 87, 250))
      .inactiveColor(ui.rgb(60, 60, 60))
      .radius(3)
      .spacing(14);
}

void updateDrumRollDemo(uint32_t nowMs, bool nextPressed, bool prevPressed)
{
  if (nextPressed)
  {
    g_drumPrevH = g_drumSelectedH;
    g_drumSelectedH = (uint8_t)((g_drumSelectedH + 1) % g_drumCountH);
    g_drumAnimateH = true;
    g_drumAnimStartMs = nowMs;
  }
  else if (prevPressed)
  {
    g_drumPrevH = g_drumSelectedH;
    g_drumSelectedH = (uint8_t)((g_drumSelectedH + g_drumCountH - 1) % g_drumCountH);
    g_drumAnimateH = true;
    g_drumAnimStartMs = nowMs;
  }

  if (prevPressed)
  {
    g_drumPrevV = g_drumSelectedV;
    g_drumSelectedV = (uint8_t)((g_drumSelectedV + g_drumCountV - 1) % g_drumCountV);
    g_drumAnimateV = true;
    g_drumAnimStartMs = nowMs;
  }

  ui.requestRedraw();
}

void handleErrorDemo(uint8_t screenId, bool nextPressed, bool prevPressed)
{
  if (ui.errorActive())
    return;

  if (prevPressed)
  {
    ui.setScreen(listMenu);
    return;
  }

  if (!nextPressed)
    return;

  if (screenId == errorOverlay)
  {
    ui.showError().message("Failed to create screen sprite").code("0xFAISPRT").type(Crash).button("OK");
    ui.showError().message("LittleFS mount failed").code("0xLFSMNT").type(Crash).button("OK");
    ui.showError().message("Heap allocation for DMA buffer failed").code("0xDMABUF").type(Crash).button("OK");
    return;
  }

  ui.showError().message("No profile found!").code("0xNOPROF").type(Warning).button("OK");
  ui.showError().message("Network is unstable").code("0xNETWRN").type(Warning).button("OK");
  ui.showError().message("Battery is below 15%").code("0xLOWBAT").type(Warning).button("OK");
}

void updatePopupMenuDemo(bool nextPressed, bool nextDown, bool prevPressed, bool prevDown)
{
  if (ui.popupMenuActive())
  {
    ui.popupMenuInput()
        .nextDown(nextDown)
        .prevDown(prevDown);

    const int16_t picked = ui.popupMenuTakeResult();
    if (picked >= 0 && picked < kPopupMenuDemoItemCount)
    {
      ui.showToast()
          .text(String("Picked: ") + kPopupMenuDemoItems[picked])
          .duration(1500);
    }
    return;
  }

  if (prevPressed)
  {
    ui.setScreen(listMenu);
    return;
  }

  ui.updateButton()
      .label("Open menu")
      .pos(center, 228)
      .size(180, 32)
      .baseColor(ui.rgb(0, 87, 250))
      .radius(10)
      .mode(true, false)
      .down(nextDown);

  if (!nextPressed)
    return;

  ui.showPopupMenu()
      .items(kPopupMenuDemoItems, kPopupMenuDemoItemCount)
      .pos(42, 80)
      .width(156)
      .selected(1)
      .maxVisible(6);
}

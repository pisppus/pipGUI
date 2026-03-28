#include <Arduino.h>
#include <math.h>
#include <pipKit.hpp>
#include <pipGUI/Systems/Network/Wifi.hpp>
#include <pipGUI/Systems/Update/Ota.hpp>

using namespace pipgui;

namespace
{
  constexpr int16_t kStatusBarHeight = 20;
  constexpr uint32_t kToastIntervalMs = 12000;
  constexpr uint32_t kBatteryStepMs = 250;
  constexpr uint32_t kFrameStepMs = 30;
  constexpr uint32_t kSettingsLoadingDurationMs = 2400;
  constexpr uint8_t kBlurRectCount = 3;

#ifndef PIPGUI_DEMO_BTN_NEXT_PIN
#define PIPGUI_DEMO_BTN_NEXT_PIN 20
#endif

#ifndef PIPGUI_DEMO_BTN_PREV_PIN
#define PIPGUI_DEMO_BTN_PREV_PIN 4
#endif

// Set to 255 to disable backlight pin control in the demo.
// This is useful if your backlight pin conflicts with buttons.
#ifndef PIPGUI_DEMO_BACKLIGHT_PIN
#define PIPGUI_DEMO_BACKLIGHT_PIN 255
#endif

  constexpr uint8_t kBtnNextPin = (uint8_t)PIPGUI_DEMO_BTN_NEXT_PIN;
  constexpr uint8_t kBtnPrevPin = (uint8_t)PIPGUI_DEMO_BTN_PREV_PIN;
  constexpr uint8_t kBacklightPin = (uint8_t)PIPGUI_DEMO_BACKLIGHT_PIN;

  static_assert(kBtnNextPin != kBtnPrevPin, "Buttons must be on different pins");
  static_assert(kBacklightPin == 255 || (kBacklightPin != kBtnNextPin && kBacklightPin != kBtnPrevPin),
                "Backlight pin conflicts with a button pin");

  struct BlurRect
  {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint32_t color;
  };

  struct BlurTrailState
  {
    bool initialized = false;
    BlurRect rects[kBlurRectCount] = {};
  };

  GUI ui;

  Button btnNext(kBtnNextPin, Pullup);
  Button btnPrev(kBtnPrevPin, Pullup);

  bool g_toggleValue = false;
  uint32_t g_toggleLockedUntil = 0;

  uint8_t g_buttonsDemoStyle = 0;
  uint8_t g_buttonsDemoSize = 1;
  int16_t g_sliderValue = 56;

  float g_graphPhase = 0.0f;
  uint32_t g_lastGraphUpdateMs = 0;
  uint8_t g_batteryLevel = 100;
  bool g_batteryDirDown = true;
  uint8_t g_progressValue = 0;
  bool g_progressDirDown = false;
  uint32_t g_lastProgressUpdateMs = 0;

  int16_t g_graphV1 = 0;
  int16_t g_graphV2 = 0;
  int16_t g_graphV3 = 0;

  uint32_t g_settingsLoadingUntil = 0;
  bool g_ditherCompareFrozen = false;
  uint8_t g_ditherCompareFrozenR = 0;
  uint8_t g_ditherCompareFrozenG = 0;
  uint8_t g_ditherCompareFrozenB = 0;

  constexpr uint8_t g_dotsCount = 15;
  uint8_t g_dotsActive = 0;

  const char *const g_drumOptionsH[] = {"Off", "5 min", "10 min", "30 min", "1 hr"};
  constexpr uint8_t g_drumCountH = 5;
  const char *const g_drumOptionsV[] = {"Small", "Medium", "Large"};
  constexpr uint8_t g_drumCountV = 3;

  uint8_t g_drumSelectedH = 0;
  uint8_t g_drumSelectedV = 0;

  float g_blurPhase = 0.0f;
  uint32_t g_blurLastUpdateMs = 0;
  BlurTrailState g_blurRow2Trail;

  inline uint32_t color565To888(uint16_t c)
  {
    return pipgui::detail::color565To888(c);
  }

  String settingsButtonLabel(uint32_t nowMs)
  {
    if (g_settingsLoadingUntil == 0 || nowMs >= g_settingsLoadingUntil)
      return String("Show modal");

    const uint32_t remainingMs = g_settingsLoadingUntil - nowMs;
    char buf[20];
    snprintf(buf, sizeof(buf), "Saving %lu.%lus",
             (unsigned long)(remainingMs / 1000U),
             (unsigned long)((remainingMs % 1000U) / 100U));
    return String(buf);
  }

  void stepPingPong(uint8_t &value, bool &dirDown)
  {
    if (dirDown)
    {
      if (value > 0)
        --value;
      else
        dirDown = false;
    }
    else
    {
      if (value < 100)
        ++value;
      else
        dirDown = true;
    }
  }

  uint32_t blurRectColor(uint8_t index)
  {
    switch (index)
    {
    case 0:
      return ui.rgb(255, 80, 80);
    case 1:
      return ui.rgb(80, 255, 120);
    default:
      return ui.rgb(80, 255, 140);
    }
  }

  String ipV4ToString(uint32_t ipV4)
  {
    if (ipV4 == 0)
      return String("-");
    const uint8_t a = (uint8_t)((ipV4 >> 24) & 0xFFu);
    const uint8_t b = (uint8_t)((ipV4 >> 16) & 0xFFu);
    const uint8_t c = (uint8_t)((ipV4 >> 8) & 0xFFu);
    const uint8_t d = (uint8_t)(ipV4 & 0xFFu);
    char buf[20];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (unsigned)a, (unsigned)b, (unsigned)c, (unsigned)d);
    return String(buf);
  }

  String fwVersionText()
  {
    const uint32_t maj = (uint32_t)PIPGUI_FIRMWARE_VER_MAJOR;
    const uint32_t min = (uint32_t)PIPGUI_FIRMWARE_VER_MINOR;
    const uint32_t pat = (uint32_t)PIPGUI_FIRMWARE_VER_PATCH;
    return String(maj) + "." + String(min) + "." + String(pat);
  }

  const char *otaErrorName(OtaError e)
  {
    switch (e)
    {
    case OtaError::None:
      return "None";
    case OtaError::WifiNotEnabled:
      return "WiFi disabled";
    case OtaError::WifiNotConnected:
      return "WiFi not connected";
    case OtaError::HttpBeginFailed:
      return "HTTP begin";
    case OtaError::HttpStatusNotOk:
      return "HTTP status";
    case OtaError::ManifestTooLarge:
      return "Manifest too large";
    case OtaError::ManifestParseFailed:
      return "Manifest parse";
    case OtaError::ManifestReplay:
      return "Manifest replay";
    case OtaError::UrlTooLong:
      return "URL too long";
    case OtaError::PayloadSizeMismatch:
      return "Size mismatch";
    case OtaError::FlashLayoutInvalid:
      return "Flash layout";
    case OtaError::RollbackUnavailable:
      return "Rollback unavailable";
    case OtaError::UpdateBeginFailed:
      return "Update begin";
    case OtaError::UpdateWriteFailed:
      return "Update write";
    case OtaError::HashMismatch:
      return "SHA mismatch";
    case OtaError::SignatureMissing:
      return "Sig missing";
    case OtaError::SignatureInvalid:
      return "Sig invalid";
    case OtaError::UpdateEndFailed:
      return "Update end";
    case OtaError::HashPipelineFailed:
      return "Hash pipeline";
    case OtaError::DownloadTruncated:
      return "Download truncated";
    default:
      return "Unknown";
    }
  }

  String otaStateText(const OtaStatus &st)
  {
    switch (st.state)
    {
    case OtaState::Idle:
      return String("Idle");
    case OtaState::UpToDate:
      return String("No updates");
    case OtaState::WifiStarting:
      return String("WiFi connecting...");
    case OtaState::FetchingManifest:
      return String("Checking update...");
    case OtaState::UpdateAvailable:
      return String("Update available");
    case OtaState::Downloading:
      return String("Downloading...");
    case OtaState::Installing:
      return String("Installing...");
    case OtaState::Success:
      return String("Installed (reboot)");
    case OtaState::Error:
      return String("Error");
    default:
      return String("?");
    }
  }

  String otaButtonLabel(const OtaStatus &st)
  {
    switch (st.state)
    {
    case OtaState::UpToDate:
      return String("Check update");
    case OtaState::UpdateAvailable:
      return String("Install update");
    case OtaState::Downloading:
      if (st.total > 0)
      {
        const uint32_t p = (st.downloaded * 100u) / st.total;
        return String("Downloading ") + String((unsigned)p) + String("%");
      }
      return String("Downloading...");
    case OtaState::Success:
      return String("Reboot");
    case OtaState::Error:
      return String("Retry");
    case OtaState::WifiStarting:
    case OtaState::FetchingManifest:
    case OtaState::Installing:
      return String("Cancel");
    case OtaState::Idle:
    default:
      return String("Check update");
    }
  }

  void buildBlurRow(float phase, int16_t screenWidth, int16_t baseY, int16_t yStep, int16_t rectHeight, BlurRect *rects)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      const float tt = phase * (0.8f + 0.3f * i);
      const int16_t cx = (int16_t)(screenWidth / 2 + sinf(tt + i) * (float)(screenWidth / 2 - 30));
      const int16_t cy = (int16_t)(baseY + i * yStep + cosf(tt * 1.3f) * 8.0f);
      rects[i] = {
          (int16_t)(cx - (60 + i * 20) / 2),
          (int16_t)(cy - rectHeight / 2),
          (int16_t)(60 + i * 20),
          rectHeight,
          blurRectColor(i)};
    }
  }

  void drawBlurRow(GUI &ui, const BlurRect *rects)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      ui.drawRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .fill(rects[i].color);
    }
  }

  void clearBlurRow(GUI &ui, const BlurRect *rects, uint16_t bg565)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      ui.drawRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .fill(bg565);
    }
  }

  void runBootAnimation(GUI &ui, BootAnimation anim, const String &title, const String &subtitle)
  {
    constexpr uint32_t bootDurationMs = 1800;
    ui.showLogo()
      .text(title, subtitle)
      .anim(anim);
    const uint32_t start = millis();
    while ((millis() - start) < (bootDurationMs + 180U))
    {
      ui.loop();
    }
  }

  // РЈРїСЂРѕС‰РµРЅРѕ: С„СѓРЅРєС†РёРё fnv1a32 Рё drawWrappedText РЅРµ РёСЃРїРѕР»СЊР·СѓСЋС‚СЃСЏ РІ С‚РµРєСѓС‰РµР№ РєРѕРЅС„РёРіСѓСЂР°С†РёРё Рё СѓРґР°Р»РµРЅС‹.
}

#include "screens/screens.hpp"

void updateClockDisplay(uint32_t nowMs)
{
  static uint32_t lastMinute = 0xFFFFFFFF;
  const uint32_t minute = (nowMs / 1000U) / 60U;
  if (minute == lastMinute)
    return;

  lastMinute = minute;
  const uint8_t m = minute % 60U;
  const uint8_t h = (minute / 60U) % 24U;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)h, (unsigned)m);
  ui.setStatusBarText()
      .left("pipGUI")
      .center(String(buf))
      .right("");
  if (!ui.notificationActive())
    ui.updateStatusBar();
}

void updateBatteryStatus(uint32_t nowMs)
{
  static uint32_t lastBatteryMs = 0;
  if (nowMs - lastBatteryMs < kBatteryStepMs)
    return;

  lastBatteryMs = nowMs;
  stepPingPong(g_batteryLevel, g_batteryDirDown);
  ui.setStatusBarBattery(g_batteryLevel, Bar);
  if (!ui.notificationActive())
    ui.updateStatusBar();
}

bool isListMenuScreen(uint8_t screenId)
{
  return screenId == listMenu || screenId == listMenuPlain;
}

bool isTileMenuScreen(uint8_t screenId)
{
  return screenId == tileMenu || screenId == tileMenuLayout || screenId == tileMenu4Cols;
}


#include "screens/demo_updates.hpp"

void setup()
{
  Serial.begin(115200);

#if PIPGUI_OTA
  ui.otaConfigure();
#endif

  ui.configDisplay()
      .pins({11, 12, 10, 9, 14})
      .size(240, 320);

  ui.begin(3);
  ui.setScreenAnim(SlideY, 320);
  #if (PIPGUI_STATUS_BAR != 0)
  ui.configStatusBar()
      .height(kStatusBarHeight)
      .pos(Top)
      .style(Blur);
  #endif
  ui.setStatusBarText()
      .left("pipGUI")
      .center("00:00")
      .right("");
  ui.setStatusBarBattery(100, Bar);
  if (kBacklightPin != 255)
    ui.setBacklight().pin(kBacklightPin);

  btnNext.begin();
  btnPrev.begin();
  runBootAnimation(ui, FadeIn, "PISPPUS", "Fade in");

  ui.setScreen(listMenu);
}

void loop()
{
  const auto input = ui.pollInput(btnNext, btnPrev);
  const uint32_t nowMs = millis();
  const bool nextPressed = input.nextPressed;
  const bool prevPressed = input.prevPressed;
  const bool nextDown = input.nextDown;
  const bool prevDown = input.prevDown;
  const bool comboDown = input.comboDown;

  if (ui.screenTransitionActive())
  {
    ui.loop();
    return;
  }

  updateClockDisplay(nowMs);
  updateBatteryStatus(nowMs);

  const uint8_t cur = ui.currentScreen();
  const bool notificationActive = ui.notificationActive();

  if (!notificationActive)
  {
    if (isListMenuScreen(cur))
    {
      ui.listInput()
          .nextDown(nextDown)
          .prevDown(prevDown);
    }
    else if (isTileMenuScreen(cur))
    {
      ui.tileInput()
          .nextDown(nextDown)
          .prevDown(prevDown);
    }
    else
    {
      switch (cur)
      {
      case glow:
        updateGlowDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case blur:
        updateBlurDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case graph:
      case graphSmall:
      case graphTall:
      case graphOsc:
        updateGraphScreen(cur, nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case progress:
        updateProgressDemoFrame(nowMs);
        if (nextPressed)
          ui.nextScreen();
        break;
      case popupMenuDemo:
        updatePopupMenuDemo(nextPressed, nextDown, prevPressed, prevDown);
        break;
      case settings:
        if (prevPressed)
          ui.prevScreen();
        else
          updateSettingsDemoFrame(nowMs, nextPressed, nextDown);
        break;
      case firmwareUpdate:
        updateFirmwareUpdateScreen(nowMs, nextPressed, nextDown, prevPressed, prevDown);
        break;
      case toggleSwitch:
        updateToggleDemo(nextPressed, prevPressed);
        break;
      case buttonsDemo:
        updateButtonsDemo(nowMs, nextPressed, nextDown, prevPressed, prevDown, comboDown);
        break;
      case sliderDemo:
        updateSliderDemo(nowMs, comboDown);
        break;
      case animatedIconsDemo:
        updateAnimatedIconsDemo(nowMs, comboDown);
        break;
      case scrollDots:
        updateScrollDotsDemo(nowMs, nextPressed, prevPressed);
        break;
      case drumRoll:
        updateDrumRollDemo(nowMs, nextPressed, prevPressed);
        break;
      case errorOverlay:
      case warningOverlay:
        handleErrorDemo(cur, nextPressed, prevPressed);
        break;
      default:
        if (nextPressed)
          ui.nextScreen();
        break;
      }
    }
  }

  if (ui.errorActive())
  {
    ui.setErrorButtonsDown(nextDown, prevDown, comboDown);
  }
  else if (ui.notificationActive())
  {
    ui.setNotificationButtonDown(prevDown);
  }

  ui.loop();
}

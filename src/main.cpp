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
#define PIPGUI_DEMO_BTN_NEXT_PIN 2
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

  ToggleSwitchState g_toggleState{false, 0, 0};

  uint8_t g_buttonsDemoStyle = 0;
  uint8_t g_buttonsDemoSize = 1;

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
  uint8_t g_dotsPrev = 0;
  bool g_dotsAnimate = false;
  int8_t g_dotsDir = 0;
  uint32_t g_dotsAnimStartMs = 0;
  constexpr uint32_t g_dotsAnimDurMs = 240;

  const char *const g_drumOptionsH[] = {"Off", "5 min", "10 min", "30 min", "1 hr"};
  constexpr uint8_t g_drumCountH = 5;
  const char *const g_drumOptionsV[] = {"Small", "Medium", "Large"};
  constexpr uint8_t g_drumCountV = 3;

  uint8_t g_drumSelectedH = 0;
  uint8_t g_drumPrevH = 0;
  bool g_drumAnimateH = false;
  uint32_t g_drumAnimStartMs = 0;
  constexpr uint32_t g_drumAnimDurMs = 280;

  uint8_t g_drumSelectedV = 0;
  uint8_t g_drumPrevV = 0;
  bool g_drumAnimateV = false;

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

  float animProgress(uint32_t now, uint32_t startMs, uint32_t durationMs)
  {
    if (durationMs == 0)
      return 1.0f;
    const uint32_t elapsed = now - startMs;
    return (elapsed >= durationMs) ? 1.0f : (float)elapsed / (float)durationMs;
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

  const char *otaStableVersionItem(void *, uint8_t idx)
  {
    return ui.otaStableListVersion(idx);
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
      ui.fillRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .color(rects[i].color);
    }
  }

  void clearBlurRow(GUI &ui, const BlurRect *rects, uint16_t bg565)
  {
    for (uint8_t i = 0; i < kBlurRectCount; ++i)
    {
      ui.fillRect()
          .pos(rects[i].x, rects[i].y)
          .size(rects[i].w, rects[i].h)
          .color(bg565);
    }
  }

  void runBootAnimation(GUI &ui, BootAnimation anim, uint32_t durationMs, const String &title, const String &subtitle)
  {
    ui.showLogo()
      .title(title)
      .subtitle(subtitle)
      .anim(anim)
      .fgColor(0xFFFF)
      .bgColor(0x0000)
      .duration(durationMs);
    const uint32_t start = millis();
    while ((millis() - start) < (durationMs + 180U))
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
  ui.setStatusBarText("pipGUI", String(buf), "");
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

void configureListMenus()
{
  ui.configureList()
      .screen(listMenu)
      .items({
          {"Main screen with widgets", "Simple home screen with status bar, overlays, and transitions", mainMenu},
          {"Settings and alerts preview", "Buttons, notifications, warning states, and input handling", settings},
          {"Glow demo with blur and bloom", "Bloom, soft edges, layered primitives, and bright composition", glow},
          {"Graph demo with multiple traces", "Grid, labels, and several moving lines in one viewport", graph},
          {"Graph small", "Compact graph with automatic scale fitting", graphSmall},
          {"Graph tall", "Tall graph layout with right-side emphasis", graphTall},
          {"Graph osc", "Digital oscilloscope style waveform preview", graphOsc},
          {"Progress demo", "Progress bars with animated fill and labels", progress},
          {"Progress + text", "Fresh progress styles with text overlays", progressText},
          {"Popup menu", "Overlay menu with button-driven navigation and selection", popupMenuDemo},
          {"Tile menu with icon cards", "Grid menu with icons, title, subtitle, and active marquee", tileMenu},
          {"Tile layout", "Custom tile layout with merged cells", tileMenuLayout},
          {"Tile 4 cols", "Four compact tiles in a single row", tileMenu4Cols},
          {"List menu 2", "Text-focused list with active highlight panel", listMenuPlain},
          {"ToggleSwitch", "Toggle switch interaction and state preview", toggleSwitch},
          {"Buttons", "Different button forms, sizes and states", buttonsDemo},
          {"Scroll dots", "Page indicator dots with animated transitions", scrollDots},
          {"Error", "Full-screen error overlay presentation", errorOverlay},
          {"Warning", "Full-screen warning overlay presentation", warningOverlay},
          {"Blur strip", "Material-style blur strip with moving content", blur},
          {"Screenshots", "Captured frames gallery", screenshotGallery},
          {"Firmware update", "WiFi OTA updater (signed manifest)", firmwareUpdate},
          {"Primitives", "Circle / Arc / Ellipse / Triangle / Squircle", primitives},
          {"Gradients", "All gradient types", gradients},
          {"Font compare", "PSDF", fontCompare},
          {"Font weights", "Test all weights", fontWeight},
          {"Drum roll", "Horizontal + vertical picker", drumRoll},
          {"Circle AA", "Gupta-Sproull optimized", circle},
          {"Test: Circles", "fillCircle + drawCircle", testCircles},
          {"Test: RoundRects", "1 & 4 radius variants", testRoundRects},
          {"Test: Ellipses", "Wu-style AA ellipses", testEllipses},
          {"Test: Triangles", "4x subpixel AA triangles", testTriangles},
          {"Test: Arcs+Lines", "sqrt_fraction AA", testArcsAndLines},
          {"Test: All Grid", "All primitives comparison", testAllPrimitivesGrid},
          {"Test: RoundTriangles", "SDF AA rounded triangles", testRoundTriangles},
          {"Test: Squircles", "fill/draw squircle AA", testSquircles},
          {"Auto text color", "BT.709 luminance test", autoTextColor},
      })
      .parent(mainMenu)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .cardSize(310, 50);

  ui.configureList()
      .screen(listMenuPlain)
      .items({
          {"Back", "Return to the card-style list menu", listMenu},
          {"Main screen with widgets and overlays", "Simple home screen with status bar, overlays, and transitions", mainMenu},
          {"Settings, alerts, and input handling", "Buttons, notifications, warning states, and input handling", settings},
          {"Glow demo with blur, bloom, and soft shapes", "Bloom, soft edges, layered primitives, and bright composition", glow},
          {"Graph osc", "Digital oscilloscope style waveform preview", graphOsc},
          {"Tile menu", "Grid menu with icons, title, and subtitle", tileMenu},
          {"ToggleSwitch", "Toggle switch interaction and state preview", toggleSwitch},
          {"Screenshots", "Captured frames gallery", screenshotGallery},
          {"Font compare", "PSDF", fontCompare},
      })
      .parent(listMenu)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(12)
      .cardSize(310, 34)
      .mode(Plain);
}
void configureTileMenus()
{
  ui.configureTile()
      .screen(tileMenu)
      .items({
          {"Main", "Home screen", mainMenu},
          {"Settings", "Alerts and controls", settings},
          {battery_l0, "Glow demo", "Glow shapes", glow},
          {battery_l1, "Graph", "Live graphs", graph},
          {battery_l2, "Toggle", "Switch", toggleSwitch},
      })
      .parent(mainMenu)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .columns(2)
      .tileSize(100, 70)
      .mode(TextSubtitle);
  ui.configureTile()
      .screen(tileMenuLayout)
      .items({
          {"Back", "", listMenu},
          {battery_l0, "Small 1", "", mainMenu},
          {battery_l1, "Small 2", "", settings},
          {"Big", "Main", glow},
      })
      .parent(listMenu)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(10)
      .columns(2)
      .layout({
          "AA",
          "BC",
          "BD",
      })
      .mode(TextOnly);
  ui.configureTile()
      .screen(tileMenu4Cols)
      .items({
          {"Back", "", listMenu},
          {battery_l0, "1", "", mainMenu},
          {battery_l1, "2", "", settings},
          {battery_l2, "3", "", glow},
          {"4", "", graph},
          {"5", "", graphSmall},
          {battery_l0, "6", "", graphTall},
          {battery_l1, "7", "", progress},
          {"8", "", graphOsc},
      })
      .parent(listMenu)
      .cardColor(ui.rgb(8, 8, 8))
      .activeColor(ui.rgb(21, 54, 140))
      .radius(13)
      .spacing(8)
      .columns(4)
      .mode(TextOnly);
}

#include "screens/demo_updates.hpp"

void setup()
{
  Serial.begin(115200);

#if PIPGUI_OTA
  ui.otaConfigure();
#endif

  ui.configureDisplay()
      .pins({11, 12, 10, 9, 14})
      .size(240, 320);

  ui.begin(3, 0);
  ui.setScreenAnim(SlideX, 320);
  ui.configureStatusBar((PIPGUI_STATUS_BAR_ENABLED_DEFAULT != 0), 0x0000, kStatusBarHeight, Top);
  ui.setStatusBarStyle((PIPGUI_STATUS_BAR_DEBUG_METRICS_DEFAULT != 0) ? StatusBarStyleSolid : StatusBarStyleBlurGradient);
  ui.setStatusBarText("pipGUI", "00:00", "");
  ui.setStatusBarBattery(100, Bar);
  if (kBacklightPin != 255)
    ui.setBacklight().pin(kBacklightPin);

  btnNext.begin();
  btnPrev.begin();
  runBootAnimation(ui, FadeIn, 900, "PISPPUS", "Fade in");
  configureListMenus();
  configureTileMenus();

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
      ui.listInput(cur)
          .nextDown(nextDown)
          .prevDown(prevDown);
    }
    else if (isTileMenuScreen(cur))
    {
      ui.tileInput(cur)
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
        if (nextPressed)
          ui.nextScreen();
        else
          updateSettingsDemoFrame(nowMs, prevPressed, prevDown);
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

#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

pipgui::ConfigureDisplayFluent configureDisplay();

void configureDisplay(const pipcore::GuiDisplayConfig &cfg);
void begin(uint8_t rotation = 0,
           uint16_t bgColor = 0x0000);

void setBacklightCallback(BacklightCallback cb) { _disp.backlightCb = cb; }

void setBacklightPin(uint8_t pin,
                     uint8_t channel = 0,
                     uint32_t freqHz = 5000,
                     uint8_t resolutionBits = 12);

void setMaxBrightness(uint8_t percent);
uint8_t maxBrightness() const { return _disp.brightnessMax; }


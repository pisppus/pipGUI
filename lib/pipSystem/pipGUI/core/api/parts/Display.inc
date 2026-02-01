#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.h>"
#endif

        void configureDisplay(int8_t mosi,
                              int8_t sclk,
                              int8_t cs,
                              int8_t dc,
                              int8_t rst,
                              uint16_t w,
                              uint16_t h,
                              uint32_t hz = 80000000);
        void begin(uint8_t rotation = 0,
                   uint16_t bgColor = 0x0000);

        void setBacklightCallback(BacklightCallback cb) { _backlightCb = cb; }

        void setBacklightPin(uint8_t pin,
                             uint8_t channel = 0,
                             uint32_t freqHz = 5000,
                             uint8_t resolutionBits = 12);
                             
        void setMaxBrightness(uint8_t percent);
        uint8_t maxBrightness() const { return _brightnessMax; }

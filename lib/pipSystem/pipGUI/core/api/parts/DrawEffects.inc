#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.h>"
#endif

        PipLGFX &tft()
        {
            assert(_tft != nullptr);
            return *_tft;
        }

        uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) const;
        uint16_t screenWidth() const;
        uint16_t screenHeight() const;

        void clear(uint16_t color = 0x0000);
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

        void drawBlurStrip(int16_t x, int16_t y,
                           int16_t w, int16_t h,
                           uint8_t radius,
                           BlurDirection dir,
                           bool gradient = true,
                           uint8_t materialStrength = 32,
                           uint8_t noiseAmount = 8,
                           int32_t materialColor = -1);

        void updateBlurStrip(int16_t x, int16_t y,
                             int16_t w, int16_t h,
                             uint8_t radius,
                             BlurDirection dir,
                             bool gradient = true,
                             uint8_t materialStrength = 32,
                             uint8_t noiseAmount = 8,
                             int32_t materialColor = -1);

        void drawScrollDots(int16_t x, int16_t y,
                            uint8_t count,
                            uint8_t activeIndex,
                            uint8_t prevIndex = 0,
                            float animProgress = 0.0f,
                            bool animate = false,
                            int8_t animDirection = 0,
                            uint16_t activeColor = 0xFFFF,
                            uint16_t inactiveColor = 0x7BEF,
                            uint8_t dotRadius = 3,
                            uint8_t spacing = 14,
                            uint8_t activeWidth = 18);

        void updateScrollDots(int16_t x, int16_t y,
                              uint8_t count,
                              uint8_t activeIndex,
                              uint8_t prevIndex = 0,
                              float animProgress = 0.0f,
                              bool animate = false,
                              int8_t animDirection = 0,
                              uint16_t activeColor = 0xFFFF,
                              uint16_t inactiveColor = 0x7BEF,
                              uint8_t dotRadius = 3,
                              uint8_t spacing = 14,
                              uint8_t activeWidth = 18);

        void drawGlowCircle(int16_t x, int16_t y,
                            int16_t r,
                            uint16_t fillColor,
                            int32_t bgColor = -1,
                            int32_t glowColor = -1,
                            uint8_t glowSize = 12,
                            uint8_t glowStrength = 220,
                            GlowAnim anim = None,
                            uint16_t pulsePeriodMs = 1000);

        void updateGlowCircle(int16_t x, int16_t y,
                              int16_t r,
                              uint16_t fillColor,
                              int32_t bgColor = -1,
                              int32_t glowColor = -1,
                              uint8_t glowSize = 12,
                              uint8_t glowStrength = 220,
                              GlowAnim anim = None,
                              uint16_t pulsePeriodMs = 1000);

        void drawGlowRoundRect(int16_t x, int16_t y,
                               int16_t w, int16_t h,
                               uint8_t radius,
                               uint16_t fillColor,
                               int32_t bgColor = -1,
                               int32_t glowColor = -1,
                               uint8_t glowSize = 12,
                               uint8_t glowStrength = 220,
                               GlowAnim anim = None,
                               uint16_t pulsePeriodMs = 1000);

        void updateGlowRoundRect(int16_t x, int16_t y,
                                 int16_t w, int16_t h,
                                 uint8_t radius,
                                 uint16_t fillColor,
                                 int32_t bgColor = -1,
                                 int32_t glowColor = -1,
                                 uint8_t glowSize = 12,
                                 uint8_t glowStrength = 220,
                                 GlowAnim anim = None,
                                 uint16_t pulsePeriodMs = 1000);

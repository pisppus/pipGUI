#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

        pipcore::GuiDisplay &display()
        {
            assert(_display != nullptr);
            return *_display;
        }

        uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) const;
        uint16_t screenWidth() const;
        uint16_t screenHeight() const;

        void clear(uint32_t color = 0x000000);

        FillRectFluent fillRect() { return FillRectFluent(this); }
        FillRectAlphaFluent fillRectAlpha() { return FillRectAlphaFluent(this); }
        FillRectGradientVerticalFluent fillRectGradientVertical() { return FillRectGradientVerticalFluent(this); }
        FillRectGradientHorizontalFluent fillRectGradientHorizontal() { return FillRectGradientHorizontalFluent(this); }
        FillRectGradient4Fluent fillRectGradient4() { return FillRectGradient4Fluent(this); }
        FillRectGradientDiagonalFluent fillRectGradientDiagonal() { return FillRectGradientDiagonalFluent(this); }
        FillRectGradientRadialFluent fillRectGradientRadial() { return FillRectGradientRadialFluent(this); }

        DrawBlurStripFluent drawBlurStrip() { return DrawBlurStripFluent(this); }
        UpdateBlurStripFluent updateBlurStrip() { return UpdateBlurStripFluent(this); }

        DrawScrollDotsFluent drawScrollDots() { return DrawScrollDotsFluent(this); }
        UpdateScrollDotsFluent updateScrollDots() { return UpdateScrollDotsFluent(this); }

        DrawGlowCircleFluent drawGlowCircle() { return DrawGlowCircleFluent(this); }
        UpdateGlowCircleFluent updateGlowCircle() { return UpdateGlowCircleFluent(this); }

        DrawGlowRoundRectFluent drawGlowRoundRect() { return DrawGlowRoundRectFluent(this); }
        UpdateGlowRoundRectFluent updateGlowRoundRect() { return UpdateGlowRoundRectFluent(this); }

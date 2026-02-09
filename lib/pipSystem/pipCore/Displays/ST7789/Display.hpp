#pragma once

#include <pipCore/Platforms/GuiDisplay.hpp>
#include <pipCore/Displays/ST7789/Driver.hpp>

namespace pipcore
{
    class ST7789Display final : public pipcore::GuiDisplay
    {
    public:
        ST7789Display() = default;

        bool configure(int8_t mosi,
                       int8_t sclk,
                       int8_t cs,
                       int8_t dc,
                       int8_t rst,
                       uint16_t width,
                       uint16_t height,
                       uint32_t hz,
                       int8_t miso = -1,
                       bool bgr = false)
        {
            return _drv.configure(mosi, sclk, cs, dc, rst, width, height, hz, miso, bgr);
        }

        bool begin(uint8_t rotation) override { return _drv.begin(rotation); }
        uint16_t width() const override { return _drv.width(); }
        uint16_t height() const override { return _drv.height(); }

        void fillScreen565(uint16_t color565) override { _drv.fillScreen565(color565); }

        void writeRect565(int16_t x,
                          int16_t y,
                          int16_t w,
                          int16_t h,
                          const uint16_t *pixels,
                          int32_t stridePixels) override;

    private:
        ST7789 _drv;
    };
}

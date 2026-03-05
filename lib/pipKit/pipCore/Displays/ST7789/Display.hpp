#pragma once

#include <pipCore/Platforms/GuiDisplay.hpp>
#include <pipCore/Displays/ST7789/Driver.hpp>

namespace pipcore
{
    class ST7789Display final : public pipcore::GuiDisplay
    {
    public:
        ST7789Display() = default;
        ~ST7789Display();

        ST7789Display(const ST7789Display &) = delete;
        ST7789Display &operator=(const ST7789Display &) = delete;

        ST7789Display(ST7789Display &&) = default;
        ST7789Display &operator=(ST7789Display &&) = default;

        bool configure(ISt7789Transport *transport,
                       uint16_t width,
                       uint16_t height,
                       uint8_t order = 0,
                       bool invert = true,
                       bool swap = false,
                       int16_t xOffset = 0,
                       int16_t yOffset = 0)
        {
            return _drv.configure(transport, width, height, order, invert, swap, xOffset, yOffset);
        }

        bool begin(uint8_t rotation) override { return _drv.begin(rotation); }
        uint16_t width() const override { return _drv.width(); }
        uint16_t height() const override { return _drv.height(); }

        void fillScreen565(uint16_t color565) override { _drv.fillScreen565(color565, _drv.swapBytes()); }

        void writeRect565(int16_t x,
                          int16_t y,
                          int16_t w,
                          int16_t h,
                          const uint16_t *pixels,
                          int32_t stridePixels) override;

    private:
        ST7789 _drv;

        uint16_t *_lineBuf = nullptr;
        size_t _lineBufCapPixels = 0;
    };
}

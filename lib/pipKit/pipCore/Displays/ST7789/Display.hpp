#pragma once

#include <pipCore/Display.hpp>
#include <pipCore/Displays/ST7789/Driver.hpp>

namespace pipcore
{
    class Platform;
}

namespace pipcore::st7789
{
    class Display final : public pipcore::Display
    {
    public:
        Display() = default;
        ~Display() override;

        Display(const Display &) = delete;
        Display &operator=(const Display &) = delete;

        Display(Display &&) = delete;
        Display &operator=(Display &&) = delete;

        bool configure(pipcore::Platform *platform,
                       Transport *transport,
                       uint16_t width,
                       uint16_t height,
                       uint8_t order = 0,
                       bool invert = true,
                       bool swap = false,
                       int16_t xOffset = 0,
                       int16_t yOffset = 0)
        {
            _platform = platform;
            return _drv.configure(transport, width, height, order, invert, swap, xOffset, yOffset);
        }

        bool begin(uint8_t rotation) override { return _drv.begin(rotation); }
        uint16_t width() const override { return _drv.width(); }
        uint16_t height() const override { return _drv.height(); }
        void reset() { _drv.reset(); }
        IoError lastError() const { return _drv.lastError(); }
        const char *lastErrorText() const { return _drv.lastErrorText(); }

        void fillScreen565(uint16_t color565) override { (void)_drv.fillScreen565(color565, _drv.swapBytes()); }

        void writeRect565(int16_t x,
                          int16_t y,
                          int16_t w,
                          int16_t h,
                          const uint16_t *pixels,
                          int32_t stridePixels) override;

    private:
        pipcore::Platform *_platform = nullptr;
        Driver _drv;

        uint16_t *_lineBuf = nullptr;
        size_t _lineBufCapPixels = 0;
    };
}

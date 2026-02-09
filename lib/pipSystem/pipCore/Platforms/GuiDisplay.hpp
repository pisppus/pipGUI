#pragma once

#include <stdint.h>
#include <stddef.h>

namespace pipcore
{
    class GuiDisplay
    {
    public:
        virtual ~GuiDisplay() = default;

        virtual bool begin(uint8_t rotation) = 0;
        virtual uint16_t width() const = 0;
        virtual uint16_t height() const = 0;

        virtual void fillScreen565(uint16_t color565) = 0;

        virtual void writeRect565(int16_t x,
                                 int16_t y,
                                 int16_t w,
                                 int16_t h,
                                 const uint16_t *pixels,
                                 int32_t stridePixels) = 0;
    };
}

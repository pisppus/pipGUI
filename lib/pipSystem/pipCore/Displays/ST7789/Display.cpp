#include <pipCore/Displays/ST7789/Display.hpp>

#include <string.h>

namespace pipcore
{
    void ST7789Display::writeRect565(int16_t x,
                                     int16_t y,
                                     int16_t w,
                                     int16_t h,
                                     const uint16_t *pixels,
                                     int32_t stridePixels)
    {
        if (!pixels)
            return;
        if (w <= 0 || h <= 0)
            return;

        _drv.setAddrWindow(static_cast<uint16_t>(x),
                           static_cast<uint16_t>(y),
                           static_cast<uint16_t>(x + w - 1),
                           static_cast<uint16_t>(y + h - 1));

        if (stridePixels == w)
        {
            _drv.writePixels565(pixels, static_cast<size_t>(w) * static_cast<size_t>(h));
            return;
        }

        for (int16_t yy = 0; yy < h; ++yy)
        {
            const uint16_t *row = pixels + static_cast<size_t>(yy) * static_cast<size_t>(stridePixels);
            _drv.writePixels565(row, static_cast<size_t>(w));
        }
    }
}

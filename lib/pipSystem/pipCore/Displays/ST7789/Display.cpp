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

        const int16_t dispW = (int16_t)_drv.width();
        if (dispW <= 0)
            return;

        const int16_t mx = (int16_t)(dispW - x - w);

        _drv.setAddrWindow(static_cast<uint16_t>(mx),
                           static_cast<uint16_t>(y),
                           static_cast<uint16_t>(mx + w - 1),
                           static_cast<uint16_t>(y + h - 1));

        uint16_t tmp[320];
        if (w > (int16_t)(sizeof(tmp) / sizeof(tmp[0])))
            return;

        for (int16_t yy = 0; yy < h; ++yy)
        {
            const uint16_t *row = pixels + static_cast<size_t>(yy) * static_cast<size_t>(stridePixels);
            for (int16_t xx = 0; xx < w; ++xx)
                tmp[xx] = row[(int16_t)(w - 1 - xx)];
            _drv.writePixels565(tmp, static_cast<size_t>(w));
        }
    }
}

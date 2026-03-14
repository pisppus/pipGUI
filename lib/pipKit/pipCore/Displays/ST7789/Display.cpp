#include <pipCore/Displays/ST7789/Display.hpp>
#include <pipCore/Platform.hpp>

namespace pipcore::st7789
{
    namespace
    {
        inline uint16_t bswap16(uint16_t v) { return (uint16_t)__builtin_bswap16(v); }
    }

    Display::~Display()
    {
        if (_lineBuf && _platform)
        {
            _platform->free(_lineBuf);
            _lineBuf = nullptr;
            _lineBufCapPixels = 0;
        }
    }

    void Display::writeRect565(int16_t x, int16_t y, int16_t w, int16_t h,
                               const uint16_t *pixels, int32_t stridePixels)
    {
        if (!pixels || w <= 0 || h <= 0 || stridePixels < w)
            return;

        const int16_t dispW = (int16_t)_drv.width();
        const int16_t dispH = (int16_t)_drv.height();
        if (dispW <= 0 || dispH <= 0)
            return;

        int16_t x0 = x, y0 = y;
        int16_t x1 = x + w - 1, y1 = y + h - 1;
        if (x1 < 0 || y1 < 0 || x0 >= dispW || y0 >= dispH)
            return;
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= dispW)
            x1 = dispW - 1;
        if (y1 >= dispH)
            y1 = dispH - 1;

        const int16_t cW = x1 - x0 + 1;
        const int16_t cH = y1 - y0 + 1;

        pixels += (size_t)(y0 - y) * stridePixels + (x0 - x);

        if (!_drv.setAddrWindow((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1))
            return;

        const bool swap = _drv.swapBytes();
        const size_t totalPixels = (size_t)cW * (size_t)cH;

        if (cH == 1 || stridePixels == cW)
        {
            if (!_drv.writePixels565(pixels, totalPixels, swap))
                return;
            return;
        }

        if (swap)
        {
            const size_t need = (size_t)cW;
            if (_lineBufCapPixels < need)
            {
                uint16_t *newBuf = _platform ? (uint16_t *)_platform->alloc(need * sizeof(uint16_t), AllocCaps::PreferExternal) : nullptr;
                if (newBuf)
                {
                    if (_lineBuf)
                        _platform->free(_lineBuf);
                    _lineBuf = newBuf;
                    _lineBufCapPixels = need;
                }
            }
        }

        for (int16_t yy = 0; yy < cH; ++yy)
        {
            const uint16_t *row = pixels + (size_t)yy * stridePixels;
            if (!swap)
            {
                if (!_drv.writePixels565(row, (size_t)cW, false))
                    return;
                continue;
            }

            if (_lineBuf && _lineBufCapPixels >= (size_t)cW)
            {
                for (int16_t xx = 0; xx < cW; ++xx)
                    _lineBuf[xx] = bswap16(row[xx]);
                if (!_drv.writePixels565(_lineBuf, (size_t)cW, false))
                    return;
            }
            else
            {
                if (!_drv.writePixels565(row, (size_t)cW, true))
                    return;
            }
        }
    }
}

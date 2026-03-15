#include <pipCore/Displays/ST7789/Display.hpp>
#include <pipCore/Platform.hpp>
#include <algorithm>

namespace pipcore::st7789
{
    namespace
    {
        [[nodiscard]] inline constexpr uint16_t bswap16(uint16_t v) noexcept { return __builtin_bswap16(v); }

        inline void copySwap565(uint16_t *dst, const uint16_t *src, size_t pixels) noexcept
        {
            if (pixels == 0)
                return;

            const bool canUse32 = (((reinterpret_cast<uintptr_t>(src) | reinterpret_cast<uintptr_t>(dst)) & 1U) == 0U) &&
                                  ((reinterpret_cast<uintptr_t>(src) & 2U) == (reinterpret_cast<uintptr_t>(dst) & 2U));

            if (canUse32)
            {
                if ((reinterpret_cast<uintptr_t>(src) & 2U) != 0U)
                {
                    *dst++ = bswap16(*src++);
                    --pixels;
                }

                auto *dst32 = reinterpret_cast<uint32_t *>(dst);
                auto *src32 = reinterpret_cast<const uint32_t *>(src);
                size_t pairs = pixels >> 1;

                while (pairs--)
                {
                    const uint32_t p = __builtin_bswap32(*src32++);
                    *dst32++ = (p >> 16) | (p << 16);
                }

                src = reinterpret_cast<const uint16_t *>(src32);
                dst = reinterpret_cast<uint16_t *>(dst32);
                pixels &= 1U;
            }

            while (pixels--)
                *dst++ = bswap16(*src++);
        }
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

        const int32_t dispW = _drv.width();
        const int32_t dispH = _drv.height();
        if (dispW <= 0 || dispH <= 0)
            return;

        int32_t x0 = x;
        int32_t y0 = y;
        int32_t x1 = static_cast<int32_t>(x) + w - 1;
        int32_t y1 = static_cast<int32_t>(y) + h - 1;
        if (x1 < 0 || y1 < 0 || x0 >= dispW || y0 >= dispH)
            return;

        x0 = std::max<int32_t>(x0, 0);
        y0 = std::max<int32_t>(y0, 0);
        x1 = std::min<int32_t>(x1, dispW - 1);
        y1 = std::min<int32_t>(y1, dispH - 1);

        const int16_t cW = static_cast<int16_t>(x1 - x0 + 1);
        const int16_t cH = static_cast<int16_t>(y1 - y0 + 1);

        pixels += static_cast<size_t>(y0 - y) * stridePixels + (x0 - x);

        if (!_drv.setAddrWindow(static_cast<uint16_t>(x0), static_cast<uint16_t>(y0), static_cast<uint16_t>(x1), static_cast<uint16_t>(y1)))
            return;

        const bool swap = _drv.swapBytes();
        const size_t totalPixels = static_cast<size_t>(cW) * static_cast<size_t>(cH);

        if (cH == 1 || stridePixels == cW)
        {
            if (!_drv.writePixels565(pixels, totalPixels, swap))
                return;
            return;
        }

        if (swap)
        {
            const size_t need = static_cast<size_t>(cW);
            if (_lineBufCapPixels < need)
            {
                uint16_t *newBuf = _platform ? static_cast<uint16_t *>(_platform->alloc(need * sizeof(uint16_t), AllocCaps::Default)) : nullptr;
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
            const uint16_t *row = pixels + static_cast<size_t>(yy) * stridePixels;
            if (!swap)
            {
                if (!_drv.writePixels565(row, static_cast<size_t>(cW), false))
                    return;
                continue;
            }

            if (_lineBuf && _lineBufCapPixels >= static_cast<size_t>(cW))
            {
                copySwap565(_lineBuf, row, static_cast<size_t>(cW));
                if (!_drv.writePixels565(_lineBuf, static_cast<size_t>(cW), false))
                    return;
            }
            else
            {
                if (!_drv.writePixels565(row, static_cast<size_t>(cW), true))
                    return;
            }
        }
    }
}

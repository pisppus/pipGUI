#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Platforms/GuiDisplay.hpp>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace pipcore
{
    void Sprite::fillScreen(uint16_t color565)
    {
        if (!_buf || _w <= 0 || _h <= 0)
            return;

        size_t pixels = (size_t)_w * (size_t)_h;

        if ((color565 >> 8) == (color565 & 0xFF))
        {
            memset(_buf, color565 & 0xFF, pixels * 2);
            return;
        }

        uint16_t v = swap16(color565);
        uint32_t v32 = ((uint32_t)v << 16) | v;
        uint16_t *p = _buf;

        if ((uintptr_t)p & 2)
        {
            *p++ = v;
            pixels--;
        }

        uint32_t *p32 = (uint32_t *)p;
        size_t n32 = pixels >> 1;

        while (n32 >= 8)
        {
            p32[0] = p32[1] = p32[2] = p32[3] =
                p32[4] = p32[5] = p32[6] = p32[7] = v32;
            p32 += 8;
            n32 -= 8;
        }
        while (n32--)
            *p32++ = v32;
        if (pixels & 1)
            *(uint16_t *)p32 = v;
    }

    void Sprite::drawPixel(int16_t x, int16_t y, uint16_t color565)
    {
        if (((uint16_t)(x - _clipX) >= (uint16_t)_clipW) |
            ((uint16_t)(y - _clipY) >= (uint16_t)_clipH))
            return;
        *(_buf + y * _w + x) = __builtin_bswap16(color565);
    }

    inline void Sprite::fillRow(uint16_t *dst, int16_t w, uint16_t v)
    {
        if (w <= 0)
            return;

        if ((uint8_t)(v >> 8) == (uint8_t)v)
        {
            memset(dst, v & 0xFF, (size_t)w * 2);
            return;
        }

        if ((uintptr_t)dst & 2)
        {
            *dst++ = v;
            w--;
        }

        uint32_t v32 = ((uint32_t)v << 16) | v;
        uint32_t *d32 = (uint32_t *)dst;
        int16_t n32 = w >> 1;

        while (n32 >= 4)
        {
            *d32++ = v32;
            *d32++ = v32;
            *d32++ = v32;
            *d32++ = v32;
            n32 -= 4;
        }
        while (n32--)
            *d32++ = v32;
        if (w & 1)
            *(uint16_t *)d32 = v;
    }

    void Sprite::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565)
    {
        if (!_buf)
            return;

        int16_t rx1 = x > _clipX ? x : _clipX;
        int16_t ry1 = y > _clipY ? y : _clipY;
        int16_t rx2 = (x + w) < (_clipX + _clipW) ? (x + w) : (_clipX + _clipW);
        int16_t ry2 = (y + h) < (_clipY + _clipH) ? (y + h) : (_clipY + _clipH);

        w = rx2 - rx1;
        h = ry2 - ry1;
        if (w <= 0 || h <= 0)
            return;

        uint16_t v = swap16(color565);
        uint16_t *ptr = _buf + ry1 * _w + rx1;

        if (w == _w)
        {
            size_t totalPixels = (size_t)w * (size_t)h;
            if ((uint8_t)(v >> 8) == (uint8_t)v)
            {
                memset(ptr, v & 0xFF, totalPixels * 2);
                return;
            }
            if ((uintptr_t)ptr & 2)
            {
                *ptr++ = v;
                totalPixels--;
            }
            uint32_t v32 = ((uint32_t)v << 16) | v;
            uint32_t *p32 = (uint32_t *)ptr;
            size_t n32 = totalPixels >> 1;
            while (n32 >= 4)
            {
                *p32++ = v32;
                *p32++ = v32;
                *p32++ = v32;
                *p32++ = v32;
                n32 -= 4;
            }
            while (n32--)
                *p32++ = v32;
            if (totalPixels & 1)
                *(uint16_t *)p32 = v;
            return;
        }

        while (h--)
        {
            fillRow(ptr, w, v);
            ptr += _w;
        }
    }

    void Sprite::pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *__restrict pixels565)
    {
        if (!_buf || !pixels565 || _clipW <= 0 || _clipH <= 0)
            return;

        int16_t rx1 = x > _clipX ? x : _clipX;
        int16_t ry1 = y > _clipY ? y : _clipY;
        int16_t rx2 = (x + w) < (_clipX + _clipW) ? (x + w) : (_clipX + _clipW);
        int16_t ry2 = (y + h) < (_clipY + _clipH) ? (y + h) : (_clipY + _clipH);

        int16_t cw = rx2 - rx1, ch = ry2 - ry1;
        if (cw <= 0 || ch <= 0)
            return;

        const uint16_t *srcLine = pixels565 + (size_t)(ry1 - y) * w + (rx1 - x);
        uint16_t *dstLine = _buf + (size_t)ry1 * _w + rx1;

        bool canUse32 = (((uintptr_t)srcLine | (uintptr_t)dstLine) & 1) == 0 &&
                        (((uintptr_t)srcLine & 2) == ((uintptr_t)dstLine & 2));

        while (ch--)
        {
            const uint16_t *src = srcLine;
            uint16_t *dst = dstLine;
            int16_t n = cw;

            if (canUse32)
            {
                if ((uintptr_t)src & 2)
                {
                    *dst++ = pipcore::Sprite::swap16(*src++);
                    n--;
                }

                uint32_t *d32 = (uint32_t *)dst;
                const uint32_t *s32 = (const uint32_t *)src;
                int16_t n32 = n >> 1;

                while (n32 >= 4)
                {
                    uint32_t p0 = __builtin_bswap32(s32[0]), p1 = __builtin_bswap32(s32[1]);
                    uint32_t p2 = __builtin_bswap32(s32[2]), p3 = __builtin_bswap32(s32[3]);
                    d32[0] = (p0 >> 16) | (p0 << 16);
                    d32[1] = (p1 >> 16) | (p1 << 16);
                    d32[2] = (p2 >> 16) | (p2 << 16);
                    d32[3] = (p3 >> 16) | (p3 << 16);
                    d32 += 4;
                    s32 += 4;
                    n32 -= 4;
                }
                while (n32--)
                {
                    uint32_t p = __builtin_bswap32(*s32++);
                    *d32++ = (p >> 16) | (p << 16);
                }

                src = (const uint16_t *)s32;
                dst = (uint16_t *)d32;
                n &= 1;
            }

            while (n >= 4)
            {
                dst[0] = pipcore::Sprite::swap16(src[0]);
                dst[1] = pipcore::Sprite::swap16(src[1]);
                dst[2] = pipcore::Sprite::swap16(src[2]);
                dst[3] = pipcore::Sprite::swap16(src[3]);
                dst += 4;
                src += 4;
                n -= 4;
            }
            while (n--)
                *dst++ = pipcore::Sprite::swap16(*src++);

            srcLine += w;
            dstLine += _w;
        }
    }
}
#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Display.hpp>
#include <pipCore/Platform.hpp>
#include <cstring>
#include <algorithm>

namespace pipcore
{
    Sprite::~Sprite() { deleteSprite(); }

    bool Sprite::createSprite(int16_t w, int16_t h)
    {
        deleteSprite();
        if (w <= 0 || h <= 0)
            return false;

        Platform *plat = platform();
        if (!plat)
            return false;

        const size_t pixels = (size_t)w * (size_t)h;
        if (pixels > SIZE_MAX / sizeof(uint16_t))
            return false;

        _buf = (uint16_t *)plat->alloc(pixels * sizeof(uint16_t), AllocCaps::PreferExternal);
        if (!_buf)
            return false;

        _w = w;
        _h = h;
        _clipX = _clipY = 0;
        _clipW = w;
        _clipH = h;
        return true;
    }

    void Sprite::deleteSprite()
    {
        if (_buf)
        {
            Platform *plat = platform();
            if (plat)
                plat->free(_buf);
            _buf = nullptr;
        }
        _w = _h = _clipX = _clipY = _clipW = _clipH = 0;
    }

    void Sprite::clipNormalize()
    {
        if (_w <= 0 || _h <= 0)
        {
            _clipX = _clipY = _clipW = _clipH = 0;
            return;
        }

        int16_t x1 = std::max<int16_t>(0, _clipX);
        int16_t y1 = std::max<int16_t>(0, _clipY);
        int16_t x2 = std::min<int16_t>(_w, _clipX + std::max<int16_t>(0, _clipW));
        int16_t y2 = std::min<int16_t>(_h, _clipY + std::max<int16_t>(0, _clipH));

        _clipX = x1;
        _clipY = y1;
        _clipW = std::max<int16_t>(0, x2 - x1);
        _clipH = std::max<int16_t>(0, y2 - y1);
    }

    void Sprite::setClipRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        _clipX = x;
        _clipY = y;
        _clipW = w;
        _clipH = h;
        clipNormalize();
    }

    void Sprite::getClipRect(int32_t *x, int32_t *y, int32_t *w, int32_t *h) const
    {
        if (x)
            *x = _clipX;
        if (y)
            *y = _clipY;
        if (w)
            *w = _clipW;
        if (h)
            *h = _clipH;
    }

    void Sprite::pushSprite(Sprite *dst, int16_t x, int16_t y) const
    {
        if (!dst || !_buf || !dst->_buf || _clipW <= 0 || _clipH <= 0 || dst->_clipW <= 0 || dst->_clipH <= 0)
            return;

        const int32_t srcClipX1 = _clipX;
        const int32_t srcClipY1 = _clipY;
        const int32_t srcClipX2 = (int32_t)_clipX + _clipW;
        const int32_t srcClipY2 = (int32_t)_clipY + _clipH;

        const int32_t dstClipX1 = dst->_clipX;
        const int32_t dstClipY1 = dst->_clipY;
        const int32_t dstClipX2 = (int32_t)dst->_clipX + dst->_clipW;
        const int32_t dstClipY2 = (int32_t)dst->_clipY + dst->_clipH;

        const int32_t x1 = std::max<int32_t>((int32_t)x + srcClipX1, dstClipX1);
        const int32_t y1 = std::max<int32_t>((int32_t)y + srcClipY1, dstClipY1);
        const int32_t x2 = std::min<int32_t>((int32_t)x + srcClipX2, dstClipX2);
        const int32_t y2 = std::min<int32_t>((int32_t)y + srcClipY2, dstClipY2);

        const int32_t cw = x2 - x1;
        const int32_t ch = y2 - y1;
        if (cw <= 0 || ch <= 0)
            return;

        const int32_t srcX = x1 - x;
        const int32_t srcY = y1 - y;
        const size_t bytes = (size_t)cw * sizeof(uint16_t);

        if (dst == this)
        {
            const int32_t srcX2 = srcX + cw;
            const int32_t srcY2 = srcY + ch;
            const int32_t dstX2 = x1 + cw;
            const int32_t dstY2 = y1 + ch;
            const bool overlap = (x1 < srcX2) && (dstX2 > srcX) && (y1 < srcY2) && (dstY2 > srcY);

            if (overlap && y1 > srcY)
            {
                for (int32_t row = ch - 1; row >= 0; --row)
                {
                    uint16_t *dstRow = dst->_buf + ((size_t)(y1 + row) * dst->_w + x1);
                    const uint16_t *srcRow = _buf + ((size_t)(srcY + row) * _w + srcX);
                    memmove(dstRow, srcRow, bytes);
                }
                return;
            }

            for (int32_t row = 0; row < ch; ++row)
            {
                uint16_t *dstRow = dst->_buf + ((size_t)(y1 + row) * dst->_w + x1);
                const uint16_t *srcRow = _buf + ((size_t)(srcY + row) * _w + srcX);
                memmove(dstRow, srcRow, bytes);
            }
            return;
        }

        const uint16_t *src = _buf + ((size_t)srcY * _w + srcX);
        uint16_t *dst_ = dst->_buf + ((size_t)y1 * dst->_w + x1);

        for (int32_t row = 0; row < ch; ++row)
        {
            memcpy(dst_, src, bytes);
            src += _w;
            dst_ += dst->_w;
        }
    }

    void Sprite::writeToDisplay(Display &display, int16_t x, int16_t y, int16_t w, int16_t h) const
    {
        if (!_buf || w <= 0 || h <= 0)
            return;

        int16_t x1 = std::max<int16_t>(x, _clipX);
        int16_t y1 = std::max<int16_t>(y, _clipY);
        int16_t x2 = std::min<int16_t>(x + w, _clipX + _clipW);
        int16_t y2 = std::min<int16_t>(y + h, _clipY + _clipH);

        int16_t cw = x2 - x1, ch = y2 - y1;
        if (cw <= 0 || ch <= 0)
            return;

        display.writeRect565(x1, y1, cw, ch, _buf + (size_t)y1 * _w + x1, _w);
    }
}

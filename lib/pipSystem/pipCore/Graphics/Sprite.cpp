#include <pipCore/Graphics/Sprite.hpp>

#include <pipCore/Platforms/GuiDisplay.hpp>

#include <stdlib.h>
#include <string.h>

namespace pipcore
{
    Sprite::~Sprite()
    {
        deleteSprite();
    }

    bool Sprite::createSprite(int16_t w, int16_t h)
    {
        deleteSprite();

        if (w <= 0 || h <= 0)
            return false;

        const size_t pixels = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (pixels == 0 || pixels > (SIZE_MAX / sizeof(uint16_t)))
            return false;

        _buf = static_cast<uint16_t *>(malloc(pixels * sizeof(uint16_t)));
        if (!_buf)
            return false;

        _w = w;
        _h = h;
        _clipX = 0;
        _clipY = 0;
        _clipW = w;
        _clipH = h;

        return true;
    }

    void Sprite::deleteSprite()
    {
        if (_buf)
        {
            free(_buf);
            _buf = nullptr;
        }
        _w = 0;
        _h = 0;
        _clipX = 0;
        _clipY = 0;
        _clipW = 0;
        _clipH = 0;
    }

    void Sprite::clipNormalize()
    {
        if (_w <= 0 || _h <= 0)
        {
            _clipX = 0;
            _clipY = 0;
            _clipW = 0;
            _clipH = 0;
            return;
        }

        if (_clipW < 0)
            _clipW = 0;
        if (_clipH < 0)
            _clipH = 0;

        if (_clipX < 0)
        {
            _clipW = static_cast<int16_t>(_clipW + _clipX);
            _clipX = 0;
        }
        if (_clipY < 0)
        {
            _clipH = static_cast<int16_t>(_clipH + _clipY);
            _clipY = 0;
        }

        if (_clipX + _clipW > _w)
            _clipW = static_cast<int16_t>(_w - _clipX);
        if (_clipY + _clipH > _h)
            _clipH = static_cast<int16_t>(_h - _clipY);

        if (_clipW < 0)
            _clipW = 0;
        if (_clipH < 0)
            _clipH = 0;
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

    bool Sprite::clipTest(int16_t x, int16_t y) const
    {
        if (x < _clipX || y < _clipY)
            return false;
        if (x >= _clipX + _clipW)
            return false;
        if (y >= _clipY + _clipH)
            return false;
        return true;
    }

    void Sprite::pushSprite(Sprite *dst, int16_t x, int16_t y) const
    {
        if (!dst)
            return;
        if (!_buf || !dst->_buf)
            return;

        const int16_t srcW = _w;
        const int16_t srcH = _h;
        const int16_t dstW = dst->_w;
        const int16_t dstH = dst->_h;

        int16_t sx0 = 0;
        int16_t sy0 = 0;
        int16_t dx0 = x;
        int16_t dy0 = y;

        int16_t w = srcW;
        int16_t h = srcH;

        if (dx0 < 0)
        {
            sx0 = static_cast<int16_t>(-dx0);
            w = static_cast<int16_t>(w + dx0);
            dx0 = 0;
        }
        if (dy0 < 0)
        {
            sy0 = static_cast<int16_t>(-dy0);
            h = static_cast<int16_t>(h + dy0);
            dy0 = 0;
        }

        if (dx0 + w > dstW)
            w = static_cast<int16_t>(dstW - dx0);
        if (dy0 + h > dstH)
            h = static_cast<int16_t>(dstH - dy0);

        if (w <= 0 || h <= 0)
            return;

        const size_t srcStride = static_cast<size_t>(srcW);
        const size_t dstStride = static_cast<size_t>(dstW);

        for (int16_t yy = 0; yy < h; ++yy)
        {
            const uint16_t *s = _buf + static_cast<size_t>(sy0 + yy) * srcStride + static_cast<size_t>(sx0);
            uint16_t *d = dst->_buf + static_cast<size_t>(dy0 + yy) * dstStride + static_cast<size_t>(dx0);
            memcpy(d, s, static_cast<size_t>(w) * sizeof(uint16_t));
        }
    }

    void Sprite::writeToDisplay(GuiDisplay &display, int16_t x, int16_t y, int16_t w, int16_t h) const
    {
        if (!_buf)
            return;
        if (w <= 0 || h <= 0)
            return;
        display.writeRect565(x, y, w, h, _buf + static_cast<size_t>(y) * static_cast<size_t>(_w) + static_cast<size_t>(x), _w);
    }
}

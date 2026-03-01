#include <pipCore/Graphics/Sprite.hpp>
#include <pipCore/Platforms/GuiDisplay.hpp>
#include <pipCore/Platforms/PlatformFactory.hpp>
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

        const size_t pixels = (size_t)w * (size_t)h;
        if (pixels > SIZE_MAX / sizeof(uint16_t))
            return false;

        _buf = (uint16_t *)GetPlatform()->guiAlloc(pixels * sizeof(uint16_t), GuiAllocCaps::PreferExternal);
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
            GetPlatform()->guiFree(_buf);
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

    bool Sprite::clipTest(int16_t x, int16_t y) const
    {
        return (uint16_t)(x - _clipX) < (uint16_t)_clipW &&
               (uint16_t)(y - _clipY) < (uint16_t)_clipH;
    }

    void Sprite::pushSprite(Sprite *dst, int16_t x, int16_t y) const
    {
        if (!dst || !_buf || !dst->_buf)
            return;

        int16_t x1 = std::max<int16_t>(x, dst->_clipX);
        int16_t y1 = std::max<int16_t>(y, dst->_clipY);
        int16_t x2 = std::min<int16_t>(x + _w, dst->_clipX + dst->_clipW);
        int16_t y2 = std::min<int16_t>(y + _h, dst->_clipY + dst->_clipH);

        int16_t cw = x2 - x1, ch = y2 - y1;
        if (cw <= 0 || ch <= 0)
            return;

        const uint16_t *src = _buf + (size_t)(y1 - y) * _w + (x1 - x);
        uint16_t *dst_ = dst->_buf + (size_t)y1 * dst->_w + x1;
        const size_t bytes = (size_t)cw * sizeof(uint16_t);

        while (ch--)
        {
            memcpy(dst_, src, bytes);
            src += _w;
            dst_ += dst->_w;
        }
    }

    void Sprite::writeToDisplay(GuiDisplay &display, int16_t x, int16_t y, int16_t w, int16_t h) const
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
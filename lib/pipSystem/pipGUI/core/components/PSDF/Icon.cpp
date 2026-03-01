#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipGUI/icons/icons.hpp>
#include <pipGUI/icons/metrics.hpp>

namespace pipgui
{
    static inline uint8_t pgmAtIcons(pipcore::GuiPlatform *plat, int x, int y)
    {
        if ((uint32_t)x >= (uint32_t)psdf_icons::AtlasWidth || (uint32_t)y >= (uint32_t)psdf_icons::AtlasHeight)
            return 0;
        uint32_t idx = (uint32_t)y * (uint32_t)psdf_icons::AtlasWidth + (uint32_t)x;
        return plat->readProgmemByte(&icons[idx]);
    }

    static inline uint8_t sampleBilinearIcons(pipcore::GuiPlatform *plat, float u, float v)
    {
        int x0 = (int)u;
        int y0 = (int)v;
        float fx = u - (float)x0;
        float fy = v - (float)y0;

        uint8_t s00 = pgmAtIcons(plat, x0, y0);
        uint8_t s10 = pgmAtIcons(plat, x0 + 1, y0);
        uint8_t s01 = pgmAtIcons(plat, x0, y0 + 1);
        uint8_t s11 = pgmAtIcons(plat, x0 + 1, y0 + 1);

        float a0 = (float)s00 + ((float)s10 - (float)s00) * fx;
        float a1 = (float)s01 + ((float)s11 - (float)s01) * fx;
        float a = a0 + (a1 - a0) * fy;
        int ia = (int)(a + 0.5f);
        if (ia < 0)
            ia = 0;
        if (ia > 255)
            ia = 255;
        return (uint8_t)ia;
    }

    static inline void iconBox(uint16_t iconId, uint16_t &outX, uint16_t &outY, uint16_t &outW, uint16_t &outH)
    {
        if (iconId >= psdf_icons::IconCount)
        {
            outX = outY = outW = outH = 0;
            return;
        }
        const psdf_icons::Icon &ic = psdf_icons::Icons[iconId];
        outX = ic.x;
        outY = ic.y;
        outW = ic.w;
        outH = ic.h;
    }

    void GUI::drawIconInternal(uint16_t iconId,
                               int16_t x,
                               int16_t y,
                               uint16_t sizePx,
                               uint16_t fg565)
    {
        uint16_t srcX = 0, srcY = 0, srcW = 0, srcH = 0;
        iconBox(iconId, srcX, srcY, srcW, srcH);
        if (srcW == 0 || srcH == 0 || sizePx == 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;

        float drScale = psdf_icons::DistanceRange * ((float)sizePx / psdf_icons::NominalSizePx);
        float weightBias = _typo.psdfWeightBias;

        pipcore::Sprite *spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int16_t ix0 = rx;
        int16_t iy0 = ry;
        int16_t ix1 = rx + (int16_t)sizePx;
        int16_t iy1 = ry + (int16_t)sizePx;

        if (ix1 <= 0 || iy1 <= 0 || ix0 >= stride || iy0 >= maxH)
            return;
        if (ix0 < 0)
            ix0 = 0;
        if (iy0 < 0)
            iy0 = 0;
        if (ix1 > stride)
            ix1 = stride;
        if (iy1 > maxH)
            iy1 = maxH;

        float invSize = 1.0f / (float)sizePx;

        for (int16_t py = iy0; py < iy1; ++py)
        {
            float v = ((float)(py - ry) + 0.5f) * invSize;
            float sy = (float)srcY + (float)srcH * v;
            int32_t row = (int32_t)py * stride;

            for (int16_t px = ix0; px < ix1; ++px)
            {
                float u = ((float)(px - rx) + 0.5f) * invSize;
                float sx = (float)srcX + (float)srcW * u;

                uint8_t s8 = sampleBilinearIcons(platform(), sx, sy);
                float sd = ((float)s8 * (1.0f / 255.0f) - 0.5f) * drScale;
                float a = sd + 0.5f + weightBias;

                if (a <= 0.0f)
                    continue;

                int32_t idx = row + px;
                if (a >= 1.0f)
                {
                    buf[idx] = pipcore::Sprite::swap16(fg565);
                }
                else
                {
                    uint8_t alpha = (uint8_t)(a * 255.0f + 0.5f);
                    uint16_t dst = pipcore::Sprite::swap16(buf[idx]);
                    buf[idx] = pipcore::Sprite::swap16(detail::blend565(dst, fg565, alpha));
                }
            }
        }
    }

    void GUI::updateIconInternal(uint16_t iconId,
                                 int16_t x,
                                 int16_t y,
                                 uint16_t sizePx,
                                 uint16_t fg565,
                                 uint16_t bg565)
    {
        if (sizePx == 0)
            return;

        int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;

        const int16_t pad = 2;
        int16_t bx = rx - pad;
        int16_t by = ry - pad;
        int16_t bw = (int16_t)sizePx + pad * 2;
        int16_t bh = (int16_t)sizePx + pad * 2;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.renderToSprite = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().at(bx, by).size(bw, bh).color(bg565).draw();
        drawIconInternal(iconId, rx, ry, sizePx, fg565);

        _flags.renderToSprite = prevRender;
        _render.activeSprite = prevActive;

        invalidateRect(bx, by, bw, bh);
        flushDirty();
    }

    void DrawIconFluent::draw()
    {
        if (_consumed || !_gui)
            return;
        _consumed = true;
        if (_sizePx == 0)
            return;
        if (_gui->_flags.spriteEnabled && _gui->_disp.display && !_gui->_flags.renderToSprite)
            _gui->updateIconInternal(_iconId, _x, _y, _sizePx, _fg565, _bg565);
        else
            _gui->drawIconInternal(_iconId, _x, _y, _sizePx, _fg565);
    }

}

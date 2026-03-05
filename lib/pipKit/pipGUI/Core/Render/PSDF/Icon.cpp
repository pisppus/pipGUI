#include <math.h>
#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipGUI/core/Render/Draw/Blend.hpp>
#include <pipGUI/icons/icons.hpp>
#include <pipGUI/icons/metrics.hpp>

namespace pipgui
{

    static inline uint8_t sampleBilinear(pipcore::GuiPlatform *plat,
                                         int32_t u16, int32_t v16,
                                         uint32_t atlasW, uint32_t atlasH)
    {
        auto rd = [&](int x, int y) -> int32_t
        {
            if ((unsigned)x >= atlasW || (unsigned)y >= atlasH)
                return 0;
            return (int32_t)plat->readProgmemByte(&icons[(uint32_t)y * atlasW + x]);
        };
        return sampleBilinearSDF(rd, u16, v16, atlasW, atlasH);
    }

    void GUI::drawIconInternal(uint16_t iconId,
                               int16_t x, int16_t y,
                               uint16_t sizePx,
                               uint16_t fg565)
    {
        if (iconId >= psdf_icons::IconCount || sizePx == 0)
            return;
        const psdf_icons::Icon &ic = psdf_icons::Icons[iconId];
        if (ic.w == 0 || ic.h == 0)
            return;

        pipcore::Sprite *spr = _render.activeSprite ? _render.activeSprite : &_render.sprite;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width(), maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        const int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        const int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;

        int16_t ix0 = rx, iy0 = ry;
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

        const float distanceScale = (float)psdf_icons::DistanceRange * ((float)sizePx / (float)psdf_icons::NominalSizePx);
        const float weightBias = _typo.psdfWeightBias;
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = 0.5f - distanceScale * 0.5f + weightBias;

        float dthr = (0.f - kOffset) / kScale;
        uint8_t s8Min = (dthr > 0.f && dthr < 255.f) ? (uint8_t)dthr : 0;

        const uint32_t atlasW = (uint32_t)psdf_icons::AtlasWidth;
        const uint32_t atlasH = (uint32_t)psdf_icons::AtlasHeight;
        const uint16_t fgNative = pipcore::Sprite::swap16(fg565);
        pipcore::GuiPlatform *plat = platform();

        const int32_t duFP = (int32_t)((float)ic.w / (float)sizePx * 65536.f);
        const int32_t dvFP = (int32_t)((float)ic.h / (float)sizePx * 65536.f);

        const int32_t u0FP = (int32_t)((float)ic.x * 65536.f) + (int32_t)(((float)(ix0 - rx) + 0.5f) / (float)sizePx * (float)ic.w * 65536.f);
        const int32_t v0FP = (int32_t)((float)ic.y * 65536.f) + (int32_t)(((float)(iy0 - ry) + 0.5f) / (float)sizePx * (float)ic.h * 65536.f);

        int32_t vFP = v0FP;
        for (int16_t py = iy0; py < iy1; ++py, vFP += dvFP)
        {
            const int32_t row = (int32_t)py * stride;
            int32_t uFP = u0FP;

            for (int16_t px = ix0; px < ix1; ++px, uFP += duFP)
            {
                uint8_t s8 = sampleBilinear(plat, uFP, vFP, atlasW, atlasH);
                if (s8 <= s8Min)
                    continue;

                const float a = (float)s8 * kScale + kOffset;
                const int32_t idx = row + px;
                if (a >= 1.f)
                {
                    buf[idx] = fgNative;
                }
                else
                {
                    const uint8_t alpha = (uint8_t)(a * 255.f + 0.5f);
                    const uint16_t dst = pipcore::Sprite::swap16(buf[idx]);
                    buf[idx] = pipcore::Sprite::swap16(detail::blend565(dst, fg565, alpha));
                }
            }
        }
    }

    void GUI::updateIconInternal(uint16_t iconId,
                                 int16_t x, int16_t y,
                                 uint16_t sizePx,
                                 uint16_t fg565,
                                 uint16_t bg565)
    {
        if (sizePx == 0)
            return;

        const int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        const int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;
        constexpr int16_t pad = 2;

        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.renderToSprite = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().at(rx - pad, ry - pad).size(sizePx + pad * 2, sizePx + pad * 2).color(bg565).draw();
        drawIconInternal(iconId, rx, ry, sizePx, fg565);

        _flags.renderToSprite = prevRender;
        _render.activeSprite = prevActive;

        invalidateRect(rx - pad, ry - pad, sizePx + pad * 2, sizePx + pad * 2);
        flushDirty();
    }

    void DrawIconFluent::draw()
    {
        if (_consumed || !_gui || _sizePx == 0)
            return;
        _consumed = true;

        if (_gui->_flags.spriteEnabled && _gui->_disp.display && !_gui->_flags.renderToSprite)
            _gui->updateIconInternal(_iconId, _x, _y, _sizePx, _fg565, _bg565);
        else
            _gui->drawIconInternal(_iconId, _x, _y, _sizePx, _fg565);
    }
}
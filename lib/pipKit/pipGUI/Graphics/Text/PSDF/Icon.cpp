#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/API/Internal/BuilderAccess.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Draw/Blend.hpp>
#include <pipGUI/Graphics/Text/Icons/icons.hpp>
#include <pipGUI/Graphics/Text/Icons/metrics.hpp>

namespace pipgui
{

    static inline uint8_t sampleBilinear(pipcore::Platform *plat,
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

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t stride = spr->width(), maxH = spr->height();
        if (stride <= 0 || maxH <= 0)
            return;

        int32_t clipX = 0;
        int32_t clipY = 0;
        int32_t clipW = stride;
        int32_t clipH = maxH;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;
        const int32_t clipR = clipX + clipW;
        const int32_t clipB = clipY + clipH;

        uint16_t renderSizePx = sizePx;
        int16_t inset = 0;
        if (iconId == psdf_icons::IconErrorLayer0)
        {
            // The triangle warning/error glyph has a larger visual footprint than the circular one.
            // Shrink it slightly so all icons look equal when placed in the same square.
            renderSizePx = (uint16_t)((sizePx * 86U) / 100U);
            if (renderSizePx == 0)
                renderSizePx = 1;
            inset = (int16_t)((int32_t)sizePx - (int32_t)renderSizePx) / 2;
        }

        const int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        const int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;

        const int16_t drawX = (int16_t)(rx + inset);
        const int16_t drawY = (int16_t)(ry + inset);

        int16_t ix0 = drawX, iy0 = drawY;
        int16_t ix1 = drawX + (int16_t)renderSizePx;
        int16_t iy1 = drawY + (int16_t)renderSizePx;

        if (ix1 <= clipX || iy1 <= clipY || ix0 >= clipR || iy0 >= clipB)
            return;
        if (ix0 < clipX)
            ix0 = (int16_t)clipX;
        if (iy0 < clipY)
            iy0 = (int16_t)clipY;
        if (ix1 > clipR)
            ix1 = (int16_t)clipR;
        if (iy1 > clipB)
            iy1 = (int16_t)clipB;

        const float distanceScale = (float)psdf_icons::DistanceRange * ((float)renderSizePx / (float)psdf_icons::NominalSizePx);
        constexpr float iconBias = 0.0f;
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = 0.5f - distanceScale * 0.5f + iconBias;

        float dthr = (0.f - kOffset) / kScale;
        uint8_t s8Min = (dthr > 0.f && dthr < 255.f) ? (uint8_t)dthr : 0;

        const uint32_t atlasW = (uint32_t)psdf_icons::AtlasWidth;
        const uint32_t atlasH = (uint32_t)psdf_icons::AtlasHeight;
        const uint16_t fgNative = pipcore::Sprite::swap16(fg565);
        pipcore::Platform *plat = platform();

        const int32_t duFP = (int32_t)((float)ic.w / (float)renderSizePx * 65536.f);
        const int32_t dvFP = (int32_t)((float)ic.h / (float)renderSizePx * 65536.f);

        const int32_t u0FP = (int32_t)((float)ic.x * 65536.f) + (int32_t)(((float)(ix0 - drawX) + 0.5f) / (float)renderSizePx * (float)ic.w * 65536.f);
        const int32_t v0FP = (int32_t)((float)ic.y * 65536.f) + (int32_t)(((float)(iy0 - drawY) + 0.5f) / (float)renderSizePx * (float)ic.h * 65536.f);

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

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        fillRect().pos(rx - pad, ry - pad).size(sizePx + pad * 2, sizePx + pad * 2).color(bg565).draw();
        drawIconInternal(iconId, rx, ry, sizePx, fg565);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect(rx - pad, ry - pad, sizePx + pad * 2, sizePx + pad * 2);
    }

    void DrawIconFluent::draw()
    {
        if (_sizePx == 0 || !beginCommit())
            return;
        detail::BuilderAccess::drawIcon(*_gui, _iconId, _x, _y, _sizePx, _fg565, _bg565);
    }
}

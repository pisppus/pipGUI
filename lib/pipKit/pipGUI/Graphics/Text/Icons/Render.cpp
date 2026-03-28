#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Graphics/Draw/Blend.hpp>
#include <pipGUI/Graphics/Text/Icons/Metrics.hpp>
#include <pipGUI/Graphics/Text/Icons/AnimMetrics.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        struct AlphaLut
        {
            uint8_t values[256];
            uint8_t firstNonZero;
        };

        struct NativeColor565
        {
            uint16_t native;
            uint32_t fgRb;
            uint32_t fgG;
        };

        static inline NativeColor565 makeNativeColor565(uint16_t color565)
        {
            return {
                pipcore::Sprite::swap16(color565),
                ((color565 & 0xF800u) << 5) | (color565 & 0x001Fu),
                color565 & 0x07E0u};
        }

        static inline void blendNative565(uint16_t *dst, const NativeColor565 &color, uint8_t alpha)
        {
            if (alpha == 0)
                return;
            if (alpha == 255)
            {
                *dst = color.native;
                return;
            }

            const uint16_t bg = pipcore::Sprite::swap16(*dst);
            const uint32_t inv = 255u - alpha;
            const uint32_t bgRb = ((uint32_t)(bg & 0xF800u) << 5) | (bg & 0x001Fu);
            const uint32_t bgG = bg & 0x07E0u;
            const uint32_t rb = ((color.fgRb * alpha + bgRb * inv) >> 8) & 0x001F001Fu;
            const uint32_t g = ((color.fgG * alpha + bgG * inv) >> 8) & 0x000007E0u;
            *dst = pipcore::Sprite::swap16((uint16_t)((rb >> 5) | rb | g));
        }

        static inline const AlphaLut &alphaLutFor(float kScale, float kOffset)
        {
            static AlphaLut lut{};
            static float cachedScale = 0.0f;
            static float cachedOffset = 0.0f;
            static bool ready = false;

            if (ready && cachedScale == kScale && cachedOffset == kOffset)
                return lut;

            for (uint32_t sample = 0; sample < 256; ++sample)
            {
                float alpha = (float)sample * kScale + kOffset;
                if (alpha <= 0.0f)
                    lut.values[sample] = 0;
                else if (alpha >= 1.0f)
                    lut.values[sample] = 255;
                else
                    lut.values[sample] = (uint8_t)(alpha * 255.0f + 0.5f);
            }
            lut.firstNonZero = 0;
            while (lut.firstNonZero < 255 && lut.values[lut.firstNonZero] == 0)
                ++lut.firstNonZero;
            cachedScale = kScale;
            cachedOffset = kOffset;
            ready = true;
            return lut;
        }

        struct IconRowSampler
        {
            pipcore::Platform *plat;
            const uint8_t *row0;
            const uint8_t *row1;
            uint32_t atlasW;
            uint32_t fy;

            inline uint8_t sample(int32_t u16) const
            {
                if (!row0 || !row1)
                    return 0;
                int x0 = u16 >> 16;
                int x1 = x0 + 1;
                if (x0 < 0)
                    x0 = 0;
                else if ((uint32_t)x0 >= atlasW)
                    x0 = (int)atlasW - 1;
                if (x1 < 0)
                    x1 = 0;
                else if ((uint32_t)x1 >= atlasW)
                    x1 = (int)atlasW - 1;

                const uint32_t fx = (uint32_t)u16 & 0xFFFFu;
                const int32_t a00 = (int32_t)plat->readProgmemByte(row0 + x0);
                const int32_t a10 = (int32_t)plat->readProgmemByte(row0 + x1);
                const int32_t a01 = (int32_t)plat->readProgmemByte(row1 + x0);
                const int32_t a11 = (int32_t)plat->readProgmemByte(row1 + x1);
                const int32_t a0 = (a00 << 16) + (a10 - a00) * (int32_t)fx;
                const int32_t a1 = (a01 << 16) + (a11 - a01) * (int32_t)fx;
                int32_t alpha = ((a0 + (int32_t)(((int64_t)(a1 - a0) * fy) >> 16)) + 0x8000) >> 16;
                if (alpha < 0)
                    alpha = 0;
                else if (alpha > 255)
                    alpha = 255;
                return (uint8_t)alpha;
            }

        };

        static inline IconRowSampler makeIconRowSampler(pipcore::Platform *plat,
                                                        const uint8_t *atlas,
                                                        int32_t v16, uint32_t atlasW, uint32_t atlasH)
        {
            int y0 = v16 >> 16;
            int y1 = y0 + 1;
            if (y0 < 0)
                y0 = 0;
            else if ((uint32_t)y0 >= atlasH)
                y0 = (int)atlasH - 1;
            if (y1 < 0)
                y1 = 0;
            else if ((uint32_t)y1 >= atlasH)
                y1 = (int)atlasH - 1;
            return {
                plat,
                &atlas[(uint32_t)y0 * atlasW],
                &atlas[(uint32_t)y1 * atlasW],
                atlasW,
                (uint32_t)v16 & 0xFFFFu};
        }

        struct Affine2D
        {
            float m00;
            float m01;
            float m02;
            float m10;
            float m11;
            float m12;
        };

        [[nodiscard]] static inline Affine2D multiplyAffine(const Affine2D &a, const Affine2D &b) noexcept
        {
            return {
                a.m00 * b.m00 + a.m01 * b.m10,
                a.m00 * b.m01 + a.m01 * b.m11,
                a.m00 * b.m02 + a.m01 * b.m12 + a.m02,
                a.m10 * b.m00 + a.m11 * b.m10,
                a.m10 * b.m01 + a.m11 * b.m11,
                a.m10 * b.m02 + a.m11 * b.m12 + a.m12,
            };
        }

        [[nodiscard]] static inline Affine2D translation(float tx, float ty) noexcept
        {
            return {1.0f, 0.0f, tx, 0.0f, 1.0f, ty};
        }

        [[nodiscard]] static inline Affine2D scaling(float sx, float sy) noexcept
        {
            return {sx, 0.0f, 0.0f, 0.0f, sy, 0.0f};
        }

        [[nodiscard]] static inline Affine2D rotation(float radians) noexcept
        {
            const float c = cosf(radians);
            const float s = sinf(radians);
            return {c, -s, 0.0f, s, c, 0.0f};
        }

        [[nodiscard]] static inline bool invertAffine(const Affine2D &m, Affine2D &out) noexcept
        {
            const float det = m.m00 * m.m11 - m.m01 * m.m10;
            if (fabsf(det) < 1e-6f)
                return false;
            const float invDet = 1.0f / det;
            out.m00 = m.m11 * invDet;
            out.m01 = -m.m01 * invDet;
            out.m10 = -m.m10 * invDet;
            out.m11 = m.m00 * invDet;
            out.m02 = -(out.m00 * m.m02 + out.m01 * m.m12);
            out.m12 = -(out.m10 * m.m02 + out.m11 * m.m12);
            return true;
        }

        [[nodiscard]] static inline float lerpFloat(float a, float b, float t) noexcept
        {
            return a + (b - a) * t;
        }

        struct AnimFrameState
        {
            float posX;
            float posY;
            float anchorX;
            float anchorY;
            float scaleX;
            float scaleY;
            float rotationDeg;
            uint8_t opacity;
        };

        [[nodiscard]] static inline AnimFrameState sampleAnimatedFrame(const psdf_anim::AnimatedIcon &icon,
                                                                       const psdf_anim::Layer &layer,
                                                                       uint32_t nowMs) noexcept
        {
            if (icon.frameCount == 0)
                return {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0u};

            const float fps = (float)icon.frameRateX100 / 100.0f;
            const float framePos = fps > 0.0f ? fmodf((float)nowMs * fps / 1000.0f, (float)icon.frameCount) : 0.0f;
            const uint16_t frameIndex = (uint16_t)framePos;
            const uint16_t nextFrame = (frameIndex + 1U <= icon.frameCount) ? (uint16_t)(frameIndex + 1U) : icon.frameCount;
            const float t = framePos - (float)frameIndex;

            const auto &a = psdf_anim::Frames[layer.frameOffset + frameIndex];
            const auto &b = psdf_anim::Frames[layer.frameOffset + nextFrame];
            constexpr float kInv = 1.0f / (float)psdf_anim::TransformScale;

            return {
                lerpFloat((float)a.posX, (float)b.posX, t) * kInv,
                lerpFloat((float)a.posY, (float)b.posY, t) * kInv,
                lerpFloat((float)a.anchorX, (float)b.anchorX, t) * kInv,
                lerpFloat((float)a.anchorY, (float)b.anchorY, t) * kInv,
                lerpFloat((float)a.scaleX, (float)b.scaleX, t) * (kInv / 100.0f),
                lerpFloat((float)a.scaleY, (float)b.scaleY, t) * (kInv / 100.0f),
                lerpFloat((float)a.rotation, (float)b.rotation, t) * kInv,
                (uint8_t)(lerpFloat((float)a.opacity, (float)b.opacity, t) + 0.5f),
            };
        }
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
        if (iconId == psdf_icons::IconError)
        {
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
        const float kScale = distanceScale * (1.f / 255.f);
        const float kOffset = 0.5f - distanceScale * 0.5f;

        const uint32_t atlasW = (uint32_t)psdf_icons::AtlasWidth;
        const uint32_t atlasH = (uint32_t)psdf_icons::AtlasHeight;
        const AlphaLut &alphaLut = alphaLutFor(kScale, kOffset);
        const uint8_t s8Min = alphaLut.firstNonZero;

        const NativeColor565 fg = makeNativeColor565(fg565);
        pipcore::Platform *const plat = platform();

        const int32_t duFP = (int32_t)((float)ic.w / (float)renderSizePx * 65536.f);
        const int32_t dvFP = (int32_t)((float)ic.h / (float)renderSizePx * 65536.f);

        const int32_t u0FP = (int32_t)((float)ic.x * 65536.f) + (int32_t)(((float)(ix0 - drawX) + 0.5f) / (float)renderSizePx * (float)ic.w * 65536.f);
        const int32_t v0FP = (int32_t)((float)ic.y * 65536.f) + (int32_t)(((float)(iy0 - drawY) + 0.5f) / (float)renderSizePx * (float)ic.h * 65536.f);

        int32_t vFP = v0FP;
        for (int16_t py = iy0; py < iy1; ++py, vFP += dvFP)
        {
            const IconRowSampler rowSampler = makeIconRowSampler(plat, icons, vFP, atlasW, atlasH);
            int32_t uFP = u0FP;
            uint16_t *dst = buf + (int32_t)py * stride + ix0;

            for (int16_t px = ix0; px < ix1; ++px, ++dst, uFP += duFP)
            {
                const uint8_t s8 = rowSampler.sample(uFP);
                if (s8 <= s8Min)
                    continue;

                const uint8_t alpha = alphaLut.values[s8];
                if (alpha)
                    blendNative565(dst, fg, alpha);
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

        drawRect().pos(rx - pad, ry - pad).size(sizePx + pad * 2, sizePx + pad * 2).fill(bg565).draw();
        drawIconInternal(iconId, rx, ry, sizePx, fg565);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect(rx - pad, ry - pad, sizePx + pad * 2, sizePx + pad * 2);
    }

    void GUI::drawAnimatedIconInternal(uint16_t iconId,
                                       int16_t x, int16_t y,
                                       uint16_t sizePx,
                                       uint16_t fg565,
                                       uint32_t timeMs)
    {
        if (iconId >= psdf_anim::AnimatedIconCount || sizePx == 0)
            return;

        const auto &icon = psdf_anim::Icons[iconId];
        if (icon.layerCount == 0 || icon.frameCount == 0 || icon.width == 0 || icon.height == 0)
            return;

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr)
            return;
        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        const int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;

        const float box = (float)sizePx;
        const float scale = box / (float)((icon.width > icon.height) ? icon.width : icon.height);
        const float drawW = (float)icon.width * scale;
        const int16_t stride = spr->width();
        const int16_t maxH = spr->height();
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

        const float drawH = (float)icon.height * scale;
        const float drawX = (float)rx + (box - drawW) * 0.5f;
        const float drawY = (float)ry + (box - drawH) * 0.5f;
        const NativeColor565 fg = makeNativeColor565(fg565);
        pipcore::Platform *const plat = platform();
        const uint32_t atlasW = (uint32_t)psdf_anim::AtlasWidth;
        const uint32_t atlasH = (uint32_t)psdf_anim::AtlasHeight;

        for (uint8_t layerOffset = 0; layerOffset < icon.layerCount; ++layerOffset)
        {
            const auto &layer = psdf_anim::Layers[icon.firstLayer + layerOffset];
            const auto &glyph = psdf_anim::Glyphs[layer.glyphIndex];
            if (glyph.w == 0 || glyph.h == 0)
                continue;
            const AnimFrameState frame = sampleAnimatedFrame(icon, layer, timeMs);
            if (frame.opacity == 0)
                continue;

            const Affine2D compToScreen = multiplyAffine(translation(drawX, drawY), scaling(scale, scale));
            const Affine2D layerTransform = multiplyAffine(
                translation(frame.posX, frame.posY),
                multiplyAffine(
                    rotation(frame.rotationDeg * (3.1415926535f / 180.0f)),
                    multiplyAffine(
                        scaling(frame.scaleX, frame.scaleY),
                        translation(-frame.anchorX, -frame.anchorY))));
            const Affine2D matrix = multiplyAffine(compToScreen, layerTransform);

            float px0 = matrix.m02;
            float py0 = matrix.m12;
            float px1 = matrix.m00 * (float)icon.width + matrix.m02;
            float py1 = matrix.m10 * (float)icon.width + matrix.m12;
            float px2 = matrix.m00 * (float)icon.width + matrix.m01 * (float)icon.height + matrix.m02;
            float py2 = matrix.m10 * (float)icon.width + matrix.m11 * (float)icon.height + matrix.m12;
            float px3 = matrix.m01 * (float)icon.height + matrix.m02;
            float py3 = matrix.m11 * (float)icon.height + matrix.m12;

            int16_t ix0 = (int16_t)floorf(fminf(fminf(px0, px1), fminf(px2, px3)));
            int16_t iy0 = (int16_t)floorf(fminf(fminf(py0, py1), fminf(py2, py3)));
            int16_t ix1 = (int16_t)ceilf(fmaxf(fmaxf(px0, px1), fmaxf(px2, px3)));
            int16_t iy1 = (int16_t)ceilf(fmaxf(fmaxf(py0, py1), fmaxf(py2, py3)));

            if (ix1 <= clipX || iy1 <= clipY || ix0 >= clipR || iy0 >= clipB)
                continue;
            if (ix0 < clipX)
                ix0 = (int16_t)clipX;
            if (iy0 < clipY)
                iy0 = (int16_t)clipY;
            if (ix1 > clipR)
                ix1 = (int16_t)clipR;
            if (iy1 > clipB)
                iy1 = (int16_t)clipB;

            Affine2D inverse{};
            if (!invertAffine(matrix, inverse))
                continue;

            const float renderSizePx = fmaxf(
                hypotf(matrix.m00 * (float)icon.width, matrix.m10 * (float)icon.width),
                hypotf(matrix.m01 * (float)icon.height, matrix.m11 * (float)icon.height));
            const float distanceScale = (float)psdf_anim::DistanceRange * (renderSizePx / (float)psdf_anim::NominalSizePx);
            const float kScale = distanceScale * (1.0f / 255.0f);
            const float kOffset = 0.5f - distanceScale * 0.5f;
            const AlphaLut &alphaLut = alphaLutFor(kScale, kOffset);
            const uint8_t s8Min = alphaLut.firstNonZero;

            for (int16_t py = iy0; py < iy1; ++py)
            {
                uint16_t *dst = buf + (int32_t)py * stride + ix0;
                for (int16_t px = ix0; px < ix1; ++px, ++dst)
                {
                    const float sx = inverse.m00 * ((float)px + 0.5f) + inverse.m01 * ((float)py + 0.5f) + inverse.m02;
                    const float sy = inverse.m10 * ((float)px + 0.5f) + inverse.m11 * ((float)py + 0.5f) + inverse.m12;
                    if (sx < 0.0f || sy < 0.0f || sx > (float)icon.width || sy > (float)icon.height)
                        continue;

                    const float u = ((sx / (float)icon.width) * (float)glyph.w) + (float)glyph.x - 0.5f;
                    const float v = ((sy / (float)icon.height) * (float)glyph.h) + (float)glyph.y - 0.5f;
                    const IconRowSampler rowSampler = makeIconRowSampler(plat, animIcons, (int32_t)(v * 65536.0f), atlasW, atlasH);
                    const uint8_t s8 = rowSampler.sample((int32_t)(u * 65536.0f));
                    if (s8 <= s8Min)
                        continue;

                    uint16_t alpha = alphaLut.values[s8];
                    alpha = (uint16_t)((alpha * frame.opacity + 127U) / 255U);
                    if (alpha)
                        blendNative565(dst, fg, (uint8_t)alpha);
                }
            }
        }
    }

    void GUI::updateAnimatedIconInternal(uint16_t iconId,
                                         int16_t x, int16_t y,
                                         uint16_t sizePx,
                                         uint16_t fg565,
                                         uint16_t bg565,
                                         uint32_t nowMs)
    {
        if (sizePx == 0)
            return;
        const int16_t rx = (x == -1) ? AutoX((int32_t)sizePx) : x;
        const int16_t ry = (y == -1) ? AutoY((int32_t)sizePx) : y;
        constexpr int16_t pad = 3;

        bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        drawRect().pos(rx - pad, ry - pad).size(sizePx + pad * 2, sizePx + pad * 2).fill(bg565).draw();
        drawAnimatedIconInternal(iconId, rx, ry, sizePx, fg565, nowMs);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (!prevRender)
            invalidateRect(rx - pad, ry - pad, sizePx + pad * 2, sizePx + pad * 2);
    }

    void DrawIconFluent::draw()
    {
        if (_sizePx == 0 || !beginCommit())
            return;
        detail::GuiAccess::drawIcon(*_gui, _iconId, _x, _y, _sizePx, _fg565, _bg565);
    }

    template <bool IsUpdate>
    void AnimIconFluentT<IsUpdate>::draw()
    {
        if (_sizePx == 0 || !beginCommit())
            return;
        if constexpr (IsUpdate)
            detail::GuiAccess::updateAnimIcon(*_gui, _iconId, _x, _y, _sizePx, _fg565, _bg565, 0);
        else
            detail::GuiAccess::drawAnimIcon(*_gui, _iconId, _x, _y, _sizePx, _fg565, 0);
    }
    template void AnimIconFluentT<false>::draw();
    template void AnimIconFluentT<true>::draw();
}

#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Easing.hpp>
#include <math.h>

namespace pipgui
{
    namespace
    {
        [[nodiscard]] uint32_t hashDrumRollKey(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t count, bool vertical) noexcept
        {
            uint32_t hash = 2166136261u;
            auto mix = [&](uint32_t value)
            {
                hash ^= value;
                hash *= 16777619u;
            };

            mix(static_cast<uint16_t>(x));
            mix(static_cast<uint16_t>(y));
            mix(static_cast<uint16_t>(w));
            mix(static_cast<uint16_t>(h));
            mix(count);
            mix(vertical ? 1u : 0u);
            return hash ? hash : 1u;
        }

        struct DrumRollVisual
        {
            uint8_t alpha = 0;
            float scale = 1.0f;
        };

        [[nodiscard]] uint16_t resolveDrumRollFontPx(uint16_t fontPx, int16_t extent, uint16_t maxPx) noexcept
        {
            if (fontPx > 0)
                return fontPx;
            if (extent <= 4)
                return 8;

            const uint16_t px = static_cast<uint16_t>(extent - 4);
            const uint16_t clamped = (px > maxPx) ? maxPx : px;
            return clamped >= 8 ? clamped : 8;
        }

        [[nodiscard]] DrumRollVisual drumRollVisual(float indexDistance) noexcept
        {
            DrumRollVisual out;
            if (indexDistance >= 1.35f)
                return out;

            float t = indexDistance / 1.35f;
            if (t < 0.0f)
                t = 0.0f;
            else if (t > 1.0f)
                t = 1.0f;

            const float focus = 1.0f - t;
            const float eased = focus * focus * (3.0f - 2.0f * focus);
            out.alpha = static_cast<uint8_t>(96.0f + 159.0f * eased + 0.5f);
            out.scale = 0.86f + 0.14f * eased;
            return out;
        }

        [[nodiscard]] float drumRollCurrentIndex(const detail::DrumRollAnimState &state, uint32_t now) noexcept
        {
            if (state.startMs == 0 || state.durationMs == 0 || now <= state.startMs)
                return state.fromIndex;

            const uint32_t elapsed = now - state.startMs;
            if (elapsed >= state.durationMs)
                return static_cast<float>(state.toIndex);

            const float t = static_cast<float>(elapsed) / static_cast<float>(state.durationMs);
            const float eased = detail::motion::easeInOutCubic(t);
            return state.fromIndex + (static_cast<float>(state.toIndex) - state.fromIndex) * eased;
        }
    }

    void GUI::drawDrumRollHorizontal(int16_t x, int16_t y, int16_t w, int16_t h,
                                     const String *options, uint8_t count, uint8_t selectedIndex,
                                     uint32_t fgColor, uint32_t bgColor, uint16_t fontPx, uint16_t animDurationMs)
    {
        if (!options || count == 0)
            return;
        if (selectedIndex >= count)
            selectedIndex = (uint8_t)(count - 1);

        const uint16_t savePx = _typo.psdfSizePx;
        const uint16_t saveWeight = _typo.psdfWeight;
        const uint16_t basePx = resolveDrumRollFontPx(fontPx, h, 255);
        setFontWeight(Medium);
        setFontSize(basePx);

        int16_t slotW = 0;
        for (uint8_t i = 0; i < count; ++i)
        {
            int16_t tw = 0, th = 0;
            if (measureText(options[i], tw, th))
            {
                if (tw + 8 > slotW)
                    slotW = (int16_t)(tw + 8);
            }
        }
        if (slotW < 8)
            slotW = 8;

        const uint32_t now = nowMs();
        detail::DrumRollAnimState &state = resolveDrumRollState(hashDrumRollKey(x, y, w, h, count, false), selectedIndex, animDurationMs);
        const float currentIndex = drumRollCurrentIndex(state, now);
        const float offsetX = (float)(x + w / 2) - (currentIndex + 0.5f) * (float)slotW;
        const uint16_t fg565 = detail::color888To565(fgColor);
        const uint16_t bg565 = detail::color888To565(bgColor);

        pipcore::Sprite *spr = getDrawTarget();
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        if (spr)
        {
            spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
            spr->setClipRect(x, y, w, h);
        }

        for (uint8_t i = 0; i < count; ++i)
        {
            setFontSize(basePx);
            int16_t tw = 0, th = 0;
            if (!measureText(options[i], tw, th))
                continue;
            const float itemCenterX = offsetX + (float)(i * slotW) + (float)slotW * 0.5f;
            const DrumRollVisual visual = drumRollVisual(fabsf(static_cast<float>(i) - currentIndex));
            if (visual.alpha == 0)
                continue;
            const uint16_t itemPx = static_cast<uint16_t>(basePx * visual.scale + 0.5f);
            setFontSize(itemPx >= 8 ? itemPx : 8);
            if (!measureText(options[i], tw, th))
                continue;
            const int16_t tx = static_cast<int16_t>(itemCenterX - tw * 0.5f);
            if (tx + tw < x || tx > x + w)
                continue;

            const uint16_t text565 = (visual.alpha < 255) ? detail::blend565(bg565, fg565, visual.alpha) : fg565;
            const int16_t itemY = y + (h - th) / 2;
            drawTextAligned(options[i], tx, itemY >= y ? itemY : y, text565, bg565, TextAlign::Left);
        }

        if (currentIndex != static_cast<float>(state.toIndex))
            requestRedraw();

        if (spr)
            spr->setClipRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
        setFontSize(savePx);
        setFontWeight(saveWeight);
    }

    void GUI::drawDrumRollVertical(int16_t x, int16_t y, int16_t w, int16_t h,
                                   const String *options, uint8_t count, uint8_t selectedIndex,
                                   uint32_t fgColor, uint32_t bgColor, uint16_t fontPx, uint16_t animDurationMs)
    {
        if (!options || count == 0)
            return;
        if (selectedIndex >= count)
            selectedIndex = (uint8_t)(count - 1);

        const uint16_t savePx = _typo.psdfSizePx;
        const uint16_t saveWeight = _typo.psdfWeight;
        const uint16_t basePx = resolveDrumRollFontPx(fontPx, w, 24);
        setFontWeight(Medium);
        setFontSize(basePx);

        int16_t slotH = 0;
        for (uint8_t i = 0; i < count; ++i)
        {
            int16_t tw = 0, th = 0;
            if (measureText(options[i], tw, th))
            {
                if (th + 4 > slotH)
                    slotH = (int16_t)(th + 4);
            }
        }
        if (slotH < 8)
            slotH = 8;

        const uint32_t now = nowMs();
        detail::DrumRollAnimState &state = resolveDrumRollState(hashDrumRollKey(x, y, w, h, count, true), selectedIndex, animDurationMs);
        const float currentIndex = drumRollCurrentIndex(state, now);
        const float offsetY = (float)(y + h / 2) - (currentIndex + 0.5f) * (float)slotH;
        const uint16_t fg565 = detail::color888To565(fgColor);
        const uint16_t bg565 = detail::color888To565(bgColor);

        pipcore::Sprite *spr = getDrawTarget();
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        if (spr)
        {
            spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
            spr->setClipRect(x, y, w, h);
        }

        for (uint8_t i = 0; i < count; ++i)
        {
            const float itemCenterY = offsetY + (float)(i * slotH) + (float)slotH * 0.5f;
            const DrumRollVisual visual = drumRollVisual(fabsf(static_cast<float>(i) - currentIndex));
            if (visual.alpha == 0)
                continue;

            const uint16_t itemPx = static_cast<uint16_t>(basePx * visual.scale + 0.5f);
            setFontSize(itemPx >= 8 ? itemPx : 8);

            int16_t tw = 0, th = 0;
            if (!measureText(options[i], tw, th))
                continue;
            const int16_t ty = static_cast<int16_t>(itemCenterY - th * 0.5f);
            if (ty + th < y || ty > y + h)
                continue;

            const uint16_t text565 = (visual.alpha < 255) ? detail::blend565(bg565, fg565, visual.alpha) : fg565;
            const int16_t itemX = x + (w - tw) / 2;
            drawTextAligned(options[i], itemX >= x ? itemX : x, ty, text565, bg565, TextAlign::Left);
        }

        if (currentIndex != static_cast<float>(state.toIndex))
            requestRedraw();

        if (spr)
            spr->setClipRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
        setFontSize(savePx);
        setFontWeight(saveWeight);
    }
}

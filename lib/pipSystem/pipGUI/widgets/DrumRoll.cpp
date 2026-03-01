#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>

namespace pipgui
{
    static float easeInOutCubic(float t)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        if (t < 0.5f)
            return 4.0f * t * t * t;
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }


    void GUI::drawDrumRollHorizontal(int16_t x, int16_t y, int16_t w, int16_t h,
                                     const String *options, uint8_t count,
                                     uint8_t selectedIndex, uint8_t prevIndex, float animProgress,
                                     uint32_t fgColor, uint32_t bgColor, uint16_t fontPx)
    {
        if (!options || count == 0)
            return;
        if (selectedIndex >= count)
            selectedIndex = (uint8_t)(count - 1);
        if (prevIndex >= count)
            prevIndex = selectedIndex;
        if (animProgress < 0.0f) animProgress = 0.0f;
        if (animProgress > 1.0f) animProgress = 1.0f;

        uint16_t savePx = _typo.psdfSizePx;
        if (fontPx > 0)
            setPSDFFontSize(fontPx);
        else if (h > 4)
            setPSDFFontSize((uint16_t)(h - 4) < 8 ? 8 : (uint16_t)(h - 4));

        int16_t slotW = 0;
        int16_t lineH = 0;
        for (uint8_t i = 0; i < count; i++)
        {
            int16_t tw = 0, th = 0;
            if (psdfMeasureText(options[i], tw, th))
            {
                if (tw + 8 > slotW)
                    slotW = (int16_t)(tw + 8);
                if (th > lineH)
                    lineH = th;
            }
        }
        if (slotW < 8)
            slotW = 8;

        float eased = easeInOutCubic(animProgress);
        float currentIndex = (float)prevIndex + ((float)selectedIndex - (float)prevIndex) * eased;
        float offsetX = (float)(x + w / 2) - (currentIndex + 0.5f) * (float)slotW;

        int16_t windowX = x;
        int16_t windowW = w;

        pipcore::Sprite *spr = getDrawTarget();
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        if (spr)
        {
            spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
            spr->setClipRect(windowX, y, windowW, h);
        }

        int16_t baseY = y + (h - lineH) / 2;
        if (baseY < y) baseY = y;

        for (uint8_t i = 0; i < count; i++)
        {
            float slotCenter = (float)i + 0.5f;
            float dist = (float)fabs((double)(slotCenter - currentIndex - 0.5f));
            float fade = (dist <= 0.5f) ? 1.0f : (float)max(0.0, 1.5 - (double)dist);
            uint8_t alpha = (uint8_t)(255.0f * fade + 0.5f);

            int16_t tx = (int16_t)(offsetX + (float)(i * slotW) + (float)(slotW / 2));
            int16_t tw = 0, th = 0;
            if (!psdfMeasureText(options[i], tw, th))
                continue;
            tx -= (int16_t)(tw / 2);
            if (tx + tw < windowX || tx > windowX + windowW)
                continue;
            uint16_t fg565 = detail::color888To565(fgColor);
            uint16_t bg565 = detail::color888To565(bgColor);
            if (alpha < 255)
                fg565 = detail::blend565(bg565, fg565, alpha);
            psdfDrawTextInternal(options[i], tx, baseY, fg565, bg565, AlignLeft);
        }

        if (spr)
            spr->setClipRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
        setPSDFFontSize(savePx);
    }

    void GUI::drawDrumRollVertical(int16_t x, int16_t y, int16_t w, int16_t h,
                                   const String *options, uint8_t count,
                                   uint8_t selectedIndex, uint8_t prevIndex, float animProgress,
                                   uint32_t fgColor, uint32_t bgColor, uint16_t fontPx)
    {
        if (!options || count == 0)
            return;
        if (selectedIndex >= count)
            selectedIndex = (uint8_t)(count - 1);
        if (prevIndex >= count)
            prevIndex = selectedIndex;
        if (animProgress < 0.0f) animProgress = 0.0f;
        if (animProgress > 1.0f) animProgress = 1.0f;

        uint16_t savePx = _typo.psdfSizePx;
        if (fontPx > 0)
            setPSDFFontSize(fontPx);
        else if (w > 4)
            setPSDFFontSize((uint16_t)((w > 24) ? 24 : w - 4) < 8 ? 8 : (uint16_t)((w > 24) ? 24 : w - 4));

        int16_t slotH = 0;
        int16_t maxW = 0;
        for (uint8_t i = 0; i < count; i++)
        {
            int16_t tw = 0, th = 0;
            if (psdfMeasureText(options[i], tw, th))
            {
                if (th + 4 > slotH)
                    slotH = (int16_t)(th + 4);
                if (tw > maxW)
                    maxW = tw;
            }
        }
        if (slotH < 8)
            slotH = 8;

        float eased = easeInOutCubic(animProgress);
        float currentIndex = (float)prevIndex + ((float)selectedIndex - (float)prevIndex) * eased;
        float offsetY = (float)(y + h / 2) - (currentIndex + 0.5f) * (float)slotH;

        int16_t windowY = y;
        int16_t windowH = h;

        pipcore::Sprite *spr = getDrawTarget();
        int32_t clipX = 0, clipY = 0, clipW = 0, clipH = 0;
        if (spr)
        {
            spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
            spr->setClipRect(x, windowY, w, windowH);
        }

        int16_t baseX = x + (w - maxW) / 2;
        if (baseX < x) baseX = x;

        for (uint8_t i = 0; i < count; i++)
        {
            float slotCenter = (float)i + 0.5f;
            float dist = (float)fabs((double)(slotCenter - currentIndex - 0.5f));
            float fade = (dist <= 0.5f) ? 1.0f : (float)max(0.0, 1.5 - (double)dist);
            uint8_t alpha = (uint8_t)(255.0f * fade + 0.5f);

            int16_t ty = (int16_t)(offsetY + (float)(i * slotH) + (float)(slotH / 2));
            int16_t tw = 0, th = 0;
            if (!psdfMeasureText(options[i], tw, th))
                continue;
            ty -= (int16_t)(th / 2);
            if (ty + th < windowY || ty > windowY + windowH)
                continue;
            uint16_t fg565 = detail::color888To565(fgColor);
            uint16_t bg565 = detail::color888To565(bgColor);
            if (alpha < 255)
                fg565 = detail::blend565(bg565, fg565, alpha);
            psdfDrawTextInternal(options[i], baseX, ty, fg565, bg565, AlignCenter);
        }

        if (spr)
            spr->setClipRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
        setPSDFFontSize(savePx);
    }
}

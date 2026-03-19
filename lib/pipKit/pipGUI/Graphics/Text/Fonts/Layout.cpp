#include "Internal.hpp"

namespace pipgui
{
    bool GUI::measureText(const String &text, int16_t &outW, int16_t &outH) const
    {
        outW = outH = 0;
        const FontData *font = fontDataForId(_typo.currentFontId);
        if (!_typo.psdfSizePx || !font)
            return false;
        int len = (int)text.length();
        if (!len)
            return true;

        const float sizePx = (float)_typo.psdfSizePx;
        uint16_t lines = 1;
        float maxX = 0.f, curX = 0.f;

        forEachGlyph(text.c_str(), len, font, sizePx,
                     [&](const Glyph *g, float penX, float, bool nl) -> bool
                     {
                         if (nl)
                         {
                             if (penX > maxX)
                                 maxX = penX;
                             curX = 0.f;
                             ++lines;
                         }
                         else
                         {
                             curX = penX + g->unpackAdvance() * sizePx;
                         }
                         return true;
                     });

        if (curX > maxX)
            maxX = curX;
        outW = (int16_t)ceilToInt(maxX);
        outH = (int16_t)ceilToInt((float)lines * font->lineHeight * sizePx);

        const int16_t weightExpandX = weightExpandXPxInt(_typo.psdfWeight, sizePx);
        const int16_t weightExpandY = weightExpandYPxInt(_typo.psdfWeight, sizePx);
        if (weightExpandX > 0)
            outW = (int16_t)(outW + weightExpandX * 2);
        if (weightExpandY > 0)
        {
            outH = (int16_t)(outH + weightExpandY * 2);
        }
        return true;
    }

    bool GUI::drawTextMarquee(const String &text, int16_t x, int16_t y,
                              int16_t maxWidth, uint16_t fg565,
                              TextAlign align, const MarqueeTextOptions &opts)
    {
        if (maxWidth <= 0)
            return false;

        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return false;

        int16_t boxX = (x == -1) ? AutoX((int32_t)maxWidth) : x;
        if (align == AlignCenter)
            boxX -= maxWidth / 2;
        else if (align == AlignRight)
            boxX -= maxWidth;

        const int16_t boxY = (y == -1) ? AutoY((int32_t)th) : y;
        if (tw <= maxWidth)
            return false;
        const int16_t weightExpandX = weightExpandXPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);
        const int16_t weightExpandY = weightExpandYPxInt(_typo.psdfWeight, (float)_typo.psdfSizePx);

        pipcore::Sprite *target = getDrawTarget();
        if (!target)
            return false;

        int32_t prevClipX = 0, prevClipY = 0, prevClipW = 0, prevClipH = 0;
        target->getClipRect(&prevClipX, &prevClipY, &prevClipW, &prevClipH);

        int32_t clipX = boxX;
        int32_t clipY = boxY;
        int32_t clipW = maxWidth;
        int32_t clipH = th;

        if (prevClipW > 0 && prevClipH > 0)
        {
            const int32_t prevRight = prevClipX + prevClipW;
            const int32_t prevBottom = prevClipY + prevClipH;
            const int32_t boxRight = clipX + clipW;
            const int32_t boxBottom = clipY + clipH;

            if (clipX < prevClipX)
                clipX = prevClipX;
            if (clipY < prevClipY)
                clipY = prevClipY;

            const int32_t finalRight = (boxRight < prevRight) ? boxRight : prevRight;
            const int32_t finalBottom = (boxBottom < prevBottom) ? boxBottom : prevBottom;
            clipW = finalRight - clipX;
            clipH = finalBottom - clipY;
        }

        if (clipW <= 0 || clipH <= 0)
        {
            target->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);
            return true;
        }

        target->setClipRect(clipX, clipY, clipW, clipH);

        const uint16_t speedPxPerSec = opts.speedPxPerSec ? opts.speedPxPerSec : 28;
        const uint16_t holdStartMs = opts.holdStartMs;
        const int32_t loopPx = tw + kMarqueeGapPx;

        const uint32_t now = nowMs();
        uint32_t elapsedMs = now;
        if (opts.phaseStartMs != 0)
            elapsedMs = (now >= opts.phaseStartMs) ? (now - opts.phaseStartMs) : 0U;

        int16_t offsetPx = 0;
        if (speedPxPerSec > 0 && loopPx > 0 && elapsedMs > holdStartMs)
        {
            const uint64_t distanceMilliPx = (uint64_t)(elapsedMs - holdStartMs) * speedPxPerSec;
            const uint64_t loopMilliPx = (uint64_t)loopPx * 1000ULL;
            const uint64_t wrappedMilliPx = loopMilliPx ? (distanceMilliPx % loopMilliPx) : 0ULL;
            offsetPx = (int16_t)((wrappedMilliPx + 500ULL) / 1000ULL);
            if (offsetPx >= loopPx)
                offsetPx = 0;
        }

        const int16_t drawX = (int16_t)(boxX - offsetPx + weightExpandX);
        drawTextImmediateMasked(text, drawX, boxY + weightExpandY,
                                tw, th, fg565, 0, AlignLeft, boxX, maxWidth, kMarqueeEdgeFadePx);
        if (speedPxPerSec > 0 && loopPx > 0)
        {
            drawTextImmediateMasked(text, (int16_t)(drawX + loopPx), boxY + weightExpandY,
                                    tw, th, fg565, 0, AlignLeft, boxX, maxWidth, kMarqueeEdgeFadePx);
        }

        target->setClipRect(prevClipX, prevClipY, prevClipW, prevClipH);
        if (speedPxPerSec > 0)
            requestRedraw();
        return true;
    }

    bool GUI::drawTextEllipsized(const String &text, int16_t x, int16_t y,
                                 int16_t maxWidth, uint16_t fg565,
                                 TextAlign align)
    {
        if (maxWidth <= 0)
            return false;

        int16_t tw = 0, th = 0;
        if (!measureText(text, tw, th) || tw <= 0 || th <= 0)
            return false;
        if (tw <= maxWidth)
            return false;

        int16_t dotsW = 0, dotsH = 0;
        const String dots("...");
        if (!measureText(dots, dotsW, dotsH))
            return false;

        String clipped;
        if (dotsW >= maxWidth)
        {
            clipped = dots;
        }
        else
        {
            String candidate = text;
            int16_t width = 0, height = 0;
            while (candidate.length() > 0)
            {
                const size_t cut = prevUtf8Boundary(candidate, candidate.length());
                candidate.remove(cut);
                String trial = candidate + dots;
                if (measureText(trial, width, height) && width <= maxWidth)
                {
                    clipped = trial;
                    break;
                }
            }
            if (clipped.length() == 0)
                clipped = dots;
        }

        if (clipped.length() == 0)
            return false;

        drawTextAligned(clipped, x, y, fg565, 0, align);
        return true;
    }

}

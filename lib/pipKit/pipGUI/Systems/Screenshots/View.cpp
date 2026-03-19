#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pipgui
{
    void GUI::renderScreenshotGallery(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint8_t cols, uint8_t rows, uint16_t padding)
    {
#if !PIPGUI_SCREENSHOTS
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)cols;
        (void)rows;
        (void)padding;
        return;
#else
        if (_render.screenWidth == 0 || _render.screenHeight == 0)
            return;

        const bool inSprite = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        pipcore::Sprite *spr = nullptr;
        if (!inSprite)
        {
            if (!_flags.spriteEnabled)
                return;
            _flags.inSpritePass = 1;
            _render.activeSprite = &_render.sprite;
        }

        int32_t savedClipX = 0, savedClipY = 0, savedClipW = 0, savedClipH = 0;
        bool clipSaved = false;

        do
        {
            spr = getDrawTarget();
            if (!spr)
                break;

            const int16_t sw = (int16_t)_render.screenWidth;
            const int16_t sh = (int16_t)_render.screenHeight;

            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;

            if (_shots.maxShots == 0)
                break;
            if (!_shots.entries && !ensureScreenshotGallery(platform()))
                break;

            const uint16_t tw = _shots.thumbW;
            const uint16_t th = _shots.thumbH;
            if (tw == 0 || th == 0)
                break;

            const uint16_t pad = padding ? padding : _shots.padding;
            const uint16_t stepX = static_cast<uint16_t>(tw + pad);
            const uint16_t stepY = static_cast<uint16_t>(th + pad);
            if (stepX == 0 || stepY == 0)
                break;

            if (w <= 0)
            {
                if (cols)
                {
                    const int32_t needW =
                        static_cast<int32_t>(cols) * static_cast<int32_t>(tw) +
                        static_cast<int32_t>(cols - 1u) * static_cast<int32_t>(pad);
                    w = static_cast<int16_t>(needW);
                }
                else
                {
                    w = static_cast<int16_t>(sw - x);
                }
            }
            if (h <= 0)
            {
                if (rows)
                {
                    const int32_t needH =
                        static_cast<int32_t>(rows) * static_cast<int32_t>(th) +
                        static_cast<int32_t>(rows - 1u) * static_cast<int32_t>(pad);
                    h = static_cast<int16_t>(needH);
                }
                else
                {
                    h = static_cast<int16_t>(sh - y);
                }
            }

            if (w <= 0 || h <= 0)
                break;

            if (x + w > sw)
                w = static_cast<int16_t>(sw - x);
            if (y + h > sh)
                h = static_cast<int16_t>(sh - y);
            if (w <= 0 || h <= 0)
                break;

            const uint16_t availW = static_cast<uint16_t>(w);
            const uint16_t availH = static_cast<uint16_t>(h);
            const uint16_t gridCols = cols ? cols : std::max<uint16_t>(1, (uint16_t)(((uint32_t)availW + pad) / stepX));
            const uint16_t gridRows = rows ? rows : std::max<uint16_t>(1, (uint16_t)(((uint32_t)availH + pad) / stepY));
            const uint16_t maxVisible = static_cast<uint16_t>(gridCols * gridRows);

#if (PIPGUI_SCREENSHOT_MODE == 2)
            const bool loadingFlash = (!_shots.flashScanDone) || (!_shots.flashThumbsDone);
#else
            const bool loadingFlash = false;
#endif

            const uint16_t visible = std::min<uint16_t>(_shots.count, maxVisible);
            const int16_t startX = x;
            const int16_t startY = y;

            spr->getClipRect(&savedClipX, &savedClipY, &savedClipW, &savedClipH);
            clipSaved = true;

            const int32_t clipX1 = std::max<int32_t>(savedClipX, x);
            const int32_t clipY1 = std::max<int32_t>(savedClipY, y);
            const int32_t clipX2 = std::min<int32_t>(savedClipX + savedClipW, (int32_t)x + w);
            const int32_t clipY2 = std::min<int32_t>(savedClipY + savedClipH, (int32_t)y + h);
            const int32_t clipW = clipX2 - clipX1;
            const int32_t clipH = clipY2 - clipY1;
            if (clipW <= 0 || clipH <= 0)
                break;
            spr->setClipRect((int16_t)clipX1, (int16_t)clipY1, (int16_t)clipW, (int16_t)clipH);

            uint16_t *buf = static_cast<uint16_t *>(spr->getBuffer());
            const int16_t bufW = spr->width();
            const int16_t bufH = spr->height();

            if (visible == 0)
            {
                uint16_t bg565 = _render.bgColor565;
                if (buf && bufW > 0 && bufH > 0)
                {
                    int32_t sx = (int32_t)x + (int32_t)w / 2;
                    int32_t sy = (int32_t)y + (int32_t)h / 2;
                    if (sx < clipX1)
                        sx = clipX1;
                    if (sx >= clipX2)
                        sx = clipX2 - 1;
                    if (sy < clipY1)
                        sy = clipY1;
                    if (sy >= clipY2)
                        sy = clipY2 - 1;
                    if (sx < 0)
                        sx = 0;
                    if (sy < 0)
                        sy = 0;
                    if (sx >= bufW)
                        sx = bufW - 1;
                    if (sy >= bufH)
                        sy = bufH - 1;
                    bg565 = pipcore::Sprite::swap16(buf[(size_t)sy * (size_t)bufW + (size_t)sx]);
                }

                const bool bgBright = (detail::autoTextColor(bg565) == 0x0000);
                const uint16_t fg565 = bgBright ? detail::blend565(bg565, 0x0000, 180) : detail::blend565(bg565, 0xFFFF, 200);

#if (PIPGUI_SCREENSHOT_MODE == 2)
                const String msg = _shots.flashScanDone ? "No screenshots" : "Loading...";
#else
                const String msg = "No screenshots";
#endif
                int16_t mtw = 0, mth = 0;
                if (measureText(msg, mtw, mth) && mtw > 0 && mth > 0)
                {
                    const int16_t tx = static_cast<int16_t>(x + (int32_t)w / 2);
                    const int16_t ty = static_cast<int16_t>(y + (int32_t)(h - mth) / 2);
                    drawTextAligned(msg, tx, ty, fg565, 0, AlignCenter);
                }
                else
                {
                    drawTextAligned(msg, static_cast<int16_t>(x + (int32_t)w / 2), static_cast<int16_t>(y + (int32_t)h / 2), fg565, 0, AlignCenter);
                }
                break;
            }

            constexpr uint8_t kMaxR = 16;
            uint8_t r = 10;
            const uint16_t minDim = (tw < th) ? tw : th;
            if ((uint16_t)r * 2u > minDim)
                r = static_cast<uint8_t>(minDim / 2u);
            if (r > kMaxR)
                r = kMaxR;

            uint16_t bgTL[kMaxR * kMaxR];
            uint16_t bgTR[kMaxR * kMaxR];
            uint16_t bgBL[kMaxR * kMaxR];
            uint16_t bgBR[kMaxR * kMaxR];

            const int32_t rr = std::max<int32_t>(0, (int32_t)r * (int32_t)r - (int32_t)r);
            const auto captureCorner = [&](uint16_t *dst, int16_t ox, int16_t oy)
            {
                for (uint8_t dy = 0; dy < r; ++dy)
                {
                    const uint16_t *src = buf + (int32_t)(oy + dy) * bufW + ox;
                    std::memcpy(dst + (size_t)dy * kMaxR, src, (size_t)r * sizeof(uint16_t));
                }
            };
            const auto restoreCorner = [&](const uint16_t *src, int16_t ox, int16_t oy, bool right, bool bottom)
            {
                for (uint8_t dy = 0; dy < r; ++dy)
                {
                    uint16_t *dst = buf + (int32_t)(oy + dy) * bufW + ox;
                    for (uint8_t dx = 0; dx < r; ++dx)
                    {
                        const int32_t ddx = right ? dx : (int32_t)r - 1 - dx;
                        const int32_t ddy = bottom ? dy : (int32_t)r - 1 - dy;
                        if (ddx * ddx + ddy * ddy > rr)
                            dst[dx] = src[(size_t)dy * kMaxR + dx];
                    }
                }
            };

            for (uint16_t i = 0; i < visible; ++i)
            {
                const uint16_t row = (uint16_t)(i / gridCols);
                const uint16_t col = (uint16_t)(i % gridCols);
                const int16_t cx = static_cast<int16_t>(startX + (int32_t)col * (int32_t)stepX);
                const int16_t cy = static_cast<int16_t>(startY + (int32_t)row * (int32_t)stepY);

                const ScreenshotEntry &entry = _shots.entries[i];
                if (entry.pixels)
                {
                    const bool canRound =
                        (r != 0) && buf &&
                        (cx >= clipX1) && (cy >= clipY1) &&
                        ((int32_t)cx + tw <= clipX2) && ((int32_t)cy + th <= clipY2) &&
                        (cx >= 0) && (cy >= 0) &&
                        ((int32_t)cx + tw <= bufW) && ((int32_t)cy + th <= bufH);

                    if (canRound)
                    {
                        const int16_t rx = static_cast<int16_t>(cx + (int16_t)(tw - r));
                        const int16_t by = static_cast<int16_t>(cy + (int16_t)(th - r));

                        captureCorner(bgTL, cx, cy);
                        captureCorner(bgTR, rx, cy);
                        captureCorner(bgBL, cx, by);
                        captureCorner(bgBR, rx, by);

                        spr->pushImage(cx, cy, (int16_t)tw, (int16_t)th, entry.pixels);

                        restoreCorner(bgTL, cx, cy, false, false);
                        restoreCorner(bgTR, rx, cy, true, false);
                        restoreCorner(bgBL, cx, by, false, true);
                        restoreCorner(bgBR, rx, by, true, true);
                    }
                    else
                    {
                        spr->pushImage(cx, cy, (int16_t)tw, (int16_t)th, entry.pixels);
                    }
                    continue;
                }

                if (!loadingFlash)
                    continue;

#if (PIPGUI_SCREENSHOT_MODE == 2)
                if (_shots.flashScanDone && entry.path[0] == '\0')
                    continue;
#endif

                uint16_t bg565 = _render.bgColor565;
                if (buf)
                {
                    int32_t sx = (int32_t)cx + (int32_t)tw / 2;
                    int32_t sy = (int32_t)cy + (int32_t)th / 2;
                    if (sx < clipX1)
                        sx = clipX1;
                    if (sx >= clipX2)
                        sx = clipX2 - 1;
                    if (sy < clipY1)
                        sy = clipY1;
                    if (sy >= clipY2)
                        sy = clipY2 - 1;
                    const uint16_t stored = buf[(size_t)sy * (size_t)bufW + (size_t)sx];
                    bg565 = pipcore::Sprite::swap16(stored);
                }

                const bool bgBright = (detail::autoTextColor(bg565) == 0x0000);
                const uint16_t skel = bgBright ? detail::blend565(bg565, 0x0000, 28) : detail::blend565(bg565, 0xFFFF, 42);

                const bool canRound =
                    (r != 0) && buf &&
                    (cx >= clipX1) && (cy >= clipY1) &&
                    ((int32_t)cx + tw <= clipX2) && ((int32_t)cy + th <= clipY2) &&
                    (cx >= 0) && (cy >= 0) &&
                    ((int32_t)cx + tw <= bufW) && ((int32_t)cy + th <= bufH);

                if (canRound)
                {
                    const int16_t rx = static_cast<int16_t>(cx + (int16_t)(tw - r));
                    const int16_t by = static_cast<int16_t>(cy + (int16_t)(th - r));

                    captureCorner(bgTL, cx, cy);
                    captureCorner(bgTR, rx, cy);
                    captureCorner(bgBL, cx, by);
                    captureCorner(bgBR, rx, by);

                    spr->fillRect(cx, cy, (int16_t)tw, (int16_t)th, skel);

                    restoreCorner(bgTL, cx, cy, false, false);
                    restoreCorner(bgTR, rx, cy, true, false);
                    restoreCorner(bgBL, cx, by, false, true);
                    restoreCorner(bgBR, rx, by, true, true);
                }
                else
                {
                    spr->fillRect(cx, cy, (int16_t)tw, (int16_t)th, skel);
                }
            }
        } while (false);

        if (clipSaved && spr)
            spr->setClipRect((int16_t)savedClipX, (int16_t)savedClipY, (int16_t)savedClipW, (int16_t)savedClipH);

        if (!inSprite)
        {
            _render.activeSprite = prevActive;
            _flags.inSpritePass = 0;
            if (spr && w > 0 && h > 0)
                presentSprite(x, y, w, h, "present");
        }
#endif
    }
}




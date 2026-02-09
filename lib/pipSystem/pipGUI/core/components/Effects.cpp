#include <pipGUI/core/api/pipGUI.hpp>
#include <math.h>

namespace pipgui
{

void GUI::drawGlowCircle(int16_t x, int16_t y,
                         int16_t r,
                         uint32_t fillColor,
                         int32_t bgColor,
                         int32_t glowColor,
                         uint8_t glowSize,
                         uint8_t glowStrength,
                         GlowAnim anim,
                         uint16_t pulsePeriodMs)
{
    if (r <= 0)
        return;

    if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
    {
        updateGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
        return;
    }

    uint32_t bg = (bgColor >= 0) ? static_cast<uint32_t>(bgColor) : _bgColor;
    uint32_t glow = (glowColor >= 0) ? static_cast<uint32_t>(glowColor) : detail::blend888(fillColor, 0xFFFFFF, 80);

    int32_t outerR = (int32_t)r + (int32_t)glowSize;
    int32_t diam = outerR * 2;

    if (x == center)
        x = (int16_t)(AutoX(diam) + diam / 2);
    if (y == center)
        y = (int16_t)(AutoY(diam) + diam / 2);

    uint16_t strength = glowStrength;
    if (anim == Pulse && pulsePeriodMs > 0)
    {
        float t = (float)(nowMs() % pulsePeriodMs) / (float)pulsePeriodMs;

        float p = 0.5f + 0.5f * sinf(t * 6.283185307f);
        float s = 0.30f + 0.70f * p;
        strength = (uint16_t)((float)glowStrength * s);
    }

    auto target = getDrawTarget();
    auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };
    auto to565_from888 = [](Color888 c) -> uint16_t { return (uint16_t)((((uint16_t)(c.r >> 3)) << 11) | (((uint16_t)(c.g >> 2)) << 5) | ((uint16_t)(c.b >> 3))); };

    if (glowSize == 0 || strength < 2)
    {
        if (_frcProfile == FrcProfile::Off)
        {
            uint16_t c565 = to565(fillColor);
            target->fillCircle(x, y, r, c565);
        }
        else
        {
            Color888 c{(uint8_t)((fillColor >> 16) & 0xFF), (uint8_t)((fillColor >> 8) & 0xFF), (uint8_t)(fillColor & 0xFF)};
            for (int16_t yy = y - r; yy <= y + r; ++yy)
                for (int16_t xx = x - r; xx <= x + r; ++xx)
                    target->drawPixel(xx, yy, detail::quantize565(c, xx, yy, _frcSeed, _frcProfile));
        }
        return;
    }

    float fGlowSize = (float)glowSize;
    bool useSmoothGlow = (glowSize <= 4);

    for (int off = (int)glowSize; off > 0; --off)
    {
        float norm = (fGlowSize - (float)off + 1.0f) / fGlowSize;
        float curve = norm * norm * norm;
        int alpha16 = (int)((float)strength * curve);
        if (alpha16 > 255)
            alpha16 = 255;
        uint8_t alpha = (uint8_t)alpha16;
        if (alpha < 2)
            continue;
        uint32_t blended = detail::blend888(bg, glow, alpha);
        Color888 bc{(uint8_t)((blended >> 16) & 0xFF), (uint8_t)((blended >> 8) & 0xFF), (uint8_t)(blended & 0xFF)};
        if (_frcProfile == FrcProfile::Off)
        {
            uint16_t col = to565_from888(bc);
            if (useSmoothGlow)
                target->fillCircle(x, y, r + off, col);
            else
                target->fillCircle(x, y, r + off, col);
        }
        else
        {
            int16_t rr = r + off;
            for (int16_t yy = y - rr; yy <= y + rr; ++yy)
                for (int16_t xx = x - rr; xx <= x + rr; ++xx)
                    target->drawPixel(xx, yy, detail::quantize565(bc, xx, yy, _frcSeed, _frcProfile));
        }
    }
    if (_frcProfile == FrcProfile::Off)
    {
        uint16_t c565 = (uint16_t)((((uint16_t)( (uint8_t)((fillColor >> 16) & 0xFF) >> 3)) << 11) | (((uint16_t)( (uint8_t)((fillColor >> 8) & 0xFF) >> 2)) << 5) | ((uint16_t)((uint8_t)(fillColor & 0xFF) >> 3)));
        target->fillCircle(x, y, r, c565);
    }
    else
    {
        Color888 c{(uint8_t)((fillColor >> 16) & 0xFF), (uint8_t)((fillColor >> 8) & 0xFF), (uint8_t)(fillColor & 0xFF)};
        for (int16_t yy = y - r; yy <= y + r; ++yy)
            for (int16_t xx = x - r; xx <= x + r; ++xx)
                target->drawPixel(xx, yy, detail::quantize565(c, xx, yy, _frcSeed, _frcProfile));
    }
}

void GUI::updateGlowCircle(int16_t x, int16_t y,
                           int16_t r,
                           uint32_t fillColor,
                           int32_t bgColor,
                           int32_t glowColor,
                           uint8_t glowSize,
                           uint8_t glowStrength,
                           GlowAnim anim,
                           uint16_t pulsePeriodMs)
{
    if (!_flags.spriteEnabled || !_display)
    {
        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;
        _flags.renderToSprite = 0;
        drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;
        return;
    }

    int32_t outerR = (int32_t)r + (int32_t)glowSize;
    int32_t diam = outerR * 2;

    int16_t cx = x;
    int16_t cy = y;

    if (cx == center)
        cx = (int16_t)(AutoX(diam) + diam / 2);
    if (cy == center)
        cy = (int16_t)(AutoY(diam) + diam / 2);

    int16_t pad = 2;
    int16_t bx = (int16_t)(cx - outerR - pad);
    int16_t by = (int16_t)(cy - outerR - pad);
    int16_t bw = (int16_t)(diam + pad * 2);
    int16_t bh = (int16_t)(diam + pad * 2);

    uint32_t bg = (bgColor >= 0) ? static_cast<uint32_t>(bgColor) : _bgColor;

    bool prevRender = _flags.renderToSprite;
    pipcore::Sprite *prevActive = _activeSprite;

    _flags.renderToSprite = 1;
    _activeSprite = &_sprite;

    auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };
    if (_frcProfile == FrcProfile::Off)
    {
        _sprite.fillRect(bx, by, bw, bh, to565(bg));
    }
    else
    {
        Color888 c{(uint8_t)((bg >> 16) & 0xFF), (uint8_t)((bg >> 8) & 0xFF), (uint8_t)(bg & 0xFF)};
        for (int16_t yy = by; yy < by + bh; ++yy)
            for (int16_t xx = bx; xx < bx + bw; ++xx)
                _sprite.drawPixel(xx, yy, detail::quantize565(c, xx, yy, _frcSeed, _frcProfile));
    }

    drawGlowCircle(x, y, r, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    _flags.renderToSprite = prevRender;
    _activeSprite = prevActive;

    invalidateRect(bx, by, bw, bh);
    flushDirty();
}

void GUI::drawGlowRoundRect(int16_t x, int16_t y,
                            int16_t w, int16_t h,
                            uint8_t radius,
                            uint32_t fillColor,
                            int32_t bgColor,
                            int32_t glowColor,
                            uint8_t glowSize,
                            uint8_t glowStrength,
                            GlowAnim anim,
                            uint16_t pulsePeriodMs)
{
    if (w <= 0 || h <= 0)
        return;

    if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
    {
        updateGlowRoundRect(x, y, w, h, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
        return;
    }

    uint32_t bg = (bgColor >= 0) ? static_cast<uint32_t>(bgColor) : _bgColor;
    uint32_t glow = (glowColor >= 0) ? static_cast<uint32_t>(glowColor) : detail::blend888(fillColor, 0xFFFFFF, 80);

    if (x == center)
        x = (int16_t)(AutoX((int32_t)w + 2 * (int32_t)glowSize) + (int32_t)glowSize);
    if (y == center)
        y = (int16_t)(AutoY((int32_t)h + 2 * (int32_t)glowSize) + (int32_t)glowSize);

    uint16_t strength = glowStrength;
    if (anim == Pulse && pulsePeriodMs > 0)
    {
        float t = (float)(nowMs() % pulsePeriodMs) / (float)pulsePeriodMs;

        float p = 0.5f + 0.5f * sinf(t * 6.283185307f);
        float s = 0.30f + 0.70f * p;
        strength = (uint16_t)((float)glowStrength * s);
    }

    auto target = getDrawTarget();

    auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };

    if (glowSize == 0 || strength < 2)
    {
        target->fillRoundRect(x, y, w, h, radius, to565(fillColor));
        return;
    }

    float fGlowSize = (float)glowSize;
    for (int off = glowSize; off > 0; --off)
    {
        float norm = (fGlowSize - off + 1.0f) / fGlowSize;
        float curve = norm * norm * norm;
        int alpha16 = (int)(strength * curve);
        if (alpha16 > 255)
            alpha16 = 255;
        uint8_t alpha = (uint8_t)alpha16;
        if (alpha < 2)
            continue;
        uint32_t col = detail::blend888(bg, glow, alpha);
        int16_t xx = x - off;
        int16_t yy = y - off;
        int16_t ww = w + off * 2;
        int16_t hh = h + off * 2;
        int16_t rr = radius + off;
        rr = (ww < hh ? ww : hh) / 2 < rr ? (ww < hh ? ww : hh) / 2 : rr;
        target->fillRoundRect(xx, yy, ww, hh, (uint8_t)rr, to565(col));
    }
    target->fillRoundRect(x, y, w, h, radius, to565(fillColor));
}

void GUI::updateGlowRoundRect(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t radius,
                              uint32_t fillColor,
                              int32_t bgColor,
                              int32_t glowColor,
                              uint8_t glowSize,
                              uint8_t glowStrength,
                              GlowAnim anim,
                              uint16_t pulsePeriodMs)
{
    if (!_flags.spriteEnabled || !_display)
    {
        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;
        _flags.renderToSprite = 0;
        drawGlowRoundRect(x, y, w, h, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;
        return;
    }

    int16_t rx = x;
    int16_t ry = y;
    if (rx == center)
        rx = (int16_t)(AutoX((int32_t)w + 2 * (int32_t)glowSize) + (int32_t)glowSize);
    if (ry == center)
        ry = (int16_t)(AutoY((int32_t)h + 2 * (int32_t)glowSize) + (int32_t)glowSize);

    int16_t pad = 2;
    int16_t bx = (int16_t)(rx - (int16_t)glowSize - pad);
    int16_t by = (int16_t)(ry - (int16_t)glowSize - pad);
    int16_t bw = (int16_t)(w + (int16_t)glowSize * 2 + pad * 2);
    int16_t bh = (int16_t)(h + (int16_t)glowSize * 2 + pad * 2);

    uint32_t bg = (bgColor >= 0) ? static_cast<uint32_t>(bgColor) : _bgColor;

    bool prevRender = _flags.renderToSprite;
    pipcore::Sprite *prevActive = _activeSprite;

    _flags.renderToSprite = 1;
    _activeSprite = &_sprite;

    auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };
    if (_frcProfile == FrcProfile::Off)
    {
        _sprite.fillRect(bx, by, bw, bh, to565(bg));
    }
    else
    {
        Color888 c{(uint8_t)((bg >> 16) & 0xFF), (uint8_t)((bg >> 8) & 0xFF), (uint8_t)(bg & 0xFF)};
        for (int16_t yy = by; yy < by + bh; ++yy)
            for (int16_t xx = bx; xx < bx + bw; ++xx)
                _sprite.drawPixel(xx, yy, detail::quantize565(c, xx, yy, _frcSeed, _frcProfile));
    }

    drawGlowRoundRect(x, y, w, h, radius, fillColor, bgColor, glowColor, glowSize, glowStrength, anim, pulsePeriodMs);
    _flags.renderToSprite = prevRender;
    _activeSprite = prevActive;

    invalidateRect(bx, by, bw, bh);
    flushDirty();
}

bool GUI::ensureBlurWorkBuffers(uint32_t smallLen, int16_t sw, int16_t sh) noexcept
{
    if (smallLen == 0 || sw <= 0 || sh <= 0)
        return false;

    pipcore::GuiPlatform *plat = platform();

    const uint16_t needRow = (uint16_t)(sw + 1);
    const uint16_t needCol = (uint16_t)(sh + 1);

    if (!_blurSmallIn || !_blurSmallTmp || _blurWorkLen < smallLen)
    {
        uint16_t *newSmallIn = (uint16_t *)detail::guiAlloc(plat, (size_t)smallLen * sizeof(uint16_t), pipcore::GuiAllocCaps::PreferExternal);
        uint16_t *newSmallTmp = (uint16_t *)detail::guiAlloc(plat, (size_t)smallLen * sizeof(uint16_t), pipcore::GuiAllocCaps::PreferExternal);

        if (!newSmallIn || !newSmallTmp)
        {
            if (newSmallIn)
                detail::guiFree(plat, newSmallIn);
            if (newSmallTmp)
                detail::guiFree(plat, newSmallTmp);
            return false;
        }

        detail::guiFree(plat, _blurSmallIn);
        detail::guiFree(plat, _blurSmallTmp);

        _blurSmallIn = newSmallIn;
        _blurSmallTmp = newSmallTmp;
        _blurWorkLen = smallLen;
    }

    if (_blurRowCap < needRow || !_blurRowR || !_blurRowG || !_blurRowB)
    {
        uint32_t *newRowR = (uint32_t *)detail::guiAlloc(plat, (size_t)needRow * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
        uint32_t *newRowG = (uint32_t *)detail::guiAlloc(plat, (size_t)needRow * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
        uint32_t *newRowB = (uint32_t *)detail::guiAlloc(plat, (size_t)needRow * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);

        if (!newRowR || !newRowG || !newRowB)
        {
            if (newRowR)
                detail::guiFree(plat, newRowR);
            if (newRowG)
                detail::guiFree(plat, newRowG);
            if (newRowB)
                detail::guiFree(plat, newRowB);
            return false;
        }

        detail::guiFree(plat, _blurRowR);
        detail::guiFree(plat, _blurRowG);
        detail::guiFree(plat, _blurRowB);

        _blurRowR = newRowR;
        _blurRowG = newRowG;
        _blurRowB = newRowB;
        _blurRowCap = needRow;
    }

    if (_blurColCap < needCol || !_blurColR || !_blurColG || !_blurColB)
    {
        uint32_t *newColR = (uint32_t *)detail::guiAlloc(plat, (size_t)needCol * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
        uint32_t *newColG = (uint32_t *)detail::guiAlloc(plat, (size_t)needCol * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);
        uint32_t *newColB = (uint32_t *)detail::guiAlloc(plat, (size_t)needCol * sizeof(uint32_t), pipcore::GuiAllocCaps::Default);

        if (!newColR || !newColG || !newColB)
        {
            if (newColR)
                detail::guiFree(plat, newColR);
            if (newColG)
                detail::guiFree(plat, newColG);
            if (newColB)
                detail::guiFree(plat, newColB);
            return false;
        }

        detail::guiFree(plat, _blurColR);
        detail::guiFree(plat, _blurColG);
        detail::guiFree(plat, _blurColB);

        _blurColR = newColR;
        _blurColG = newColG;
        _blurColB = newColB;
        _blurColCap = needCol;
    }

    return _blurSmallIn && _blurSmallTmp &&
           _blurRowR && _blurRowG && _blurRowB &&
           _blurColR && _blurColG && _blurColB;
}
 
 void GUI::drawBlurStrip(int16_t x, int16_t y,
                         int16_t w, int16_t h,
                         uint8_t radius,
                         BlurDirection dir,
                         bool gradient,
                         uint8_t materialStrength,
                         uint8_t noiseAmount,
                         int32_t materialColor)
{
    (void)noiseAmount;

    if (w <= 0 || h <= 0)
        return;
    if (radius < 1)
        radius = 1;

    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (x + w > _screenWidth)
        w = _screenWidth - x;
    if (y + h > _screenHeight)
        h = _screenHeight - y;
    if (w <= 0 || h <= 0)
        return;

    if (!(_flags.renderToSprite && _flags.spriteEnabled))
    {
        if (_flags.spriteEnabled && _display && !_flags.renderToSprite)
        {
            updateBlurStrip(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
            return;
        }

    }

    pipcore::Sprite *spr = _activeSprite ? _activeSprite : &_sprite;
    uint16_t *screenBuf = (uint16_t *)spr->getBuffer();
    if (!screenBuf)
        return;

    const int16_t stride = spr->width();
    if (stride <= 0)
        return;

    auto swap16 = [](uint16_t v) -> uint16_t
    { return (uint16_t)((v >> 8) | (v << 8)); };
    auto read565 = [&](int32_t idx) -> uint16_t
    { return swap16(screenBuf[idx]); };
    auto write565 = [&](int32_t idx, uint16_t c) -> void
    { screenBuf[idx] = swap16(c); };

    uint8_t baseScale = 2;
    if (w < 20 || h < 20)
        baseScale = 1;

    uint8_t scale = baseScale;
    int16_t sw = 0;
    int16_t sh = 0;
    uint32_t smallLen = 0;
    bool ok = false;

    for (uint8_t tryScale = baseScale; tryScale <= 4; ++tryScale)
    {
        scale = tryScale;
        sw = (int16_t)((w + scale - 1) / scale);
        sh = (int16_t)((h + scale - 1) / scale);
        if (sw <= 0 || sh <= 0)
            continue;

        smallLen = (uint32_t)sw * (uint32_t)sh;
        if (smallLen == 0)
            continue;

        if (ensureBlurWorkBuffers(smallLen, sw, sh))
        {
            ok = true;
            break;
        }
    }

    if (!ok)
        return;

    uint16_t *smallIn = _blurSmallIn;
    uint16_t *smallTmp = _blurSmallTmp;
    uint32_t *rowR = _blurRowR;
    uint32_t *rowG = _blurRowG;
    uint32_t *rowB = _blurRowB;
    uint32_t *colR = _blurColR;
    uint32_t *colG = _blurColG;
    uint32_t *colB = _blurColB;
    if (!smallIn || !smallTmp || !rowR || !rowG || !rowB || !colR || !colG || !colB)
        return;

    for (int16_t sy = 0; sy < sh; ++sy)
    {
        int16_t y0 = (int16_t)(y + sy * scale);
        int16_t y1 = (int16_t)(y0 + scale);
        if (y1 > y + h)
            y1 = (int16_t)(y + h);

        for (int16_t sx = 0; sx < sw; ++sx)
        {
            int16_t x0 = (int16_t)(x + sx * scale);
            int16_t x1 = (int16_t)(x0 + scale);
            if (x1 > x + w)
                x1 = (int16_t)(x + w);

            uint32_t sumR = 0, sumG = 0, sumB = 0;
            uint32_t cnt = 0;

            for (int16_t iy = y0; iy < y1; ++iy)
            {
                int32_t rowOff = (int32_t)iy * (int32_t)stride;
                for (int16_t ix = x0; ix < x1; ++ix)
                {
                    uint16_t c = read565(rowOff + ix);
                    sumR += (c >> 11) & 0x1F;
                    sumG += (c >> 5) & 0x3F;
                    sumB += c & 0x1F;
                    ++cnt;
                }
            }

            if (cnt == 0)
                cnt = 1;
            uint16_t r5 = (uint16_t)(sumR / cnt);
            uint16_t g6 = (uint16_t)(sumG / cnt);
            uint16_t b5 = (uint16_t)(sumB / cnt);
            smallIn[(uint32_t)sy * (uint32_t)sw + (uint32_t)sx] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
        }
    }

    uint32_t matColor = (materialColor >= 0) ? static_cast<uint32_t>(materialColor) : _bgColor;

    uint8_t radiusSmall = (uint8_t)((radius + scale - 1) / scale);
    if (radiusSmall < 1)
        radiusSmall = 1;

    auto blurHorizontal = [&]()
    {
        for (int16_t iy = 0; iy < sh; ++iy)
        {
            uint32_t rowOff = (uint32_t)iy * (uint32_t)sw;
            rowR[0] = rowG[0] = rowB[0] = 0;
            for (int16_t ix = 0; ix < sw; ++ix)
            {
                uint16_t c = smallIn[rowOff + (uint32_t)ix];
                uint16_t r = (c >> 11) & 0x1F;
                uint16_t g = (c >> 5) & 0x3F;
                uint16_t b = c & 0x1F;
                rowR[ix + 1] = rowR[ix] + r;
                rowG[ix + 1] = rowG[ix] + g;
                rowB[ix + 1] = rowB[ix] + b;
            }

            for (int16_t ix = 0; ix < sw; ++ix)
            {
                int xa = ix - (int)radiusSmall;
                int xb = ix + (int)radiusSmall;
                if (xa < 0)
                    xa = 0;
                if (xb >= sw)
                    xb = sw - 1;
                uint32_t count = (uint32_t)(xb - xa + 1);

                uint32_t sumR = rowR[xb + 1] - rowR[xa];
                uint32_t sumG = rowG[xb + 1] - rowG[xa];
                uint32_t sumB = rowB[xb + 1] - rowB[xa];

                uint16_t r = (uint16_t)(sumR / count);
                uint16_t g = (uint16_t)(sumG / count);
                uint16_t b = (uint16_t)(sumB / count);
                smallTmp[rowOff + (uint32_t)ix] = (uint16_t)((r << 11) | (g << 5) | b);
            }
        }
    };

    auto blurVertical = [&]()
    {
        for (int16_t ix = 0; ix < sw; ++ix)
        {
            colR[0] = colG[0] = colB[0] = 0;
            for (int16_t iy = 0; iy < sh; ++iy)
            {
                uint32_t idx = (uint32_t)iy * (uint32_t)sw + (uint32_t)ix;
                uint16_t c = smallTmp[idx];
                uint16_t r = (c >> 11) & 0x1F;
                uint16_t g = (c >> 5) & 0x3F;
                uint16_t b = c & 0x1F;
                colR[iy + 1] = colR[iy] + r;
                colG[iy + 1] = colG[iy] + g;
                colB[iy + 1] = colB[iy] + b;
            }

            for (int16_t iy = 0; iy < sh; ++iy)
            {
                int ya = iy - (int)radiusSmall;
                int yb = iy + (int)radiusSmall;
                if (ya < 0)
                    ya = 0;
                if (yb >= sh)
                    yb = sh - 1;
                uint32_t count = (uint32_t)(yb - ya + 1);

                uint32_t sumR = colR[yb + 1] - colR[ya];
                uint32_t sumG = colG[yb + 1] - colG[ya];
                uint32_t sumB = colB[yb + 1] - colB[ya];

                uint16_t r = (uint16_t)(sumR / count);
                uint16_t g = (uint16_t)(sumG / count);
                uint16_t b = (uint16_t)(sumB / count);

                uint32_t idx = (uint32_t)iy * (uint32_t)sw + (uint32_t)ix;
                smallIn[idx] = (uint16_t)((r << 11) | (g << 5) | b);
            }
        }
    };

    blurHorizontal();
    blurVertical();
    blurHorizontal();
    blurVertical();

    for (int16_t iy = 0; iy < h; ++iy)
    {
        uint8_t alphaBase = 255;

        if (gradient)
        {
            if (dir == TopDown)
                alphaBase = (h <= 1) ? 255 : (uint8_t)(255 - (iy * 255) / (h - 1));
            else if (dir == BottomUp)
                alphaBase = (h <= 1) ? 255 : (uint8_t)((iy * 255) / (h - 1));
        }

        int32_t screenOffset = (int32_t)(y + iy) * (int32_t)stride + (int32_t)x;

        for (int16_t ix = 0; ix < w; ++ix)
        {
            uint8_t alpha = alphaBase;
            if (gradient)
            {
                if (dir == LeftRight)
                    alpha = (w <= 1) ? 255 : (uint8_t)(255 - (ix * 255) / (w - 1));
                else if (dir == RightLeft)
                    alpha = (w <= 1) ? 255 : (uint8_t)((ix * 255) / (w - 1));
            }

            int16_t sx = (int16_t)(ix / scale);
            int16_t sy = (int16_t)(iy / scale);
            if (sx >= sw)
                sx = (int16_t)(sw - 1);
            if (sy >= sh)
                sy = (int16_t)(sh - 1);

            uint16_t cOrig = read565(screenOffset + ix);
            uint16_t cBlur = smallIn[(uint32_t)sy * (uint32_t)sw + (uint32_t)sx];

            auto expand565 = [](uint16_t c) -> uint32_t { uint8_t r5 = (c >> 11) & 0x1F; uint8_t g6 = (c >> 5) & 0x3F; uint8_t b5 = c & 0x1F; uint8_t r8 = (uint8_t)((r5 * 255U) / 31U); uint8_t g8 = (uint8_t)((g6 * 255U) / 63U); uint8_t b8 = (uint8_t)((b5 * 255U) / 31U); return (uint32_t)((r8 << 16) | (g8 << 8) | b8); };
            auto to565 = [](uint32_t c) -> uint16_t { uint8_t r = (uint8_t)((c >> 16) & 0xFF); uint8_t g = (uint8_t)((c >> 8) & 0xFF); uint8_t b = (uint8_t)(c & 0xFF); return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3))); };

            uint32_t orig888 = expand565(cOrig);
            uint32_t blur888 = expand565(cBlur);

            uint32_t mixed888 = detail::blend888(orig888, blur888, alpha);

            if (materialStrength > 0)
            {
                uint8_t mat = materialStrength;
                if (gradient)
                    mat = (uint8_t)(((uint16_t)mat * alpha) / 255U);
                uint32_t mat888 = (materialColor >= 0) ? static_cast<uint32_t>(materialColor) : _bgColor;
                mixed888 = detail::blend888(mixed888, mat888, mat);
            }

            uint16_t out565;
            if (_frcProfile == FrcProfile::Off)
                out565 = to565(mixed888);
            else
            {
                Color888 cc{(uint8_t)((mixed888 >> 16) & 0xFF), (uint8_t)((mixed888 >> 8) & 0xFF), (uint8_t)(mixed888 & 0xFF)};
                out565 = detail::quantize565(cc, (int16_t)ix + x, (int16_t)iy + y, _frcSeed, _frcProfile);
            }

            write565(screenOffset + ix, out565);
        }
    }
}

void GUI::updateBlurStrip(int16_t x, int16_t y,
                          int16_t w, int16_t h,
                          uint8_t radius,
                          BlurDirection dir,
                          bool gradient,
                          uint8_t materialStrength,
                          uint8_t noiseAmount,
                          int32_t materialColor)
{
    if (!_flags.spriteEnabled || !_display)
    {
        bool prevRender = _flags.renderToSprite;
        pipcore::Sprite *prevActive = _activeSprite;
        _flags.renderToSprite = 0;
        drawBlurStrip(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;
        return;
    }

    bool prevRender = _flags.renderToSprite;
    pipcore::Sprite *prevActive = _activeSprite;

    _flags.renderToSprite = 1;
    _activeSprite = &_sprite;
    drawBlurStrip(x, y, w, h, radius, dir, gradient, materialStrength, noiseAmount, materialColor);
    _flags.renderToSprite = prevRender;
    _activeSprite = prevActive;

    invalidateRect(x, y, w, h);
    flushDirty();
}

}

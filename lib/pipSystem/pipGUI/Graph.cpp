#include <pipGUI/core/api/pipGUI.h>
namespace pipgui
{
    static pipcore::GuiPlatform *graphPlatform()
    {
        return pipgui::GUI::sharedPlatform();
    }

    static void drawBoldGraphLine(LovyanGFX *t, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, uint16_t bg)
    {
        bool steep = abs(y1 - y0) > abs(x1 - x0);

        if (steep)
        {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        int16_t dx = x1 - x0;
        int16_t dy = y1 - y0;

        int32_t gradient = 0;
        if (dx != 0)
            gradient = (dy << 8) / dx;

        int32_t y = y0 << 8;
        const int baseIntensity = 90;

        for (int16_t x = x0; x <= x1; ++x)
        {
            uint8_t frac = (uint8_t)(y & 0xFF);
            int16_t yi = y >> 8;

            uint16_t w1 = (255 - frac) + baseIntensity;
            uint16_t w2 = frac + baseIntensity;
            if (w1 > 255)
                w1 = 255;
            if (w2 > 255)
                w2 = 255;

            if (steep)
            {
                t->drawPixel(yi, x, pipgui::detail::blend565(bg, color, (uint8_t)w1));
                t->drawPixel(yi + 1, x, pipgui::detail::blend565(bg, color, (uint8_t)w2));
            }
            else
            {
                t->drawPixel(x, yi, pipgui::detail::blend565(bg, color, (uint8_t)w1));
                t->drawPixel(x, yi + 1, pipgui::detail::blend565(bg, color, (uint8_t)w2));
            }
            y += gradient;
        }
    }

    static bool ensureGraphLineStorage(GraphArea &area, uint16_t lineIndex)
    {
        if (area.lineCount > lineIndex && area.samples && area.sampleCounts && area.sampleHead)
            return true;

        uint16_t newLineCount = (uint16_t)(lineIndex + 1);

        pipcore::GuiPlatform *plat = graphPlatform();
        int16_t **newSamples = (int16_t **)detail::guiAlloc(plat, sizeof(int16_t *) * newLineCount, pipcore::GuiAllocCaps::Default);
        uint16_t *newCounts = (uint16_t *)detail::guiAlloc(plat, sizeof(uint16_t) * newLineCount, pipcore::GuiAllocCaps::Default);
        uint16_t *newHead = (uint16_t *)detail::guiAlloc(plat, sizeof(uint16_t) * newLineCount, pipcore::GuiAllocCaps::Default);

        if (!newSamples || !newCounts || !newHead)
        {
            if (newSamples)
                detail::guiFree(plat, newSamples);
            if (newCounts)
                detail::guiFree(plat, newCounts);
            if (newHead)
                detail::guiFree(plat, newHead);
            return false;
        }

        for (uint16_t i = 0; i < newLineCount; ++i)
        {
            newSamples[i] = nullptr;
            newCounts[i] = 0;
            newHead[i] = 0;
        }

        if (area.lineCount && area.samples && area.sampleCounts && area.sampleHead)
        {
            for (uint16_t i = 0; i < area.lineCount; ++i)
            {
                newSamples[i] = area.samples[i];
                newCounts[i] = area.sampleCounts[i];
                newHead[i] = area.sampleHead[i];
            }
        }

        if (area.samples)
            detail::guiFree(plat, area.samples);
        if (area.sampleCounts)
            detail::guiFree(plat, area.sampleCounts);
        if (area.sampleHead)
            detail::guiFree(plat, area.sampleHead);

        area.samples = newSamples;
        area.sampleCounts = newCounts;
        area.sampleHead = newHead;
        area.lineCount = newLineCount;
        return true;
    }

    static bool ensureGraphSampleCapacity(GraphArea &area, uint16_t desiredCap)
    {
        if (desiredCap < 2)
            desiredCap = 2;
        if (area.sampleCapacity >= desiredCap)
            return true;
        if (area.lineCount == 0 || !area.samples || !area.sampleCounts || !area.sampleHead)
        {
            area.sampleCapacity = desiredCap;
            return true;
        }

        pipcore::GuiPlatform *plat = graphPlatform();

        for (uint16_t line = 0; line < area.lineCount; ++line)
        {
            int16_t *oldBuf = area.samples[line];
            uint16_t oldCap = area.sampleCapacity;
            uint16_t oldCount = area.sampleCounts[line];
            uint16_t oldHead = area.sampleHead[line];

            int16_t *newBuf = (int16_t *)detail::guiAlloc(plat, sizeof(int16_t) * desiredCap, pipcore::GuiAllocCaps::PreferExternal);
            if (!newBuf)
                return false;

            uint16_t keep = (oldCount < desiredCap) ? oldCount : desiredCap;
            for (uint16_t i = 0; i < keep; ++i)
            {
                if (!oldBuf || oldCap < 2 || oldCount < 1)
                {
                    newBuf[i] = 0;
                    continue;
                }
                uint16_t start = (uint16_t)((oldHead + oldCap - keep) % oldCap);
                uint16_t idx = (uint16_t)((start + i) % oldCap);
                newBuf[i] = oldBuf[idx];
            }

            if (oldBuf)
                detail::guiFree(plat, oldBuf);
            area.samples[line] = newBuf;
            area.sampleCounts[line] = keep;
            area.sampleHead[line] = (keep >= desiredCap) ? 0 : keep;
        }

        area.sampleCapacity = desiredCap;
        return true;
    }

    static bool ensureGraphLineBuffer(GraphArea &area, uint16_t lineIndex)
    {
        if (!area.samples || !area.sampleCounts || !area.sampleHead)
            return false;
        if (lineIndex >= area.lineCount)
            return false;
        if (area.samples[lineIndex])
            return true;
        if (area.sampleCapacity < 2)
            return false;

        pipcore::GuiPlatform *plat = graphPlatform();
        int16_t *buf = (int16_t *)detail::guiAlloc(plat, sizeof(int16_t) * area.sampleCapacity, pipcore::GuiAllocCaps::PreferExternal);
        if (!buf)
            return false;
        for (uint16_t i = 0; i < area.sampleCapacity; ++i)
            buf[i] = 0;

        area.samples[lineIndex] = buf;
        area.sampleCounts[lineIndex] = 0;
        area.sampleHead[lineIndex] = 0;
        return true;
    }

    void GUI::drawGraphGrid(int16_t x, int16_t y,
                            int16_t w, int16_t h,
                            uint8_t radius,
                            GraphDirection dir,
                            uint16_t bgColor,
                            uint16_t gridColor,
                            float speed)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            updateGraphGrid(x, y, w, h, radius, dir, bgColor, gridColor, speed);
            return;
        }

        uint8_t idx = (_currentScreen < _screenCapacity) ? _currentScreen : 0;
        GraphArea &area = ensureGraphArea(idx);
        auto t = getDrawTarget();

        if (x == center)
        {
            int16_t left = 0, availW = _screenWidth;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == StatusBarLeft)
                {
                    left += _statusBarHeight;
                    availW -= _statusBarHeight;
                }
                else if (_statusBarPos == Right)
                {
                    availW -= _statusBarHeight;
                }
            }
            if (availW < w)
                availW = w;
            x = left + (availW - w) / 2;
        }

        if (y == center)
        {
            int16_t top = 0, availH = _screenHeight;
            if (_flags.statusBarEnabled && _statusBarHeight > 0)
            {
                if (_statusBarPos == Top)
                {
                    top += _statusBarHeight;
                    availH -= _statusBarHeight;
                }
                else if (_statusBarPos == Bottom)
                {
                    availH -= _statusBarHeight;
                }
            }
            if (availH < h)
                availH = h;
            y = top + (availH - h) / 2;
        }

        area.x = x;
        area.y = y;
        area.w = w;
        area.h = h;
        area.radius = radius;
        area.direction = dir;
        area.speed = speed;
        area.bgColor = bgColor;
        area.gridColor = gridColor;
        area.gridCellsX = 0;
        area.gridCellsY = 0;

        uint8_t r = (radius < 1) ? 1 : radius;

        t->fillSmoothRoundRect(x, y, w, h, r, gridColor);
        if (w > 4 && h > 4)
        {
            int16_t rInner = (r > 2) ? (r - 2) : (r > 0 ? r - 1 : 0);
            t->fillSmoothRoundRect(x + 2, y + 2, w - 4, h - 4, rInner, bgColor);
        }

        int16_t innerX = x + 2;
        int16_t innerY = y + 2;
        int16_t innerW = (w > 4) ? (w - 4) : 0;
        int16_t innerH = (h > 4) ? (h - 4) : 0;

        if (innerW <= 0 || innerH <= 0)
        {
            area.innerX = innerX;
            area.innerY = innerY;
            area.innerW = innerW;
            area.innerH = innerH;
            return;
        }

        const float desiredCell = 12.0f;
        int16_t cellsX = (int16_t)((float)innerW / desiredCell + 0.5f);
        int16_t cellsY = (int16_t)((float)innerH / desiredCell + 0.5f);
        if (cellsX < 3)
            cellsX = 3;
        if (cellsY < 3)
            cellsY = 3;

        area.gridCellsX = (cellsX > 255) ? 255 : (uint8_t)cellsX;
        area.gridCellsY = (cellsY > 255) ? 255 : (uint8_t)cellsY;

        area.innerX = innerX;
        area.innerY = innerY;
        area.innerW = innerW;
        area.innerH = innerH;

        if (dir == Oscilloscope)
        {
            area.autoScaleInitialized = false;
            if (area.sampleCounts && area.sampleHead)
            {
                for (uint16_t line = 0; line < area.lineCount; ++line)
                {
                    area.sampleCounts[line] = 0;
                    area.sampleHead[line] = 0;
                }
            }
        }

        for (int16_t i = 1; i < cellsX; ++i)
        {
            int16_t gx = innerX + (int16_t)((int32_t)innerW * i / cellsX);
            t->drawLine(gx, innerY, gx, innerY + innerH - 1, gridColor);
        }
        for (int16_t j = 1; j < cellsY; ++j)
        {
            int16_t gy = innerY + (int16_t)((int32_t)innerH * j / cellsY);
            t->drawLine(innerX, gy, innerX + innerW - 1, gy, gridColor);
        }
    }

    void GUI::updateGraphGrid(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t radius,
                              GraphDirection dir,
                              uint16_t bgColor,
                              uint16_t gridColor,
                              float speed)
    {
        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;
            _flags.renderToSprite = 0;
            drawGraphGrid(x, y, w, h, radius, dir, bgColor, gridColor, speed);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;
        drawGraphGrid(x, y, w, h, radius, dir, bgColor, gridColor, speed);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        if (_currentScreen >= _screenCapacity)
            return;
        GraphArea &area = ensureGraphArea(_currentScreen);
        int16_t pad = 2;
        invalidateRect((int16_t)(area.x - pad), (int16_t)(area.y - pad), (int16_t)(area.w + pad * 2), (int16_t)(area.h + pad * 2));
        flushDirty();
    }

    void GUI::setGraphAutoScale(bool enabled)
    {
        if (_currentScreen >= _screenCapacity)
            return;
        GraphArea &area = ensureGraphArea(_currentScreen);
        area.autoScaleEnabled = enabled;
        if (!enabled)
            area.autoScaleInitialized = false;
    }

    void GUI::drawGraphLine(uint8_t lineIndex,
                            int16_t value,
                            uint16_t color,
                            int16_t valueMin,
                            int16_t valueMax)
    {
        if (_flags.spriteEnabled && _tft && !_flags.renderToSprite)
        {
            updateGraphLine(lineIndex, value, color, valueMin, valueMax);
            return;
        }
        if (_currentScreen >= _screenCapacity)
            return;

        GraphArea &area = ensureGraphArea(_currentScreen);
        if (area.innerW <= 1 || area.innerH <= 1)
            return;

        uint16_t desiredCap = (uint16_t)((area.innerW > 2) ? area.innerW : 2);

        if (!ensureGraphLineStorage(area, lineIndex))
            return;
        if (!ensureGraphSampleCapacity(area, desiredCap))
            return;
        if (!ensureGraphLineBuffer(area, lineIndex))
            return;

        const uint16_t cap = area.sampleCapacity;
        uint16_t head = area.sampleHead[lineIndex];
        uint16_t totalCount = area.sampleCounts[lineIndex];

        uint16_t maxVisible = (area.direction == Oscilloscope)
                                  ? (uint16_t)((area.innerW > 2) ? area.innerW : 2)
                                  : cap;
        if (maxVisible > cap)
            maxVisible = cap;

        uint16_t count = 0;
        uint16_t startIndex = 0;

        bool doSweepClear = false;
        uint16_t sweepIndex = 0;
        uint16_t sweepMaxVisible = 0;

        if (area.direction == Oscilloscope)
        {
            if (head >= maxVisible)
                head = 0;
            area.samples[lineIndex][head] = value;

            head++;
            if (head >= maxVisible)
                head = 0;
            area.sampleHead[lineIndex] = head;

            sweepIndex = head;
            sweepMaxVisible = maxVisible;
            doSweepClear = (totalCount >= maxVisible);

            if (totalCount < maxVisible)
            {
                totalCount++;
                area.sampleCounts[lineIndex] = totalCount;
            }
            if (totalCount < 2)
                return;

            count = totalCount;
            startIndex = 0;
        }
        else
        {
            if (head >= cap)
                head = 0;
            area.samples[lineIndex][head] = value;
            head = (head + 1) % cap;
            area.sampleHead[lineIndex] = head;

            if (totalCount < cap)
            {
                totalCount++;
                area.sampleCounts[lineIndex] = totalCount;
            }
            if (totalCount < 2)
                return;

            count = (totalCount < maxVisible) ? totalCount : maxVisible;
            if (count < 2)
                return;

            startIndex = (head + cap - count) % cap;
        }

        int16_t useMin = valueMin;
        int16_t useMax = valueMax;

        if (area.autoScaleEnabled)
        {
            uint16_t idx0 = startIndex;
            int16_t dataMin = area.samples[lineIndex][idx0];
            int16_t dataMax = dataMin;

            for (uint16_t i = 1; i < count; ++i)
            {
                uint16_t idx = (startIndex + i);
                if (idx >= cap)
                    idx -= cap;
                int16_t v = area.samples[lineIndex][idx];
                if (v < dataMin)
                    dataMin = v;
                if (v > dataMax)
                    dataMax = v;
            }

            if (dataMax <= dataMin)
                dataMax = dataMin + 1;

            int16_t range = dataMax - dataMin;
            int16_t margin = range / 6;
            if (margin < 1)
                margin = 1;

            int16_t targetMin = dataMin - margin;
            int16_t targetMax = dataMax + margin;

            if (!area.autoScaleInitialized)
            {
                area.autoMin = targetMin;
                area.autoMax = targetMax;
                area.autoScaleInitialized = true;
            }
            else
            {
                uint32_t now = nowMs();
                bool expanded = false;
                if (targetMax > area.autoMax)
                {
                    area.autoMax += (targetMax - area.autoMax + 1) / 2;
                    expanded = true;
                }
                else if (now - area.lastPeakMs > 800)
                {
                    int16_t diff = targetMax - area.autoMax;
                    if (diff < -2)
                        area.autoMax += (diff / 20 ? diff / 20 : -1);
                }

                if (targetMin < area.autoMin)
                {
                    area.autoMin += (targetMin - area.autoMin - 1) / 2;
                    expanded = true;
                }
                else if (now - area.lastPeakMs > 800)
                {
                    int16_t diff = targetMin - area.autoMin;
                    if (diff > 2)
                        area.autoMin += (diff / 20 ? diff / 20 : 1);
                }

                if (expanded)
                    area.lastPeakMs = now;
            }
            if (area.autoMax <= area.autoMin)
                area.autoMax = area.autoMin + 1;
            useMin = area.autoMin;
            useMax = area.autoMax;
        }

        if (useMax <= useMin)
            useMax = useMin + 1;

        auto t = getDrawTarget();
        int16_t x0 = area.innerX;
        int16_t w = area.innerW;
        int16_t h = area.innerH;

        if (!t || w <= 0 || h <= 0)
            return;

        uint32_t stepX_fixed;
        if (area.direction == Oscilloscope)
        {
            stepX_fixed = (maxVisible > 1) ? (((uint32_t)(w - 1) << 16) / (maxVisible - 1)) : 0;
        }
        else
        {
            stepX_fixed = (count > 1) ? (((uint32_t)(w - 1) << 16) / (count - 1)) : 0;
        }

        if (area.direction != Oscilloscope && lineIndex == 0)
        {
            uint8_t r = (area.radius < 1) ? 1 : area.radius;
            int16_t rInner = (r > 2) ? (r - 2) : (r > 0 ? r - 1 : 0);
            if (area.w > 4 && area.h > 4)
                t->fillSmoothRoundRect(area.x + 2, area.y + 2, area.w - 4, area.h - 4, rInner, area.bgColor);
            else
                t->fillRect(area.innerX, area.innerY, area.innerW, area.innerH, area.bgColor);

            if (area.gridCellsX >= 2)
            {
                for (uint16_t i = 1; i < area.gridCellsX; ++i)
                {
                    int16_t gx = area.innerX + (int16_t)((int32_t)area.innerW * i / area.gridCellsX);
                    t->drawLine(gx, area.innerY, gx, area.innerY + area.innerH - 1, area.gridColor);
                }
            }
            if (area.gridCellsY >= 2)
            {
                for (uint16_t j = 1; j < area.gridCellsY; ++j)
                {
                    int16_t gy = area.innerY + (int16_t)((int32_t)area.innerH * j / area.gridCellsY);
                    t->drawLine(area.innerX, gy, area.innerX + area.innerW - 1, gy, area.gridColor);
                }
            }
        }

        if (area.direction == Oscilloscope && doSweepClear && sweepMaxVisible > 1)
        {
            int16_t localX = (int16_t)((((uint32_t)sweepIndex * stepX_fixed) + 32768) >> 16);
            int16_t sx = area.innerX + localX;

            int16_t stripX = (int16_t)(sx - 1);
            int16_t stripW = 3;

            if (stripX < area.innerX)
            {
                stripW = (int16_t)(stripW - (area.innerX - stripX));
                stripX = area.innerX;
            }
            int16_t innerRight = (int16_t)(area.innerX + area.innerW);
            if (stripX + stripW > innerRight)
                stripW = (int16_t)(innerRight - stripX);

            if (stripW > 0)
            {
                t->fillRect(stripX, area.innerY, stripW, area.innerH, area.bgColor);

                if (area.gridCellsX >= 2)
                {
                    for (uint16_t i = 1; i < area.gridCellsX; ++i)
                    {
                        int16_t gx = area.innerX + (int16_t)((int32_t)area.innerW * i / area.gridCellsX);
                        if (gx >= stripX && gx < (int16_t)(stripX + stripW))
                            t->drawLine(gx, area.innerY, gx, area.innerY + area.innerH - 1, area.gridColor);
                    }
                }
                if (area.gridCellsY >= 2)
                {
                    for (uint16_t j = 1; j < area.gridCellsY; ++j)
                    {
                        int16_t gy = area.innerY + (int16_t)((int32_t)area.innerH * j / area.gridCellsY);
                        t->drawLine(stripX, gy, (int16_t)(stripX + stripW - 1), gy, area.gridColor);
                    }
                }
            }
        }

        int32_t rangeY = useMax - useMin;
        int32_t heightY = h - 1;

        int16_t prevX = 0;
        int16_t prevY = 0;
        uint32_t currX_fixed = 0;

        for (uint16_t i = 0; i < count; ++i)
        {
            int16_t localX = (int16_t)((currX_fixed + 32768) >> 16);
            currX_fixed += stepX_fixed;

            int16_t x;
            if (area.direction == LeftToRight || area.direction == Oscilloscope)
                x = x0 + localX;
            else
                x = x0 + (w - 1 - localX);

            uint16_t idx = (startIndex + i);
            if (idx >= cap)
                idx -= cap;

            int16_t v = area.samples[lineIndex][idx];
            if (v < useMin)
                v = useMin;
            else if (v > useMax)
                v = useMax;

            int32_t valOffset = v - useMin;
            int16_t y = area.innerY + heightY - (int16_t)((valOffset * heightY) / rangeY);

            if (i > 0)
            {
                bool skipLine = false;
                if (area.direction == Oscilloscope)
                {
                    if (idx == head)
                        skipLine = true;
                }

                if (!skipLine)
                {
                    drawBoldGraphLine(t, prevX, prevY, x, y, color, area.bgColor);
                }
            }

            prevX = x;
            prevY = y;
        }
    }

    void GUI::updateGraphLine(uint8_t lineIndex,
                              int16_t value,
                              uint16_t color,
                              int16_t valueMin,
                              int16_t valueMax)
    {
        if (!_flags.spriteEnabled || !_tft)
        {
            bool prevRender = _flags.renderToSprite;
            lgfx::LGFX_Sprite *prevActive = _activeSprite;
            _flags.renderToSprite = 0;
            drawGraphLine(lineIndex, value, color, valueMin, valueMax);
            _flags.renderToSprite = prevRender;
            _activeSprite = prevActive;
            return;
        }

        bool prevRender = _flags.renderToSprite;
        lgfx::LGFX_Sprite *prevActive = _activeSprite;

        _flags.renderToSprite = 1;
        _activeSprite = &_sprite;
        drawGraphLine(lineIndex, value, color, valueMin, valueMax);
        _flags.renderToSprite = prevRender;
        _activeSprite = prevActive;

        if (_currentScreen >= _screenCapacity)
            return;
        GraphArea &area = ensureGraphArea(_currentScreen);
        int16_t pad = 2;
        invalidateRect((int16_t)(area.innerX - pad), (int16_t)(area.innerY - pad), (int16_t)(area.innerW + pad * 2), (int16_t)(area.innerH + pad * 2));
        flushDirty();
    }
}
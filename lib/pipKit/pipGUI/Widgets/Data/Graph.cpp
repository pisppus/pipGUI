#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Internal/GuiAccess.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <cstring>

namespace pipgui
{
    namespace
    {
        constexpr uint8_t kDefaultGraphRadius = 17;

        struct SeriesWindow
        {
            uint16_t start = 0;
            uint16_t count = 0;
            uint16_t visible = 0;
        };

        static pipcore::Platform *graphPlatform() noexcept
        {
            return pipcore::GetPlatform();
        }

        static uint16_t resolveOscilloscopeVisibleSamples(const GraphArea &area, uint16_t fallbackWidth) noexcept
        {
            uint32_t desired = area.oscVisibleSamples;
            if (desired == 0 && area.oscSampleRateHz > 0 && area.oscTimebaseMs > 0)
                desired = ((uint32_t)area.oscSampleRateHz * (uint32_t)area.oscTimebaseMs + 999U) / 1000U;
            if (desired < 2)
                desired = fallbackWidth;
            if (desired < 2)
                desired = 2;
            if (desired > 1024U)
                desired = 1024U;
            return (uint16_t)desired;
        }

        static uint16_t deriveGraphGridColor565(uint16_t bg565) noexcept
        {
            return detail::blend565(bg565, (uint16_t)0xFFFF, 13);
        }

        static void freeGraphBuffers(GraphArea &area) noexcept
        {
            pipcore::Platform *plat = graphPlatform();
            if (!plat)
                return;

            if (area.samples)
            {
                for (uint16_t i = 0; i < area.lineCount; ++i)
                {
                    if (area.samples[i])
                        detail::free(plat, area.samples[i]);
                }
                detail::free(plat, area.samples);
            }

            if (area.lineColors565)
                detail::free(plat, area.lineColors565);
            if (area.lineValueMins)
                detail::free(plat, area.lineValueMins);
            if (area.lineValueMaxs)
                detail::free(plat, area.lineValueMaxs);
            if (area.lineThicknesses)
                detail::free(plat, area.lineThicknesses);
            if (area.sampleCounts)
                detail::free(plat, area.sampleCounts);
            if (area.sampleHead)
                detail::free(plat, area.sampleHead);
            if (area.renderCounts)
                detail::free(plat, area.renderCounts);
            if (area.renderHead)
                detail::free(plat, area.renderHead);
            if (area.innerCache)
                detail::free(plat, area.innerCache);

            area.samples = nullptr;
            area.lineColors565 = nullptr;
            area.lineValueMins = nullptr;
            area.lineValueMaxs = nullptr;
            area.lineThicknesses = nullptr;
            area.sampleCounts = nullptr;
            area.sampleHead = nullptr;
            area.renderCounts = nullptr;
            area.renderHead = nullptr;
            area.innerCache = nullptr;
            area.innerCacheW = 0;
            area.innerCacheH = 0;
            area.lineCount = 0;
            area.sampleCapacity = 0;
            area.renderSnapshotValid = false;
            area.autoScaleInitialized = false;
            area.lastPeakMs = 0;
            area.drawEpoch = 0;
            area.oscClearEpoch = 0;
            area.x = 0;
            area.y = 0;
            area.w = 0;
            area.h = 0;
            area.innerX = 0;
            area.innerY = 0;
            area.innerW = 0;
            area.innerH = 0;
            area.gridCellsX = 0;
            area.gridCellsY = 0;
            area.frameUsed = false;
            area.pendingRender = false;
        }

        static bool ensureGraphLineStorage(GraphArea &area, uint16_t lineIndex)
        {
            if (area.lineCount > lineIndex &&
                area.samples &&
                area.lineColors565 &&
                area.lineValueMins &&
                area.lineValueMaxs &&
                area.lineThicknesses &&
                area.sampleCounts &&
                area.sampleHead &&
                area.renderCounts &&
                area.renderHead)
                return true;

            const uint16_t newLineCount = (uint16_t)(lineIndex + 1);
            pipcore::Platform *plat = graphPlatform();
            if (!plat)
                return false;

            int16_t **newSamples = (int16_t **)detail::alloc(plat, sizeof(int16_t *) * newLineCount, pipcore::AllocCaps::Default);
            uint16_t *newLineColors565 = (uint16_t *)detail::alloc(plat, sizeof(uint16_t) * newLineCount, pipcore::AllocCaps::Default);
            int16_t *newLineValueMins = (int16_t *)detail::alloc(plat, sizeof(int16_t) * newLineCount, pipcore::AllocCaps::Default);
            int16_t *newLineValueMaxs = (int16_t *)detail::alloc(plat, sizeof(int16_t) * newLineCount, pipcore::AllocCaps::Default);
            uint8_t *newLineThicknesses = (uint8_t *)detail::alloc(plat, sizeof(uint8_t) * newLineCount, pipcore::AllocCaps::Default);
            uint16_t *newCounts = (uint16_t *)detail::alloc(plat, sizeof(uint16_t) * newLineCount, pipcore::AllocCaps::Default);
            uint16_t *newHead = (uint16_t *)detail::alloc(plat, sizeof(uint16_t) * newLineCount, pipcore::AllocCaps::Default);
            uint16_t *newRenderCounts = (uint16_t *)detail::alloc(plat, sizeof(uint16_t) * newLineCount, pipcore::AllocCaps::Default);
            uint16_t *newRenderHead = (uint16_t *)detail::alloc(plat, sizeof(uint16_t) * newLineCount, pipcore::AllocCaps::Default);

            if (!newSamples || !newLineColors565 || !newLineValueMins || !newLineValueMaxs || !newLineThicknesses || !newCounts || !newHead || !newRenderCounts || !newRenderHead)
            {
                if (newSamples)
                    detail::free(plat, newSamples);
                if (newLineColors565)
                    detail::free(plat, newLineColors565);
                if (newLineValueMins)
                    detail::free(plat, newLineValueMins);
                if (newLineValueMaxs)
                    detail::free(plat, newLineValueMaxs);
                if (newLineThicknesses)
                    detail::free(plat, newLineThicknesses);
                if (newCounts)
                    detail::free(plat, newCounts);
                if (newHead)
                    detail::free(plat, newHead);
                if (newRenderCounts)
                    detail::free(plat, newRenderCounts);
                if (newRenderHead)
                    detail::free(plat, newRenderHead);
                return false;
            }

            for (uint16_t i = 0; i < newLineCount; ++i)
            {
                newSamples[i] = nullptr;
                newLineColors565[i] = 0;
                newLineValueMins[i] = 0;
                newLineValueMaxs[i] = 1;
                newLineThicknesses[i] = 1;
                newCounts[i] = 0;
                newHead[i] = 0;
                newRenderCounts[i] = 0;
                newRenderHead[i] = 0;
            }

            for (uint16_t i = 0; i < area.lineCount; ++i)
            {
                newSamples[i] = area.samples ? area.samples[i] : nullptr;
                newLineColors565[i] = area.lineColors565 ? area.lineColors565[i] : 0;
                newLineValueMins[i] = area.lineValueMins ? area.lineValueMins[i] : 0;
                newLineValueMaxs[i] = area.lineValueMaxs ? area.lineValueMaxs[i] : 1;
                newLineThicknesses[i] = area.lineThicknesses ? area.lineThicknesses[i] : 1;
                newCounts[i] = area.sampleCounts ? area.sampleCounts[i] : 0;
                newHead[i] = area.sampleHead ? area.sampleHead[i] : 0;
                newRenderCounts[i] = area.renderCounts ? area.renderCounts[i] : 0;
                newRenderHead[i] = area.renderHead ? area.renderHead[i] : 0;
            }

            if (area.samples)
                detail::free(plat, area.samples);
            if (area.lineColors565)
                detail::free(plat, area.lineColors565);
            if (area.lineValueMins)
                detail::free(plat, area.lineValueMins);
            if (area.lineValueMaxs)
                detail::free(plat, area.lineValueMaxs);
            if (area.lineThicknesses)
                detail::free(plat, area.lineThicknesses);
            if (area.sampleCounts)
                detail::free(plat, area.sampleCounts);
            if (area.sampleHead)
                detail::free(plat, area.sampleHead);
            if (area.renderCounts)
                detail::free(plat, area.renderCounts);
            if (area.renderHead)
                detail::free(plat, area.renderHead);

            area.samples = newSamples;
            area.lineColors565 = newLineColors565;
            area.lineValueMins = newLineValueMins;
            area.lineValueMaxs = newLineValueMaxs;
            area.lineThicknesses = newLineThicknesses;
            area.sampleCounts = newCounts;
            area.sampleHead = newHead;
            area.renderCounts = newRenderCounts;
            area.renderHead = newRenderHead;
            area.lineCount = newLineCount;
            return true;
        }

        static bool ensureGraphSampleCapacity(GraphArea &area, uint16_t desiredCap)
        {
            if (desiredCap < 2)
                desiredCap = 2;
            if (area.sampleCapacity >= desiredCap)
                return true;
            if (area.lineCount == 0)
            {
                area.sampleCapacity = desiredCap;
                return true;
            }
            if (!area.samples || !area.sampleCounts || !area.sampleHead)
                return false;

            pipcore::Platform *plat = graphPlatform();
            if (!plat)
                return false;

            int16_t **newBuffers = (int16_t **)detail::alloc(plat, sizeof(int16_t *) * area.lineCount, pipcore::AllocCaps::Default);
            if (!newBuffers)
                return false;

            for (uint16_t i = 0; i < area.lineCount; ++i)
                newBuffers[i] = nullptr;

            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                newBuffers[line] = (int16_t *)detail::alloc(plat, sizeof(int16_t) * desiredCap, pipcore::AllocCaps::Default);
                if (!newBuffers[line])
                {
                    for (uint16_t i = 0; i < line; ++i)
                        detail::free(plat, newBuffers[i]);
                    detail::free(plat, newBuffers);
                    return false;
                }

                for (uint16_t i = 0; i < desiredCap; ++i)
                    newBuffers[line][i] = 0;

                const int16_t *oldBuf = area.samples[line];
                const uint16_t oldCap = area.sampleCapacity;
                const uint16_t oldCount = area.sampleCounts[line];
                const uint16_t keep = (oldCount < desiredCap) ? oldCount : desiredCap;

                if (!oldBuf || oldCap < 2 || keep == 0)
                    continue;

                const uint16_t start = (uint16_t)((area.sampleHead[line] + oldCap - keep) % oldCap);
                for (uint16_t i = 0; i < keep; ++i)
                {
                    const uint16_t idx = (uint16_t)((start + i) % oldCap);
                    newBuffers[line][i] = oldBuf[idx];
                }
            }

            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                if (area.samples[line])
                    detail::free(plat, area.samples[line]);
                area.samples[line] = newBuffers[line];

                const uint16_t keep = (area.sampleCounts[line] < desiredCap) ? area.sampleCounts[line] : desiredCap;
                area.sampleCounts[line] = keep;
                area.sampleHead[line] = (keep >= desiredCap) ? 0 : keep;
            }

            detail::free(plat, newBuffers);
            area.sampleCapacity = desiredCap;
            return true;
        }

        static bool ensureGraphLineBuffer(GraphArea &area, uint16_t lineIndex)
        {
            if (!area.samples || !area.sampleCounts || !area.sampleHead || lineIndex >= area.lineCount || area.sampleCapacity < 2)
                return false;
            if (area.samples[lineIndex])
                return true;

            pipcore::Platform *plat = graphPlatform();
            if (!plat)
                return false;

            int16_t *buf = (int16_t *)detail::alloc(plat, sizeof(int16_t) * area.sampleCapacity, pipcore::AllocCaps::Default);
            if (!buf)
                return false;

            for (uint16_t i = 0; i < area.sampleCapacity; ++i)
                buf[i] = 0;

            area.samples[lineIndex] = buf;
            area.sampleCounts[lineIndex] = 0;
            area.sampleHead[lineIndex] = 0;
            return true;
        }

        static bool ensureGraphInnerCache(GraphArea &area)
        {
            if (area.innerW <= 0 || area.innerH <= 0)
                return false;
            if (area.innerCache && area.innerCacheW == area.innerW && area.innerCacheH == area.innerH)
                return true;

            pipcore::Platform *plat = graphPlatform();
            if (!plat)
                return false;

            if (area.innerCache)
            {
                detail::free(plat, area.innerCache);
                area.innerCache = nullptr;
                area.innerCacheW = 0;
                area.innerCacheH = 0;
            }

            const size_t pixels = static_cast<size_t>(area.innerW) * static_cast<size_t>(area.innerH);
            area.innerCache = (uint16_t *)detail::alloc(plat, pixels * sizeof(uint16_t), pipcore::AllocCaps::Default);
            if (!area.innerCache)
                return false;

            area.innerCacheW = area.innerW;
            area.innerCacheH = area.innerH;
            return true;
        }

        static void buildGraphInnerCache(GraphArea &area)
        {
            if (!ensureGraphInnerCache(area) || !area.innerCache)
                return;

            const uint16_t bg = pipcore::Sprite::swap16(area.bgColor565);
            const uint16_t grid = pipcore::Sprite::swap16(deriveGraphGridColor565(area.bgColor565));

            for (int16_t y = 0; y < area.innerH; ++y)
            {
                uint16_t *row = area.innerCache + static_cast<size_t>(y) * area.innerW;
                for (int16_t x = 0; x < area.innerW; ++x)
                    row[x] = bg;
            }

            for (uint16_t i = 1; i < area.gridCellsX; ++i)
            {
                const int16_t gx = (int16_t)((int32_t)area.innerW * i / area.gridCellsX);
                if (gx <= 0 || gx >= area.innerW)
                    continue;
                for (int16_t y = 0; y < area.innerH; ++y)
                    area.innerCache[static_cast<size_t>(y) * area.innerW + gx] = grid;
            }

            for (uint16_t j = 1; j < area.gridCellsY; ++j)
            {
                const int16_t gy = (int16_t)((int32_t)area.innerH * j / area.gridCellsY);
                if (gy <= 0 || gy >= area.innerH)
                    continue;
                uint16_t *row = area.innerCache + static_cast<size_t>(gy) * area.innerW;
                for (int16_t x = 0; x < area.innerW; ++x)
                    row[x] = grid;
            }
        }

        static void snapshotRenderedGraph(GraphArea &area) noexcept
        {
            if (!area.renderCounts || !area.renderHead || !area.sampleCounts || !area.sampleHead)
            {
                area.renderSnapshotValid = false;
                return;
            }

            for (uint16_t i = 0; i < area.lineCount; ++i)
            {
                area.renderCounts[i] = area.sampleCounts[i];
                area.renderHead[i] = area.sampleHead[i];
            }
            area.renderSnapshotValid = true;
        }

        static inline int16_t graphValueToY(const GraphArea &area, int16_t value, int16_t valueMin, int16_t valueMax) noexcept
        {
            if (valueMax <= valueMin)
                valueMax = valueMin + 1;
            if (value < valueMin)
                value = valueMin;
            else if (value > valueMax)
                value = valueMax;

            const int32_t rangeY = valueMax - valueMin;
            const int32_t heightY = area.innerH - 1;
            return (int16_t)(area.innerY + heightY - ((int32_t)(value - valueMin) * heightY) / rangeY);
        }

        static bool canUseIncrementalScrollRender(const GraphArea &area) noexcept
        {
            if (!area.renderSnapshotValid ||
                area.autoScaleEnabled ||
                area.direction == Oscilloscope ||
                area.innerW < 2 ||
                area.sampleCapacity < 2 ||
                area.sampleCapacity != static_cast<uint16_t>(area.innerW) ||
                !area.samples ||
                !area.sampleCounts ||
                !area.sampleHead ||
                !area.renderCounts ||
                !area.renderHead)
                return false;

            bool hasActiveLine = false;
            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                if (!area.samples[line])
                    continue;
                hasActiveLine = true;

                if (area.sampleCounts[line] != area.sampleCapacity || area.renderCounts[line] != area.sampleCapacity)
                    return false;

                const uint16_t expectedHead = (uint16_t)((area.renderHead[line] + 1) % area.sampleCapacity);
                if (area.sampleHead[line] != expectedHead)
                    return false;
            }

            return hasActiveLine;
        }

        static bool shiftGraphInnerOnePixel(pipcore::Sprite *t, const GraphArea &area)
        {
            if (!t || !area.innerCache || area.innerW < 2 || area.innerH < 1)
                return false;

            uint16_t *buf = static_cast<uint16_t *>(t->getBuffer());
            const int32_t stride = t->width();
            const int32_t height = t->height();
            if (!buf || stride <= 0 || height <= 0)
                return false;

            int32_t clipX = 0;
            int32_t clipY = 0;
            int32_t clipW = stride;
            int32_t clipH = height;
            t->getClipRect(&clipX, &clipY, &clipW, &clipH);
            const int32_t clipR = clipX + clipW - 1;
            const int32_t clipB = clipY + clipH - 1;
            if (area.innerX < clipX || area.innerY < clipY ||
                area.innerX + area.innerW - 1 > clipR ||
                area.innerY + area.innerH - 1 > clipB)
                return false;

            const bool leftToRight = (area.direction == LeftToRight);
            for (int16_t y = 0; y < area.innerH; ++y)
            {
                uint16_t *row = buf + static_cast<size_t>(area.innerY + y) * stride + area.innerX;
                if (leftToRight)
                {
                    std::memmove(row, row + 1, static_cast<size_t>(area.innerW - 1) * sizeof(uint16_t));
                    row[area.innerW - 1] = area.innerCache[static_cast<size_t>(y) * area.innerW + (area.innerW - 1)];
                }
                else
                {
                    std::memmove(row + 1, row, static_cast<size_t>(area.innerW - 1) * sizeof(uint16_t));
                    row[0] = area.innerCache[static_cast<size_t>(y) * area.innerW];
                }
            }

            return true;
        }

        static bool renderBufferedGraphIncremental(GUI &gui, pipcore::Sprite *t, GraphArea &area, bool useAutoScale, int16_t autoMin, int16_t autoMax)
        {
            (void)useAutoScale;
            (void)autoMin;
            (void)autoMax;

            if (!shiftGraphInnerOnePixel(t, area))
                return false;

            const bool leftToRight = (area.direction == LeftToRight);
            const int16_t xPrev = leftToRight ? (int16_t)(area.innerX + area.innerW - 2) : (int16_t)(area.innerX + 1);
            const int16_t xCurr = leftToRight ? (int16_t)(area.innerX + area.innerW - 1) : area.innerX;

            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                if (!area.samples || !area.samples[line] || area.sampleCounts[line] < 2)
                    continue;

                const uint16_t currIdx = (uint16_t)((area.sampleHead[line] + area.sampleCapacity - 1) % area.sampleCapacity);
                const uint16_t prevIdx = (uint16_t)((currIdx + area.sampleCapacity - 1) % area.sampleCapacity);
                const int16_t drawMin = area.lineValueMins[line];
                const int16_t drawMax = area.lineValueMaxs[line];
                const int16_t yPrev = graphValueToY(area, area.samples[line][prevIdx], drawMin, drawMax);
                const int16_t yCurr = graphValueToY(area, area.samples[line][currIdx], drawMin, drawMax);
                const uint8_t lineThickness = area.lineThicknesses ? area.lineThicknesses[line] : 1;

                detail::GuiAccess::drawLineSegment(gui, xPrev, yPrev, xCurr, yCurr, lineThickness, area.lineColors565[line], false, false);
            }

            snapshotRenderedGraph(area);
            return true;
        }

        static void redrawGraphInner(pipcore::Sprite *t, const GraphArea &area)
        {
            if (!t || area.innerW <= 0 || area.innerH <= 0)
                return;

            GraphArea &mutableArea = const_cast<GraphArea &>(area);
            if (!mutableArea.innerCache || mutableArea.innerCacheW != area.innerW || mutableArea.innerCacheH != area.innerH)
                buildGraphInnerCache(mutableArea);

            if (mutableArea.innerCache)
            {
                uint16_t *buf = static_cast<uint16_t *>(t->getBuffer());
                const int32_t stride = t->width();
                const int32_t height = t->height();
                if (buf && stride > 0 && height > 0)
                {
                    int32_t clipX = 0;
                    int32_t clipY = 0;
                    int32_t clipW = stride;
                    int32_t clipH = height;
                    t->getClipRect(&clipX, &clipY, &clipW, &clipH);
                    const int32_t clipR = clipX + clipW - 1;
                    const int32_t clipB = clipY + clipH - 1;
                    const int16_t copyX = area.innerX < clipX ? static_cast<int16_t>(clipX) : area.innerX;
                    const int16_t copyY = area.innerY < clipY ? static_cast<int16_t>(clipY) : area.innerY;
                    const int16_t copyR = (area.innerX + area.innerW - 1 > clipR) ? static_cast<int16_t>(clipR) : static_cast<int16_t>(area.innerX + area.innerW - 1);
                    const int16_t copyB = (area.innerY + area.innerH - 1 > clipB) ? static_cast<int16_t>(clipB) : static_cast<int16_t>(area.innerY + area.innerH - 1);
                    if (copyX <= copyR && copyY <= copyB)
                    {
                        const int16_t copyW = static_cast<int16_t>(copyR - copyX + 1);
                        const int16_t copyH = static_cast<int16_t>(copyB - copyY + 1);
                        const int16_t srcX = static_cast<int16_t>(copyX - area.innerX);
                        const int16_t srcY = static_cast<int16_t>(copyY - area.innerY);
                        const size_t rowBytes = static_cast<size_t>(copyW) * sizeof(uint16_t);
                        for (int16_t y = 0; y < copyH; ++y)
                        {
                            uint16_t *dst = buf + static_cast<size_t>(copyY + y) * stride + copyX;
                            const uint16_t *src = mutableArea.innerCache + static_cast<size_t>(srcY + y) * area.innerW + srcX;
                            std::memcpy(dst, src, rowBytes);
                        }
                        return;
                    }
                }
            }

            const uint16_t grid565 = deriveGraphGridColor565(area.bgColor565);
            t->fillRect(area.innerX, area.innerY, area.innerW, area.innerH, area.bgColor565);

            for (uint16_t i = 1; i < area.gridCellsX; ++i)
            {
                const int16_t gx = area.innerX + (int16_t)((int32_t)area.innerW * i / area.gridCellsX);
                for (int16_t yy = area.innerY; yy < area.innerY + area.innerH; ++yy)
                    t->drawPixel(gx, yy, grid565);
            }

            for (uint16_t j = 1; j < area.gridCellsY; ++j)
            {
                const int16_t gy = area.innerY + (int16_t)((int32_t)area.innerH * j / area.gridCellsY);
                for (int16_t xx = area.innerX; xx < area.innerX + area.innerW; ++xx)
                    t->drawPixel(xx, gy, grid565);
            }
        }

        static SeriesWindow resolveSeriesWindow(const GraphArea &area,
                                                uint16_t lineIndex,
                                                uint16_t maxVisible) noexcept
        {
            SeriesWindow window;
            if (!area.sampleCounts || !area.sampleHead || lineIndex >= area.lineCount || area.sampleCapacity < 2)
                return window;

            const uint16_t total = area.sampleCounts[lineIndex];
            if (total < 2)
                return window;

            if (area.direction == Oscilloscope)
            {
                window.visible = (maxVisible < area.sampleCapacity) ? maxVisible : area.sampleCapacity;
                if (window.visible < 2)
                    return {};

                uint16_t head = area.sampleHead[lineIndex];
                if (head >= window.visible)
                    head = 0;

                window.count = (total < window.visible) ? total : window.visible;
                window.start = (uint16_t)((head + window.visible - window.count) % window.visible);
                return window;
            }

            uint16_t head = area.sampleHead[lineIndex];
            if (head >= area.sampleCapacity)
                head = 0;

            window.count = (total < area.sampleCapacity) ? total : area.sampleCapacity;
            window.visible = window.count;
            window.start = (uint16_t)((head + area.sampleCapacity - window.count) % area.sampleCapacity);
            return window;
        }

        static void appendGraphSample(GraphArea &area,
                                      uint16_t lineIndex,
                                      int16_t value,
                                      uint16_t maxVisible)
        {
            const uint16_t cap = area.sampleCapacity;
            uint16_t head = area.sampleHead[lineIndex];
            uint16_t count = area.sampleCounts[lineIndex];

            if (area.direction == Oscilloscope)
            {
                const uint16_t visible = (maxVisible < cap) ? maxVisible : cap;
                if (visible < 2)
                    return;
                if (head >= visible)
                    head = 0;

                if (count == 0)
                {
                    for (uint16_t i = 0; i < visible; ++i)
                        area.samples[lineIndex][i] = value;
                    count = visible;
                    head = (uint16_t)((head + 1) % visible);
                }
                else
                {
                    area.samples[lineIndex][head] = value;
                    head = (uint16_t)((head + 1) % visible);
                }
            }
            else
            {
                if (head >= cap)
                    head = 0;
                area.samples[lineIndex][head] = value;
                head = (uint16_t)((head + 1) % cap);
                if (count < cap)
                    ++count;
            }

            area.sampleHead[lineIndex] = head;
            area.sampleCounts[lineIndex] = count;
        }

        static bool resolveAutoScale(GraphArea &area, uint16_t maxVisible, int16_t &outMin, int16_t &outMax)
        {
            bool hasData = false;
            int16_t dataMin = 0;
            int16_t dataMax = 0;

            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                if (!area.samples || !area.samples[line])
                    continue;

                const SeriesWindow window = resolveSeriesWindow(area, line, maxVisible);
                if (window.count < 2)
                    continue;

                for (uint16_t i = 0; i < window.count; ++i)
                {
                    uint16_t idx = (uint16_t)(window.start + i);
                    if (idx >= area.sampleCapacity)
                        idx = (uint16_t)(idx % area.sampleCapacity);

                    const int16_t v = area.samples[line][idx];
                    if (!hasData)
                    {
                        dataMin = v;
                        dataMax = v;
                        hasData = true;
                    }
                    else
                    {
                        if (v < dataMin)
                            dataMin = v;
                        if (v > dataMax)
                            dataMax = v;
                    }
                }
            }

            if (!hasData)
                return false;

            if (dataMax <= dataMin)
                dataMax = dataMin + 1;

            const int16_t range = (int16_t)(dataMax - dataMin);
            int16_t margin = (int16_t)(range / 6);
            if (margin < 1)
                margin = 1;

            const int16_t targetMin = (int16_t)(dataMin - margin);
            const int16_t targetMax = (int16_t)(dataMax + margin);

            if (!area.autoScaleInitialized)
            {
                area.autoMin = targetMin;
                area.autoMax = targetMax;
                area.autoScaleInitialized = true;
            }
            else
            {
                const uint32_t now = graphPlatform() ? graphPlatform()->nowMs() : 0;
                bool expanded = false;

                if (targetMax > area.autoMax)
                {
                    area.autoMax += (targetMax - area.autoMax + 1) / 2;
                    expanded = true;
                }
                else if (now >= area.lastPeakMs && now - area.lastPeakMs > 800)
                {
                    const int16_t diff = (int16_t)(targetMax - area.autoMax);
                    if (diff < -2)
                        area.autoMax += (diff / 20) ? (diff / 20) : -1;
                }

                if (targetMin < area.autoMin)
                {
                    area.autoMin += (targetMin - area.autoMin - 1) / 2;
                    expanded = true;
                }
                else if (now >= area.lastPeakMs && now - area.lastPeakMs > 800)
                {
                    const int16_t diff = (int16_t)(targetMin - area.autoMin);
                    if (diff > 2)
                        area.autoMin += (diff / 20) ? (diff / 20) : 1;
                }

                if (expanded)
                    area.lastPeakMs = now;
            }

            if (area.autoMax <= area.autoMin)
                area.autoMax = area.autoMin + 1;

            outMin = area.autoMin;
            outMax = area.autoMax;
            return true;
        }

        static void renderSeries(GUI &gui,
                                 pipcore::Sprite *t,
                                 const GraphArea &area,
                                 const int16_t *samples,
                                 const SeriesWindow &window,
                                 uint16_t sampleCapacity,
                                 uint16_t line565,
                                 int16_t valueMin,
                                 int16_t valueMax,
                                 uint8_t lineThickness)
        {
            if (!t || !samples || window.count < 2 || window.visible < 2 || sampleCapacity < 2 || area.innerW <= 1 || area.innerH <= 1)
                return;

            if (valueMax <= valueMin)
                valueMax = valueMin + 1;

            const int32_t rangeY = valueMax - valueMin;
            const int32_t heightY = area.innerH - 1;
            const uint32_t stepXFixed = (window.visible > 1) ? (((uint32_t)(area.innerW - 1) << 16) / (window.visible - 1)) : 0;

            auto drawSegment = [&](int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool &drewSegment)
            {
                if (x0 == x1 && y0 == y1)
                    return;
                const bool roundStart = !drewSegment;
                detail::GuiAccess::drawLineSegment(gui, x0, y0, x1, y1, lineThickness, line565, roundStart, false);
                drewSegment = true;
            };

            if (area.direction == Oscilloscope && window.visible > static_cast<uint16_t>(area.innerW))
            {
                int16_t prevX = 0;
                int16_t prevY = 0;
                uint32_t currXFixed = 0;
                bool havePrev = false;
                bool drewSegment = false;
                bool haveColumn = false;
                int16_t colX = 0;
                int16_t colFirstY = 0;
                int16_t colLastY = 0;
                int16_t colMinY = 0;
                int16_t colMaxY = 0;

                auto flushColumn = [&]()
                {
                    if (!haveColumn)
                        return;
                    if (havePrev)
                        drawSegment(prevX, prevY, colX, colFirstY, drewSegment);
                    if (colMinY != colMaxY)
                        drawSegment(colX, colMinY, colX, colMaxY, drewSegment);
                    prevX = colX;
                    prevY = colLastY;
                    havePrev = true;
                    haveColumn = false;
                };

                for (uint16_t i = 0; i < window.count; ++i)
                {
                    const int16_t localX = (int16_t)((currXFixed + 32768U) >> 16);
                    currXFixed += stepXFixed;
                    const int16_t x = (area.direction == RightToLeft)
                                          ? (int16_t)(area.innerX + area.innerW - 1 - localX)
                                          : (int16_t)(area.innerX + localX);

                    uint16_t idx = (uint16_t)(window.start + i);
                    if (idx >= sampleCapacity)
                        idx = (uint16_t)(idx % sampleCapacity);

                    int16_t v = samples[idx];
                    if (v < valueMin)
                        v = valueMin;
                    else if (v > valueMax)
                        v = valueMax;

                    const int16_t y = (int16_t)(area.innerY + heightY - ((int32_t)(v - valueMin) * heightY) / rangeY);
                    if (!haveColumn)
                    {
                        colX = x;
                        colFirstY = y;
                        colLastY = y;
                        colMinY = y;
                        colMaxY = y;
                        haveColumn = true;
                        continue;
                    }

                    if (x == colX)
                    {
                        colLastY = y;
                        if (y < colMinY)
                            colMinY = y;
                        if (y > colMaxY)
                            colMaxY = y;
                        continue;
                    }

                    flushColumn();
                    colX = x;
                    colFirstY = y;
                    colLastY = y;
                    colMinY = y;
                    colMaxY = y;
                    haveColumn = true;
                }

                flushColumn();
                return;
            }

            int16_t prevX = 0;
            int16_t prevY = 0;
            uint32_t currXFixed = 0;
            bool havePrev = false;
            bool drewSegment = false;

            for (uint16_t i = 0; i < window.count; ++i)
            {
                const int16_t localX = (int16_t)((currXFixed + 32768U) >> 16);
                currXFixed += stepXFixed;
                const int16_t x = (area.direction == RightToLeft)
                                      ? (int16_t)(area.innerX + area.innerW - 1 - localX)
                                      : (int16_t)(area.innerX + localX);

                uint16_t idx = (uint16_t)(window.start + i);
                if (idx >= sampleCapacity)
                    idx = (uint16_t)(idx % sampleCapacity);

                int16_t v = samples[idx];
                if (v < valueMin)
                    v = valueMin;
                else if (v > valueMax)
                    v = valueMax;

                const int16_t y = (int16_t)(area.innerY + heightY - ((int32_t)(v - valueMin) * heightY) / rangeY);
                if (havePrev && (x != prevX || y != prevY))
                    drawSegment(prevX, prevY, x, y, drewSegment);

                prevX = x;
                prevY = y;
                havePrev = true;
            }
        }

        static void renderBufferedGraph(GUI &gui, pipcore::Sprite *t, GraphArea &area, uint16_t maxVisible)
        {
            int16_t autoMin = 0;
            int16_t autoMax = 1;
            const bool useAutoScale = area.autoScaleEnabled && resolveAutoScale(area, maxVisible, autoMin, autoMax);
            if (!useAutoScale && canUseIncrementalScrollRender(area) &&
                renderBufferedGraphIncremental(gui, t, area, useAutoScale, autoMin, autoMax))
                return;

            redrawGraphInner(t, area);

            for (uint16_t line = 0; line < area.lineCount; ++line)
            {
                if (!area.samples || !area.samples[line])
                    continue;

                const SeriesWindow window = resolveSeriesWindow(area, line, maxVisible);
                if (window.count < 2)
                    continue;

                const int16_t drawMin = useAutoScale ? autoMin : area.lineValueMins[line];
                const int16_t drawMax = useAutoScale ? autoMax : area.lineValueMaxs[line];
                const uint8_t lineThickness = area.lineThicknesses ? area.lineThicknesses[line] : 1;
                renderSeries(gui, t, area, area.samples[line], window, area.sampleCapacity, area.lineColors565[line], drawMin, drawMax, lineThickness);
            }

            snapshotRenderedGraph(area);
        }
    }

    void GUI::beginGraphFrame(uint8_t screenId) noexcept
    {
        if (screenId >= _screen.capacity || !_screen.graphAreas)
            return;

        GraphArea *area = _screen.graphAreas[screenId];
        if (area)
            area->frameUsed = false;
    }

    void GUI::flushPendingGraphRender(uint8_t screenId) noexcept
    {
        if (!_flags.spriteEnabled || !_disp.display || screenId >= _screen.capacity || !_screen.graphAreas)
            return;

        GraphArea *area = _screen.graphAreas[screenId];
        if (!area || !area->pendingRender || area->innerW <= 1 || area->innerH <= 1)
            return;

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;

        uint16_t visibleSamples = (uint16_t)((area->innerW > 2) ? area->innerW : 2);
        if (area->direction == Oscilloscope)
            visibleSamples = resolveOscilloscopeVisibleSamples(*area, visibleSamples);

        renderBufferedGraph(*this, &_render.sprite, *area, visibleSamples);

        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
        area->pendingRender = false;

        invalidateRect((int16_t)(area->innerX - 1), (int16_t)(area->innerY - 1),
                       (int16_t)(area->innerW + 2), (int16_t)(area->innerH + 2));
    }

    void GUI::endGraphFrame(uint8_t screenId) noexcept
    {
        if (screenId >= _screen.capacity || !_screen.graphAreas)
            return;

        GraphArea *area = _screen.graphAreas[screenId];
        if (area && !area->frameUsed)
            freeGraphBuffers(*area);
    }

    void GUI::releaseGraphBuffers(uint8_t screenId) noexcept
    {
        if (screenId >= _screen.capacity || !_screen.graphAreas)
            return;

        GraphArea *area = _screen.graphAreas[screenId];
        if (area)
            freeGraphBuffers(*area);
    }

    void GUI::drawGraphGrid(int16_t x, int16_t y,
                            int16_t w, int16_t h,
                            uint8_t radius,
                            GraphDirection dir,
                            uint32_t bgColor,
                            float speed,
                            bool autoScale,
                            uint16_t scopeRateHz,
                            uint16_t scopeTimebaseMs,
                            uint16_t scopeVisibleSamples)
    {
        if (w <= 0 || h <= 0)
            return;

        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGraphGrid(x, y, w, h, radius, dir, bgColor, speed, autoScale,
                            scopeRateHz, scopeTimebaseMs, scopeVisibleSamples);
            return;
        }

        const uint8_t screenId = (_screen.current < _screen.capacity) ? _screen.current : 0;
        GraphArea *area = ensureGraphArea(screenId);
        pipcore::Sprite *t = getDrawTarget();
        if (!area || !t)
            return;

        area->frameUsed = true;

        if (x == center)
        {
            int16_t availW = _render.screenWidth;
            if (availW < w)
                availW = w;
            x = (int16_t)((availW - w) / 2);
        }

        if (y == center)
        {
            int16_t top = 0;
            int16_t availH = _render.screenHeight;
            const int16_t sb = statusBarHeight();
            if (_flags.statusBarEnabled && sb > 0)
            {
                if (_status.pos == Top)
                {
                    top += sb;
                    availH -= sb;
                }
                else if (_status.pos == Bottom)
                {
                    availH -= sb;
                }
            }
            if (availH < h)
                availH = h;
            y = (int16_t)(top + (availH - h) / 2);
        }

        const uint16_t bg565 = detail::color888To565(bgColor);
        const uint8_t resolvedRadius = radius ? radius : kDefaultGraphRadius;
        const bool graphChanged =
            area->x != x || area->y != y ||
            area->w != w || area->h != h ||
            area->radius != resolvedRadius ||
            area->direction != dir ||
            area->bgColor != bgColor;

        const uint8_t outerRadius = resolvedRadius;
        drawSquircleRect().pos(x, y).size(w, h).radius(outerRadius).fill(deriveGraphGridColor565(bg565));

        const int16_t innerX = (int16_t)(x + 2);
        const int16_t innerY = (int16_t)(y + 2);
        const int16_t innerW = (w > 4) ? (int16_t)(w - 4) : 0;
        const int16_t innerH = (h > 4) ? (int16_t)(h - 4) : 0;

        if (graphChanged)
        {
            const bool keepAutoScale = area->autoScaleEnabled;
            freeGraphBuffers(*area);
            area->autoScaleEnabled = autoScale;
            if (keepAutoScale != autoScale)
                area->autoScaleInitialized = false;
        }

        area->x = x;
        area->y = y;
        area->w = w;
        area->h = h;
        area->innerX = innerX;
        area->innerY = innerY;
        area->innerW = innerW;
        area->innerH = innerH;
        area->radius = resolvedRadius;
        area->direction = dir;
        area->speed = speed;
        area->autoScaleEnabled = autoScale;
        area->oscSampleRateHz = scopeRateHz;
        area->oscTimebaseMs = scopeTimebaseMs;
        area->oscVisibleSamples = scopeVisibleSamples;
        area->bgColor = bgColor;
        area->bgColor565 = bg565;
        area->drawEpoch = (area->drawEpoch == 0xFFFFFFFFU) ? 1U : (area->drawEpoch + 1U);

        if (innerW <= 0 || innerH <= 0)
            return;

        const int16_t innerRadius = (outerRadius > 2) ? (int16_t)(outerRadius - 2) : (outerRadius > 0 ? (int16_t)(outerRadius - 1) : 0);
        drawSquircleRect().pos(innerX, innerY).size(innerW, innerH).radius((uint8_t)innerRadius).fill(bg565);

        int16_t cellsX = (int16_t)((float)innerW / 12.0f + 0.5f);
        int16_t cellsY = (int16_t)((float)innerH / 12.0f + 0.5f);
        if (cellsX < 3)
            cellsX = 3;
        if (cellsY < 3)
            cellsY = 3;
        area->gridCellsX = (cellsX > 255) ? 255 : (uint8_t)cellsX;
        area->gridCellsY = (cellsY > 255) ? 255 : (uint8_t)cellsY;

        redrawGraphInner(t, *area);
    }

    void GUI::updateGraphGrid(int16_t x, int16_t y,
                              int16_t w, int16_t h,
                              uint8_t radius,
                              GraphDirection dir,
                              uint32_t bgColor,
                              float speed,
                              bool autoScale,
                              uint16_t scopeRateHz,
                              uint16_t scopeTimebaseMs,
                              uint16_t scopeVisibleSamples)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGraphGrid(x, y, w, h, radius, dir, bgColor, speed, autoScale,
                          scopeRateHz, scopeTimebaseMs, scopeVisibleSamples);
            return;
        }

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawGraphGrid(x, y, w, h, radius, dir, bgColor, speed, autoScale,
                      scopeRateHz, scopeTimebaseMs, scopeVisibleSamples);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (prevRender || _screen.current >= _screen.capacity)
            return;

        GraphArea *area = ensureGraphArea(_screen.current);
        if (!area)
            return;

        invalidateRect((int16_t)(area->x - 2), (int16_t)(area->y - 2), (int16_t)(area->w + 4), (int16_t)(area->h + 4));
    }

    void GUI::drawGraphLine(uint8_t lineIndex,
                            int16_t value,
                            uint32_t color,
                            int16_t valueMin,
                            int16_t valueMax,
                            uint8_t thickness)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGraphLine(lineIndex, value, color, valueMin, valueMax, thickness);
            return;
        }
        if (_screen.current >= _screen.capacity)
            return;

        GraphArea *area = ensureGraphArea(_screen.current);
        if (!area || area->innerW <= 1 || area->innerH <= 1)
            return;

        area->frameUsed = true;

        uint16_t visibleSamples = (uint16_t)((area->innerW > 2) ? area->innerW : 2);
        if (area->direction == Oscilloscope)
            visibleSamples = resolveOscilloscopeVisibleSamples(*area, visibleSamples);

        if (!ensureGraphLineStorage(*area, lineIndex) ||
            !ensureGraphSampleCapacity(*area, visibleSamples) ||
            !ensureGraphLineBuffer(*area, lineIndex))
            return;

        area->lineColors565[lineIndex] = detail::color888To565(color);
        area->lineValueMins[lineIndex] = valueMin;
        area->lineValueMaxs[lineIndex] = (valueMax > valueMin) ? valueMax : (int16_t)(valueMin + 1);
        area->lineThicknesses[lineIndex] = thickness < 1 ? 1 : thickness;

        appendGraphSample(*area, lineIndex, value, visibleSamples);

        pipcore::Sprite *t = getDrawTarget();
        if (!t)
            return;

        renderBufferedGraph(*this, t, *area, visibleSamples);
        area->pendingRender = false;
    }

    void GUI::updateGraphLine(uint8_t lineIndex,
                              int16_t value,
                              uint32_t color,
                              int16_t valueMin,
                              int16_t valueMax,
                              uint8_t thickness)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGraphLine(lineIndex, value, color, valueMin, valueMax, thickness);
            return;
        }
        if (_screen.current >= _screen.capacity)
            return;

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawGraphLine(lineIndex, value, color, valueMin, valueMax, thickness);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        GraphArea *area = ensureGraphArea(_screen.current);
        if (area && !prevRender)
            area->pendingRender = true;
    }

    void GUI::drawGraphSamples(uint8_t lineIndex,
                               const int16_t *samples,
                               uint16_t sampleCount,
                               uint32_t color,
                               int16_t valueMin,
                               int16_t valueMax,
                               uint8_t thickness)
    {
        if (_flags.spriteEnabled && _disp.display && !_flags.inSpritePass)
        {
            updateGraphSamples(lineIndex, samples, sampleCount, color, valueMin, valueMax, thickness);
            return;
        }
        if (_screen.current >= _screen.capacity || !samples || sampleCount < 2)
            return;

        GraphArea *area = ensureGraphArea(_screen.current);
        if (!area || area->innerW <= 1 || area->innerH <= 1)
            return;

        area->frameUsed = true;

        pipcore::Sprite *t = getDrawTarget();
        if (!t)
            return;

        if (lineIndex == 0 || area->oscClearEpoch != area->drawEpoch)
        {
            area->oscClearEpoch = area->drawEpoch;
            redrawGraphInner(t, *area);
        }

        const SeriesWindow window{0, sampleCount, sampleCount};
        renderSeries(*this, t, *area, samples, window, sampleCount, detail::color888To565(color), valueMin, valueMax, thickness);
    }

    void GUI::updateGraphSamples(uint8_t lineIndex,
                                 const int16_t *samples,
                                 uint16_t sampleCount,
                                 uint32_t color,
                                 int16_t valueMin,
                                 int16_t valueMax,
                                 uint8_t thickness)
    {
        if (!_flags.spriteEnabled || !_disp.display)
        {
            drawGraphSamples(lineIndex, samples, sampleCount, color, valueMin, valueMax, thickness);
            return;
        }

        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;

        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawGraphSamples(lineIndex, samples, sampleCount, color, valueMin, valueMax, thickness);
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;

        if (prevRender || _screen.current >= _screen.capacity)
            return;

        GraphArea *area = ensureGraphArea(_screen.current);
        if (!area)
            return;

        invalidateRect((int16_t)(area->innerX - 1), (int16_t)(area->innerY - 1),
                       (int16_t)(area->innerW + 2), (int16_t)(area->innerH + 2));
    }
}

#pragma once

#include <cstdint>
#include <cstddef>

namespace pipgui
{

    struct DebugMetrics
    {
        uint32_t freeHeapTotal = 0;
        uint32_t freeHeapInternal = 0;
        uint32_t largestFreeBlock = 0;
        uint32_t minFreeHeap = 0;

        DebugMetrics() = default;
    };

    struct DirtyRect
    {
        int16_t x, y, w, h;
    };

    class Debug
    {
    public:
        static void init();
        static void update();

        static const DebugMetrics &metrics() { return _metrics; }

        static void formatStatusBar(char *out, size_t len);

        static bool isEnabled() { return _enabled; }
        static void setEnabled(bool enable) { _enabled = enable; }

        static void setMetricsStatusBarEnabled(bool enabled) { _metricsStatusBarEnabled = enabled; }
        static bool isMetricsStatusBarEnabled() { return _metricsStatusBarEnabled; }

        static void setDirtyRectEnabled(bool enabled) { _dirtyRectEnabled = enabled; }
        static bool dirtyRectEnabled() { return _dirtyRectEnabled; }

        static void setDirtyRectColor(uint16_t color) { _dirtyRectColor = color; }
        static void setDirtyRectActiveColor(uint16_t color) { _dirtyRectActiveColor = color; }
        static uint16_t dirtyRectColor() { return _dirtyRectColor; }
        static uint16_t dirtyRectActiveColor() { return _dirtyRectActiveColor; }

        static void recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h);

        static void drawOverlay(uint16_t *buf, int16_t stride,
                                int16_t x0, int16_t y0, int16_t w, int16_t h);

        static void clearRects();

    private:
        static DebugMetrics _metrics;
        static bool _enabled;
        static bool _metricsStatusBarEnabled;

        static bool _dirtyRectEnabled;
        static uint16_t _dirtyRectColor;
        static uint16_t _dirtyRectActiveColor;
        static DirtyRect *_rects;
        static uint16_t _rectCapacity;
        static uint16_t _rectCount;
    };

}

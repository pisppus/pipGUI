#pragma once

#include <cstdint>
#include <cstring>

namespace pipgui
{

    struct DebugMetrics
    {
        // CPU metrics (percentages 0-100)
        uint8_t cpuPercent;

        // RAM metrics (in bytes)
        uint32_t freeHeapTotal;    // Total free heap (DRAM + IRAM + PSRAM)
        uint32_t freeHeapInternal; // Free internal DRAM
        uint32_t largestFreeBlock; // Largest continuous block
        uint32_t minFreeHeap;      // Minimum free since boot

        DebugMetrics() : cpuPercent(0),
                         freeHeapTotal(0),
                         freeHeapInternal(0),
                         largestFreeBlock(0),
                         minFreeHeap(0) {}
    };

    struct DirtyRect
    {
        int16_t x, y, w, h;
    };

    class Debug
    {
    public:
        // Initialize debug subsystem (timers, etc)
        static void init();

        // Update metrics (call periodically, e.g. every 100ms)
        static void update();

        // Mark render timing for CPU measurement
        static void beginRender();
        static void endRender();

        // Get current metrics
        static const DebugMetrics &metrics() { return _metrics; }

        // Format metrics for status bar display
        // Returns: "CPU:xx% T:xxk D:xxk L:xxk M:xxk"
        static void formatStatusBar(char *out, size_t len);

        // Check if debug features are enabled
        static bool isEnabled() { return _enabled; }
        static void setEnabled(bool enable) { _enabled = enable; }

        // Metrics status bar mode
        static void setMetricsStatusBarEnabled(bool enabled) { _metricsStatusBarEnabled = enabled; }
        static bool isMetricsStatusBarEnabled() { return _metricsStatusBarEnabled; }

        // Dirty rect debug overlay
        static void setDirtyRectEnabled(bool enabled) { _dirtyRectEnabled = enabled; }
        static bool dirtyRectEnabled() { return _dirtyRectEnabled; }

        static void setDirtyRectColor(uint16_t color) { _dirtyRectColor = color; }
        static void setDirtyRectActiveColor(uint16_t color) { _dirtyRectActiveColor = color; }
        static uint16_t dirtyRectColor() { return _dirtyRectColor; }
        static uint16_t dirtyRectActiveColor() { return _dirtyRectActiveColor; }

        // Record a dirty rect for visualization
        static void recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h);

        // Draw dirty rect overlay (call during flush)
        static void drawOverlay(uint16_t *buf, int16_t stride, int16_t sw, int16_t sh,
                                int16_t x0, int16_t y0, int16_t w, int16_t h);

        // Clear recorded rects after flush
        static void clearRects() { _rectCount = 0; }

    private:
        static DebugMetrics _metrics;
        static bool _enabled;
        static bool _metricsStatusBarEnabled;

        // For time-based CPU measurement
        static uint32_t _lastUpdateTime;
        static uint32_t _busyTimeAccum;
        static uint32_t _renderStartTime;
        static bool _isRendering;

        static uint32_t getRuntimeCounter();

        // Dirty rect debug
        static bool _dirtyRectEnabled;
        static uint16_t _dirtyRectColor;
        static uint16_t _dirtyRectActiveColor;
        static DirtyRect _rects[8];
        static uint8_t _rectCount;
    };

}

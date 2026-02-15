#pragma once

#include <cstdint>
#include <cstring>

namespace pipgui
{

    struct DebugMetrics
    {
        // LVGL-style performance metrics (based on GUI::loop duration)
        uint8_t cpuPercent;        // 0-100 (busy time of GUI::loop vs period)
        uint16_t handlerTimeMs;    // duration of GUI::loop in ms
        uint8_t fps;               // calls per second
        
        // RAM metrics (in bytes)
        uint32_t freeHeapInternal;
        uint32_t minFreeHeapInternal;
        uint32_t largestFreeBlockInternal;
        
        DebugMetrics() : cpuPercent(0), handlerTimeMs(0), fps(0),
                         freeHeapInternal(0), minFreeHeapInternal(0), largestFreeBlockInternal(0) {}
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
        
        // Frame timing methods (LVGL-style)
        static void frameStart();
        static void frameEnd();
        
        // Update metrics (call periodically, e.g. every 100ms)
        static void update();
        
        // Get current metrics
        static const DebugMetrics& metrics() { return _metrics; }
        
        // Format metrics for status bar display
        // Returns: "CPU:xx% FPS:xx T:xxms F:xxk"
        static void formatStatusBar(char* out, size_t len);
        
        // Check if debug features are enabled
        static bool isEnabled() { return _enabled; }
        static void setEnabled(bool enable) { _enabled = enable; }

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
        // Returns true if any overlay was drawn
        static bool drawOverlay(uint16_t* buf, int16_t stride, int16_t sw, int16_t sh,
                                 int16_t x0, int16_t y0, int16_t w, int16_t h);
        
        // Clear recorded rects after flush
        static void clearRects() { _rectCount = 0; }
        
    private:
        static DebugMetrics _metrics;
        static bool _enabled;
        
        // For loop timing calculation (LVGL-style)
        static uint32_t _frameStartMs;
        static uint32_t _prevFrameStartMs;
        static uint32_t _frameCounter;
        static uint32_t _fpsUpdateMs;
        
        // Dirty rect debug
        static bool _dirtyRectEnabled;
        static uint16_t _dirtyRectColor;
        static uint16_t _dirtyRectActiveColor;
        static DirtyRect _rects[8];
        static uint8_t _rectCount;
    };

}

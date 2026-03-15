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

        [[nodiscard]] static const DebugMetrics &metrics() noexcept { return _metrics; }

        static void formatStatusBar(char *out, size_t len);

        [[nodiscard]] static bool isEnabled() noexcept { return _enabled; }
        static void setEnabled(bool enable) noexcept { _enabled = enable; }

        static void setDirtyRectEnabled(bool enabled) noexcept { _dirtyRectEnabled = enabled; }
        [[nodiscard]] static bool dirtyRectEnabled() noexcept { return _dirtyRectEnabled; }

        static void setDirtyRectActiveColor(uint16_t color) noexcept { _dirtyRectActiveColor = color; }
        [[nodiscard]] static uint16_t dirtyRectActiveColor() noexcept { return _dirtyRectActiveColor; }

        static void recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h);

        static void drawOverlay(uint16_t *buf, int16_t stride,
                                int16_t x0, int16_t y0, int16_t w, int16_t h);

        static void clearRects();

    private:
        static DebugMetrics _metrics;
        static bool _enabled;

        static bool _dirtyRectEnabled;
        static uint16_t _dirtyRectActiveColor;
        static DirtyRect *_rects;
        static uint16_t _rectCapacity;
        static uint16_t _rectCount;
    };

}

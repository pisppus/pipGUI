#include "Debug.hpp"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

namespace pipgui
{

    DebugMetrics Debug::_metrics;
    bool Debug::_enabled = false;
    bool Debug::_metricsStatusBarEnabled = false;

    // Time-based CPU measurement
    uint32_t Debug::_lastUpdateTime = 0;
    uint32_t Debug::_busyTimeAccum = 0;
    uint32_t Debug::_renderStartTime = 0;
    bool Debug::_isRendering = false;

    // Dirty rect debug static members
    bool Debug::_dirtyRectEnabled = false;
    uint16_t Debug::_dirtyRectColor = 0x39E7;       // Default: magenta-ish
    uint16_t Debug::_dirtyRectActiveColor = 0xF81F; // Default: bright magenta
    DirtyRect Debug::_rects[8] = {};
    uint8_t Debug::_rectCount = 0;

    void Debug::init()
    {
        _lastUpdateTime = micros();
        _busyTimeAccum = 0;
        _renderStartTime = 0;
        _isRendering = false;
        _enabled = true;

        // Initialize metrics with current values
        update();
    }

    void Debug::beginRender()
    {
        if (!_enabled)
            return;
        if (_isRendering)
            return; // Already rendering, ignore nested call
        _renderStartTime = micros();
        _isRendering = true;
    }

    void Debug::endRender()
    {
        if (!_enabled || !_isRendering)
            return;
        uint32_t elapsed = micros() - _renderStartTime;
        _busyTimeAccum += elapsed;
        _isRendering = false;
    }

    uint32_t Debug::getRuntimeCounter()
    {
        // Use micros() as a high-resolution timer
        // This works on ESP32 and gives us microsecond resolution
        return (uint32_t)(micros() / 1000); // Convert to milliseconds for percentage calc
    }

    void Debug::update()
    {
        if (!_enabled)
            return;

        uint32_t now = micros();
        uint32_t totalElapsed = now - _lastUpdateTime;

        // Update every 50ms
        if (totalElapsed >= 50000)
        {
            // Calculate CPU%
            if (totalElapsed > 0)
            {
                uint32_t cpuPercent = (_busyTimeAccum * 100) / totalElapsed;
                _metrics.cpuPercent = (uint8_t)(cpuPercent > 100 ? 100 : cpuPercent);
            }

            // Reset accumulator
            _busyTimeAccum = 0;
            _lastUpdateTime = now;

            // Update RAM metrics
            _metrics.freeHeapTotal = esp_get_free_heap_size();
            _metrics.freeHeapInternal = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            _metrics.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
            _metrics.minFreeHeap = esp_get_minimum_free_heap_size();
        }
    }

    void Debug::formatStatusBar(char *out, size_t len)
    {
        if (!_enabled || len == 0)
        {
            if (len > 0)
                out[0] = '\0';
            return;
        }

        // Format: "CPU:xx% T:xxk D:xxk L:xxk M:xxk"
        int written = snprintf(out, len, "CPU:%d%% T:%dk D:%dk L:%dk M:%dk",
                               _metrics.cpuPercent,
                               (int)(_metrics.freeHeapTotal / 1024),
                               (int)(_metrics.freeHeapInternal / 1024),
                               (int)(_metrics.largestFreeBlock / 1024),
                               (int)(_metrics.minFreeHeap / 1024));

        if (written < 0 || (size_t)written >= len)
        {
            // Truncated
            out[len - 1] = '\0';
        }
    }

    void Debug::recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled)
            return;

        // Record original rect without expansion
        if (_rectCount < 8)
        {
            _rects[_rectCount++] = {x, y, w, h};
        }
        else
        {
            // Shift and add at end
            for (uint8_t i = 1; i < _rectCount; ++i)
                _rects[i - 1] = _rects[i];
            _rects[_rectCount - 1] = {x, y, w, h};
        }
    }

    static inline uint16_t swap16(uint16_t v)
    {
        return (uint16_t)((v >> 8) | (v << 8));
    }

    void Debug::drawOverlay(uint16_t *buf, int16_t stride, int16_t sw, int16_t sh,
                            int16_t x0, int16_t y0, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled || !buf || _rectCount == 0)
            return;

        // Check if current flush rect intersects with any recorded debug rect
        for (uint8_t i = 0; i < _rectCount; ++i)
        {
            const DirtyRect &dr = _rects[i];

            // Check for intersection
            int16_t drx2 = dr.x + dr.w;
            int16_t dry2 = dr.y + dr.h;
            int16_t x2 = x0 + w;
            int16_t y2 = y0 + h;

            bool intersects = !(x2 <= dr.x || x0 >= drx2 || y2 <= dr.y || y0 >= dry2);

            if (intersects)
            {
                // Calculate intersection rect
                int16_t ix = (x0 > dr.x) ? x0 : dr.x;
                int16_t iy = (y0 > dr.y) ? y0 : dr.y;
                int16_t ix2 = (x2 < drx2) ? x2 : drx2;
                int16_t iy2 = (y2 < dry2) ? y2 : dry2;
                int16_t iw = ix2 - ix;
                int16_t ih = iy2 - iy;

                if (iw > 0 && ih > 0)
                {
                    // Draw border overlay in buffer coordinates
                    const uint16_t col = swap16(_dirtyRectActiveColor);

                    // NOTE: buf points to the full sprite buffer, not a flush-local sub-buffer.
                    // Draw using absolute coordinates (screen/sprite space) and clamp to both
                    // the sprite bounds and current flush rect.
                    const int16_t fx0 = x0;
                    const int16_t fy0 = y0;
                    const int16_t fx1 = (int16_t)(x0 + w);
                    const int16_t fy1 = (int16_t)(y0 + h);

                    auto inFlush = [&](int16_t px, int16_t py) -> bool
                    {
                        return (px >= fx0 && px < fx1 && py >= fy0 && py < fy1);
                    };

                    // Top and bottom edges
                    for (int16_t xx = 0; xx < iw; ++xx)
                    {
                        int16_t px = (int16_t)(ix + xx);
                        int16_t pyTop = iy;
                        int16_t pyBot = (int16_t)(iy + ih - 1);

                        if ((uint16_t)px < (uint16_t)sw && (uint16_t)pyTop < (uint16_t)sh && inFlush(px, pyTop))
                            buf[(int32_t)pyTop * (int32_t)stride + px] = col;
                        if (ih > 1 && (uint16_t)px < (uint16_t)sw && (uint16_t)pyBot < (uint16_t)sh && inFlush(px, pyBot))
                            buf[(int32_t)pyBot * (int32_t)stride + px] = col;
                    }

                    // Left and right edges
                    for (int16_t yy = 1; yy < ih - 1; ++yy)
                    {
                        int16_t py = (int16_t)(iy + yy);
                        int16_t pxLeft = ix;
                        int16_t pxRight = (int16_t)(ix + iw - 1);

                        if ((uint16_t)pxLeft < (uint16_t)sw && (uint16_t)py < (uint16_t)sh && inFlush(pxLeft, py))
                            buf[(int32_t)py * (int32_t)stride + pxLeft] = col;
                        if (iw > 1 && (uint16_t)pxRight < (uint16_t)sw && (uint16_t)py < (uint16_t)sh && inFlush(pxRight, py))
                            buf[(int32_t)py * (int32_t)stride + pxRight] = col;
                    }
                }
            }
        }
    }

}

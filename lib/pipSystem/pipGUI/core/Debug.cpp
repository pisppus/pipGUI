#include "Debug.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

namespace pipgui
{

    DebugMetrics Debug::_metrics;
    bool Debug::_enabled = false;

    // Loop timing static members (LVGL-style)
    uint32_t Debug::_frameStartMs = 0;
    uint32_t Debug::_prevFrameStartMs = 0;
    uint32_t Debug::_frameCounter = 0;
    uint32_t Debug::_fpsUpdateMs = 0;

    // Dirty rect debug static members
    bool Debug::_dirtyRectEnabled = false;
    uint16_t Debug::_dirtyRectColor = 0x39E7;        // Default: magenta-ish
    uint16_t Debug::_dirtyRectActiveColor = 0xF81F;  // Default: bright magenta
    DirtyRect Debug::_rects[8] = {};
    uint8_t Debug::_rectCount = 0;

    void Debug::init()
    {
        _frameStartMs = millis();
        _prevFrameStartMs = _frameStartMs;
        _frameCounter = 0;
        _fpsUpdateMs = millis();
        _enabled = true;
        
        // Initialize metrics with current values
        update();
    }

    // Called at the start of a frame to mark frame begin time
    void Debug::frameStart()
    {
        _prevFrameStartMs = _frameStartMs;
        _frameStartMs = millis();
    }

    // Called at the end of a frame to calculate frame time and FPS
    void Debug::frameEnd()
    {
        uint32_t now = millis();
        uint32_t busyMs = now - _frameStartMs;
        uint32_t periodMs = _frameStartMs - _prevFrameStartMs;

        _metrics.handlerTimeMs = (uint16_t)busyMs;
        if (periodMs > 0)
        {
            uint32_t busyPct = (busyMs * 100U) / periodMs;
            if (busyPct > 100U) busyPct = 100U;
            _metrics.cpuPercent = (uint8_t)busyPct;
        }

        _frameCounter++;
        
        // Update FPS every 1000ms (1 second)
        if ((now - _fpsUpdateMs) >= 1000) {
            _metrics.fps = (uint8_t)_frameCounter;
            _frameCounter = 0;
            _fpsUpdateMs = now;
        }
    }

    void Debug::update()
    {
        if (!_enabled) return;

        // Frame timing is updated automatically via frameStart/frameEnd
        // This method is kept for compatibility and periodic RAM updates
        
        // Update RAM metrics
        // Internal DRAM (most important for tracking leaks)
        _metrics.freeHeapInternal = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        
        // Largest continuous block (fragmentation indicator)
        _metrics.largestFreeBlockInternal = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        
        // Minimum free internal heap since boot (leak detection)
        _metrics.minFreeHeapInternal = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    }

    void Debug::formatStatusBar(char* out, size_t len)
    {
        if (!_enabled || len == 0) {
            if (len > 0) out[0] = '\0';
            return;
        }

        // Format: "CPU:xx% FPS:xx T:xxms F:xxk"
        // CPU = busy% of GUI::loop vs period between calls
        // FPS = calls per second
        // T = handler time (GUI::loop duration)
        // F = free heap in KB
        
        int written = snprintf(out, len, "CPU:%u%% FPS:%u T:%ums F:%uk",
                               (unsigned)_metrics.cpuPercent,
                               (unsigned)_metrics.fps,
                               (unsigned)_metrics.handlerTimeMs,
                               (unsigned)(_metrics.freeHeapInternal / 1024U));
        
        if (written < 0 || (size_t)written >= len) {
            out[len - 1] = '\0';
        }
    }

    void Debug::recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled) return;

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

    bool Debug::drawOverlay(uint16_t* buf, int16_t stride, int16_t sw, int16_t sh,
                            int16_t x0, int16_t y0, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled || !buf || _rectCount == 0) return false;
        
        // Check if current flush rect matches any recorded debug rect
        for (uint8_t i = 0; i < _rectCount; ++i)
        {
            const DirtyRect& dr = _rects[i];
            if (dr.x == x0 && dr.y == y0 && dr.w == w && dr.h == h)
            {
                // Draw border overlay
                const uint16_t col = swap16(_dirtyRectActiveColor);
                
                // Top and bottom edges
                for (int16_t xx = 0; xx < w; ++xx)
                {
                    buf[xx] = col;
                    if (h > 1)
                        buf[(h - 1) * stride + xx] = col;
                }
                
                // Left and right edges
                for (int16_t yy = 1; yy < h - 1; ++yy)
                {
                    buf[yy * stride] = col;
                    if (w > 1)
                        buf[yy * stride + (w - 1)] = col;
                }
                return true;
            }
        }
        return false;
    }

}

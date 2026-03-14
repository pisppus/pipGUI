#include "Debug.hpp"
#include <pipCore/Platform.hpp>
#include <pipCore/Platforms/Select.hpp>
#include <pipCore/Graphics/Sprite.hpp>

namespace pipgui
{

    DebugMetrics Debug::_metrics;
    bool Debug::_enabled = false;
    bool Debug::_dirtyRectEnabled = false;
    uint16_t Debug::_dirtyRectActiveColor = 0xF81F;
    DirtyRect *Debug::_rects = nullptr;
    uint16_t Debug::_rectCapacity = 0;
    uint16_t Debug::_rectCount = 0;

    void Debug::init()
    {
        _enabled = true;
        update();
    }

    void Debug::update()
    {
        if (!_enabled)
            return;

        pipcore::Platform *plat = pipcore::GetPlatform();
        _metrics.freeHeapTotal = plat->freeHeapTotal();
        _metrics.freeHeapInternal = plat->freeHeapInternal();
        _metrics.largestFreeBlock = plat->largestFreeBlock();
        _metrics.minFreeHeap = plat->minFreeHeap();
    }

    void Debug::formatStatusBar(char *out, size_t len)
    {
        if (!_enabled || len == 0)
        {
            if (len > 0)
                out[0] = '\0';
            return;
        }

        // Keep this string compact: it must fit into the status bar.
        // T  = total free heap (KB)
        // In = internal free heap (KB)
        // Bl = largest free block (KB)
        // Mn = minimum free heap seen (KB)
        int written = snprintf(out, len, "T:%dk In:%dk Bl:%dk Mn:%dk",
                               (int)(_metrics.freeHeapTotal / 1024),
                               (int)(_metrics.freeHeapInternal / 1024),
                               (int)(_metrics.largestFreeBlock / 1024),
                               (int)(_metrics.minFreeHeap / 1024));

        if (written < 0 || (size_t)written >= len)
        {
            out[len - 1] = '\0';
        }
    }

    void Debug::recordDirtyRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled)
            return;

        if (_rectCount < _rectCapacity)
        {
            _rects[_rectCount++] = {x, y, w, h};
        }
        else
        {
            uint16_t newCap = _rectCapacity ? _rectCapacity * 2 : 16;
            pipcore::Platform *plat = pipcore::GetPlatform();
            DirtyRect *newRects = (DirtyRect *)plat->alloc(sizeof(DirtyRect) * newCap, pipcore::AllocCaps::Default);
            if (!newRects)
                return;
            for (uint16_t i = 0; i < _rectCount; ++i)
                newRects[i] = _rects[i];
            plat->free(_rects);
            _rects = newRects;
            _rectCapacity = newCap;
            _rects[_rectCount++] = {x, y, w, h};
        }
    }

    void Debug::drawOverlay(uint16_t *buf, int16_t stride,
                            int16_t x0, int16_t y0, int16_t w, int16_t h)
    {
        if (!_dirtyRectEnabled || !buf || !_rects || _rectCount == 0)
            return;

        const uint16_t col = pipcore::Sprite::swap16(_dirtyRectActiveColor);
        const int16_t x1 = x0 + w;
        const int16_t y1 = y0 + h;

        for (uint16_t i = 0; i < _rectCount; ++i)
        {
            const DirtyRect &dr = _rects[i];

            int16_t ix0 = (x0 > dr.x) ? x0 : dr.x;
            int16_t iy0 = (y0 > dr.y) ? y0 : dr.y;
            int16_t ix1 = (x1 < (dr.x + dr.w)) ? x1 : (dr.x + dr.w);
            int16_t iy1 = (y1 < (dr.y + dr.h)) ? y1 : (dr.y + dr.h);

            if (ix1 <= ix0 || iy1 <= iy0)
                continue;

            for (int16_t x = ix0; x < ix1; ++x)
            {
                if (iy0 == dr.y)
                    buf[(int32_t)iy0 * stride + x] = col;
                if (iy1 - 1 == dr.y + dr.h - 1)
                    buf[(int32_t)(iy1 - 1) * stride + x] = col;
            }

            for (int16_t y = iy0; y < iy1; ++y)
            {
                if (ix0 == dr.x)
                    buf[(int32_t)y * stride + ix0] = col;
                if (ix1 - 1 == dr.x + dr.w - 1)
                    buf[(int32_t)y * stride + (ix1 - 1)] = col;
            }
        }
    }

    void Debug::clearRects()
    {
        _rectCount = 0;
        pipcore::Platform *plat = pipcore::GetPlatform();
        plat->free(_rects);
        _rects = nullptr;
        _rectCapacity = 0;
    }

}


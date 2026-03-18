#include <pipGUI/Systems/Screenshots/Internals.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pipgui
{
    namespace ssd = screenshots::detail;

#if (PIPGUI_SCREENSHOT_MODE == 2)
    namespace
    {

        constexpr const char *kCounterPath = "/pipKit/screenshots/.counter";

        static bool gShotsCounterReady = false;
        static uint32_t gShotsCounterNext = 0;

        [[nodiscard]] inline bool readShotsCounter(uint32_t &out) noexcept
        {
            out = 0;
            fs::File f = LittleFS.open(kCounterPath, FILE_READ);
            if (!f)
                return false;
            char buf[20];
            const int n = f.read(reinterpret_cast<uint8_t *>(buf), sizeof(buf) - 1u);
            f.close();
            if (n <= 0)
                return false;
            buf[static_cast<size_t>(n)] = '\0';

            char *end = nullptr;
            const unsigned long v = std::strtoul(buf, &end, 10);
            if (end == buf)
                return false;
            out = static_cast<uint32_t>(v);
            return true;
        }

        inline void writeShotsCounter(uint32_t value) noexcept
        {
            char tmp[64];
            if (!ssd::tmpPathFromFinal(tmp, sizeof(tmp), kCounterPath))
                return;

            fs::File f = LittleFS.open(tmp, FILE_WRITE);
            if (!f)
                return;

            char buf[20];
            const int len = std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
            bool ok = (len > 0) && (f.write(reinterpret_cast<const uint8_t *>(buf), static_cast<size_t>(len)) == static_cast<size_t>(len));
            if (ok)
            {
                f.flush();
                ok = (f.size() == static_cast<size_t>(len));
            }
            f.close();

            if (ok)
            {
                if (!LittleFS.rename(tmp, kCounterPath))
                {
                    LittleFS.remove(tmp);
                }
            }
            else
            {
                LittleFS.remove(tmp);
            }
        }

        [[nodiscard]] inline uint32_t scanMaxShotStamp() noexcept
        {
            fs::File d = LittleFS.open(ssd::kShotsDir);
            if (!d)
                return 0;

            uint32_t maxStamp = 0;
            while (true)
            {
                fs::File file = d.openNextFile();
                if (!file)
                    break;
                if (file.isDirectory())
                {
                    file.close();
                    continue;
                }

                const char *name = file.name();
                uint32_t stamp = 0;
                if (name && ssd::hasSuffix(name, ".pscr") && ssd::parseStamp(name, stamp))
                {
                    if (stamp > maxStamp)
                        maxStamp = stamp;
                }
                file.close();
            }
            d.close();
            return maxStamp;
        }

        [[nodiscard]] inline uint32_t allocShotStamp() noexcept
        {
            if (!gShotsCounterReady)
            {
                uint32_t last = 0;
                (void)readShotsCounter(last);
                const uint32_t maxExisting = scanMaxShotStamp();
                const uint32_t baseline = (last > maxExisting) ? last : maxExisting;
                gShotsCounterNext = baseline + 1u;
                if (gShotsCounterNext == 0)
                    gShotsCounterNext = 1;
                gShotsCounterReady = true;
            }

            uint32_t stamp = gShotsCounterNext++;
            if (stamp == 0)
                stamp = gShotsCounterNext++;
            return stamp;
        }

        inline void commitShotStamp(uint32_t stamp) noexcept
        {
            writeShotsCounter(stamp);
        }

    }
#endif

    bool GUI::startScreenshot()
    {
#if !PIPGUI_SCREENSHOTS
        return false;
#else
        if (_shotStream.active)
            return false;
        startScreenshotStream();
        return _shotStream.active;
#endif
    }

    void GUI::configureScreenshotGallery(uint8_t maxShots, uint16_t thumbW, uint16_t thumbH, uint16_t padding)
    {
#if !PIPGUI_SCREENSHOTS
        (void)maxShots;
        (void)thumbW;
        (void)thumbH;
        (void)padding;
        return;
#else
        if (maxShots == 0)
            maxShots = 1;
        if (thumbW == 0 || thumbH == 0)
        {
            thumbW = 64;
            thumbH = 40;
        }
        if (_shots.entries)
            freeScreenshotGallery(platform());
        _shots.maxShots = maxShots;
        _shots.thumbW = thumbW;
        _shots.thumbH = thumbH;
        _shots.padding = padding;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        _shots.flashLoadIndex = 0;
        _shots.fsDirsReady = false;
        _shots.flashScanDone = false;
        _shots.flashScanActive = false;
        _shots.flashThumbsDone = false;
        _shots.thumbIndexReady = false;
        _shots.thumbIndexW = 0;
        _shots.thumbIndexH = 0;
        if (_shots.scanDir)
            _shots.scanDir.close();
#endif
#endif
    }

    void GUI::setScreenshotShortcut(Button *next, Button *prev, uint16_t holdMs)
    {
#if !PIPGUI_SCREENSHOTS
        (void)next;
        (void)prev;
        (void)holdMs;
        return;
#else
        _diag.screenshotNext = next;
        _diag.screenshotPrev = prev;
        _diag.screenshotHoldMs = (holdMs == 0) ? 500 : holdMs;
        _diag.screenshotHoldStartMs = 0;
        _diag.screenshotCaptured = false;
#endif
    }

    void GUI::handleScreenshotShortcut(bool nextDown, bool prevDown)
    {
#if !PIPGUI_SCREENSHOTS
        (void)nextDown;
        (void)prevDown;
        return;
#else
        const bool comboDown = nextDown && prevDown;
        if (comboDown)
        {
            if (_diag.screenshotHoldStartMs == 0)
                _diag.screenshotHoldStartMs = nowMs();

            const uint32_t now = nowMs();
            if (!_diag.screenshotCaptured && (now - _diag.screenshotHoldStartMs) >= _diag.screenshotHoldMs)
            {
                if (_shotStream.active)
                {
                    _diag.screenshotCaptured = true;
                    return;
                }

                const bool started = startScreenshot();
                if (started)
                {
                    _shotStream.notifyOnComplete = true;
#if (PIPGUI_SCREENSHOT_MODE != 2)
                    captureScreenshotToGallery();
#endif
                }
                else
                {
                    showToastInternal("Screenshot failed", 1800, true,
                                      static_cast<IconId>(psdf_icons::IconCount), 0);
                }
                _diag.screenshotCaptured = true;
            }
        }
        else
        {
            _diag.screenshotHoldStartMs = 0;
            _diag.screenshotCaptured = false;
        }
#endif
    }

    void GUI::freeScreenshotGallery(pipcore::Platform *plat) noexcept
    {
#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (_shots.scanDir)
            _shots.scanDir.close();
        if (_shots.rowBuf)
        {
            detail::free(plat, _shots.rowBuf);
            _shots.rowBuf = nullptr;
            _shots.rowBufSize = 0;
        }
        _shots.flashLoadIndex = 0;
        _shots.flashScanDone = false;
        _shots.flashScanActive = false;
        _shots.flashThumbsDone = false;
        _shots.thumbIndexReady = false;
        _shots.thumbIndexW = 0;
        _shots.thumbIndexH = 0;
#endif
        if (!_shots.entries)
            return;
        for (uint8_t i = 0; i < _shots.maxShots; ++i)
        {
            if (_shots.entries[i].pixels)
                detail::free(plat, _shots.entries[i].pixels);
        }
        detail::free(plat, _shots.entries);
        _shots.entries = nullptr;
        _shots.count = 0;
    }

    void GUI::resetScreenshotStreamState(pipcore::Platform *plat, bool keepBuffer) noexcept
    {
        if (!keepBuffer && _shotStream.buffer)
        {
            if (plat)
                detail::free(plat, _shotStream.buffer);
            _shotStream.buffer = nullptr;
        }
        if (!keepBuffer)
            _shotStream.bufferSize = 0;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (_shotStream.file)
            _shotStream.file.close();
        _shotStream.file = {};
        _shotStream.stamp = 0;
        if (_shotStream.path[0] != '\0')
        {
            char tmpPath[64];
            if (ssd::tmpPathFromFinal(tmpPath, sizeof(tmpPath), _shotStream.path))
                LittleFS.remove(tmpPath);
        }
        _shotStream.path[0] = '\0';
#endif
        _shotStream.width = 0;
        _shotStream.height = 0;
        _shotStream.payloadSize = 0;
        _shotStream.payloadOffset = 0;
        _shotStream.payloadCrc = 0;
        _shotStream.headerOffset = 0;
        _shotStream.headerReady = false;
        _shotStream.active = false;
        _shotStream.notifyOnComplete = false;

        _shotStream.qoiSrc16 = nullptr;
        _shotStream.qoiSrcBE = false;
        _shotStream.qoiPos = 0;
        _shotStream.qoiPayloadBytes = 0;
        _shotStream.qoiPrev = 0x000000FFu;
        std::memset(_shotStream.qoiIndex, 0, sizeof(_shotStream.qoiIndex));
        _shotStream.qoiRun = 0;
        _shotStream.qoiTailOffset = 0;
        _shotStream.qoiOutOff = 0;
        _shotStream.qoiOutLen = 0;
    }

    void GUI::freeScreenshotStream(pipcore::Platform *plat) noexcept
    {
        resetScreenshotStreamState(plat, false);
    }

    bool GUI::ensureScreenshotGallery(pipcore::Platform *plat)
    {
        if (_shots.entries)
            return true;
        if (!plat)
            return false;
        void *mem = detail::alloc(plat, sizeof(ScreenshotEntry) * _shots.maxShots, pipcore::AllocCaps::PreferExternal);
        if (!mem)
            return false;
        _shots.entries = static_cast<ScreenshotEntry *>(mem);
        for (uint8_t i = 0; i < _shots.maxShots; ++i)
            _shots.entries[i] = {};
        return true;
    }

    void GUI::insertShotToGalleryFrom565(const uint16_t *src, uint16_t w, uint16_t h, uint32_t stamp, const char *path)
    {
#if !PIPGUI_SCREENSHOTS
        (void)src;
        (void)w;
        (void)h;
        (void)stamp;
        (void)path;
        return;
#else
        if (!src || w == 0 || h == 0)
            return;
        if (!ensureScreenshotGallery(platform()))
            return;

        const uint16_t tw = _shots.thumbW;
        const uint16_t th = _shots.thumbH;
        if (tw == 0 || th == 0)
            return;

#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (_shots.scanDir)
            _shots.scanDir.close();
        _shots.flashScanActive = false;
#endif

        const uint8_t cap = _shots.maxShots;
        if (cap == 0)
            return;

        ScreenshotEntry tmp = _shots.entries[cap - 1];
        for (int i = cap - 1; i > 0; --i)
            _shots.entries[i] = _shots.entries[i - 1];
        _shots.entries[0] = tmp;

        ScreenshotEntry &entry = _shots.entries[0];
        const size_t pixels = static_cast<size_t>(tw) * static_cast<size_t>(th);
        if (!entry.pixels)
        {
            entry.pixels = static_cast<uint16_t *>(
                detail::alloc(platform(), pixels * sizeof(uint16_t), pipcore::AllocCaps::PreferExternal));
            if (!entry.pixels)
                return;
        }

        const bool intScale = (w >= tw && h >= th && (w % tw) == 0 && (h % th) == 0);
        if (intScale)
        {
            const uint16_t sxStep = static_cast<uint16_t>(w / tw);
            const uint16_t syStep = static_cast<uint16_t>(h / th);
            const uint32_t area = static_cast<uint32_t>(sxStep) * static_cast<uint32_t>(syStep);
            const uint32_t half = area / 2u;

            for (uint16_t dy = 0; dy < th; ++dy)
            {
                uint16_t *dstRow = entry.pixels + static_cast<uint32_t>(dy) * tw;
                const uint32_t syBase = static_cast<uint32_t>(dy) * syStep;

                for (uint16_t dx = 0; dx < tw; ++dx)
                {
                    const uint32_t sxBase = static_cast<uint32_t>(dx) * sxStep;
                    uint32_t sr = 0, sg = 0, sb = 0;
                    for (uint16_t iy = 0; iy < syStep; ++iy)
                    {
                        const uint16_t *srcRow = src + (syBase + iy) * w + sxBase;
                        for (uint16_t ix = 0; ix < sxStep; ++ix)
                        {
                            const uint16_t px = pipcore::Sprite::swap16(srcRow[ix]);
                            sr += (px >> 11) & 31u;
                            sg += (px >> 5) & 63u;
                            sb += px & 31u;
                        }
                    }
                    const uint32_t r = (sr + half) / area;
                    const uint32_t g = (sg + half) / area;
                    const uint32_t b = (sb + half) / area;
                    dstRow[dx] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
                }
            }
        }
        else
        {
            const uint32_t srcW16 = static_cast<uint32_t>(w) << 16;
            const uint32_t srcH16 = static_cast<uint32_t>(h) << 16;
            const uint32_t scaleX16 = srcW16 / tw;
            const uint32_t scaleY16 = srcH16 / th;

            for (uint16_t dy = 0; dy < th; ++dy)
            {
                const uint32_t y0 = static_cast<uint32_t>(dy) * scaleY16;
                const uint32_t y1 = (dy + 1u == th) ? srcH16 : (y0 + scaleY16);
                const uint32_t yLen = y1 - y0;
                const uint16_t sy0 = static_cast<uint16_t>(y0 >> 16);
                const uint16_t sy1 = static_cast<uint16_t>((y1 - 1u) >> 16);

                uint16_t *dstRow = entry.pixels + static_cast<uint32_t>(dy) * tw;
                for (uint16_t dx = 0; dx < tw; ++dx)
                {
                    const uint32_t x0 = static_cast<uint32_t>(dx) * scaleX16;
                    const uint32_t x1 = (dx + 1u == tw) ? srcW16 : (x0 + scaleX16);
                    const uint32_t xLen = x1 - x0;

                    uint64_t ar = 0, ag = 0, ab = 0;
                    const uint16_t sx0 = static_cast<uint16_t>(x0 >> 16);
                    const uint16_t sx1 = static_cast<uint16_t>((x1 - 1u) >> 16);

                    for (uint16_t sy = sy0; sy <= sy1; ++sy)
                    {
                        const uint32_t wy = ssd::overlap16(y0, y1, static_cast<uint32_t>(sy) << 16);
                        if (!wy)
                            continue;
                        const uint16_t *srcRow = src + static_cast<uint32_t>(sy) * w;
                        for (uint16_t sx = sx0; sx <= sx1; ++sx)
                        {
                            const uint32_t wx = ssd::overlap16(x0, x1, static_cast<uint32_t>(sx) << 16);
                            if (!wx)
                                continue;
                            const uint64_t a = static_cast<uint64_t>(wx) * static_cast<uint64_t>(wy);
                            const uint16_t px = pipcore::Sprite::swap16(srcRow[sx]);
                            ar += static_cast<uint64_t>((px >> 11) & 31u) * a;
                            ag += static_cast<uint64_t>((px >> 5) & 63u) * a;
                            ab += static_cast<uint64_t>(px & 31u) * a;
                        }
                    }

                    const uint64_t total = static_cast<uint64_t>(xLen) * static_cast<uint64_t>(yLen);
                    if (!total)
                    {
                        dstRow[dx] = 0;
                        continue;
                    }
                    const uint64_t half = total / 2u;
                    const uint64_t r = (ar + half) / total;
                    const uint64_t g = (ag + half) / total;
                    const uint64_t b = (ab + half) / total;
                    dstRow[dx] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
                }
            }
        }
#if (PIPGUI_SCREENSHOT_MODE == 2)
        entry.stamp = stamp;
        entry.thumbOnFlash = false;
        if (path && path[0])
        {
            std::strncpy(entry.path, path, sizeof(entry.path) - 1);
            entry.path[sizeof(entry.path) - 1] = '\0';
        }
        else
        {
            entry.path[0] = '\0';
        }
#endif
        if (_shots.count < cap)
            ++_shots.count;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        _shots.flashLoadIndex = 0;
        if (_shots.flashScanDone)
        {
            bool allLoaded = true;
            for (uint8_t i = 0; i < _shots.count; ++i)
            {
                if (_shots.entries[i].path[0] != '\0' && !_shots.entries[i].pixels)
                {
                    allLoaded = false;
                    break;
                }
            }
            _shots.flashThumbsDone = allLoaded;
        }
        else
        {
            _shots.flashThumbsDone = false;
        }
#endif
        _flags.needRedraw = 1;
#endif
    }

    void GUI::captureScreenshotToGallery()
    {
#if !PIPGUI_SCREENSHOTS
        return;
#else
        if (!_flags.spriteEnabled || _render.screenWidth == 0 || _render.screenHeight == 0)
            return;
        const uint16_t *src = static_cast<const uint16_t *>(_render.sprite.getBuffer());
        if (!src)
            return;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        // Never show a flash screenshot before the file is fully committed.
        return;
#else
        insertShotToGalleryFrom565(src, _render.screenWidth, _render.screenHeight, 0, nullptr);
#endif
#endif
    }

    void GUI::startScreenshotStream()
    {
#if !PIPGUI_SCREENSHOTS
        return;
#else
        if (_shotStream.active)
            return;
        if (!_flags.spriteEnabled || !_disp.display)
            return;

        const uint16_t w = _render.screenWidth;
        const uint16_t h = _render.screenHeight;
        if (w == 0 || h == 0)
            return;

        const void *buf = _render.sprite.getBuffer();
        if (!buf)
            return;

        const uint32_t payloadSize = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * 2u;
        pipcore::Platform *plat = platform();
        if (!plat)
            return;
        resetScreenshotStreamState(plat, true);
        if (!_shotStream.buffer || _shotStream.bufferSize < payloadSize)
        {
            if (_shotStream.buffer)
                detail::free(plat, _shotStream.buffer);
            void *mem = detail::alloc(plat, payloadSize, pipcore::AllocCaps::PreferExternal);
            if (!mem)
                return;
            _shotStream.buffer = static_cast<uint8_t *>(mem);
            _shotStream.bufferSize = payloadSize;
        }
#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (!_shots.fsReady)
        {
            _shots.fsReady = LittleFS.begin(false);
            _shots.fsDirsReady = false;
            if (!_shots.fsReady)
            {
                return;
            }
        }
        ssd::ensureDirs(_shots.fsDirsReady);
        if (!_shots.fsDirsReady)
            return;
        char path[64];
        char tmpPath[64];
        uint32_t stamp = 0;
        fs::File f;
        for (uint8_t attempt = 0; attempt < 8; ++attempt)
        {
            stamp = allocShotStamp();
            std::snprintf(path, sizeof(path), "%s/pscr_%08lu.pscr",
                          ssd::kShotsDir,
                          static_cast<unsigned long>(stamp));
            std::snprintf(tmpPath, sizeof(tmpPath), "%s/pscr_%08lu.tmp",
                          ssd::kShotsDir,
                          static_cast<unsigned long>(stamp));
            f = LittleFS.open(tmpPath, FILE_WRITE);
            if (f)
                break;
        }
        if (!f)
            return;
        _shotStream.file = f;
        _shotStream.stamp = stamp;
        std::strncpy(_shotStream.path, path, sizeof(_shotStream.path) - 1);
        _shotStream.path[sizeof(_shotStream.path) - 1] = '\0';
#endif
        std::memcpy(_shotStream.buffer, buf, payloadSize);

        _shotStream.width = w;
        _shotStream.height = h;
        _shotStream.payloadSize = 0;
        _shotStream.payloadOffset = 0;
        _shotStream.payloadCrc = 0;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        _shotStream.payloadCrc = ssd::crc32Init();
#endif
        _shotStream.headerOffset = 0;
        _shotStream.headerReady = true;
        _shotStream.active = true;
        _shotStream.notifyOnComplete = false;

        _shotStream.qoiSrc16 = reinterpret_cast<const uint16_t *>(_shotStream.buffer);
        _shotStream.qoiSrcBE = true;
        _shotStream.qoiPos = 0;
        _shotStream.qoiPayloadBytes = 0;
        _shotStream.qoiPrev = 0x000000FFu;
        std::memset(_shotStream.qoiIndex, 0, sizeof(_shotStream.qoiIndex));
        _shotStream.qoiRun = 0;
        _shotStream.qoiTailOffset = 0;
        _shotStream.qoiOutOff = 0;
        _shotStream.qoiOutLen = 0;

        _shotStream.header[0] = 'P';
        _shotStream.header[1] = 'S';
        _shotStream.header[2] = 'C';
        _shotStream.header[3] = 'R';
        _shotStream.header[4] = static_cast<uint8_t>(w & 0xFFu);
        _shotStream.header[5] = static_cast<uint8_t>((w >> 8) & 0xFFu);
        _shotStream.header[6] = static_cast<uint8_t>(h & 0xFFu);
        _shotStream.header[7] = static_cast<uint8_t>((h >> 8) & 0xFFu);
        _shotStream.header[8] = static_cast<uint8_t>(ScreenshotFormat::QoiRgb);
        const uint32_t headerSizeField = _shotStream.payloadSize;
        _shotStream.header[9] = static_cast<uint8_t>(headerSizeField & 0xFFu);
        _shotStream.header[10] = static_cast<uint8_t>((headerSizeField >> 8) & 0xFFu);
        _shotStream.header[11] = static_cast<uint8_t>((headerSizeField >> 16) & 0xFFu);
        _shotStream.header[12] = static_cast<uint8_t>((headerSizeField >> 24) & 0xFFu);
#endif
    }

    void GUI::serviceScreenshotStream()
    {
#if !PIPGUI_SCREENSHOTS
        return;
#else
        if (!_shotStream.active)
            return;

        if (!_flags.spriteEnabled || !_disp.display || !_shotStream.buffer)
            return resetScreenshotStreamState(platform(), true);

#if (PIPGUI_SCREENSHOT_MODE == 1)
        size_t budget = Serial.availableForWrite();
        if (budget == 0)
            return;
        if (budget > 8192)
            budget = 8192;
#else
        size_t budget = 8192;
#endif

        const auto finish = [&](bool ok)
        {
            const bool notify = _shotStream.notifyOnComplete;
#if (PIPGUI_SCREENSHOT_MODE == 2)
            uint32_t payloadBytes = _shotStream.payloadSize;
            if (ok && _shotStream.file)
            {
                const uint32_t sz = _shotStream.qoiPayloadBytes;
                payloadBytes = sz;
                if (!_shotStream.file.seek(9))
                {
                    ok = false;
                }
                else
                {
                    uint8_t b[4];
                    b[0] = static_cast<uint8_t>(sz & 0xFFu);
                    b[1] = static_cast<uint8_t>((sz >> 8) & 0xFFu);
                    b[2] = static_cast<uint8_t>((sz >> 16) & 0xFFu);
                    b[3] = static_cast<uint8_t>((sz >> 24) & 0xFFu);
                    ok = (_shotStream.file.write(b, sizeof(b)) == sizeof(b));
                    if (ok)
                        _shotStream.file.flush();
                }
            }

            if (ok && _shotStream.file)
            {
                const uint32_t crc = ssd::crc32Finish(_shotStream.payloadCrc);
                const size_t endPos = static_cast<size_t>(ssd::kHdrSize) + static_cast<size_t>(payloadBytes);
                if (!_shotStream.file.seek(endPos))
                {
                    ok = false;
                }
                else
                {
                    ok = ssd::writeTrailer(_shotStream.file, payloadBytes, crc, 0);
                    if (ok)
                    {
                        _shotStream.file.flush();
                        ok = (_shotStream.file.size() == (endPos + ssd::kTrlSize));
                    }
                }
            }

            char tmpPath[64];
            const bool haveTmp = ssd::tmpPathFromFinal(tmpPath, sizeof(tmpPath), _shotStream.path);
            if (_shotStream.file)
                _shotStream.file.close();
            _shotStream.file = {};

            if (haveTmp)
            {
                if (ok)
                {
                    if (!LittleFS.rename(tmpPath, _shotStream.path))
                    {
                        LittleFS.remove(tmpPath);
                        ok = false;
                    }
                    else
                    {
                        commitShotStamp(_shotStream.stamp);
                    }
                }
                else
                {
                    LittleFS.remove(tmpPath);
                }
            }

            if (ok && _shotStream.buffer)
            {
                insertShotToGalleryFrom565(reinterpret_cast<const uint16_t *>(_shotStream.buffer),
                                           _shotStream.width, _shotStream.height,
                                           _shotStream.stamp, _shotStream.path);
            }
#endif
            // Avoid redundant tmp cleanup in reset: after rename, tmp path won't exist.
#if (PIPGUI_SCREENSHOT_MODE == 2)
            _shotStream.path[0] = '\0';
            // Free the full-screen buffer after commit so gallery thumbnail loading
            // can't get starved by a large retained capture buffer.
            resetScreenshotStreamState(platform(), false);
#else
            resetScreenshotStreamState(platform(), true);
#endif
            if (notify)
            {
                showToastInternal(ok ? "Screenshot saved" : "Screenshot failed", 1800, true,
                                  static_cast<IconId>(psdf_icons::IconCount), 0);
            }
        };

#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (!_shotStream.file)
        {
            finish(false);
            return;
        }
#endif

        if (_shotStream.headerReady && _shotStream.headerOffset < sizeof(_shotStream.header))
        {
            const size_t remaining = sizeof(_shotStream.header) - _shotStream.headerOffset;
            const size_t n = (budget < remaining) ? budget : remaining;
            if (n == 0)
                return;
#if (PIPGUI_SCREENSHOT_MODE == 1)
            const size_t wrote = Serial.write(_shotStream.header + _shotStream.headerOffset, n);
#else
            const size_t wrote = _shotStream.file.write(_shotStream.header + _shotStream.headerOffset, n);
#endif
            if (wrote == 0)
            {
#if (PIPGUI_SCREENSHOT_MODE == 1)
                return;
#else
                finish(false);
                return;
#endif
            }
            _shotStream.headerOffset = static_cast<uint8_t>(_shotStream.headerOffset + wrote);
            if (wrote >= budget || _shotStream.headerOffset < sizeof(_shotStream.header))
                return;
            budget -= wrote;
        }

        const uint32_t pixelsTotal = static_cast<uint32_t>(_shotStream.width) * static_cast<uint32_t>(_shotStream.height);
        const uint8_t *srcBytes = _shotStream.buffer;
        const auto fillOut = [&]()
        {
            if (_shotStream.qoiOutOff < _shotStream.qoiOutLen)
                return;
            _shotStream.qoiOutOff = 0;
            _shotStream.qoiOutLen = 0;

            constexpr size_t cap = sizeof(_shotStream.qoiOut);
            auto have = [&](size_t need)
            { return _shotStream.qoiOutLen + need <= cap; };
            auto put1 = [&](uint8_t b)
            { _shotStream.qoiOut[_shotStream.qoiOutLen++] = b; };
            auto put2 = [&](uint8_t a, uint8_t b)
            {
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = a;
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = b;
            };
            auto put4 = [&](uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            {
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = a;
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = b;
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = c;
                _shotStream.qoiOut[_shotStream.qoiOutLen++] = d;
            };

            while (_shotStream.qoiOutLen < cap)
            {
                if (_shotStream.qoiPos >= pixelsTotal)
                {
                    if (_shotStream.qoiRun)
                    {
                        if (!have(1))
                            break;
                        put1(static_cast<uint8_t>(0xC0u | (_shotStream.qoiRun - 1u)));
                        _shotStream.qoiRun = 0;
                        continue;
                    }

                    static constexpr uint8_t kTail[8] = {0, 0, 0, 0, 0, 0, 0, 1};
                    while (_shotStream.qoiOutLen < cap && _shotStream.qoiTailOffset < 8)
                        put1(kTail[_shotStream.qoiTailOffset++]);
                    break;
                }

                const uint16_t *src16 = _shotStream.qoiSrc16 ? _shotStream.qoiSrc16 : reinterpret_cast<const uint16_t *>(srcBytes);
                uint16_t px565 = src16[_shotStream.qoiPos];
                if (_shotStream.qoiSrcBE)
                    px565 = __builtin_bswap16(px565);
                const uint32_t rgba = ssd::rgbaFrom565(px565);

                if (rgba == _shotStream.qoiPrev)
                {
                    ++_shotStream.qoiRun;
                    ++_shotStream.qoiPos;
                    if (_shotStream.qoiRun == 62 || _shotStream.qoiPos == pixelsTotal)
                    {
                        if (!have(1))
                            break;
                        put1(static_cast<uint8_t>(0xC0u | (_shotStream.qoiRun - 1u)));
                        _shotStream.qoiRun = 0;
                    }
                    continue;
                }

                if (_shotStream.qoiRun)
                {
                    if (!have(1))
                        break;
                    put1(static_cast<uint8_t>(0xC0u | (_shotStream.qoiRun - 1u)));
                    _shotStream.qoiRun = 0;
                    continue;
                }

                const uint8_t idx = ssd::qoiHash(rgba);
                if (_shotStream.qoiIndex[idx] == rgba)
                {
                    if (!have(1))
                        break;
                    put1(static_cast<uint8_t>(idx & 63u));
                }
                else
                {
                    const int pr = static_cast<int>((_shotStream.qoiPrev >> 24) & 0xFFu);
                    const int pg = static_cast<int>((_shotStream.qoiPrev >> 16) & 0xFFu);
                    const int pb = static_cast<int>((_shotStream.qoiPrev >> 8) & 0xFFu);
                    const int r = static_cast<int>((rgba >> 24) & 0xFFu);
                    const int g = static_cast<int>((rgba >> 16) & 0xFFu);
                    const int b = static_cast<int>((rgba >> 8) & 0xFFu);

                    const int dr = r - pr;
                    const int dg = g - pg;
                    const int db = b - pb;

                    if ((unsigned)(dr + 2) < 4u && (unsigned)(dg + 2) < 4u && (unsigned)(db + 2) < 4u)
                    {
                        if (!have(1))
                            break;
                        put1(static_cast<uint8_t>(0x40u |
                                                  ((dr + 2) << 4) |
                                                  ((dg + 2) << 2) |
                                                  (db + 2)));
                    }
                    else
                    {
                        const int dr_dg = dr - dg;
                        const int db_dg = db - dg;
                        if ((unsigned)(dg + 32) < 64u && (unsigned)(dr_dg + 8) < 16u && (unsigned)(db_dg + 8) < 16u)
                        {
                            if (!have(2))
                                break;
                            put2(static_cast<uint8_t>(0x80u | (dg + 32)),
                                 static_cast<uint8_t>(((dr_dg + 8) << 4) | (db_dg + 8)));
                        }
                        else
                        {
                            if (!have(4))
                                break;
                            put4(0xFEu,
                                 static_cast<uint8_t>(r),
                                 static_cast<uint8_t>(g),
                                 static_cast<uint8_t>(b));
                        }
                    }
                    _shotStream.qoiIndex[idx] = rgba;
                }

                _shotStream.qoiPrev = rgba;
                ++_shotStream.qoiPos;
            }
        };

        while (budget)
        {
            fillOut();
            if (_shotStream.qoiOutOff >= _shotStream.qoiOutLen)
                break;
            const size_t remaining = static_cast<size_t>(_shotStream.qoiOutLen - _shotStream.qoiOutOff);
            const size_t n = (budget < remaining) ? budget : remaining;
            if (n == 0)
                break;

            size_t wrote =
#if (PIPGUI_SCREENSHOT_MODE == 1)
                Serial.write(_shotStream.qoiOut + _shotStream.qoiOutOff, n);
#else
                _shotStream.file.write(_shotStream.qoiOut + _shotStream.qoiOutOff, n);
#endif
            if (wrote == 0)
            {
#if (PIPGUI_SCREENSHOT_MODE == 1)
                return;
#else
                finish(false);
                return;
#endif
            }
#if (PIPGUI_SCREENSHOT_MODE == 2)
            if (_shotStream.file)
                ssd::crc32Update(_shotStream.payloadCrc, _shotStream.qoiOut + _shotStream.qoiOutOff, wrote);
#endif
            _shotStream.qoiOutOff = static_cast<uint16_t>(_shotStream.qoiOutOff + wrote);
            _shotStream.qoiPayloadBytes += static_cast<uint32_t>(wrote);
            budget -= wrote;
            if (wrote < n)
                break;
        }

        if (_shotStream.qoiPos >= pixelsTotal && !_shotStream.qoiRun &&
            _shotStream.qoiTailOffset >= 8 && _shotStream.qoiOutOff >= _shotStream.qoiOutLen)
            finish(true);
        return;
#endif
    }

}

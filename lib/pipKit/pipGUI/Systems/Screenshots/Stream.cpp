#include <pipGUI/Systems/Screenshots/Internals.hpp>
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

        [[nodiscard]] bool readShotsCounter(uint32_t &out) noexcept
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

        void writeShotsCounter(uint32_t value) noexcept
        {
            char tmp[64];
            if (!ssd::tmpPathFromFinal(tmp, sizeof(tmp), kCounterPath))
                return;

            fs::File f = LittleFS.open(tmp, FILE_WRITE);
            if (!f)
                return;

            char buf[20];
            const int len = std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
            bool ok = (len > 0) &&
                      (f.write(reinterpret_cast<const uint8_t *>(buf), static_cast<size_t>(len)) == static_cast<size_t>(len));
            if (ok)
            {
                f.flush();
                ok = (f.size() == static_cast<size_t>(len));
            }
            f.close();

            if (ok)
            {
                if (!LittleFS.rename(tmp, kCounterPath))
                    LittleFS.remove(tmp);
            }
            else
            {
                LittleFS.remove(tmp);
            }
        }

        [[nodiscard]] uint32_t scanMaxShotStamp() noexcept
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
                if (name && ssd::hasSuffix(name, ".pscr") && ssd::parseStamp(name, stamp) && stamp > maxStamp)
                    maxStamp = stamp;
                file.close();
            }
            d.close();
            return maxStamp;
        }

        [[nodiscard]] uint32_t allocShotStamp() noexcept
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

        void commitShotStamp(uint32_t stamp) noexcept
        {
            writeShotsCounter(stamp);
        }
    }
#endif

    namespace
    {
        void releaseGalleryThumbPixels(pipcore::Platform *plat, detail::ScreenshotGalleryState &shots) noexcept
        {
            if (!plat || !shots.entries)
                return;

            for (uint8_t i = 0; i < shots.count; ++i)
            {
                if (!shots.entries[i].pixels)
                    continue;
                detail::free(plat, shots.entries[i].pixels);
                shots.entries[i].pixels = nullptr;
            }
        }

        void releaseGalleryScratch(pipcore::Platform *plat, detail::ScreenshotGalleryState &shots) noexcept
        {
#if (PIPGUI_SCREENSHOT_MODE == 2)
            if (shots.scanDir)
                shots.scanDir.close();
            shots.flashScanActive = false;
            shots.flashLoadIndex = 0;
            shots.flashThumbsDone = false;
            if (shots.rowBuf)
            {
                detail::free(plat, shots.rowBuf);
                shots.rowBuf = nullptr;
                shots.rowBufSize = 0;
            }
#else
            (void)plat;
            (void)shots;
#endif
        }

        [[nodiscard]] bool snapshotSpriteBuffer(pipcore::Platform *plat,
                                                const void *src,
                                                uint32_t bytes,
                                                detail::ScreenshotStreamState &stream) noexcept
        {
            if (!plat || !src || bytes == 0)
                return false;

            void *mem = detail::alloc(plat, bytes, pipcore::AllocCaps::Default);
            if (!mem)
                return false;

            std::memcpy(mem, src, bytes);
            stream.buffer = static_cast<uint8_t *>(mem);
            return true;
        }
    }

    void GUI::startScreenshotStream()
    {
#if !PIPGUI_SCREENSHOTS
        return;
#else
        if (_shotStream.active || !_flags.spriteEnabled || !_disp.display)
            return;

        const uint16_t w = _render.screenWidth;
        const uint16_t h = _render.screenHeight;
        if (w == 0 || h == 0)
            return;

        const void *src = _render.sprite.getBuffer();
        if (!src)
            return;

        pipcore::Platform *plat = platform();
        if (!plat)
            return;

        const uint32_t snapshotBytes = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * 2u;
        resetScreenshotStreamState(plat);

        const auto failStart = [&]()
        {
            resetScreenshotStreamState(plat);
        };

        bool haveSnapshot = snapshotSpriteBuffer(plat, src, snapshotBytes, _shotStream);
        if (!haveSnapshot)
        {
            freeBlurBuffers(plat);
            releaseGalleryScratch(plat, _shots);
            releaseGalleryThumbPixels(plat, _shots);
            haveSnapshot = snapshotSpriteBuffer(plat, src, snapshotBytes, _shotStream);
        }

        if (!haveSnapshot)
        {
            failStart();
            return;
        }

#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (!_shots.fsReady)
        {
            _shots.fsReady = LittleFS.begin(false);
            _shots.fsDirsReady = false;
            if (!_shots.fsReady)
            {
                failStart();
                return;
            }
        }
        ssd::ensureDirs(_shots.fsDirsReady);
        if (!_shots.fsDirsReady)
        {
            failStart();
            return;
        }

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
        {
            failStart();
            return;
        }

        _shotStream.file = f;
        _shotStream.stamp = stamp;
        std::strncpy(_shotStream.path, path, sizeof(_shotStream.path) - 1);
        _shotStream.path[sizeof(_shotStream.path) - 1] = '\0';
#endif

        _shotStream.width = w;
        _shotStream.height = h;
        _shotStream.payloadCrc = 0;
#if (PIPGUI_SCREENSHOT_MODE == 2)
        _shotStream.payloadCrc = ssd::crc32Init();
#endif
        _shotStream.headerOffset = 0;
        _shotStream.headerReady = true;
        _shotStream.active = true;
        _shotStream.notifyOnComplete = false;
        _shotStream.encPos = 0;
        _shotStream.encPayloadBytes = 0;
        _shotStream.encPrev565 = 0;
        std::memset(_shotStream.encIndex, 0, sizeof(_shotStream.encIndex));
        std::memset(_shotStream.encLiteral565, 0, sizeof(_shotStream.encLiteral565));
        _shotStream.encRun = 0;
        _shotStream.encLiteralLen = 0;
        _shotStream.encOutOff = 0;
        _shotStream.encOutLen = 0;

        _shotStream.header[0] = 'P';
        _shotStream.header[1] = 'S';
        _shotStream.header[2] = 'C';
        _shotStream.header[3] = 'R';
        _shotStream.header[4] = static_cast<uint8_t>(w & 0xFFu);
        _shotStream.header[5] = static_cast<uint8_t>((w >> 8) & 0xFFu);
        _shotStream.header[6] = static_cast<uint8_t>(h & 0xFFu);
        _shotStream.header[7] = static_cast<uint8_t>((h >> 8) & 0xFFu);
        _shotStream.header[8] = 0;
        _shotStream.header[9] = 0;
        _shotStream.header[10] = 0;
        _shotStream.header[11] = 0;
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
            return resetScreenshotStreamState(platform());

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
            uint32_t payloadBytes = 0;
            if (ok && _shotStream.file)
            {
                const uint32_t sz = _shotStream.encPayloadBytes;
                payloadBytes = sz;
                if (!_shotStream.file.seek(8))
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

            _shotStream.path[0] = '\0';
            resetScreenshotStreamState(platform());
#else
            resetScreenshotStreamState(platform());
#endif
            if (notify)
            {
                showToastInternal(ok ? "Screenshot saved" : "Screenshot failed", true,
                                  static_cast<IconId>(psdf_icons::IconCount));
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
        const uint16_t *const src16 = reinterpret_cast<const uint16_t *>(_shotStream.buffer);
        const auto fillOut = [&]()
        {
            if (_shotStream.encOutOff < _shotStream.encOutLen)
                return;
            _shotStream.encOutOff = 0;
            _shotStream.encOutLen = 0;

            constexpr size_t cap = sizeof(_shotStream.encOut);
            auto have = [&](size_t need)
            { return _shotStream.encOutLen + need <= cap; };
            auto put1 = [&](uint8_t b)
            { _shotStream.encOut[_shotStream.encOutLen++] = b; };
            auto put2 = [&](uint8_t a, uint8_t b)
            {
                _shotStream.encOut[_shotStream.encOutLen++] = a;
                _shotStream.encOut[_shotStream.encOutLen++] = b;
            };
            auto emitLiteralBlock = [&]() -> bool
            {
                if (!_shotStream.encLiteralLen)
                    return true;
                const size_t need = 1u + static_cast<size_t>(_shotStream.encLiteralLen) * 2u;
                if (!have(need))
                    return false;
                put1(static_cast<uint8_t>(ssd::kPacked565LiteralBase | (_shotStream.encLiteralLen - 1u)));
                for (uint8_t i = 0; i < _shotStream.encLiteralLen; ++i)
                {
                    const uint16_t px = _shotStream.encLiteral565[i];
                    put2(static_cast<uint8_t>((px >> 8) & 0xFFu),
                         static_cast<uint8_t>(px & 0xFFu));
                }
                _shotStream.encLiteralLen = 0;
                return true;
            };
            auto emitRun = [&]() -> bool
            {
                if (!_shotStream.encRun)
                    return true;
                if (!have(1))
                    return false;
                put1(static_cast<uint8_t>(ssd::kPacked565RunBase | (_shotStream.encRun - 1u)));
                _shotStream.encRun = 0;
                return true;
            };

            while (_shotStream.encOutLen < cap)
            {
                if (_shotStream.encPos >= pixelsTotal)
                {
                    if (!emitRun() || !emitLiteralBlock())
                        break;
                    return;
                }

                uint16_t px565 = __builtin_bswap16(src16[_shotStream.encPos]);

                if (px565 == _shotStream.encPrev565)
                {
                    if (!emitLiteralBlock())
                        break;
                    ++_shotStream.encRun;
                    ++_shotStream.encPos;
                    if (_shotStream.encRun == 64u || _shotStream.encPos == pixelsTotal)
                    {
                        if (!emitRun())
                            break;
                    }
                    _shotStream.encIndex[ssd::hash565(px565)] = px565;
                    _shotStream.encPrev565 = px565;
                    continue;
                }

                if (!emitRun())
                    break;

                const uint8_t idx = ssd::hash565(px565);
                if (_shotStream.encIndex[idx] == px565)
                {
                    if (!emitLiteralBlock() || !have(1))
                        break;
                    put1(static_cast<uint8_t>(idx & ssd::kPacked565IndexMask));
                }
                else
                {
                    const int pr = static_cast<int>((_shotStream.encPrev565 >> 11) & 31u);
                    const int pg = static_cast<int>((_shotStream.encPrev565 >> 5) & 63u);
                    const int pb = static_cast<int>(_shotStream.encPrev565 & 31u);
                    const int r = static_cast<int>((px565 >> 11) & 31u);
                    const int g = static_cast<int>((px565 >> 5) & 63u);
                    const int b = static_cast<int>(px565 & 31u);
                    const int dr = r - pr;
                    const int dg = g - pg;
                    const int db = b - pb;

                    if ((unsigned)(dr + 2) < 4u && (unsigned)(dg + 2) < 4u && (unsigned)(db + 2) < 4u)
                    {
                        if (!emitLiteralBlock() || !have(1))
                            break;
                        put1(static_cast<uint8_t>(ssd::kPacked565DiffBase |
                                                  ((dr + 2) << 4) |
                                                  ((dg + 2) << 2) |
                                                  (db + 2)));
                    }
                    else
                    {
                        const int dr_dg = dr - dg;
                        const int db_dg = db - dg;
                        if ((unsigned)(dg + 16) < 32u &&
                            (unsigned)(dr_dg + 8) < 16u &&
                            (unsigned)(db_dg + 8) < 16u)
                        {
                            if (!emitLiteralBlock() || !have(2))
                                break;
                            put2(static_cast<uint8_t>(ssd::kPacked565LumaBase | (dg + 16)),
                                 static_cast<uint8_t>(((dr_dg + 8) << 4) | (db_dg + 8)));
                        }
                        else
                        {
                            _shotStream.encLiteral565[_shotStream.encLiteralLen++] = px565;
                            if (_shotStream.encLiteralLen == ssd::kPacked565LiteralMax)
                            {
                                if (!emitLiteralBlock())
                                    break;
                            }
                        }
                    }
                }

                _shotStream.encIndex[idx] = px565;
                _shotStream.encPrev565 = px565;
                ++_shotStream.encPos;
            }
        };

        while (budget)
        {
            fillOut();
            if (_shotStream.encOutOff >= _shotStream.encOutLen)
                break;
            const size_t remaining = static_cast<size_t>(_shotStream.encOutLen - _shotStream.encOutOff);
            const size_t n = (budget < remaining) ? budget : remaining;
            if (n == 0)
                break;

            size_t wrote =
#if (PIPGUI_SCREENSHOT_MODE == 1)
                Serial.write(_shotStream.encOut + _shotStream.encOutOff, n);
#else
                _shotStream.file.write(_shotStream.encOut + _shotStream.encOutOff, n);
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
                ssd::crc32Update(_shotStream.payloadCrc, _shotStream.encOut + _shotStream.encOutOff, wrote);
#endif
            _shotStream.encOutOff = static_cast<uint16_t>(_shotStream.encOutOff + wrote);
            _shotStream.encPayloadBytes += static_cast<uint32_t>(wrote);
            budget -= wrote;
            if (wrote < n)
                break;
        }

        if (_shotStream.encPos >= pixelsTotal && _shotStream.encRun == 0 &&
            _shotStream.encLiteralLen == 0 && _shotStream.encOutOff >= _shotStream.encOutLen)
            finish(true);
#endif
    }
}

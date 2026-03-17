#include <pipGUI/Systems/Screenshots/Internals.hpp>
#include <pipGUI/Systems/Screenshots/Codec.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>
#include <pipGUI/Core/API/Internal/RuntimeState.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pipgui
{
    namespace ssd = screenshots::detail;

#if (PIPGUI_SCREENSHOT_MODE == 2)
    [[nodiscard]] static bool writeThumbFile(fs::File &tf, const uint16_t *src565, uint16_t w, uint16_t h) noexcept
    {
        if (!tf || !src565 || w == 0 || h == 0)
            return false;

        uint8_t hbuf[ssd::kHdrSize];
        hbuf[0] = 'P';
        hbuf[1] = 'S';
        hbuf[2] = 'C';
        hbuf[3] = 'R';
        hbuf[4] = static_cast<uint8_t>(w & 0xFFu);
        hbuf[5] = static_cast<uint8_t>((w >> 8) & 0xFFu);
        hbuf[6] = static_cast<uint8_t>(h & 0xFFu);
        hbuf[7] = static_cast<uint8_t>((h >> 8) & 0xFFu);
        hbuf[8] = static_cast<uint8_t>(ScreenshotFormat::QoiRgb);
        hbuf[9] = 0;
        hbuf[10] = 0;
        hbuf[11] = 0;
        hbuf[12] = 0;

        if (tf.write(hbuf, sizeof(hbuf)) != sizeof(hbuf))
            return false;

        uint32_t qoiSize = 0;
        uint32_t qoiCrc = 0;
        if (!detail::qoiEncode565ToFile(tf, src565, w, h, qoiSize, &qoiCrc))
            return false;

        if (!tf.seek(9))
            return false;
        uint8_t sz[4];
        sz[0] = static_cast<uint8_t>(qoiSize & 0xFFu);
        sz[1] = static_cast<uint8_t>((qoiSize >> 8) & 0xFFu);
        sz[2] = static_cast<uint8_t>((qoiSize >> 16) & 0xFFu);
        sz[3] = static_cast<uint8_t>((qoiSize >> 24) & 0xFFu);
        if (tf.write(sz, sizeof(sz)) != sizeof(sz))
            return false;

        const size_t endPos = static_cast<size_t>(ssd::kHdrSize) + static_cast<size_t>(qoiSize);
        if (!tf.seek(endPos))
            return false;
        if (!ssd::writeTrailer(tf, qoiSize, qoiCrc, 0))
            return false;
        tf.flush();
        return tf.size() == (endPos + ssd::kTrlSize);
    }
#endif

    void GUI::serviceScreenshotGalleryFlash()
    {
#if (PIPGUI_SCREENSHOT_MODE != 2)
        return;
  #else
        const uint32_t now = nowMs();

        if (_shotStream.active)
            return;

        if (_shots.flashScanDone && _shots.flashThumbsDone)
            return;

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

        if (!ensureScreenshotGallery(platform()))
            return;

        auto storePath = [](char *dst, size_t dstSize, const char *src)
        {
            if (!dst || dstSize == 0)
                return;
            dst[0] = '\0';
            if (!src || !src[0])
                return;
            if (src[0] == '/')
            {
                std::strncpy(dst, src, dstSize - 1);
                dst[dstSize - 1] = '\0';
                return;
            }
            if (std::strchr(src, '/'))
            {
                dst[0] = '/';
                if (dstSize == 1)
                    return;
                std::strncpy(dst + 1, src, dstSize - 2);
                dst[dstSize - 1] = '\0';
                return;
            }
            std::snprintf(dst, dstSize, "%s/%s", ssd::kShotsDir, src);
        };

        auto insertMeta = [&](const char *path, uint32_t stamp)
        {
            const uint8_t cap = _shots.maxShots;
            uint8_t n = _shots.count;
            if (cap == 0)
                return;

            if (n < cap)
            {
                uint8_t pos = n;
                while (pos > 0 && _shots.entries[pos - 1].stamp < stamp)
                {
                    _shots.entries[pos] = _shots.entries[pos - 1];
                    --pos;
                }
                _shots.entries[pos] = {};
                _shots.entries[pos].stamp = stamp;
                storePath(_shots.entries[pos].path, sizeof(_shots.entries[pos].path), path);
                _shots.count = static_cast<uint8_t>(n + 1);
                return;
            }

            if (stamp <= _shots.entries[n - 1].stamp)
                return;

            _shots.entries[n - 1] = {};
            _shots.entries[n - 1].stamp = stamp;
            storePath(_shots.entries[n - 1].path, sizeof(_shots.entries[n - 1].path), path);
            for (int i = static_cast<int>(n) - 1; i > 0; --i)
            {
                if (_shots.entries[i - 1].stamp >= _shots.entries[i].stamp)
                    break;
                std::swap(_shots.entries[i - 1], _shots.entries[i]);
            }
        };

        if (!_shots.flashScanDone)
        {
            if (!_shots.flashScanActive)
            {
                _flags.needRedraw = 1;
                for (uint8_t i = 0; i < _shots.maxShots; ++i)
                {
                    if (_shots.entries[i].pixels)
                        detail::free(platform(), _shots.entries[i].pixels);
                    _shots.entries[i] = {};
                }
                _shots.count = 0;
                _shots.flashLoadIndex = 0;
                _shots.flashThumbsDone = false;
                _shots.thumbIndexReady = false;
                _shots.thumbIndexW = 0;
                _shots.thumbIndexH = 0;
                _shots.scanDir = LittleFS.open(ssd::kShotsDir);
                if (!_shots.scanDir)
                {
                    _shots.flashScanDone = true;
                    _shots.flashThumbsDone = true;
                    return;
                }
                _shots.flashScanActive = true;
            }

            const uint32_t t0 = micros();
            while ((micros() - t0) < 1500)
            {
                fs::File file = _shots.scanDir.openNextFile();
                if (!file)
                {
                    _shots.scanDir.close();
                    _shots.flashScanActive = false;
                    _shots.flashScanDone = true;
                    _shots.flashLoadIndex = 0;
                    _shots.flashThumbsDone = (_shots.count == 0);
                    _flags.needRedraw = 1;
                    break;
                }

                if (file.isDirectory())
                {
                    file.close();
                    continue;
                }

                const char *name = file.name();
                const char *base = ssd::baseName(name);
                if (ssd::hasSuffix(base, ".tmp") && (std::strncmp(base, "pscr_", 5) == 0 || std::strcmp(base, ".counter.tmp") == 0))
                {
                    char tmpFull[64];
                    storePath(tmpFull, sizeof(tmpFull), base);
                    file.close();
                    if (tmpFull[0] != '\0')
                        LittleFS.remove(tmpFull);
                    continue;
                }
                if (!ssd::hasSuffix(base, ".pscr"))
                {
                    file.close();
                    continue;
                }

                uint32_t stamp = 0;
                if (!ssd::parseStamp(base, stamp))
                {
                    file.close();
                    continue;
                }

                const uint8_t before = _shots.count;
                insertMeta(base, stamp);
                if (_shots.count != before)
                    _flags.needRedraw = 1;
                file.close();
            }
            _flags.needRedraw = 1;
            return;
        }

        if (_shots.flashThumbsDone || _shots.count == 0)
            return;

        while (_shots.flashLoadIndex < _shots.count && _shots.entries[_shots.flashLoadIndex].pixels)
            ++_shots.flashLoadIndex;

        if (_shots.flashLoadIndex >= _shots.count)
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
            if (!allLoaded)
                _shots.flashLoadIndex = 0;
            return;
        }

        ScreenshotEntry &entry = _shots.entries[_shots.flashLoadIndex];
        if (entry.path[0] == '\0')
        {
            // Shouldn't happen, but avoid "blank tiles" if metadata gets out of sync.
            for (uint8_t j = _shots.flashLoadIndex; (j + 1u) < _shots.count; ++j)
                _shots.entries[j] = _shots.entries[j + 1u];
            _shots.entries[_shots.count - 1u] = {};
            --_shots.count;
            _shots.flashThumbsDone = (_shots.count == 0);
            _flags.needRedraw = 1;
            return;
        }

        const auto dropEntry = [&]()
        {
            if (_shots.entries[_shots.flashLoadIndex].pixels)
            {
                detail::free(platform(), _shots.entries[_shots.flashLoadIndex].pixels);
                _shots.entries[_shots.flashLoadIndex].pixels = nullptr;
            }
            for (uint8_t j = _shots.flashLoadIndex; (j + 1u) < _shots.count; ++j)
                _shots.entries[j] = _shots.entries[j + 1u];
            _shots.entries[_shots.count - 1u] = {};
            --_shots.count;
            _shots.flashThumbsDone = (_shots.count == 0);
            _flags.needRedraw = 1;
        };
        const auto dropEntryAndDelete = [&]()
        {
            const char *p = _shots.entries[_shots.flashLoadIndex].path;
            if (p && p[0])
                LittleFS.remove(p);
            dropEntry();
        };

        const uint16_t tw = _shots.thumbW;
        const uint16_t th = _shots.thumbH;
        if (tw == 0 || th == 0)
        {
            ++_shots.flashLoadIndex;
            return;
        }

        const auto ensureThumbIndex = [&]()
        {
            if (_shots.thumbIndexReady && _shots.thumbIndexW == tw && _shots.thumbIndexH == th)
                return;
            _shots.thumbIndexReady = true;
            _shots.thumbIndexW = tw;
            _shots.thumbIndexH = th;

            for (uint8_t i = 0; i < _shots.count; ++i)
                _shots.entries[i].thumbOnFlash = false;

            char tdir[64];
            ssd::thumbDir(tdir, sizeof(tdir), tw, th);
            LittleFS.mkdir(tdir);
            fs::File d = LittleFS.open(tdir);
            if (!d || !d.isDirectory())
            {
                if (d)
                    d.close();
                return;
            }

            uint8_t remaining = _shots.count;
            while (remaining)
            {
                fs::File f = d.openNextFile();
                if (!f)
                    break;
                if (f.isDirectory())
                {
                    f.close();
                    continue;
                }
                const char *name = f.name();
                const char *base = ssd::baseName(name);
                uint32_t stamp = 0;
                if (ssd::hasSuffix(base, ".pscr") && ssd::parseStamp(base, stamp))
                {
                    for (uint8_t i = 0; i < _shots.count; ++i)
                    {
                        if (_shots.entries[i].stamp == stamp && _shots.entries[i].path[0] != '\0')
                        {
                            if (!_shots.entries[i].thumbOnFlash)
                            {
                                _shots.entries[i].thumbOnFlash = true;
                                --remaining;
                            }
                            break;
                        }
                    }
                }
                f.close();
            }
            d.close();
        };
        ensureThumbIndex();

        char thumbPath[96];
        ssd::thumbPath(thumbPath, sizeof(thumbPath), entry.path, tw, th);
        if (thumbPath[0] != '\0')
        {
            if (entry.thumbOnFlash)
            {
                fs::File tf = LittleFS.open(thumbPath, FILE_READ);
                if (tf)
                {
                    bool removeThumb = false;
                    uint8_t thead[13] = {};
                    if (tf.read(thead, sizeof(thead)) == sizeof(thead) &&
                        thead[0] == 'P' && thead[1] == 'S' && thead[2] == 'C' && thead[3] == 'R')
                    {
                        const uint16_t tw2 = static_cast<uint16_t>(thead[4] | (thead[5] << 8));
                        const uint16_t th2 = static_cast<uint16_t>(thead[6] | (thead[7] << 8));
                        const uint8_t tfmt = thead[8];
                        const uint32_t tsize =
                            static_cast<uint32_t>(thead[9]) |
                            (static_cast<uint32_t>(thead[10]) << 8) |
                            (static_cast<uint32_t>(thead[11]) << 16) |
                            (static_cast<uint32_t>(thead[12]) << 24);
                        if (tw2 == tw && th2 == th)
                        {
                            uint32_t tcrcExpected = 0;
                            uint8_t tflags = 0;
                            (void)tflags;
                            if (!ssd::readTrailer(tf, static_cast<uint32_t>(ssd::kHdrSize), tsize,
                                                      tcrcExpected, tflags) ||
                                !tf.seek(ssd::kHdrSize))
                            {
                                removeThumb = true;
                            }

                            if (!removeThumb)
                            {
                                const size_t pixels = static_cast<size_t>(tw) * static_cast<size_t>(th);
                                uint16_t *dst = static_cast<uint16_t *>(detail::alloc(platform(), pixels * sizeof(uint16_t), pipcore::AllocCaps::PreferExternal));
                                if (dst)
                                {
                                    bool ok = false;
                                    if (tfmt == static_cast<uint8_t>(ScreenshotFormat::QoiRgb))
                                    {
                                        uint32_t got = 0;
                                        ok = detail::qoiDecodeFileTo565(tf, tw, th, dst, tsize, &got) && (got == tcrcExpected);
                                    }

                                    if (ok)
                                    {
                                        tf.close();
                                        entry.pixels = dst;
                                        entry.timestampMs = now;
                                        entry.thumbOnFlash = true;
                                        _flags.needRedraw = 1;
                                        ++_shots.flashLoadIndex;
                                        return;
                                    }

                                    removeThumb = true;
                                    detail::free(platform(), dst);
                                }
                            }
                        }
                    }
                    tf.close();
                    if (removeThumb)
                    {
                        entry.thumbOnFlash = false;
                        LittleFS.remove(thumbPath);
                    }
                }
                else
                {
                    entry.thumbOnFlash = false;
                }
            }
        }

        fs::File file = LittleFS.open(entry.path, FILE_READ);
        if (!file)
        {
            dropEntry();
            return;
        }

        uint8_t header[13] = {};
        if (file.read(header, sizeof(header)) != sizeof(header) ||
            header[0] != 'P' || header[1] != 'S' || header[2] != 'C' || header[3] != 'R')
        {
            file.close();
            dropEntryAndDelete();
            return;
        }

        const uint16_t w = static_cast<uint16_t>(header[4] | (header[5] << 8));
        const uint16_t h = static_cast<uint16_t>(header[6] | (header[7] << 8));
        const uint8_t fmt = header[8];
        const uint32_t payloadSize =
            static_cast<uint32_t>(header[9]) |
            (static_cast<uint32_t>(header[10]) << 8) |
            (static_cast<uint32_t>(header[11]) << 16) |
            (static_cast<uint32_t>(header[12]) << 24);

        if (w == 0 || h == 0)
        {
            file.close();
            dropEntryAndDelete();
            return;
        }

        uint32_t fcrcExpected = 0;
        uint8_t fflags = 0;
        (void)fflags;
        if (!ssd::readTrailer(file, static_cast<uint32_t>(ssd::kHdrSize), payloadSize,
                                  fcrcExpected, fflags) ||
            !file.seek(ssd::kHdrSize))
        {
            file.close();
            dropEntryAndDelete();
            return;
        }

        if (fmt == static_cast<uint8_t>(ScreenshotFormat::QoiRgb))
        {
            const size_t dstPixels = static_cast<size_t>(tw) * static_cast<size_t>(th);
            uint16_t *dst = static_cast<uint16_t *>(detail::alloc(platform(), dstPixels * sizeof(uint16_t), pipcore::AllocCaps::PreferExternal));
            if (!dst)
            {
                file.close();
                ++_shots.flashLoadIndex;
                return;
            }

            const uint32_t rowBytes = static_cast<uint32_t>(w) * 2u;
            const uint32_t scratchBytes = 8u + 6u * static_cast<uint32_t>(tw) * static_cast<uint32_t>(sizeof(uint64_t));
            const uint32_t needed = rowBytes + scratchBytes;
            if (!_shots.rowBuf || _shots.rowBufSize < needed)
            {
                if (_shots.rowBuf)
                    detail::free(platform(), _shots.rowBuf);
                _shots.rowBuf = static_cast<uint8_t *>(detail::alloc(platform(), needed, pipcore::AllocCaps::Default));
                _shots.rowBufSize = _shots.rowBuf ? needed : 0;
            }

            if (!_shots.rowBuf)
            {
                detail::free(platform(), dst);
                file.close();
                ++_shots.flashLoadIndex;
                return;
            }

            uint8_t *scratch0 = _shots.rowBuf + rowBytes;
            scratch0 = reinterpret_cast<uint8_t *>((reinterpret_cast<uintptr_t>(scratch0) + 7u) & ~static_cast<uintptr_t>(7u));
            uint64_t *accR0 = reinterpret_cast<uint64_t *>(scratch0);
            uint64_t *accG0 = accR0 + tw;
            uint64_t *accB0 = accG0 + tw;
            uint64_t *accR1 = accB0 + tw;
            uint64_t *accG1 = accR1 + tw;
            uint64_t *accB1 = accG1 + tw;

            std::memset(accR0, 0, (size_t)tw * sizeof(uint64_t));
            std::memset(accG0, 0, (size_t)tw * sizeof(uint64_t));
            std::memset(accB0, 0, (size_t)tw * sizeof(uint64_t));
            std::memset(accR1, 0, (size_t)tw * sizeof(uint64_t));
            std::memset(accG1, 0, (size_t)tw * sizeof(uint64_t));
            std::memset(accB1, 0, (size_t)tw * sizeof(uint64_t));

            bool ok = true;
            ssd::QoiReader qr;
            qr.f = &file;
            if (payloadSize == 0)
                ok = false;
            qr.limited = true;
            qr.remaining = payloadSize;
            uint32_t qcrc = ssd::crc32Init();
            qr.crc = &qcrc;

            const uint32_t srcW16 = static_cast<uint32_t>(w) << 16;
            const uint32_t srcH16 = static_cast<uint32_t>(h) << 16;
            const uint32_t scaleX16 = srcW16 / tw;
            const uint32_t scaleY16 = srcH16 / th;

            uint16_t dy = 0;
            uint32_t y0 = 0;
            uint32_t y1 = (th == 1) ? srcH16 : scaleY16;

            uint16_t *row565 = reinterpret_cast<uint16_t *>(_shots.rowBuf);

            const auto processRow = [&](uint16_t sy) -> bool
            {
                const uint32_t rowStart = static_cast<uint32_t>(sy) << 16;
                while (dy < th && rowStart >= y1)
                {
                    const uint32_t yLen = y1 - y0;
                    uint16_t *dstRow = dst + static_cast<uint32_t>(dy) * tw;
                    for (uint16_t dx = 0; dx < tw; ++dx)
                    {
                        const uint32_t x0 = static_cast<uint32_t>(dx) * scaleX16;
                        const uint32_t x1 = (dx + 1u == tw) ? srcW16 : (x0 + scaleX16);
                        const uint64_t total = static_cast<uint64_t>(x1 - x0) * static_cast<uint64_t>(yLen);
                        if (!total)
                        {
                            dstRow[dx] = 0;
                            continue;
                        }
                        const uint64_t half = total / 2u;
                        const uint64_t r = (accR0[dx] + half) / total;
                        const uint64_t g = (accG0[dx] + half) / total;
                        const uint64_t b = (accB0[dx] + half) / total;
                        dstRow[dx] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
                    }

                    std::swap(accR0, accR1);
                    std::swap(accG0, accG1);
                    std::swap(accB0, accB1);
                    std::memset(accR1, 0, (size_t)tw * sizeof(uint64_t));
                    std::memset(accG1, 0, (size_t)tw * sizeof(uint64_t));
                    std::memset(accB1, 0, (size_t)tw * sizeof(uint64_t));

                    ++dy;
                    y0 = y1;
                    y1 = (dy + 1u >= th) ? srcH16 : (y0 + scaleY16);
                }

                if (dy >= th)
                    return true;

                const uint32_t wy0 = ssd::overlap16(y0, y1, rowStart);
                const uint32_t y2 = (dy + 2u >= th) ? srcH16 : (y1 + scaleY16);
                const uint32_t wy1 = (dy + 1u < th) ? ssd::overlap16(y1, y2, rowStart) : 0u;

                for (uint16_t dx = 0; dx < tw; ++dx)
                {
                    const uint32_t x0 = static_cast<uint32_t>(dx) * scaleX16;
                    const uint32_t x1 = (dx + 1u == tw) ? srcW16 : (x0 + scaleX16);
                    const uint16_t sx0 = static_cast<uint16_t>(x0 >> 16);
                    const uint16_t sx1 = static_cast<uint16_t>((x1 - 1u) >> 16);

                    uint64_t sr = 0, sg = 0, sb = 0;
                    for (uint16_t sx = sx0; sx <= sx1; ++sx)
                    {
                        const uint32_t wx = ssd::overlap16(x0, x1, static_cast<uint32_t>(sx) << 16);
                        if (!wx)
                            continue;
                        const uint16_t px = row565[sx];
                        sr += static_cast<uint64_t>((px >> 11) & 31u) * wx;
                        sg += static_cast<uint64_t>((px >> 5) & 63u) * wx;
                        sb += static_cast<uint64_t>(px & 31u) * wx;
                    }

                    if (wy0)
                    {
                        accR0[dx] += sr * wy0;
                        accG0[dx] += sg * wy0;
                        accB0[dx] += sb * wy0;
                    }
                    if (wy1)
                    {
                        accR1[dx] += sr * wy1;
                        accG1[dx] += sg * wy1;
                        accB1[dx] += sb * wy1;
                    }
                }
                return true;
            };

            const auto sink = [&](uint16_t sx, uint16_t sy, uint32_t rgba) -> bool
            {
                row565[sx] = ssd::rgb565FromRgba(rgba);
                if (static_cast<uint16_t>(sx + 1u) == w)
                    return processRow(sy);
                return true;
            };

            ok = ok && ssd::qoiDecodeRaster(qr, w, h, sink, true);
            if (ok)
                ok = (ssd::crc32Finish(qcrc) == fcrcExpected);

            while (ok && dy < th)
            {
                const uint32_t yLen = y1 - y0;
                uint16_t *dstRow = dst + static_cast<uint32_t>(dy) * tw;
                for (uint16_t dx = 0; dx < tw; ++dx)
                {
                    const uint32_t x0 = static_cast<uint32_t>(dx) * scaleX16;
                    const uint32_t x1 = (dx + 1u == tw) ? srcW16 : (x0 + scaleX16);
                    const uint64_t total = static_cast<uint64_t>(x1 - x0) * static_cast<uint64_t>(yLen);
                    if (!total)
                    {
                        dstRow[dx] = 0;
                        continue;
                    }
                    const uint64_t half = total / 2u;
                    const uint64_t r = (accR0[dx] + half) / total;
                    const uint64_t g = (accG0[dx] + half) / total;
                    const uint64_t b = (accB0[dx] + half) / total;
                    dstRow[dx] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
                }

                std::swap(accR0, accR1);
                std::swap(accG0, accG1);
                std::swap(accB0, accB1);
                std::memset(accR1, 0, (size_t)tw * sizeof(uint64_t));
                std::memset(accG1, 0, (size_t)tw * sizeof(uint64_t));
                std::memset(accB1, 0, (size_t)tw * sizeof(uint64_t));

                ++dy;
                y0 = y1;
                y1 = (dy + 1u >= th) ? srcH16 : (y0 + scaleY16);
            }

            file.close();

            if (!ok)
            {
                detail::free(platform(), dst);
                dropEntryAndDelete();
                return;
            }

            // Persist thumbnail for this (thumbW x thumbH) so next entries are instant.
            if (_shots.fsDirsReady && entry.path[0] != '\0' && thumbPath[0] != '\0')
            {
                char tdir[64];
                ssd::thumbDir(tdir, sizeof(tdir), tw, th);
                LittleFS.mkdir(tdir);

                char thumbTmp[96];
                const bool haveTmp = ssd::tmpPathFromFinal(thumbTmp, sizeof(thumbTmp), thumbPath);

                fs::File tf = LittleFS.open(haveTmp ? thumbTmp : thumbPath, FILE_WRITE);
                if (tf)
                {
                    bool wroteOk = writeThumbFile(tf, dst, tw, th);
                    tf.close();

                    if (haveTmp)
                    {
                        if (wroteOk)
                        {
                            wroteOk = LittleFS.rename(thumbTmp, thumbPath);
                            if (!wroteOk)
                                LittleFS.remove(thumbTmp);
                        }
                        else
                        {
                            LittleFS.remove(thumbTmp);
                        }
                    }

                    if (wroteOk)
                        entry.thumbOnFlash = true;
                }
            }

            entry.pixels = dst;
            entry.timestampMs = now;
            _flags.needRedraw = 1;
            ++_shots.flashLoadIndex;
            return;
        }

        file.close();
        dropEntryAndDelete();
        return;
#endif
    }

}

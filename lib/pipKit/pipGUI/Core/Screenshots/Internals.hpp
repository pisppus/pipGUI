#pragma once

#include <pipGUI/Core/API/pipGUI.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace pipgui::screenshots::detail
{
    inline constexpr size_t kHdrSize = 13;

    [[nodiscard]] inline uint8_t qoiHash(uint32_t rgba) noexcept
    {
        const uint32_t r = (rgba >> 24) & 0xFFu;
        const uint32_t g = (rgba >> 16) & 0xFFu;
        const uint32_t b = (rgba >> 8) & 0xFFu;
        const uint32_t a = rgba & 0xFFu;
        return static_cast<uint8_t>((r * 3u + g * 5u + b * 7u + a * 11u) & 63u);
    }

    [[nodiscard]] inline uint32_t rgbaFrom565(uint16_t px565) noexcept
    {
        const uint32_t r5 = (px565 >> 11) & 31u;
        const uint32_t g6 = (px565 >> 5) & 63u;
        const uint32_t b5 = px565 & 31u;

        const uint32_t r = (r5 << 3) | (r5 >> 2);
        const uint32_t g = (g6 << 2) | (g6 >> 4);
        const uint32_t b = (b5 << 3) | (b5 >> 2);

        return (r << 24) | (g << 16) | (b << 8) | 255u;
    }

    [[nodiscard]] inline uint16_t rgb565FromRgba(uint32_t rgba) noexcept
    {
        const uint32_t r = (rgba >> 24) & 0xFFu;
        const uint32_t g = (rgba >> 16) & 0xFFu;
        const uint32_t b = (rgba >> 8) & 0xFFu;
        return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }

    [[nodiscard]] inline uint32_t overlap16(uint32_t a0, uint32_t a1, uint32_t b0) noexcept
    {
        const uint32_t b1 = b0 + 65536u;
        const uint32_t lo = (a0 > b0) ? a0 : b0;
        const uint32_t hi = (a1 < b1) ? a1 : b1;
        return (hi > lo) ? (hi - lo) : 0u;
    }

#if (PIPGUI_SCREENSHOT_MODE == 2)
    inline constexpr uint8_t kTrlMagic[4] = {'P', 'S', 'C', 'T'};
    inline constexpr uint8_t kTrlVer = 1;
    inline constexpr size_t kTrlSize = 16;

    [[nodiscard]] inline uint32_t readU32LE(const uint8_t *p) noexcept
    {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }

    inline void writeU32LE(uint8_t *p, uint32_t v) noexcept
    {
        p[0] = static_cast<uint8_t>(v & 0xFFu);
        p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
        p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
        p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    }

    constexpr uint32_t crc32TableEntry(uint32_t r) noexcept
    {
        for (uint8_t k = 0; k < 8; ++k)
            r = (r & 1u) ? (0xEDB88320u ^ (r >> 1)) : (r >> 1);
        return r;
    }

    constexpr std::array<uint32_t, 256> makeCrc32Table() noexcept
    {
        std::array<uint32_t, 256> t = {};
        for (uint32_t i = 0; i < 256u; ++i)
            t[i] = crc32TableEntry(i);
        return t;
    }

    inline constexpr auto kCrc32Table = makeCrc32Table();

    [[nodiscard]] inline uint32_t crc32Init() noexcept { return 0xFFFFFFFFu; }
    [[nodiscard]] inline uint32_t crc32Finish(uint32_t crc) noexcept { return crc ^ 0xFFFFFFFFu; }

    inline void crc32UpdateByte(uint32_t &crc, uint8_t b) noexcept
    {
        crc = (crc >> 8) ^ kCrc32Table[(crc ^ static_cast<uint32_t>(b)) & 0xFFu];
    }

    inline void crc32Update(uint32_t &crc, const uint8_t *data, size_t len) noexcept
    {
        if (!data || len == 0)
            return;
        for (size_t i = 0; i < len; ++i)
            crc32UpdateByte(crc, data[i]);
    }

    [[nodiscard]] inline bool writeTrailer(fs::File &f, uint32_t payloadSize, uint32_t payloadCrc32, uint8_t flags = 0) noexcept
    {
        uint8_t t[kTrlSize] = {};
        t[0] = kTrlMagic[0];
        t[1] = kTrlMagic[1];
        t[2] = kTrlMagic[2];
        t[3] = kTrlMagic[3];
        t[4] = kTrlVer;
        t[5] = flags;
        t[6] = 0;
        t[7] = 0;
        writeU32LE(t + 8, payloadSize);
        writeU32LE(t + 12, payloadCrc32);
        return f.write(t, sizeof(t)) == sizeof(t);
    }

    [[nodiscard]] inline bool readTrailer(fs::File &f,
                                              uint32_t headerSize,
                                              uint32_t payloadSize,
                                              uint32_t &outCrc32,
                                              uint8_t &outFlags) noexcept
    {
        outCrc32 = 0;
        outFlags = 0;

        const uint32_t expectedBase = headerSize + payloadSize;
        const size_t fileSize = f.size();
        if (fileSize != static_cast<size_t>(expectedBase + kTrlSize))
            return false; // No trailer or size mismatch.

        if (!f.seek(expectedBase))
            return false;
        uint8_t t[kTrlSize] = {};
        if (f.read(t, sizeof(t)) != sizeof(t))
            return false;

        if (t[0] != kTrlMagic[0] || t[1] != kTrlMagic[1] ||
            t[2] != kTrlMagic[2] || t[3] != kTrlMagic[3])
            return false;
        if (t[4] != kTrlVer)
            return false;

        const uint32_t size2 = readU32LE(t + 8);
        if (size2 != payloadSize)
            return false;

        outFlags = t[5];
        outCrc32 = readU32LE(t + 12);
        return true;
    }

    inline constexpr const char *kRootDir = "/pipKit";
    inline constexpr const char *kShotsDir = "/pipKit/screenshots";
    inline constexpr const char *kThumbsDir = "/pipKit/thumbnails";

    [[nodiscard]] inline bool hasSuffix(const char *s, const char *suffix) noexcept
    {
        if (!s || !suffix)
            return false;
        const size_t sl = std::strlen(s);
        const size_t tl = std::strlen(suffix);
        return (sl >= tl) && (std::memcmp(s + sl - tl, suffix, tl) == 0);
    }

    [[nodiscard]] inline const char *baseName(const char *path) noexcept
    {
        if (!path)
            return "";
        const char *p = std::strrchr(path, '/');
        return (p && p[1]) ? (p + 1) : path;
    }

    [[nodiscard]] inline bool parseStamp(const char *path, uint32_t &out) noexcept
    {
        if (!path)
            return false;
        const char *p = std::strstr(path, "pscr_");
        if (!p)
            return false;
        p += 5;
        uint32_t v = 0;
        bool any = false;
        while (*p >= '0' && *p <= '9')
        {
            any = true;
            const uint32_t d = static_cast<uint32_t>(*p - '0');
            if (v > 429496729u || (v == 429496729u && d > 5u))
            {
                v = 0xFFFFFFFFu;
                while (*p >= '0' && *p <= '9')
                    ++p;
                break;
            }
            v = v * 10u + d;
            ++p;
        }
        if (!any)
            return false;
        out = v;
        return true;
    }

    inline void ensureDir(bool &ok, const char *path)
    {
        if (ok)
            return;
        fs::File d = LittleFS.open(path);
        if (d)
        {
            ok = d.isDirectory();
            d.close();
            if (ok)
                return;
        }
        LittleFS.mkdir(path);
        d = LittleFS.open(path);
        ok = d && d.isDirectory();
        if (d)
            d.close();
    }

    inline void ensureDirs(bool &ready)
    {
        if (ready)
            return;

        bool okRoot = false, okShots = false, okThumbs = false;
        ensureDir(okRoot, kRootDir);
        ensureDir(okShots, kShotsDir);
        ensureDir(okThumbs, kThumbsDir);
        ready = okRoot && okShots && okThumbs;
    }

    inline void thumbDir(char *out, size_t outSize, uint16_t tw, uint16_t th)
    {
        if (!out || outSize == 0)
            return;
        std::snprintf(out, outSize, "%s/%ux%u", kThumbsDir,
                      static_cast<unsigned>(tw), static_cast<unsigned>(th));
    }

    inline void thumbPath(char *out, size_t outSize, const char *shotPath, uint16_t tw, uint16_t th)
    {
        if (!out || outSize == 0)
            return;
        out[0] = '\0';
        if (!shotPath || !shotPath[0])
            return;
        const char *base = std::strrchr(shotPath, '/');
        base = (base && base[1]) ? (base + 1) : shotPath;
        std::snprintf(out, outSize, "%s/%ux%u/%s", kThumbsDir,
                      static_cast<unsigned>(tw), static_cast<unsigned>(th), base);
    }

    [[nodiscard]] inline bool tmpPathFromFinal(char *out, size_t outSize, const char *finalPath) noexcept
    {
        if (!out || outSize == 0)
            return false;
        out[0] = '\0';
        if (!finalPath || !finalPath[0])
            return false;

        const size_t len = std::strlen(finalPath);
        static constexpr char kExt[] = ".pscr";
        static constexpr size_t kExtLen = 5;
        static constexpr char kTmpExt[] = ".tmp";
        static constexpr size_t kTmpLen = 4;

        if (len >= kExtLen && std::memcmp(finalPath + len - kExtLen, kExt, kExtLen) == 0)
        {
            const size_t need = (len - kExtLen) + kTmpLen + 1u;
            if (need > outSize)
                return false;
            const size_t baseLen = len - kExtLen;
            std::memcpy(out, finalPath, baseLen);
            std::memcpy(out + baseLen, kTmpExt, kTmpLen);
            out[baseLen + kTmpLen] = '\0';
            return true;
        }

        const size_t need = len + kTmpLen + 1u;
        if (need > outSize)
            return false;
        std::memcpy(out, finalPath, len);
        std::memcpy(out + len, kTmpExt, kTmpLen);
        out[len + kTmpLen] = '\0';
        return true;
    }

    struct QoiReader
    {
        fs::File *f = nullptr;
        uint8_t buf[256] = {};
        uint16_t off = 0;
        uint16_t len = 0;
        uint32_t remaining = 0;
        bool limited = false;
        uint32_t *crc = nullptr;

        [[nodiscard]] inline bool readByte(uint8_t &out) noexcept
        {
            if (!f)
                return false;
            if (off >= len)
            {
                if (limited && remaining == 0)
                    return false;
                size_t want = sizeof(buf);
                if (limited && remaining < want)
                    want = static_cast<size_t>(remaining);
                const int n = f->read(buf, want);
                if (n <= 0)
                    return false;
                off = 0;
                len = static_cast<uint16_t>(n);
                if (limited)
                    remaining -= static_cast<uint32_t>(n);
            }
            out = buf[off++];
            if (crc)
                crc32UpdateByte(*crc, out);
            return true;
        }
    };

    [[nodiscard]] inline bool qoiNext(QoiReader &r, uint32_t index[64], uint32_t &px, uint32_t &run) noexcept
    {
        if (run)
        {
            --run;
        }
        else
        {
            uint8_t b1 = 0;
            if (!r.readByte(b1))
                return false;

            if (b1 == 0xFEu)
            {
                uint8_t rr = 0, gg = 0, bb = 0;
                if (!r.readByte(rr) || !r.readByte(gg) || !r.readByte(bb))
                    return false;
                px = (static_cast<uint32_t>(rr) << 24) |
                     (static_cast<uint32_t>(gg) << 16) |
                     (static_cast<uint32_t>(bb) << 8) |
                     (px & 0xFFu);
            }
            else if (b1 == 0xFFu)
            {
                uint8_t rr = 0, gg = 0, bb = 0, aa = 0;
                if (!r.readByte(rr) || !r.readByte(gg) || !r.readByte(bb) || !r.readByte(aa))
                    return false;
                px = (static_cast<uint32_t>(rr) << 24) |
                     (static_cast<uint32_t>(gg) << 16) |
                     (static_cast<uint32_t>(bb) << 8) |
                     static_cast<uint32_t>(aa);
            }
            else
            {
                switch (b1 & 0xC0u)
                {
                case 0x00u:
                    px = index[b1 & 63u];
                    break;
                case 0x40u:
                {
                    const int dr = static_cast<int>((b1 >> 4) & 3u) - 2;
                    const int dg = static_cast<int>((b1 >> 2) & 3u) - 2;
                    const int db = static_cast<int>(b1 & 3u) - 2;
                    const uint32_t rr = ((px >> 24) + dr) & 0xFFu;
                    const uint32_t gg = ((px >> 16) + dg) & 0xFFu;
                    const uint32_t bb = ((px >> 8) + db) & 0xFFu;
                    px = (rr << 24) | (gg << 16) | (bb << 8) | (px & 0xFFu);
                    break;
                }
                case 0x80u:
                {
                    uint8_t b2 = 0;
                    if (!r.readByte(b2))
                        return false;
                    const int dg = static_cast<int>(b1 & 63u) - 32;
                    const int dr_dg = static_cast<int>((b2 >> 4) & 15u) - 8;
                    const int db_dg = static_cast<int>(b2 & 15u) - 8;
                    const uint32_t rr = ((px >> 24) + dg + dr_dg) & 0xFFu;
                    const uint32_t gg = ((px >> 16) + dg) & 0xFFu;
                    const uint32_t bb = ((px >> 8) + dg + db_dg) & 0xFFu;
                    px = (rr << 24) | (gg << 16) | (bb << 8) | (px & 0xFFu);
                    break;
                }
                case 0xC0u:
                    run = static_cast<uint32_t>(b1 & 63u);
                    break;
                }
            }
        }

        index[qoiHash(px)] = px;
        return true;
    }

    template <typename Sink>
    [[nodiscard]] inline bool qoiDecodeRaster(QoiReader &r, uint16_t w, uint16_t h, Sink &&sink, bool checkTail = true) noexcept
    {
        if (w == 0 || h == 0)
            return false;

        uint32_t index[64] = {};
        uint32_t px = 0x000000FFu; // 0,0,0,255
        uint32_t run = 0;

        uint16_t x = 0;
        uint16_t y = 0;
        const uint32_t pixelsTotal = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
        for (uint32_t i = 0; i < pixelsTotal; ++i)
        {
            if (!qoiNext(r, index, px, run))
                return false;

            if constexpr (std::is_convertible_v<decltype(sink(x, y, px)), bool>)
            {
                if (!sink(x, y, px))
                    return false;
            }
            else
            {
                sink(x, y, px);
            }

            ++x;
            if (x == w)
            {
                x = 0;
                ++y;
            }
        }

        if (checkTail)
        {
            static constexpr uint8_t kTail[8] = {0, 0, 0, 0, 0, 0, 0, 1};
            for (uint8_t i = 0; i < 8; ++i)
            {
                uint8_t b = 0;
                if (!r.readByte(b) || b != kTail[i])
                    return false;
            }

            uint8_t extra = 0;
            if (r.readByte(extra))
                return false;
        }

        return true;
    }
#endif
}

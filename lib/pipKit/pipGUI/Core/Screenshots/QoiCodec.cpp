#include <pipGUI/Core/Screenshots/Codec.hpp>
#include <pipGUI/Core/Screenshots/Internals.hpp>

namespace pipgui::detail
{
#if (PIPGUI_SCREENSHOT_MODE == 2)
    [[nodiscard]] bool qoiDecodeFileTo565(fs::File &f, uint16_t w, uint16_t h, uint16_t *dst, uint32_t payloadSize, uint32_t *outPayloadCrc32) noexcept
    {
        if (!dst || w == 0 || h == 0)
            return false;
        if (payloadSize == 0)
            return false;

        screenshots::detail::QoiReader r;
        r.f = &f;
        r.limited = true;
        r.remaining = payloadSize;
        uint32_t crc = screenshots::detail::crc32Init();
        r.crc = outPayloadCrc32 ? &crc : nullptr;

        const uint32_t stride = static_cast<uint32_t>(w);
        const auto sink = [&](uint16_t x, uint16_t y, uint32_t rgba) -> void
        {
            dst[static_cast<uint32_t>(y) * stride + static_cast<uint32_t>(x)] =
                screenshots::detail::rgb565FromRgba(rgba);
        };

        if (!screenshots::detail::qoiDecodeRaster(r, w, h, sink, true))
            return false;

        if (outPayloadCrc32)
            *outPayloadCrc32 = screenshots::detail::crc32Finish(crc);

        return true;
    }

    [[nodiscard]] bool qoiEncode565ToFile(fs::File &f, const uint16_t *src565, uint16_t w, uint16_t h, uint32_t &outBytes, uint32_t *outPayloadCrc32) noexcept
    {
        outBytes = 0;
        if (!src565 || w == 0 || h == 0)
            return false;

        uint32_t crc = screenshots::detail::crc32Init();

        uint32_t index[64] = {};
        uint32_t prev = 0x000000FFu;
        uint8_t run = 0;

        uint8_t out[512];
        uint16_t outLen = 0;

        auto flush = [&]() -> bool
        {
            if (!outLen)
                return true;
            const size_t wrote = f.write(out, outLen);
            if (wrote != outLen)
                return false;
            if (outPayloadCrc32)
                screenshots::detail::crc32Update(crc, out, outLen);
            outBytes += static_cast<uint32_t>(wrote);
            outLen = 0;
            return true;
        };

        auto have = [&](uint16_t need) -> bool
        {
            if (static_cast<uint32_t>(outLen) + need <= sizeof(out))
                return true;
            return flush() && (static_cast<uint32_t>(outLen) + need <= sizeof(out));
        };

        auto put1 = [&](uint8_t b)
        { out[outLen++] = b; };
        auto put2 = [&](uint8_t a, uint8_t b)
        {
            out[outLen++] = a;
            out[outLen++] = b;
        };
        auto put4 = [&](uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        {
            out[outLen++] = a;
            out[outLen++] = b;
            out[outLen++] = c;
            out[outLen++] = d;
        };

        const uint32_t pixelsTotal = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
        for (uint32_t i = 0; i < pixelsTotal; ++i)
        {
            const uint32_t rgba = screenshots::detail::rgbaFrom565(src565[i]);

            if (rgba == prev)
            {
                ++run;
                if (run == 62 || i + 1u == pixelsTotal)
                {
                    if (!have(1))
                        return false;
                    put1(static_cast<uint8_t>(0xC0u | (run - 1u)));
                    run = 0;
                }
                continue;
            }

            if (run)
            {
                if (!have(1))
                    return false;
                put1(static_cast<uint8_t>(0xC0u | (run - 1u)));
                run = 0;
            }

            const uint8_t idx = screenshots::detail::qoiHash(rgba);
            if (index[idx] == rgba)
            {
                if (!have(1))
                    return false;
                put1(static_cast<uint8_t>(idx & 63u));
            }
            else
            {
                const int pr = static_cast<int>((prev >> 24) & 0xFFu);
                const int pg = static_cast<int>((prev >> 16) & 0xFFu);
                const int pb = static_cast<int>((prev >> 8) & 0xFFu);
                const int r = static_cast<int>((rgba >> 24) & 0xFFu);
                const int g = static_cast<int>((rgba >> 16) & 0xFFu);
                const int b = static_cast<int>((rgba >> 8) & 0xFFu);

                const int dr = r - pr;
                const int dg = g - pg;
                const int db = b - pb;

                if ((unsigned)(dr + 2) < 4u && (unsigned)(dg + 2) < 4u && (unsigned)(db + 2) < 4u)
                {
                    if (!have(1))
                        return false;
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
                            return false;
                        put2(static_cast<uint8_t>(0x80u | (dg + 32)),
                             static_cast<uint8_t>(((dr_dg + 8) << 4) | (db_dg + 8)));
                    }
                    else
                    {
                        if (!have(4))
                            return false;
                        put4(0xFEu,
                             static_cast<uint8_t>(r),
                             static_cast<uint8_t>(g),
                             static_cast<uint8_t>(b));
                    }
                }

                index[idx] = rgba;
            }

            prev = rgba;
        }

        static constexpr uint8_t kTail[8] = {0, 0, 0, 0, 0, 0, 0, 1};
        for (uint8_t i = 0; i < 8; ++i)
        {
            if (!have(1))
                return false;
            put1(kTail[i]);
        }

        if (!flush())
            return false;
        if (outPayloadCrc32)
            *outPayloadCrc32 = screenshots::detail::crc32Finish(crc);
        return true;
    }
#endif
}

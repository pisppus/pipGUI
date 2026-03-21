#include <pipGUI/Systems/Screenshots/Codec.hpp>
#include <pipGUI/Systems/Screenshots/Internals.hpp>

namespace pipgui::detail
{
#if (PIPGUI_SCREENSHOT_MODE == 2)
    namespace
    {
        [[nodiscard]] bool decodePacked565To565(fs::File &f, uint16_t w, uint16_t h, uint16_t *dst, uint32_t payloadSize, uint32_t *outPayloadCrc32) noexcept
        {
            if (!dst || w == 0 || h == 0 || payloadSize == 0)
                return false;

            screenshots::detail::PayloadReader r;
            r.f = &f;
            r.limited = true;
            r.remaining = payloadSize;
            uint32_t crc = screenshots::detail::crc32Init();
            r.crc = outPayloadCrc32 ? &crc : nullptr;

            uint32_t pos = 0;
            const uint32_t total = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
            const auto sink = [&](uint16_t px) noexcept
            {
                dst[pos++] = px;
            };

            if (!screenshots::detail::decodePacked565Raster(r, total, sink))
                return false;

            if (outPayloadCrc32)
                *outPayloadCrc32 = screenshots::detail::crc32Finish(crc);
            return true;
        }

        [[nodiscard]] bool encodePacked565ToFile(fs::File &f, const uint16_t *src565, uint16_t w, uint16_t h, uint32_t &outBytes, uint32_t *outPayloadCrc32) noexcept
        {
            outBytes = 0;
            if (!src565 || w == 0 || h == 0)
                return false;

            uint32_t crc = screenshots::detail::crc32Init();
            uint16_t index[64] = {};
            uint16_t prev = 0;
            uint16_t literal[screenshots::detail::kPacked565LiteralMax] = {};
            uint8_t run = 0;
            uint8_t literalLen = 0;

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

            auto emitRun = [&]() -> bool
            {
                if (!run)
                    return true;
                if (!have(1))
                    return false;
                put1(static_cast<uint8_t>(screenshots::detail::kPacked565RunBase | (run - 1u)));
                run = 0;
                return true;
            };

            auto emitLiteralBlock = [&]() -> bool
            {
                if (!literalLen)
                    return true;
                const uint16_t need = static_cast<uint16_t>(1u + static_cast<uint16_t>(literalLen) * 2u);
                if (!have(need))
                    return false;
                put1(static_cast<uint8_t>(screenshots::detail::kPacked565LiteralBase | (literalLen - 1u)));
                for (uint8_t i = 0; i < literalLen; ++i)
                {
                    const uint16_t px = literal[i];
                    put2(static_cast<uint8_t>((px >> 8) & 0xFFu),
                         static_cast<uint8_t>(px & 0xFFu));
                }
                literalLen = 0;
                return true;
            };

            const uint32_t pixelsTotal = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
            for (uint32_t i = 0; i < pixelsTotal; ++i)
            {
                const uint16_t px = src565[i];
                if (px == prev)
                {
                    if (!emitLiteralBlock())
                        return false;
                    ++run;
                    if (run == 64u || (i + 1u) == pixelsTotal)
                    {
                        if (!emitRun())
                            return false;
                    }
                    index[screenshots::detail::hash565(px)] = px;
                    prev = px;
                    continue;
                }

                if (!emitRun())
                    return false;

                const uint8_t idx = screenshots::detail::hash565(px);
                if (index[idx] == px)
                {
                    if (!emitLiteralBlock())
                        return false;
                    if (!have(1))
                        return false;
                    put1(static_cast<uint8_t>(idx & screenshots::detail::kPacked565IndexMask));
                }
                else
                {
                    const int pr = static_cast<int>((prev >> 11) & 31u);
                    const int pg = static_cast<int>((prev >> 5) & 63u);
                    const int pb = static_cast<int>(prev & 31u);
                    const int r = static_cast<int>((px >> 11) & 31u);
                    const int g = static_cast<int>((px >> 5) & 63u);
                    const int b = static_cast<int>(px & 31u);

                    const int dr = r - pr;
                    const int dg = g - pg;
                    const int db = b - pb;

                    if ((unsigned)(dr + 2) < 4u && (unsigned)(dg + 2) < 4u && (unsigned)(db + 2) < 4u)
                    {
                        if (!emitLiteralBlock())
                            return false;
                        if (!have(1))
                            return false;
                        put1(static_cast<uint8_t>(screenshots::detail::kPacked565DiffBase |
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
                            if (!emitLiteralBlock())
                                return false;
                            if (!have(2))
                                return false;
                            put2(static_cast<uint8_t>(screenshots::detail::kPacked565LumaBase | (dg + 16)),
                                 static_cast<uint8_t>(((dr_dg + 8) << 4) | (db_dg + 8)));
                        }
                        else
                        {
                            literal[literalLen++] = px;
                            if (literalLen == screenshots::detail::kPacked565LiteralMax)
                            {
                                if (!emitLiteralBlock())
                                    return false;
                            }
                        }
                    }
                }

                index[idx] = px;
                prev = px;
            }

            if (!emitRun() || !emitLiteralBlock() || !flush())
                return false;
            if (outPayloadCrc32)
                *outPayloadCrc32 = screenshots::detail::crc32Finish(crc);
            return true;
        }
    }

    [[nodiscard]] bool decodeScreenshotPayloadTo565(fs::File &f, uint16_t w, uint16_t h, uint16_t *dst, uint32_t payloadSize, uint32_t *outPayloadCrc32) noexcept
    {
        return decodePacked565To565(f, w, h, dst, payloadSize, outPayloadCrc32);
    }

    [[nodiscard]] bool encodeScreenshotPayload565ToFile(fs::File &f, const uint16_t *src565, uint16_t w, uint16_t h, uint32_t &outBytes, uint32_t *outPayloadCrc32) noexcept
    {
        return encodePacked565ToFile(f, src565, w, h, outBytes, outPayloadCrc32);
    }
#endif
}

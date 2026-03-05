#pragma once

#include <cstdint>
#include <cmath>

namespace pipgui
{
    inline uint8_t sqrtU8(uint32_t num)
    {
        if (num > 0x40000000u)
            return 0;

        uint32_t bsh = 0x00004000u;
        uint32_t fpr = 0;
        uint32_t osh = 0;

        while (num > bsh)
        {
            bsh <<= 2;
            osh++;
        }

        do
        {
            uint32_t bod = bsh + fpr;
            if (num >= bod)
            {
                num -= bod;
                fpr = bsh + bod;
            }
            num <<= 1;
        } while (bsh >>= 1);

        return (uint8_t)(fpr >> osh);
    }

    inline uint8_t gammaAA(uint8_t a)
    {
        return (uint8_t)(255.0f * powf(a * (1.0f / 255.0f), 0.4545454545f));
    }

    template<typename AtlasAccessor>
    inline uint8_t sampleBilinearSDF(AtlasAccessor readAtlas,
                                     int32_t u16, int32_t v16,
                                     uint32_t atlasW, uint32_t atlasH)
    {
        const int x0 = u16 >> 16, y0 = v16 >> 16;
        const uint32_t fx = (uint32_t)u16 & 0xFFFFu;
        const uint32_t fy = (uint32_t)v16 & 0xFFFFu;
        const int x1 = (x0 + 1 < (int)atlasW) ? x0 + 1 : (int)atlasW - 1;
        const int y1 = (y0 + 1 < (int)atlasH) ? y0 + 1 : (int)atlasH - 1;

        const int32_t a00 = readAtlas(x0, y0);
        const int32_t a10 = readAtlas(x1, y0);
        const int32_t a01 = readAtlas(x0, y1);
        const int32_t a11 = readAtlas(x1, y1);
        
        const int32_t a0 = (a00 << 16) + (a10 - a00) * (int32_t)fx;
        const int32_t a1 = (a01 << 16) + (a11 - a01) * (int32_t)fx;
        int32_t ia = ((a0 + (int32_t)(((int64_t)(a1 - a0) * fy) >> 16)) + 0x8000) >> 16;
        if (ia < 0) ia = 0;
        else if (ia > 255) ia = 255;
        return (uint8_t)ia;
    }
}

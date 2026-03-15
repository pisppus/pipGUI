#pragma once

#include <cstdint>
namespace pipgui
{
    inline uint32_t isqrt32(uint32_t n)
    {
        uint32_t res = 0;
        uint32_t bit = 1u << 30;
        while (bit > n)
            bit >>= 2;
        while (bit)
        {
            if (n >= res + bit)
            {
                n -= res + bit;
                res = (res >> 1) + bit;
            }
            else
            {
                res >>= 1;
            }
            bit >>= 2;
        }
        return res;
    }

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
        static constexpr uint8_t kGammaTable[256] = {
            0, 21, 28, 34, 39, 43, 46, 50, 53, 56, 59, 61, 64, 66, 68, 70,
            72, 74, 76, 78, 80, 82, 84, 85, 87, 89, 90, 92, 93, 95, 96, 98,
            99, 101, 102, 103, 105, 106, 107, 109, 110, 111, 112, 114, 115, 116, 117, 118,
            119, 120, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
            136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145, 146, 147, 148, 149, 150,
            151, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160, 160, 161, 162, 163,
            164, 164, 165, 166, 167, 167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175,
            175, 176, 177, 178, 178, 179, 180, 180, 181, 182, 182, 183, 184, 184, 185, 186,
            186, 187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195, 195, 196,
            197, 197, 198, 199, 199, 200, 200, 201, 202, 202, 203, 203, 204, 205, 205, 206,
            206, 207, 207, 208, 209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214, 215,
            215, 216, 217, 217, 218, 218, 219, 219, 220, 220, 221, 221, 222, 223, 223, 224,
            224, 225, 225, 226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232,
            232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239, 240,
            240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248,
            248, 249, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255,
        };
        return kGammaTable[a];
    }

    template <typename AtlasAccessor>
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
        if (ia < 0)
            ia = 0;
        else if (ia > 255)
            ia = 255;
        return (uint8_t)ia;
    }
}

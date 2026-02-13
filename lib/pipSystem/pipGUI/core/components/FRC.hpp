#pragma once

#include <stdint.h>

namespace pipgui
{

    struct Color888
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        static Color888 fromUint32(uint32_t c)
        {
            return Color888{(uint8_t)((c >> 16) & 0xFF), (uint8_t)((c >> 8) & 0xFF), (uint8_t)(c & 0xFF)};
        }

        uint32_t toUint32() const
        {
            return (uint32_t)((r << 16) | (g << 8) | b);
        }

        bool isBlack() const { return r == 0 && g == 0 && b == 0; }
        bool isWhite() const { return r == 255 && g == 255 && b == 255; }
    };

    enum class FrcProfile : uint8_t
    {
        Off = 0,
        BlueNoise = 1,
    };

    namespace detail
    {
        uint16_t quantize565(Color888 c, int16_t x, int16_t y, uint32_t seed, FrcProfile profile);
    }

    class FrcManager
    {
    public:
        static FrcManager& instance()
        {
            static FrcManager inst;
            return inst;
        }

        void setProfile(FrcProfile p) { _profile = p; }
        FrcProfile profile() const { return _profile; }

        uint16_t quantize(Color888 c, int16_t x, int16_t y, uint32_t seed) const
        {
            if (c.isBlack() || c.isWhite())
                return detail::quantize565(c, x, y, seed, FrcProfile::Off);
            return detail::quantize565(c, x, y, seed, _profile);
        }

    private:
        FrcManager() : _profile(FrcProfile::BlueNoise) {}
        FrcProfile _profile;
    };

    namespace detail
    {

        static constexpr uint8_t blueNoiseLut16x16[256] = {
                232,38,74,243,136,181,0,248,202,154,51,212,158,121,2,78,
                207,178,85,122,77,233,67,83,159,23,99,204,15,59,13,239,
                50,28,187,3,252,144,184,44,7,235,129,196,84,251,146,213,
                132,89,109,228,21,137,30,60,192,82,136,8,62,134,119,24,
                168,63,6,119,71,93,249,178,125,146,227,163,187,19,41,236,
                206,42,150,238,181,65,226,101,203,190,13,1,198,221,52,110,
                188,18,2,32,205,145,34,120,244,76,143,245,56,161,176,139,
                211,157,162,75,246,165,23,183,153,231,118,158,109,7,34,93,
                29,39,181,227,68,98,125,88,209,177,202,136,253,214,83,230,
                174,138,128,58,46,210,51,37,110,11,44,93,25,0,75,101,
                217,244,14,185,215,230,118,139,27,55,242,232,35,124,236,209,
                122,191,221,147,103,131,4,68,153,78,71,174,86,188,64,199,
                54,47,240,178,39,188,135,25,180,102,250,13,120,95,9,175,
                29,142,113,75,28,161,64,227,49,1,53,200,186,4,208,61,
                12,195,7,183,223,69,123,116,110,253,68,227,180,254,133,182,
                107,65,200,98,33,20,141,42,232,28,38,18,231,97,198,34,
        };

        static inline uint8_t blueNoise16x16(uint8_t ox, uint8_t oy, int16_t x, int16_t y)
        {
            uint8_t xx = (uint8_t)((uint16_t)(x + (int16_t)ox) & 15U);
            uint8_t yy = (uint8_t)((uint16_t)(y + (int16_t)oy) & 15U);
            return blueNoiseLut16x16[(uint16_t)yy * 16U + (uint16_t)xx];
        }

        static inline uint8_t blueNoise16x16(uint32_t seed, int16_t x, int16_t y)
        {
            uint8_t ox = (uint8_t)(seed & 15U);
            uint8_t oy = (uint8_t)((seed >> 4) & 15U);
            return blueNoise16x16(ox, oy, x, y);
        }

        inline uint16_t quantize565(Color888 c, int16_t x, int16_t y, uint32_t seed, FrcProfile profile)
        {
            if (profile == FrcProfile::Off)
            {
                uint16_t r5 = (uint16_t)(c.r >> 3);
                uint16_t g6 = (uint16_t)(c.g >> 2);
                uint16_t b5 = (uint16_t)(c.b >> 3);
                return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
            }

            uint8_t ox = (uint8_t)(seed & 15U);
            uint8_t oy = (uint8_t)((seed >> 4) & 15U);

            uint8_t thrR8 = blueNoise16x16(ox, oy, x, y);
            uint8_t thrG8 = blueNoise16x16(ox, oy, (int16_t)(x + 5), (int16_t)(y + 7));
            uint8_t thrB8 = blueNoise16x16(ox, oy, (int16_t)(x + 11), (int16_t)(y + 3));

            uint16_t rBase = (uint16_t)(c.r >> 3);
            uint16_t gBase = (uint16_t)(c.g >> 2);
            uint16_t bBase = (uint16_t)(c.b >> 3);

            uint8_t rFrac = (uint8_t)(c.r & 7U);
            uint8_t gFrac = (uint8_t)(c.g & 3U);
            uint8_t bFrac = (uint8_t)(c.b & 7U);

            uint16_t r5 = (uint16_t)(rBase + (((uint8_t)(rFrac << 5)) > thrR8));
            uint16_t g6 = (uint16_t)(gBase + (((uint8_t)(gFrac << 6)) > thrG8));
            uint16_t b5 = (uint16_t)(bBase + (((uint8_t)(bFrac << 5)) > thrB8));

            if (r5 > 31U)
                r5 = 31U;
            if (g6 > 63U)
                g6 = 63U;
            if (b5 > 31U)
                b5 = 31U;

            return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
        }

    }

}
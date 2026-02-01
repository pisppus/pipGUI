#pragma once

#include <stdint.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace pipcore
{
    class Esp32St7789Display : public lgfx::LGFX_Device
    {
    public:
        Esp32St7789Display() = default;

        void configure(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst,
                       uint16_t w, uint16_t h, uint32_t hz, int8_t miso = -1);

    private:
        lgfx::Panel_ST7789 _panel;
        lgfx::Bus_SPI _bus;
    };
}

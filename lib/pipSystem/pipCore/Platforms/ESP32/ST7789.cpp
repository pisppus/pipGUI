#include <pipCore/Platforms/ESP32/ST7789.h>

namespace pipcore
{
    void Esp32St7789Display::configure(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst,
                                       uint16_t w, uint16_t h, uint32_t hz, int8_t miso)
    {
        auto bus = _bus.config();
        bus.spi_host = SPI2_HOST;
        bus.spi_mode = 0;
        bus.freq_write = hz;
        bus.freq_read = hz / 2;
        bus.spi_3wire = (miso < 0);
        bus.use_lock = true;
        bus.dma_channel = 1;
        bus.pin_sclk = sclk;
        bus.pin_mosi = mosi;
        bus.pin_miso = miso;
        bus.pin_dc = dc;
        _bus.config(bus);
        _panel.setBus(&_bus);

        auto p = _panel.config();
        p.pin_cs = cs;
        p.pin_rst = rst;
        p.pin_busy = -1;
        p.memory_width = w;
        p.memory_height = h;
        p.panel_width = w;
        p.panel_height = h;
        p.offset_x = 0;
        p.offset_y = 0;
        p.dummy_read_pixel = 16;
        p.readable = (miso >= 0);
        p.invert = true;
        p.bus_shared = true;
        _panel.config(p);
        setPanel(&_panel);
    }
}

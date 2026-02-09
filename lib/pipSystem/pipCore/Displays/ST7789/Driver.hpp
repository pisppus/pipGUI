#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(ESP32)
#include <driver/spi_master.h>
#endif

namespace pipcore
{
    class ST7789
    {
    public:
        ST7789() = default;

        bool configure(int8_t mosi,
                       int8_t sclk,
                       int8_t cs,
                       int8_t dc,
                       int8_t rst,
                       uint16_t width,
                       uint16_t height,
                       uint32_t hz,
                       int8_t miso = -1,
                       bool bgr = false);

        bool begin(uint8_t rotation);

        uint16_t width() const { return _width; }
        uint16_t height() const { return _height; }

        void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        void writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes = false);

        void fillScreen565(uint16_t color565, bool swapBytes = false);

    private:
        bool initSpi();
        void hardReset();
        void setRotationInternal(uint8_t rotation);

        inline void dcCommand();
        inline void dcData();

        void writeCmd(uint8_t cmd);
        void writeData(const void *data, size_t len);
        void writeDataQueued(const void *data, size_t len);

        int8_t resolveDefaultMosi() const;
        int8_t resolveDefaultSclk() const;
        int8_t resolveDefaultMiso() const;
        int8_t resolveDefaultCs() const;

        bool usingIomuxSpiPins() const;

    private:
        void *_spiHandle = nullptr;

#if defined(ESP32)
        size_t _dmaBufSize = 0;
        uint8_t *_dmaBuf[2] = {nullptr, nullptr};
        spi_transaction_t _dmaTrans[2]{};
        int _dmaNext = 0;
        int _dmaInflight = 0;
#endif

        int8_t _pinMosi = -1;
        int8_t _pinSclk = -1;
        int8_t _pinMiso = -1;
        int8_t _pinCs = -1;
        int8_t _pinDc = -1;
        int8_t _pinRst = -1;

        uint16_t _width = 0;
        uint16_t _height = 0;
        uint16_t _physWidth = 0;
        uint16_t _physHeight = 0;
        uint32_t _hz = 0;

        uint8_t _rotation = 0;
        uint16_t _xStart = 0;
        uint16_t _yStart = 0;

        bool _bgr = false;
    };
}

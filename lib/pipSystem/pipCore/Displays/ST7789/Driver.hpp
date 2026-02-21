#pragma once

#include <cstdint>
#include <cstddef>

namespace pipcore
{
    class ISt7789Transport
    {
    public:
        virtual ~ISt7789Transport() = default;
        virtual bool init() = 0;
        virtual void deinit() = 0;
        virtual void setDc(bool level) = 0;
        virtual void setRst(bool level) = 0;
        virtual void delayMs(uint32_t ms) = 0;
        virtual void write(const void *data, size_t len) = 0;
        virtual void writeCommand(uint8_t cmd) = 0;
        virtual void writePixels(const void *data, size_t len) = 0;
        virtual void flush() = 0;
    };

    class ST7789
    {
    public:
        ST7789() = default;

        bool configure(ISt7789Transport *transport,
                       uint16_t width,
                       uint16_t height,
                       uint8_t order = 0,
                       bool invert = true,
                       bool swap = false,
                       int16_t xOffset = 0,
                       int16_t yOffset = 0);

        bool begin(uint8_t rotation);

        uint16_t width() const { return _width; }
        uint16_t height() const { return _height; }
        bool swapBytes() const { return _swap; }

        void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        void writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes = false);

        void fillScreen565(uint16_t color565, bool swapBytes = false);

        void setInversion(bool enabled);

    private:
        void hardReset();
        void setRotationInternal(uint8_t rotation);

        void writeCmd(uint8_t cmd);
        void writeData(const void *data, size_t len);
        void writeDataQueued(const void *data, size_t len);

    private:
        ISt7789Transport *_transport = nullptr;

        uint16_t _width = 0;
        uint16_t _height = 0;
        uint16_t _physWidth = 0;
        uint16_t _physHeight = 0;

        uint8_t _rotation = 0;
        int16_t _xStart = 0;
        int16_t _yStart = 0;
        int16_t _xOffsetCfg = 0;
        int16_t _yOffsetCfg = 0;

        uint8_t _order = 0;
        bool _invert = true;
        bool _swap = false;
        bool _initialized = false;
    };
}

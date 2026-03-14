#pragma once

#include <cstdint>
#include <cstddef>

namespace pipcore::st7789
{
    enum class IoError : uint8_t
    {
        None = 0,
        InvalidConfig,
        NotReady,
        Gpio,
        SpiBusInit,
        SpiDeviceAdd,
        DmaBufferAlloc,
        TransactionAlloc,
        CommandTransmit,
        DataTransmit,
        QueueTransmit,
        QueueResult,
        UnexpectedTransaction
    };

    const char *ioErrorText(IoError error);

    class Transport
    {
    public:
        virtual ~Transport() = default;
        virtual bool init() = 0;
        virtual void deinit() = 0;
        virtual IoError lastError() const = 0;
        virtual void clearError() = 0;
        virtual bool setRst(bool level) = 0;
        virtual void delayMs(uint32_t ms) = 0;
        virtual bool write(const void *data, size_t len) = 0;
        virtual bool writeCommand(uint8_t cmd) = 0;
        virtual bool writePixels(const void *data, size_t len) = 0;
        virtual bool flush() = 0;
    };

    class Driver
    {
    public:
        Driver() = default;

        bool configure(Transport *transport,
                       uint16_t width,
                       uint16_t height,
                       uint8_t order = 0,
                       bool invert = true,
                       bool swap = false,
                       int16_t xOffset = 0,
                       int16_t yOffset = 0);

        bool begin(uint8_t rotation);
        void reset();
        IoError lastError() const { return _lastError; }
        const char *lastErrorText() const { return ioErrorText(_lastError); }

        uint16_t width() const { return _width; }
        uint16_t height() const { return _height; }
        bool swapBytes() const { return _swap; }

        bool setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        bool writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes = false);

        bool fillScreen565(uint16_t color565, bool swapBytes = false);

        void setInversion(bool enabled);

    private:
        bool hardReset();
        bool setRotationInternal(uint8_t rotation);
        bool sendCommand(uint8_t cmd);
        bool sendBytes(const void *data, size_t len);
        bool sendPixels(const void *data, size_t len);
        bool flushTransport();
        bool failFromTransport(IoError fallback);

    private:
        Transport *_transport = nullptr;

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
        IoError _lastError = IoError::None;
    };
}

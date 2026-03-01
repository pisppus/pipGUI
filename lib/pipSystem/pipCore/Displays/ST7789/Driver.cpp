#include <pipCore/Displays/ST7789/Driver.hpp>
#include <string.h>

namespace pipcore
{
    namespace
    {
        constexpr uint8_t CmdSWRESET = 0x01;
        constexpr uint8_t CmdSLPOUT = 0x11;
        constexpr uint8_t CmdNORON = 0x13;
        constexpr uint8_t CmdINVOFF = 0x20;
        constexpr uint8_t CmdINVON = 0x21;
        constexpr uint8_t CmdDISPON = 0x29;
        constexpr uint8_t CmdCASET = 0x2A;
        constexpr uint8_t CmdRASET = 0x2B;
        constexpr uint8_t CmdRAMWR = 0x2C;
        constexpr uint8_t CmdMADCTL = 0x36;
        constexpr uint8_t CmdCOLMOD = 0x3A;
        constexpr uint8_t Colmod16bpp = 0x55;
        constexpr uint8_t MadctlMY = 0x80;
        constexpr uint8_t MadctlMX = 0x40;
        constexpr uint8_t MadctlMV = 0x20;
        constexpr uint8_t MadctlBGR = 0x08;

        inline uint16_t bswap16(uint16_t v) { return (uint16_t)__builtin_bswap16(v); }

        inline void pack16BE(uint8_t *buf, uint16_t a, uint16_t b)
        {
            buf[0] = (uint8_t)(a >> 8);
            buf[1] = (uint8_t)(a & 0xFF);
            buf[2] = (uint8_t)(b >> 8);
            buf[3] = (uint8_t)(b & 0xFF);
        }
    }

    bool ST7789::configure(ISt7789Transport *transport,
                           uint16_t width, uint16_t height, uint8_t order,
                           bool invert, bool swap,
                           int16_t xOffset, int16_t yOffset)
    {
        if (!transport || !width || !height || xOffset < 0 || yOffset < 0)
            return false;

        _transport = transport;
        _width = _physWidth = width;
        _height = _physHeight = height;
        _xStart = _xOffsetCfg = xOffset;
        _yStart = _yOffsetCfg = yOffset;
        _order = (order == 1) ? 1 : 0;
        _invert = invert;
        _swap = swap;
        _initialized = false;
        return true;
    }

    void ST7789::hardReset()
    {
        if (!_transport)
            return;
        _transport->setRst(0);
        _transport->delayMs(20);
        _transport->setRst(1);
        _transport->delayMs(150);
    }

    bool ST7789::begin(uint8_t rotation)
    {
        if (!_transport || !_width || !_height || !_transport->init())
            return false;

        hardReset();

        _transport->writeCommand(CmdSWRESET);
        _transport->delayMs(150);
        _transport->writeCommand(CmdSLPOUT);
        _transport->delayMs(200);

        _transport->writeCommand(CmdCOLMOD);
        {
            uint8_t v = Colmod16bpp;
            _transport->write(&v, 1);
        }
        _transport->delayMs(10);

        _transport->writeCommand(_invert ? CmdINVON : CmdINVOFF);
        _transport->delayMs(10);
        _transport->writeCommand(CmdNORON);
        _transport->delayMs(10);

        setRotationInternal(rotation);

        _transport->writeCommand(CmdDISPON);
        _transport->delayMs(100);

        _initialized = true;
        return true;
    }

    void ST7789::setInversion(bool enabled)
    {
        _invert = enabled;
        if (!_transport || !_initialized)
            return;
        _transport->writeCommand(_invert ? CmdINVON : CmdINVOFF);
        _transport->delayMs(10);
    }

    void ST7789::setRotationInternal(uint8_t rotation)
    {
        _rotation = rotation & 3U;

        const uint8_t bgr = (_order == 1) ? MadctlBGR : 0;

        uint8_t madctl;
        switch (_rotation)
        {
        case 0:
            madctl = bgr;
            _width = _physWidth;
            _height = _physHeight;
            _xStart = _xOffsetCfg;
            _yStart = _yOffsetCfg;
            break;
        case 1:
            madctl = MadctlMX | MadctlMV | bgr;
            _width = _physHeight;
            _height = _physWidth;
            _xStart = _yOffsetCfg;
            _yStart = _xOffsetCfg;
            break;
        case 2:
            madctl = MadctlMX | MadctlMY | bgr;
            _width = _physWidth;
            _height = _physHeight;
            _xStart = _xOffsetCfg;
            _yStart = _yOffsetCfg;
            break;
        case 3:
            madctl = MadctlMV | MadctlMY | bgr;
            _width = _physHeight;
            _height = _physWidth;
            _xStart = _yOffsetCfg;
            _yStart = _xOffsetCfg;
            break;
        default:
            return;
        }

        _transport->writeCommand(CmdMADCTL);
        _transport->write(&madctl, 1);
    }

    void ST7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        if (!_transport || x1 < x0 || y1 < y0 || !_width || !_height)
            return;
        if (x0 >= _width || y0 >= _height)
            return;

        if (x1 >= _width)
            x1 = (uint16_t)(_width - 1U);
        if (y1 >= _height)
            y1 = (uint16_t)(_height - 1U);

        const int32_t xs32 = x0 + _xStart, xe32 = x1 + _xStart;
        const int32_t ys32 = y0 + _yStart, ye32 = y1 + _yStart;
        if (xs32 < 0 || ys32 < 0)
            return;

        uint8_t buf[4];

        _transport->writeCommand(CmdCASET);
        pack16BE(buf, (uint16_t)xs32, (uint16_t)xe32);
        _transport->write(buf, 4);

        _transport->writeCommand(CmdRASET);
        pack16BE(buf, (uint16_t)ys32, (uint16_t)ye32);
        _transport->write(buf, 4);

        _transport->writeCommand(CmdRAMWR);
    }

    void ST7789::writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes)
    {
        if (!_transport || !pixelCount)
            return;

        if (!swapBytes)
        {
            constexpr size_t Chunk = 1024;
            while (pixelCount)
            {
                size_t n = pixelCount > Chunk ? Chunk : pixelCount;
                _transport->writePixels(pixels, n * sizeof(uint16_t));
                pixels += n;
                pixelCount -= n;
            }
            return;
        }

        constexpr size_t Chunk = 256;
        uint16_t tmp[Chunk];
        while (pixelCount)
        {
            size_t n = pixelCount > Chunk ? Chunk : pixelCount;
            for (size_t i = 0; i < n; ++i)
                tmp[i] = bswap16(pixels[i]);
            _transport->writePixels(tmp, n * sizeof(uint16_t));
            pixels += n;
            pixelCount -= n;
        }
    }

    void ST7789::fillScreen565(uint16_t color565, bool swapBytes)
    {
        if (!_transport)
            return;

        setAddrWindow(0, 0, (uint16_t)(_width - 1U), (uint16_t)(_height - 1U));

        constexpr size_t Chunk = 256;
        uint16_t tmp[Chunk];
        const uint16_t v = swapBytes ? bswap16(color565) : color565;
        for (size_t i = 0; i < Chunk; ++i)
            tmp[i] = v;

        size_t remaining = (size_t)_width * (size_t)_height;
        while (remaining)
        {
            size_t n = remaining > Chunk ? Chunk : remaining;
            _transport->writePixels(tmp, n * sizeof(uint16_t));
            remaining -= n;
        }
        _transport->flush();
    }

    void ST7789::writeCmd(uint8_t cmd)
    {
        if (_transport)
            _transport->writeCommand(cmd);
    }
    void ST7789::writeData(const void *d, size_t l)
    {
        if (_transport)
            _transport->write(d, l);
    }
    void ST7789::writeDataQueued(const void *d, size_t l)
    {
        if (_transport)
            _transport->writePixels(d, l);
    }
}
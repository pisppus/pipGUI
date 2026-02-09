#include <pipCore/Displays/ST7789/Driver.hpp>

#include <string.h>

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <esp_rom_sys.h>
 
 #include <esp_heap_caps.h>

#if __has_include(<soc/spi_periph.h>)
#include <soc/spi_periph.h>
#endif

namespace pipcore
{
    namespace
    {
        constexpr uint8_t CmdSWRESET = 0x01;
        constexpr uint8_t CmdSLPOUT = 0x11;
        constexpr uint8_t CmdNORON = 0x13;
        constexpr uint8_t CmdINVON = 0x21;
        constexpr uint8_t CmdDISPON = 0x29;
        constexpr uint8_t CmdCASET = 0x2A;
        constexpr uint8_t CmdRASET = 0x2B;
        constexpr uint8_t CmdRAMWR = 0x2C;
        constexpr uint8_t CmdMADCTL = 0x36;
        constexpr uint8_t CmdCOLMOD = 0x3A;

        constexpr uint8_t CmdPORCTRL = 0xB2;
        constexpr uint8_t CmdGCTRL = 0xB7;
        constexpr uint8_t CmdVCOMS = 0xBB;
        constexpr uint8_t CmdLCMCTRL = 0xC0;
        constexpr uint8_t CmdVDVVRHEN = 0xC2;
        constexpr uint8_t CmdVRHS = 0xC3;
        constexpr uint8_t CmdVDVS = 0xC4;
        constexpr uint8_t CmdFRCTRL2 = 0xC6;
        constexpr uint8_t CmdPWCTRL1 = 0xD0;

        constexpr uint8_t Colmod16bpp = 0x55;

        inline uint16_t bswap16(uint16_t v)
        {
            return static_cast<uint16_t>(__builtin_bswap16(v));
        }

        inline void delayMs(uint32_t ms)
        {
            esp_rom_delay_us(ms * 1000U);
        }

        inline bool isPinValid(int8_t pin)
        {
            return pin >= 0;
        }

        constexpr size_t DmaBufferBytes = 8192;

        static void IRAM_ATTR st7789_spi_pre_cb(spi_transaction_t *t)
        {
            const intptr_t packed = (intptr_t)t->user;
            const int dc = (int)(packed & 1);
            const int pin = (int)((packed >> 1) & 0xFF);
            gpio_set_level((gpio_num_t)pin, dc);
        }

#if __has_include(<soc/spi_periph.h>)
        inline int8_t getSpi2IomuxMosi()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spid_iomux_pin;
#else
            return -1;
#endif
        }

        inline int8_t getSpi2IomuxSclk()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spiclk_iomux_pin;
#else
            return -1;
#endif
        }

        inline int8_t getSpi2IomuxMiso()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spiq_iomux_pin;
#else
            return -1;
#endif
        }

        inline int8_t getSpi2IomuxCs0()
        {
#if defined(spi_periph_signal)
            return (int8_t)spi_periph_signal[SPI2_HOST].spics0_iomux_pin;
#else
            return -1;
#endif
        }
#endif
    }

    inline void IRAM_ATTR ST7789::dcCommand()
    {
        gpio_set_level(static_cast<gpio_num_t>(_pinDc), 0);
    }

    inline void IRAM_ATTR ST7789::dcData()
    {
        gpio_set_level(static_cast<gpio_num_t>(_pinDc), 1);
    }

    namespace
    {
        inline void waitQueued(spi_device_handle_t h, int &inflight)
        {
            if (inflight <= 0)
                return;
            spi_transaction_t *r = nullptr;
            spi_device_get_trans_result(h, &r, portMAX_DELAY);
            --inflight;
        }

        inline void flushQueued(spi_device_handle_t h, int &inflight)
        {
            while (inflight > 0)
                waitQueued(h, inflight);
        }
    }

    bool ST7789::configure(int8_t mosi,
                           int8_t sclk,
                           int8_t cs,
                           int8_t dc,
                           int8_t rst,
                           uint16_t width,
                           uint16_t height,
                           uint32_t hz,
                           int8_t miso,
                           bool bgr)
    {
        if (!isPinValid(dc))
            return false;

        const bool customBusPins = isPinValid(mosi) || isPinValid(sclk) || isPinValid(miso);

        _pinMosi = mosi;
        _pinSclk = sclk;
        _pinCs = cs;
        _pinDc = dc;
        _pinRst = rst;
        _pinMiso = miso;

        _bgr = bgr;

        _width = width;
        _height = height;
        _physWidth = width;
        _physHeight = height;
        _hz = hz ? hz : 80000000U;

        if (!isPinValid(_pinMosi))
            _pinMosi = resolveDefaultMosi();
        if (!isPinValid(_pinSclk))
            _pinSclk = resolveDefaultSclk();
        _pinMiso = -1;
        if (!isPinValid(_pinCs) && !customBusPins)
            _pinCs = resolveDefaultCs();

        return true;
    }

    bool ST7789::begin(uint8_t rotation)
    {
        if (_width == 0 || _height == 0)
            return false;

        if (!initSpi())
            return false;

        gpio_config_t io{};
        io.intr_type = GPIO_INTR_DISABLE;
        io.mode = GPIO_MODE_OUTPUT;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.pull_up_en = GPIO_PULLUP_DISABLE;

        io.pin_bit_mask = 0;
        if (isPinValid(_pinDc))
            io.pin_bit_mask |= (1ULL << static_cast<uint8_t>(_pinDc));
        if (isPinValid(_pinRst))
            io.pin_bit_mask |= (1ULL << static_cast<uint8_t>(_pinRst));
        if (io.pin_bit_mask)
            gpio_config(&io);

        hardReset();

        writeCmd(CmdSWRESET);
        delayMs(150);

        writeCmd(CmdSLPOUT);
        delayMs(120);

        writeCmd(CmdCOLMOD);
        {
            uint8_t v = Colmod16bpp;
            writeData(&v, 1);
        }
        delayMs(10);

        writeCmd(CmdPORCTRL);
        {
            const uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
            writeData(data, sizeof(data));
        }

        writeCmd(CmdGCTRL);
        {
            uint8_t v = 0x35;
            writeData(&v, 1);
        }

        writeCmd(CmdVCOMS);
        {
            uint8_t v = 0x19;
            writeData(&v, 1);
        }

        writeCmd(CmdLCMCTRL);
        {
            uint8_t v = 0x2C;
            writeData(&v, 1);
        }

        writeCmd(CmdVDVVRHEN);
        {
            uint8_t v = 0x01;
            writeData(&v, 1);
        }

        writeCmd(CmdVRHS);
        {
            uint8_t v = 0x12;
            writeData(&v, 1);
        }

        writeCmd(CmdVDVS);
        {
            uint8_t v = 0x20;
            writeData(&v, 1);
        }

        writeCmd(CmdFRCTRL2);
        {
            uint8_t v = 0x0F;
            writeData(&v, 1);
        }

        writeCmd(CmdPWCTRL1);
        {
            const uint8_t data[] = {0xA4, 0xA1};
            writeData(data, sizeof(data));
        }

        writeCmd(CmdINVON);
        delayMs(10);

        writeCmd(CmdNORON);
        delayMs(10);

        setRotationInternal(rotation);

        writeCmd(CmdDISPON);
        delayMs(120);

        return true;
    }

    void ST7789::hardReset()
    {
        if (!isPinValid(_pinRst))
            return;

        gpio_set_level(static_cast<gpio_num_t>(_pinRst), 0);
        delayMs(20);
        gpio_set_level(static_cast<gpio_num_t>(_pinRst), 1);
        delayMs(150);
    }

    void ST7789::setRotationInternal(uint8_t rotation)
    {
        _rotation = rotation & 3U;

        uint8_t madctl = 0;

        const uint8_t BGR = _bgr ? 0x08 : 0x00;
        switch (_rotation)
        {
        case 0:
            madctl = (uint8_t)(0x00 | BGR);
            _width = _physWidth;
            _height = _physHeight;
            _xStart = 0;
            _yStart = 0;
            if (_physWidth == 240 && _physHeight == 240)
                _yStart = 0;
            break;
        case 1:
            madctl = (uint8_t)(0x60 | BGR);
            _width = _physHeight;
            _height = _physWidth;
            _xStart = 0;
            _yStart = 0;
            if (_physWidth == 240 && _physHeight == 240)
                _yStart = 0;
            break;
        case 2:
            madctl = (uint8_t)(0xC0 | BGR);
            _width = _physWidth;
            _height = _physHeight;
            _xStart = 0;
            _yStart = 0;
            if (_physWidth == 240 && _physHeight == 240)
                _yStart = 80;
            break;
        case 3:
            madctl = (uint8_t)(0xA0 | BGR);
            _width = _physHeight;
            _height = _physWidth;
            _xStart = 0;
            _yStart = 0;
            if (_physWidth == 240 && _physHeight == 240)
                _xStart = 80;
            break;
        default:
            break;
        }

        writeCmd(CmdMADCTL);
        writeData(&madctl, 1);
    }

    bool ST7789::initSpi()
    {
        if (_spiHandle)
            return true;

        spi_bus_config_t bus{};
        bus.mosi_io_num = _pinMosi;
        bus.miso_io_num = -1;
        bus.sclk_io_num = _pinSclk;
        bus.quadwp_io_num = -1;
        bus.quadhd_io_num = -1;
        bus.max_transfer_sz = (int)DmaBufferBytes;

        if (usingIomuxSpiPins())
            bus.flags = SPICOMMON_BUSFLAG_IOMUX_PINS;

        esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
        if (err != ESP_OK)
            return false;

        spi_device_interface_config_t dev{};
        dev.mode = 0;
        dev.clock_speed_hz = static_cast<int>(_hz);
        dev.spics_io_num = isPinValid(_pinCs) ? _pinCs : -1;
        dev.queue_size = 2;
        dev.flags = SPI_DEVICE_HALFDUPLEX;
        dev.pre_cb = st7789_spi_pre_cb;

        spi_device_handle_t h = nullptr;
        err = spi_bus_add_device(SPI2_HOST, &dev, &h);
        if (err != ESP_OK)
            return false;

        _spiHandle = h;

        _dmaBufSize = DmaBufferBytes;
        for (int i = 0; i < 2; ++i)
        {
            _dmaBuf[i] = static_cast<uint8_t *>(heap_caps_aligned_alloc(4, _dmaBufSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
            if (!_dmaBuf[i])
                return false;
        }

        _dmaNext = 0;
        _dmaInflight = 0;

        return true;
    }

    bool ST7789::usingIomuxSpiPins() const
    {
        return (_pinMosi == resolveDefaultMosi()) && (_pinSclk == resolveDefaultSclk()) && (_pinMiso == resolveDefaultMiso());
    }

    void ST7789::writeCmd(uint8_t cmd)
    {
        flushQueued(static_cast<spi_device_handle_t>(_spiHandle), _dmaInflight);

        spi_transaction_t t{};
        t.flags = SPI_TRANS_USE_TXDATA;
        t.length = 8;
        t.tx_data[0] = cmd;

        t.user = (void *)(intptr_t)(((intptr_t)_pinDc << 1) | 0);

        spi_device_polling_transmit(static_cast<spi_device_handle_t>(_spiHandle), &t);
    }

    void ST7789::writeData(const void *data, size_t len)
    {
        if (!len)
            return;

        flushQueued(static_cast<spi_device_handle_t>(_spiHandle), _dmaInflight);

        spi_transaction_t t{};
        t.length = static_cast<int>(len * 8U);
        t.tx_buffer = data;

        t.user = (void *)(intptr_t)(((intptr_t)_pinDc << 1) | 1);

        spi_device_polling_transmit(static_cast<spi_device_handle_t>(_spiHandle), &t);
    }

    void IRAM_ATTR ST7789::writeDataQueued(const void *data, size_t len)
    {
        if (!len)
            return;

        spi_device_handle_t h = static_cast<spi_device_handle_t>(_spiHandle);

        while (_dmaInflight >= 2)
            waitQueued(h, _dmaInflight);

        const int slot = _dmaNext & 1;
        _dmaNext = (_dmaNext + 1) & 1;

        if (len > _dmaBufSize)
            len = _dmaBufSize;

        memcpy(_dmaBuf[slot], data, len);

        spi_transaction_t &t = _dmaTrans[slot];
        memset(&t, 0, sizeof(t));
        t.length = static_cast<int>(len * 8U);
        t.tx_buffer = _dmaBuf[slot];
        t.user = (void *)(intptr_t)(((intptr_t)_pinDc << 1) | 1);

        spi_device_queue_trans(h, &t, portMAX_DELAY);
        ++_dmaInflight;
    }

    void ST7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        flushQueued(static_cast<spi_device_handle_t>(_spiHandle), _dmaInflight);

        x0 = static_cast<uint16_t>(x0 + _xStart);
        x1 = static_cast<uint16_t>(x1 + _xStart);
        y0 = static_cast<uint16_t>(y0 + _yStart);
        y1 = static_cast<uint16_t>(y1 + _yStart);

        uint8_t buf[4];

        writeCmd(CmdCASET);
        buf[0] = static_cast<uint8_t>(x0 >> 8);
        buf[1] = static_cast<uint8_t>(x0 & 0xFF);
        buf[2] = static_cast<uint8_t>(x1 >> 8);
        buf[3] = static_cast<uint8_t>(x1 & 0xFF);
        writeData(buf, sizeof(buf));

        writeCmd(CmdRASET);
        buf[0] = static_cast<uint8_t>(y0 >> 8);
        buf[1] = static_cast<uint8_t>(y0 & 0xFF);
        buf[2] = static_cast<uint8_t>(y1 >> 8);
        buf[3] = static_cast<uint8_t>(y1 & 0xFF);
        writeData(buf, sizeof(buf));

        writeCmd(CmdRAMWR);
    }

    void ST7789::writePixels565(const uint16_t *pixels, size_t pixelCount, bool swapBytes)
    {
        if (!pixelCount)
            return;

        if (!swapBytes)
        {
            constexpr size_t ChunkPixels = DmaBufferBytes / sizeof(uint16_t);
            const uint16_t *p = pixels;
            size_t remaining = pixelCount;
            while (remaining)
            {
                const size_t n = remaining > ChunkPixels ? ChunkPixels : remaining;
                writeDataQueued(p, n * sizeof(uint16_t));
                p += n;
                remaining -= n;
            }
            return;
        }

        constexpr size_t SwapChunkPixels = 256;
        uint16_t tmp[SwapChunkPixels];

        const uint16_t *p = pixels;
        size_t remaining = pixelCount;
        while (remaining)
        {
            const size_t n = remaining > SwapChunkPixels ? SwapChunkPixels : remaining;
            for (size_t i = 0; i < n; ++i)
                tmp[i] = bswap16(p[i]);
            writeDataQueued(tmp, n * sizeof(uint16_t));
            p += n;
            remaining -= n;
        }
    }

    void ST7789::fillScreen565(uint16_t color565, bool swapBytes)
    {
        setAddrWindow(0, 0, static_cast<uint16_t>(_width - 1U), static_cast<uint16_t>(_height - 1U));

        constexpr size_t ChunkPixels = 256;
        uint16_t tmp[ChunkPixels];

        const uint16_t v = swapBytes ? bswap16(color565) : color565;
        for (size_t i = 0; i < ChunkPixels; ++i)
            tmp[i] = v;

        const size_t total = static_cast<size_t>(_width) * static_cast<size_t>(_height);
        size_t remaining = total;
        while (remaining)
        {
            const size_t n = remaining > ChunkPixels ? ChunkPixels : remaining;
            writeDataQueued(tmp, n * sizeof(uint16_t));
            remaining -= n;
        }

        flushQueued(static_cast<spi_device_handle_t>(_spiHandle), _dmaInflight);
    }

    int8_t ST7789::resolveDefaultMosi() const
    {
#if __has_include(<soc/spi_periph.h>)
        {
            int8_t pin = getSpi2IomuxMosi();
            if (isPinValid(pin))
                return pin;
        }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
        return 13;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        return 11;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
        return 11;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
        return 7;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
        return 7;
#else
        return -1;
#endif
    }

    int8_t ST7789::resolveDefaultSclk() const
    {
#if __has_include(<soc/spi_periph.h>)
        {
            int8_t pin = getSpi2IomuxSclk();
            if (isPinValid(pin))
                return pin;
        }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
        return 14;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        return 12;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
        return 12;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
        return 6;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
        return 6;
#else
        return -1;
#endif
    }

    int8_t ST7789::resolveDefaultMiso() const
    {
#if __has_include(<soc/spi_periph.h>)
        {
            int8_t pin = getSpi2IomuxMiso();
            if (isPinValid(pin))
                return pin;
        }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
        return 12;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        return 13;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
        return 13;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
        return 2;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
        return 2;
#else
        return -1;
#endif
    }

    int8_t ST7789::resolveDefaultCs() const
    {
#if __has_include(<soc/spi_periph.h>)
        {
            int8_t pin = getSpi2IomuxCs0();
            if (isPinValid(pin))
                return pin;
        }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
        return 15;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
        return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
        return 10;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
        return 10;
#else
        return -1;
#endif
    }
}

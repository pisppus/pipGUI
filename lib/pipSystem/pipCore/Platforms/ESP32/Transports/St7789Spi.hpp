#pragma once

#include <pipCore/Displays/ST7789/Driver.hpp>

#if !defined(ESP32)
#error "Esp32St7789Spi requires ESP32"
#endif

struct spi_transaction_t;

typedef void (*transaction_cb_t)(spi_transaction_t *trans);

namespace pipcore
{
    class Esp32St7789Spi final : public ISt7789Transport
    {
    public:
        Esp32St7789Spi(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst, uint32_t hz = 80000000);
        ~Esp32St7789Spi();

        bool init() override;
        void deinit() override;
        void setDc(bool level) override;
        void setRst(bool level) override;
        void delayMs(uint32_t ms) override;
        void write(const void *data, size_t len) override;
        void writeCommand(uint8_t cmd) override;
        void writePixels(const void *data, size_t len) override;
        void flush() override;

    private:
        bool initSpi();
        void waitQueued();
        void flushQueued();

        int8_t _pinMosi;
        int8_t _pinSclk;
        int8_t _pinCs;
        int8_t _pinDc;
        int8_t _pinRst;
        uint32_t _hz;

        void *_spiHandle;
        size_t _dmaBufSize;
        uint8_t *_dmaBuf[2];
        spi_transaction_t *_trans[2];
        bool _transInFlight[2];
        int _dmaNext;
        int _dmaInflight;
        bool _initialized;
    };
}

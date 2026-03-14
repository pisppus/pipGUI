#pragma once

#include <pipCore/Display.hpp>
#include <cstdint>
#include <cstddef>

namespace pipcore
{
    enum class InputMode : uint8_t
    {
        Floating = 0,
        Pullup = 1,
        Pulldown = 2
    };

    enum class AllocCaps : uint8_t
    {
        Default = 0,
        PreferExternal = 1
    };

    enum class PlatformError : uint8_t
    {
        None = 0,
        PrefsOpenFailed,
        InvalidDisplayConfig,
        DisplayConfigureFailed,
        DisplayBeginFailed,
        DisplayIoFailed
    };

    inline const char *platformErrorText(PlatformError error)
    {
        switch (error)
        {
        case PlatformError::None:
            return "ok";
        case PlatformError::PrefsOpenFailed:
            return "preferences open failed";
        case PlatformError::InvalidDisplayConfig:
            return "invalid display config";
        case PlatformError::DisplayConfigureFailed:
            return "display configure failed";
        case PlatformError::DisplayBeginFailed:
            return "display begin failed";
        case PlatformError::DisplayIoFailed:
            return "display io failed";
        default:
            return "unknown platform error";
        }
    }

    struct DisplayConfig
    {
        int8_t mosi = -1;
        int8_t sclk = -1;
        int8_t cs = -1;
        int8_t dc = -1;
        int8_t rst = -1;
        uint16_t width = 0;
        uint16_t height = 0;
        uint32_t hz = 0;
        uint8_t order = 0;
        bool invert = true;
        bool swap = false;
        int16_t xOffset = 0;
        int16_t yOffset = 0;
    };

    class Platform
    {
    public:
        virtual ~Platform() = default;
        virtual uint32_t nowMs() = 0;

        virtual void pinModeInput(uint8_t, InputMode) {}
        virtual bool digitalRead(uint8_t) { return false; }

        virtual void configureBacklightPin(uint8_t, uint8_t = 0, uint32_t = 5000, uint8_t = 12) {}
        virtual uint8_t loadMaxBrightnessPercent() { return 100; }
        virtual void storeMaxBrightnessPercent(uint8_t) {}
        virtual void setBacklightPercent(uint8_t) {}

        virtual void *alloc(size_t bytes, AllocCaps caps = AllocCaps::Default) = 0;
        virtual void free(void *ptr) = 0;

        virtual bool configureDisplay(const DisplayConfig &)
        {
            return false;
        }

        virtual bool beginDisplay(uint8_t)
        {
            return false;
        }

        virtual Display *display() { return nullptr; }

        virtual uint32_t freeHeapTotal() { return 0; }
        virtual uint32_t freeHeapInternal() { return 0; }
        virtual uint32_t largestFreeBlock() { return 0; }
        virtual uint32_t minFreeHeap() { return 0; }

        virtual PlatformError lastError() const { return PlatformError::None; }
        virtual const char *lastErrorText() const { return platformErrorText(lastError()); }

        virtual uint8_t readProgmemByte(const void *addr) { return *(const uint8_t *)addr; }
    };
}

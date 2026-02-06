#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace pipcore
{
    enum class GuiAllocCaps : uint8_t
    {
        Default = 0,
        PreferExternal = 1
    };

    struct GuiDisplayConfig
    {
        int8_t mosi = -1;
        int8_t sclk = -1;
        int8_t cs = -1;
        int8_t dc = -1;
        int8_t rst = -1;
        int8_t miso = -1;
        uint16_t width = 0;
        uint16_t height = 0;
        uint32_t hz = 0;
    };

    class GuiPlatform
    {
    public:
        virtual ~GuiPlatform() = default;
        virtual uint32_t nowMs() = 0;
        virtual void configureBacklightPin(uint8_t, uint8_t = 0, uint32_t = 5000, uint8_t = 12) {}
        virtual uint8_t loadMaxBrightnessPercent() { return 100; }
        virtual void storeMaxBrightnessPercent(uint8_t) {}
        virtual void setBacklightPercent(uint8_t) {}

        virtual void *guiAlloc(size_t bytes, GuiAllocCaps caps = GuiAllocCaps::Default)
        {
            (void)caps;
            return malloc(bytes);
        }

        virtual void guiFree(void *ptr)
        {
            free(ptr);
        }

        virtual bool guiConfigureDisplay(const GuiDisplayConfig &)
        {
            return false;
        }

        virtual bool guiBeginDisplay(uint8_t)
        {
            return false;
        }

        virtual lgfx::LovyanGFX *guiDisplay() { return nullptr; }
    };
}

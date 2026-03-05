#pragma once

#include <pipCore/Platforms/GuiPlatform.hpp>

#if !defined(PIPCORE_PLATFORM)
#if defined(ESP32)
#define PIPCORE_PLATFORM ESP32
#endif
#endif

#ifndef PIPCORE_PLATFORM
#error "Platform not selected. Define PIPCORE_PLATFORM (e.g., -DPIPCORE_PLATFORM=NAME) or ensure target macro (ESP32, STM32, etc.) is defined"
#endif

#if PIPCORE_PLATFORM == ESP32
#include <pipCore/Platforms/ESP32/GUI.hpp>
#endif

namespace pipcore
{
    inline GuiPlatform *GetPlatform()
    {
#if PIPCORE_PLATFORM == ESP32
        static ESP32Platform instance;
        return &instance;
#else
#error "Unsupported PIPCORE_PLATFORM value"
#endif
    }
}

#pragma once

#include <pipCore/Platform.hpp>

#if !defined(PIPCORE_PLATFORM)
#if defined(ESP32)
#define PIPCORE_PLATFORM ESP32
#endif
#endif

#ifndef PIPCORE_PLATFORM
#error "Platform not selected. Define PIPCORE_PLATFORM (e.g., -DPIPCORE_PLATFORM=NAME) or ensure target macro (ESP32, STM32, etc.) is defined"
#endif

#if defined(ESP32) && (PIPCORE_PLATFORM == ESP32)
#include <pipCore/Platforms/ESP32/Platform.hpp>
#else
#error "Unsupported PIPCORE_PLATFORM value for this target"
#endif

namespace pipcore
{
    using SelectedPlatform = esp32::Platform;

    [[nodiscard]] inline Platform *GetPlatform() noexcept
    {
        static SelectedPlatform instance;
        return &instance;
    }
}

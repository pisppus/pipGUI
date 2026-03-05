#pragma once

#include <pipCore/Platforms/GuiDisplay.hpp>

#if !defined(PIPCORE_DISPLAY)
#define PIPCORE_DISPLAY ST7789
#endif

#ifndef PIPCORE_DISPLAY
#error "Display not selected. Define PIPCORE_DISPLAY (e.g., -DPIPCORE_DISPLAY=NAME)"
#endif

#if PIPCORE_DISPLAY == ST7789
#include <pipCore/Displays/ST7789/Display.hpp>
#endif

namespace pipcore
{
#if PIPCORE_DISPLAY == ST7789
    using SelectedDisplay = ST7789Display;
#else
#error "Unsupported PIPCORE_DISPLAY value"
#endif

    inline SelectedDisplay *GetDisplayTyped()
    {
        static SelectedDisplay instance;
        return &instance;
    }

    inline GuiDisplay *GetDisplay()
    {
        return GetDisplayTyped();
    }
}

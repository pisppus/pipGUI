#pragma once

#include "GUI.hpp"
#include "API/Builders.hpp"
#include "Internal/ScreenRegistry.hpp"

#define SCREEN(ID, ORDER)                                                                       \
    static constexpr uint8_t ID = (ORDER);                                                      \
    static void __pipgui_screen_##ID(pipgui::GUI &ui);                                          \
    static ::pipgui::detail::ScreenRegistration __pipgui_reg_##ID(__pipgui_screen_##ID, ORDER); \
    static void __pipgui_screen_##ID(pipgui::GUI &ui)


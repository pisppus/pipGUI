#pragma once

#define PIPGUI_GUI_PARTS 1

class GUI
{
public:
    GUI();
    ~GUI() noexcept;

    GUI(const GUI &) = delete;
    GUI &operator=(const GUI &) = delete;

#include "Platform.inc"
#include "Display.inc"
#include "Drawing.inc"
#include "Public.inc"

private:
#include "Private.inc"
};

#undef PIPGUI_GUI_PARTS

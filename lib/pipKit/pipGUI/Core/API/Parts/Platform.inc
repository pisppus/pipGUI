#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

pipcore::GuiPlatform *platform() const;
static pipcore::GuiPlatform *sharedPlatform();

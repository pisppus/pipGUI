#if !defined(PIPGUI_GUI_PARTS)
#error "Do not include this file directly. Include <pipGUI/core/api/pipGUI.hpp>"
#endif

        void setPlatform(pipcore::GuiPlatform *platform);
        pipcore::GuiPlatform *platform() const;
        static pipcore::GuiPlatform *sharedPlatform();

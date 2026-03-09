#if !defined(PIPGUI_FLUENT_BUILDERS_INC)
#error "Do not include this file directly. Include <pipGUI/Core/API/pipGUI.hpp>"
#endif

#define PIPGUI_FLUENT_GUARD(...) \
    do                          \
    {                           \
        if (_consumed)          \
            return *this;       \
    } while (0)

#include "Builders/Primitives.inc"
#include "Builders/Effects.inc"
#include "Builders/Widgets.inc"
#include "Builders/Text.inc"

#undef PIPGUI_FLUENT_GUARD

#pragma once

#include <pipGUI/Core/API/Builders/Base.hpp>
#include <pipGUI/Core/Utils/Colors.hpp>

#define PIPGUI_DEFAULT_FLUENT_MOVE(Type) \
    Type(Type &&) = default;             \
    Type &operator=(Type &&) = default

#include "Builders/Config.hpp"
#include "Builders/Primitives.hpp"
#include "Builders/Effects.hpp"
#include "Builders/Widgets.hpp"
#include "Builders/Text.hpp"
#include "Builders/Factories.hpp"

#undef PIPGUI_DEFAULT_FLUENT_MOVE

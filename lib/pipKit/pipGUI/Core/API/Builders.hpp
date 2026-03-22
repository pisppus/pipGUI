#pragma once

#include <pipGUI/Core/API/Builders/Base.hpp>
#include <pipGUI/Graphics/Utils/Colors.hpp>

#define PIPGUI_DEFAULT_FLUENT_MOVE(Type) \
    Type(const Type &) = default;        \
    Type &operator=(const Type &) = default; \
    Type(Type &&) = default;             \
    Type &operator=(Type &&) = default;  \
    Type derive()                        \
    {                                    \
        Type copy(*this);                \
        this->_consumed = true;          \
        return copy;                     \
    }

#include "Builders/BuilderConfig.hpp"
#include "Builders/Primitives.hpp"
#include "Builders/Effects.hpp"
#include "Builders/Widgets.hpp"
#include "Builders/Text.hpp"
#include "Builders/Factories.hpp"

#undef PIPGUI_DEFAULT_FLUENT_MOVE

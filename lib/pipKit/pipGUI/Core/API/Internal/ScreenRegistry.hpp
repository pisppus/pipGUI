#pragma once

#include <pipGUI/Core/Common.hpp>

namespace pipgui
{
    namespace detail
    {
        struct ScreenRegistration
        {
            ScreenCallback callback;
            uint8_t order;
            ScreenRegistration *next;

            [[nodiscard]] static ScreenRegistration *&head() noexcept
            {
                static ScreenRegistration *listHead = nullptr;
                return listHead;
            }

            ScreenRegistration(ScreenCallback cb, uint8_t screenOrder) noexcept
                : callback(cb), order(screenOrder), next(head())
            {
                head() = this;
            }
        };
    }
}

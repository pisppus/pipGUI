#pragma once

#include <pipGUI/Core/pipGUI.hpp>

namespace pipgui
{
    namespace detail
    {
        [[nodiscard]] inline uint16_t resolveOptionalColor565(int32_t color, uint16_t fallback) noexcept
        {
            return color >= 0 ? (uint16_t)color : fallback;
        }
    }

    template <typename Fn>
    void GUI::renderToSpriteAndInvalidate(int16_t x, int16_t y, int16_t w, int16_t h, Fn &&drawCall)
    {
        const bool prevRender = _flags.inSpritePass;
        pipcore::Sprite *prevActive = _render.activeSprite;
        _flags.inSpritePass = 1;
        _render.activeSprite = &_render.sprite;
        drawCall();
        _flags.inSpritePass = prevRender;
        _render.activeSprite = prevActive;
        if (!prevRender)
            invalidateRect(x, y, w, h);
    }
}

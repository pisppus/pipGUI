#include "Internal.hpp"

namespace pipgui
{
    template <bool IsUpdate>
    void TextFluentT<IsUpdate>::draw()
    {
        if (_text.length() == 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::callByMode<IsUpdate>(
            [&]
            { detail::GuiAccess::updateText(*_gui, _text, _x, _y, _fg565, _bg565, _align); },
            [&]
            { detail::GuiAccess::drawText(*_gui, _text, _x, _y, _fg565, _bg565, _align); });
    }
    template void TextFluentT<false>::draw();
    template void TextFluentT<true>::draw();

    void DrawTextMarqueeFluent::draw()
    {
        if (_text.length() == 0 || _maxWidth <= 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::GuiAccess::drawTextMarquee(*_gui, _text, _x, _y, _maxWidth, _fg565, _align, _opts);
    }

    void DrawTextEllipsizedFluent::draw()
    {
        if (_text.length() == 0 || _maxWidth <= 0 || !beginCommit())
            return;

        detail::TextFontGuard guard(_gui);

        if (_fontId && !_gui->setFont(*_fontId))
            return;
        if (_sizePx)
            _gui->setFontSize(_sizePx);
        if (_weight)
            _gui->setFontWeight(_weight);
        detail::GuiAccess::drawTextEllipsized(*_gui, _text, _x, _y, _maxWidth, _fg565, _align);
    }
}

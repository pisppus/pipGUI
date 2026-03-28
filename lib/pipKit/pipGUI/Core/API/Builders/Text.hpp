#pragma once

namespace pipgui
{

    template <bool IsUpdate>
    struct TextFluentT : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(TextFluentT);
        int16_t _x, _y;
        std::optional<FontId> _fontId;
        uint16_t _sizePx;
        uint16_t _weight;
        String _text;
        uint16_t _fg565;
        uint16_t _bg565;
        TextAlign _align;
        TextFluentT(GUI *g)
            : detail::FluentLifetime(g),
              _x(-1), _y(-1),
              _fontId(std::nullopt),
              _sizePx(0),
              _weight(0),
              _text(),
              _fg565(0xFFFF),
              _bg565(0x0000),
              _align(TextAlign::Left)
        {
        }

        ~TextFluentT() { draw(); }

        TextFluentT &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        TextFluentT &font(FontId fontId, uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _fontId = fontId;
            _sizePx = sizePx;
            return *this;
        }

        TextFluentT &weight(uint16_t weight)
        {
            if (!canMutate())
                return *this;
            _weight = weight;
            return *this;
        }

        TextFluentT &weight(WeightToken weight)
        {
            return this->weight(weight.value);
        }

        TextFluentT &text(const String &t)
        {
            if (!canMutate())
                return *this;
            _text = t;
            return *this;
        }

        TextFluentT &color(uint16_t fg565)
        {
            if (!canMutate())
                return *this;
            _fg565 = fg565;
            return *this;
        }

        TextFluentT &bgColor(uint16_t bg565)
        {
            if (!canMutate())
                return *this;
            _bg565 = bg565;
            return *this;
        }

        TextFluentT &align(TextAlign a)
        {
            if (!canMutate())
                return *this;
            _align = a;
            return *this;
        }

        void draw();
    };

    struct DrawTextMarqueeFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawTextMarqueeFluent);
        int16_t _x, _y, _maxWidth;
        std::optional<FontId> _fontId;
        uint16_t _sizePx;
        uint16_t _weight;
        String _text;
        uint16_t _fg565;
        TextAlign _align;
        MarqueeTextOptions _opts;
        DrawTextMarqueeFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(-1), _y(-1), _maxWidth(0),
              _fontId(std::nullopt),
              _sizePx(0),
              _weight(0),
              _text(),
              _fg565(0xFFFF),
              _align(TextAlign::Left),
              _opts()
        {
        }

        ~DrawTextMarqueeFluent() { draw(); }

        DrawTextMarqueeFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        DrawTextMarqueeFluent &width(int16_t width)
        {
            if (!canMutate())
                return *this;
            _maxWidth = width;
            return *this;
        }

        DrawTextMarqueeFluent &font(FontId fontId, uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _fontId = fontId;
            _sizePx = sizePx;
            return *this;
        }

        DrawTextMarqueeFluent &weight(uint16_t weight)
        {
            if (!canMutate())
                return *this;
            _weight = weight;
            return *this;
        }

        DrawTextMarqueeFluent &weight(WeightToken weight)
        {
            return this->weight(weight.value);
        }

        DrawTextMarqueeFluent &text(const String &t)
        {
            if (!canMutate())
                return *this;
            _text = t;
            return *this;
        }

        DrawTextMarqueeFluent &color(uint16_t fg565)
        {
            if (!canMutate())
                return *this;
            _fg565 = fg565;
            return *this;
        }

        DrawTextMarqueeFluent &align(TextAlign a)
        {
            if (!canMutate())
                return *this;
            _align = a;
            return *this;
        }

        DrawTextMarqueeFluent &options(const MarqueeTextOptions &opts)
        {
            if (!canMutate())
                return *this;
            _opts = opts;
            return *this;
        }

        DrawTextMarqueeFluent &speed(uint16_t pxPerSec)
        {
            if (!canMutate())
                return *this;
            _opts.speedPxPerSec = pxPerSec;
            return *this;
        }

        DrawTextMarqueeFluent &holdStart(uint16_t ms)
        {
            if (!canMutate())
                return *this;
            _opts.holdStartMs = ms;
            return *this;
        }

        DrawTextMarqueeFluent &phaseStart(uint32_t ms)
        {
            if (!canMutate())
                return *this;
            _opts.phaseStartMs = ms;
            return *this;
        }

        void draw();
    };

    struct DrawTextEllipsizedFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(DrawTextEllipsizedFluent);
        int16_t _x, _y, _maxWidth;
        std::optional<FontId> _fontId;
        uint16_t _sizePx;
        uint16_t _weight;
        String _text;
        uint16_t _fg565;
        TextAlign _align;
        DrawTextEllipsizedFluent(GUI *g)
            : detail::FluentLifetime(g),
              _x(-1), _y(-1), _maxWidth(0),
              _fontId(std::nullopt),
              _sizePx(0),
              _weight(0),
              _text(),
              _fg565(0xFFFF),
              _align(TextAlign::Left)
        {
        }

        ~DrawTextEllipsizedFluent() { draw(); }

        DrawTextEllipsizedFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            return *this;
        }

        DrawTextEllipsizedFluent &width(int16_t width)
        {
            if (!canMutate())
                return *this;
            _maxWidth = width;
            return *this;
        }

        DrawTextEllipsizedFluent &font(FontId fontId, uint16_t sizePx)
        {
            if (!canMutate())
                return *this;
            _fontId = fontId;
            _sizePx = sizePx;
            return *this;
        }

        DrawTextEllipsizedFluent &weight(uint16_t weight)
        {
            if (!canMutate())
                return *this;
            _weight = weight;
            return *this;
        }

        DrawTextEllipsizedFluent &weight(WeightToken weight)
        {
            return this->weight(weight.value);
        }

        DrawTextEllipsizedFluent &text(const String &t)
        {
            if (!canMutate())
                return *this;
            _text = t;
            return *this;
        }

        DrawTextEllipsizedFluent &color(uint16_t fg565)
        {
            if (!canMutate())
                return *this;
            _fg565 = fg565;
            return *this;
        }

        DrawTextEllipsizedFluent &align(TextAlign a)
        {
            if (!canMutate())
                return *this;
            _align = a;
            return *this;
        }

        void draw();
    };

}

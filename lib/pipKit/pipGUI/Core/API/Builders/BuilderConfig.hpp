#pragma once

namespace pipgui
{
    struct ConfigureDisplayFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ConfigureDisplayFluent);
        pipcore::DisplayConfig _cfg;
        bool _touched;

        ConfigureDisplayFluent(GUI *g)
            : detail::FluentLifetime(g), _cfg(detail::defaultDisplayConfig()), _touched(false)
        {
        }

        ~ConfigureDisplayFluent() { apply(); }

        ConfigureDisplayFluent &pins(const DisplayPins &p)
        {
            if (!canMutate())
                return *this;
            _cfg.mosi = p.mosi;
            _cfg.sclk = p.sclk;
            _cfg.cs = p.cs;
            _cfg.dc = p.dc;
            _cfg.rst = p.rst;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &pins(int8_t mosi, int8_t sclk, int8_t cs, int8_t dc, int8_t rst)
        {
            if (!canMutate())
                return *this;
            _cfg.mosi = mosi;
            _cfg.sclk = sclk;
            _cfg.cs = cs;
            _cfg.dc = dc;
            _cfg.rst = rst;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &size(uint16_t w, uint16_t h)
        {
            if (!canMutate())
                return *this;
            _cfg.width = w;
            _cfg.height = h;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &hz(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _cfg.hz = v;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &order(const char *v)
        {
            if (!canMutate())
                return *this;
            if (!v)
                return *this;

            if ((v[0] == 'B' || v[0] == 'b') && (v[1] == 'G' || v[1] == 'g') && (v[2] == 'R' || v[2] == 'r') && v[3] == 0)
                _cfg.order = 1;
            else
                _cfg.order = 0;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &invert(bool v = true)
        {
            if (!canMutate())
                return *this;
            _cfg.invert = v;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &swap(bool v = true)
        {
            if (!canMutate())
                return *this;
            _cfg.swap = v;
            _touched = true;
            return *this;
        }

        ConfigureDisplayFluent &offset(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _cfg.xOffset = x;
            _cfg.yOffset = y;
            _touched = true;
            return *this;
        }

        void apply();
    };

    struct ConfigureBacklightFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ConfigureBacklightFluent);
        uint8_t _pin = 255;
        uint8_t _channel = 0;
        uint32_t _freqHz = 5000;
        uint8_t _resolutionBits = 12;
        bool _touched = false;

        explicit ConfigureBacklightFluent(GUI *g)
            : detail::FluentLifetime(g)
        {
        }

        ~ConfigureBacklightFluent() { apply(); }

        ConfigureBacklightFluent &pin(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _pin = v;
            _touched = true;
            return *this;
        }

        ConfigureBacklightFluent &channel(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _channel = v;
            _touched = true;
            return *this;
        }

        ConfigureBacklightFluent &freq(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _freqHz = v;
            _touched = true;
            return *this;
        }

        ConfigureBacklightFluent &resolution(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _resolutionBits = v;
            _touched = true;
            return *this;
        }

        void apply();
    };

    struct SetClipFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(SetClipFluent);
        int16_t _x = 0;
        int16_t _y = 0;
        int16_t _w = 0;
        int16_t _h = 0;
        bool _touched = false;

        explicit SetClipFluent(GUI *g)
            : detail::FluentLifetime(g)
        {
        }

        ~SetClipFluent() { apply(); }

        SetClipFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            _touched = true;
            return *this;
        }

        SetClipFluent &size(int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _w = w;
            _h = h;
            _touched = true;
            return *this;
        }

        SetClipFluent &rect(int16_t x, int16_t y, int16_t w, int16_t h)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            _w = w;
            _h = h;
            _touched = true;
            return *this;
        }

        void apply();
    };

    struct ShowLogoFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ShowLogoFluent);
        String _title;
        String _subtitle;
        BootAnimation _anim = None;
        uint32_t _fg = 0xFFFFFF;
        uint32_t _bg = 0x000000;
        uint32_t _dur = 0;
        int16_t _x = -1;
        int16_t _y = -1;
        bool _touched = false;

        explicit ShowLogoFluent(GUI *g)
            : detail::FluentLifetime(g)
        {
        }

        ~ShowLogoFluent() { apply(); }

        ShowLogoFluent &title(const String &v)
        {
            if (!canMutate())
                return *this;
            _title = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &subtitle(const String &v)
        {
            if (!canMutate())
                return *this;
            _subtitle = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &anim(BootAnimation v)
        {
            if (!canMutate())
                return *this;
            _anim = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &fgColor(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _fg = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &fgColor(int32_t v)
        {
            if (!canMutate())
                return *this;
            _fg = (v >= 0 && static_cast<uint32_t>(v) <= 0xFFFFu) ? detail::color565To888(static_cast<uint16_t>(v)) : static_cast<uint32_t>(v);
            _touched = true;
            return *this;
        }

        ShowLogoFluent &fgColor(uint16_t v)
        {
            if (!canMutate())
                return *this;
            _fg = detail::color565To888(v);
            _touched = true;
            return *this;
        }

        ShowLogoFluent &bgColor(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _bg = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &bgColor(int32_t v)
        {
            if (!canMutate())
                return *this;
            _bg = (v >= 0 && static_cast<uint32_t>(v) <= 0xFFFFu) ? detail::color565To888(static_cast<uint16_t>(v)) : static_cast<uint32_t>(v);
            _touched = true;
            return *this;
        }

        ShowLogoFluent &bgColor(uint16_t v)
        {
            if (!canMutate())
                return *this;
            _bg = detail::color565To888(v);
            _touched = true;
            return *this;
        }

        ShowLogoFluent &duration(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _dur = v;
            _touched = true;
            return *this;
        }

        ShowLogoFluent &pos(int16_t x, int16_t y)
        {
            if (!canMutate())
                return *this;
            _x = x;
            _y = y;
            _touched = true;
            return *this;
        }

        void apply();
    };

    struct ListInputFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ListInputFluent);
        uint8_t _screenId;
        bool _nextDown;
        bool _prevDown;

        ListInputFluent(GUI *g, uint8_t screenId)
            : detail::FluentLifetime(g), _screenId(screenId), _nextDown(false), _prevDown(false)
        {
        }

        ListInputFluent &nextDown(bool v)
        {
            if (!canMutate())
                return *this;
            _nextDown = v;
            return *this;
        }

        ListInputFluent &prevDown(bool v)
        {
            if (!canMutate())
                return *this;
            _prevDown = v;
            return *this;
        }

        ~ListInputFluent() { apply(); }

        void apply();
    };

    struct PopupMenuInputFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(PopupMenuInputFluent);
        bool _nextDown;
        bool _prevDown;

        PopupMenuInputFluent(GUI *g)
            : detail::FluentLifetime(g), _nextDown(false), _prevDown(false)
        {
        }

        PopupMenuInputFluent &nextDown(bool v)
        {
            if (!canMutate())
                return *this;
            _nextDown = v;
            return *this;
        }

        PopupMenuInputFluent &prevDown(bool v)
        {
            if (!canMutate())
                return *this;
            _prevDown = v;
            return *this;
        }

        ~PopupMenuInputFluent() { apply(); }

        void apply();
    };

    struct ConfigureListFluent : detail::FluentLifetime
    {
        uint8_t _screenId;
        const ListItemDef *_items;
        uint8_t _itemCount;
        detail::OwnedHeapArray<ListItemDef> _ownedItems;
        uint8_t _parentScreen;
        uint16_t _cardColor;
        uint16_t _cardActiveColor;
        uint8_t _radius;
        int16_t _cardWidth;
        int16_t _cardHeight;
        ListMode _mode;

        ConfigureListFluent(GUI *g)
            : detail::FluentLifetime(g), _screenId(INVALID_SCREEN_ID), _items(nullptr), _itemCount(0), _ownedItems(detail::resolvePlatform(g)),
              _parentScreen(INVALID_SCREEN_ID), _cardColor(0), _cardActiveColor(0), _radius(8),
              _cardWidth(0), _cardHeight(0), _mode(Cards)
        {
        }

        ConfigureListFluent(ConfigureListFluent &&other) noexcept
            : detail::FluentLifetime(std::move(other)),
              _screenId(other._screenId),
              _items(std::exchange(other._items, nullptr)),
              _itemCount(std::exchange(other._itemCount, 0)),
              _ownedItems(std::move(other._ownedItems)),
              _parentScreen(other._parentScreen),
              _cardColor(other._cardColor),
              _cardActiveColor(other._cardActiveColor),
              _radius(other._radius),
              _cardWidth(other._cardWidth),
              _cardHeight(other._cardHeight),
              _mode(other._mode)
        {
        }

        ConfigureListFluent &operator=(ConfigureListFluent &&other) noexcept
        {
            if (this != &other)
            {
                detail::FluentLifetime::operator=(std::move(other));
                _screenId = other._screenId;
                _items = std::exchange(other._items, nullptr);
                _itemCount = std::exchange(other._itemCount, 0);
                _ownedItems = std::move(other._ownedItems);
                _parentScreen = other._parentScreen;
                _cardColor = other._cardColor;
                _cardActiveColor = other._cardActiveColor;
                _radius = other._radius;
                _cardWidth = other._cardWidth;
                _cardHeight = other._cardHeight;
                _mode = other._mode;
            }
            return *this;
        }

        ConfigureListFluent &screen(uint8_t screenId)
        {
            if (!canMutate())
                return *this;
            _screenId = screenId;
            return *this;
        }

        ConfigureListFluent &items(std::initializer_list<ListItemDef> items)
        {
            if (!canMutate())
                return *this;
            _items = nullptr;
            _itemCount = 0;

            if (items.size() == 0)
                return *this;

            ListItemDef *copy = _ownedItems.copyFrom(items);
            if (!copy)
                return *this;

            _items = copy;
            _itemCount = static_cast<uint8_t>(items.size());
            return *this;
        }

        ConfigureListFluent &items(const ListItemDef *items, uint8_t itemCount)
        {
            if (!canMutate())
                return *this;
            _ownedItems.reset();
            _items = items;
            _itemCount = itemCount;
            return *this;
        }

        ConfigureListFluent &parent(uint8_t parentScreen)
        {
            if (!canMutate())
                return *this;
            _parentScreen = parentScreen;
            return *this;
        }

        ConfigureListFluent &cardColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _cardColor = color565;
            return *this;
        }

        ConfigureListFluent &activeColor(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _cardActiveColor = color565;
            return *this;
        }

        ConfigureListFluent &radius(uint8_t px)
        {
            if (!canMutate())
                return *this;
            _radius = px;
            return *this;
        }

        ConfigureListFluent &cardSize(int16_t width, int16_t height)
        {
            if (!canMutate())
                return *this;
            _cardWidth = width;
            _cardHeight = height;
            return *this;
        }

        ConfigureListFluent &mode(ListMode m)
        {
            if (!canMutate())
                return *this;
            _mode = m;
            return *this;
        }

        ~ConfigureListFluent() { apply(); }

        void apply();
    };

    struct TileInputFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(TileInputFluent);
        uint8_t _screenId;
        bool _nextDown;
        bool _prevDown;

        TileInputFluent(GUI *g, uint8_t screenId)
            : detail::FluentLifetime(g), _screenId(screenId), _nextDown(false), _prevDown(false)
        {
        }

        TileInputFluent &nextDown(bool v)
        {
            if (!canMutate())
                return *this;
            _nextDown = v;
            return *this;
        }

        TileInputFluent &prevDown(bool v)
        {
            if (!canMutate())
                return *this;
            _prevDown = v;
            return *this;
        }

        ~TileInputFluent() { apply(); }

        void apply();
    };

    struct ConfigureTileFluent : detail::FluentLifetime
    {
        uint8_t _screenId;
        const TileItemDef *_items;
        uint8_t _itemCount;
        detail::OwnedHeapArray<TileItemDef> _ownedItems;
        const char *const *_layoutRowsSpec;
        detail::OwnedHeapArray<const char *> _ownedLayoutRowsSpec;
        uint8_t _layoutRowCount;
        bool _layoutConfigured;
        uint8_t _parentScreen;
        uint32_t _cardColor;
        uint32_t _cardActiveColor;
        uint8_t _radius;
        uint8_t _spacing;
        uint8_t _columns;
        int16_t _tileWidth;
        int16_t _tileHeight;
        uint16_t _lineGapPx;
        TileMode _contentMode;

        ConfigureTileFluent(GUI *g)
            : detail::FluentLifetime(g), _screenId(INVALID_SCREEN_ID), _items(nullptr), _itemCount(0), _ownedItems(detail::resolvePlatform(g)),
              _layoutRowsSpec(nullptr), _ownedLayoutRowsSpec(detail::resolvePlatform(g)), _layoutRowCount(0), _layoutConfigured(false),
              _parentScreen(INVALID_SCREEN_ID), _cardColor(0), _cardActiveColor(0), _radius(8), _spacing(8),
              _columns(2), _tileWidth(0), _tileHeight(0), _lineGapPx(0),
              _contentMode(TextSubtitle)
        {
        }

        ConfigureTileFluent(ConfigureTileFluent &&other) noexcept
            : detail::FluentLifetime(std::move(other)),
              _screenId(other._screenId),
              _items(std::exchange(other._items, nullptr)),
              _itemCount(std::exchange(other._itemCount, 0)),
              _ownedItems(std::move(other._ownedItems)),
              _layoutRowsSpec(std::exchange(other._layoutRowsSpec, nullptr)),
              _ownedLayoutRowsSpec(std::move(other._ownedLayoutRowsSpec)),
              _layoutRowCount(std::exchange(other._layoutRowCount, 0)),
              _layoutConfigured(std::exchange(other._layoutConfigured, false)),
              _parentScreen(other._parentScreen),
              _cardColor(other._cardColor),
              _cardActiveColor(other._cardActiveColor),
              _radius(other._radius),
              _spacing(other._spacing),
              _columns(other._columns),
              _tileWidth(other._tileWidth),
              _tileHeight(other._tileHeight),
              _lineGapPx(other._lineGapPx),
              _contentMode(other._contentMode)
        {
        }

        ConfigureTileFluent &operator=(ConfigureTileFluent &&other) noexcept
        {
            if (this != &other)
            {
                detail::FluentLifetime::operator=(std::move(other));
                _screenId = other._screenId;
                _items = std::exchange(other._items, nullptr);
                _itemCount = std::exchange(other._itemCount, 0);
                _ownedItems = std::move(other._ownedItems);
                _layoutRowsSpec = std::exchange(other._layoutRowsSpec, nullptr);
                _ownedLayoutRowsSpec = std::move(other._ownedLayoutRowsSpec);
                _layoutRowCount = std::exchange(other._layoutRowCount, 0);
                _layoutConfigured = std::exchange(other._layoutConfigured, false);
                _parentScreen = other._parentScreen;
                _cardColor = other._cardColor;
                _cardActiveColor = other._cardActiveColor;
                _radius = other._radius;
                _spacing = other._spacing;
                _columns = other._columns;
                _tileWidth = other._tileWidth;
                _tileHeight = other._tileHeight;
                _lineGapPx = other._lineGapPx;
                _contentMode = other._contentMode;
            }
            return *this;
        }

        ConfigureTileFluent &screen(uint8_t screenId)
        {
            if (!canMutate())
                return *this;
            _screenId = screenId;
            return *this;
        }

        ConfigureTileFluent &items(std::initializer_list<TileItemDef> items)
        {
            if (!canMutate())
                return *this;
            _items = nullptr;
            _itemCount = 0;

            if (items.size() == 0)
                return *this;

            TileItemDef *copy = _ownedItems.copyFrom(items);
            if (!copy)
                return *this;

            _items = copy;
            _itemCount = static_cast<uint8_t>(items.size());
            return *this;
        }

        ConfigureTileFluent &items(const TileItemDef *items, uint8_t itemCount)
        {
            if (!canMutate())
                return *this;
            _ownedItems.reset();
            _items = items;
            _itemCount = itemCount;
            return *this;
        }

        ConfigureTileFluent &layout(std::initializer_list<const char *> rows)
        {
            if (!canMutate())
                return *this;
            _layoutRowsSpec = nullptr;
            _layoutRowCount = 0;
            _layoutConfigured = false;

            if (rows.size() == 0)
                return *this;

            const char **copy = _ownedLayoutRowsSpec.copyFrom(rows);
            if (!copy)
                return *this;

            _layoutRowsSpec = copy;
            _layoutRowCount = static_cast<uint8_t>(rows.size());
            _layoutConfigured = true;
            return *this;
        }

        ConfigureTileFluent &layout(const char *const *rows, uint8_t rowCount)
        {
            if (!canMutate())
                return *this;
            _ownedLayoutRowsSpec.reset();
            _layoutRowsSpec = rows;
            _layoutRowCount = rowCount;
            _layoutConfigured = rows && rowCount > 0;
            return *this;
        }

        ConfigureTileFluent &parent(uint8_t parentScreen)
        {
            if (!canMutate())
                return *this;
            _parentScreen = parentScreen;
            return *this;
        }

        ConfigureTileFluent &cardColor(uint32_t color888)
        {
            if (!canMutate())
                return *this;
            _cardColor = color888;
            return *this;
        }

        ConfigureTileFluent &activeColor(uint32_t color888)
        {
            if (!canMutate())
                return *this;
            _cardActiveColor = color888;
            return *this;
        }

        ConfigureTileFluent &radius(uint8_t px)
        {
            if (!canMutate())
                return *this;
            _radius = px;
            return *this;
        }

        ConfigureTileFluent &spacing(uint8_t px)
        {
            if (!canMutate())
                return *this;
            _spacing = px;
            return *this;
        }

        ConfigureTileFluent &columns(uint8_t value)
        {
            if (!canMutate())
                return *this;
            _columns = value;
            return *this;
        }

        ConfigureTileFluent &tileSize(int16_t width, int16_t height)
        {
            if (!canMutate())
                return *this;
            _tileWidth = width;
            _tileHeight = height;
            return *this;
        }

        ConfigureTileFluent &lineGap(uint16_t px)
        {
            if (!canMutate())
                return *this;
            _lineGapPx = px;
            return *this;
        }

        ConfigureTileFluent &mode(TileMode value)
        {
            if (!canMutate())
                return *this;
            _contentMode = value;
            return *this;
        }

        ~ConfigureTileFluent() { apply(); }

        void apply();
    };
}

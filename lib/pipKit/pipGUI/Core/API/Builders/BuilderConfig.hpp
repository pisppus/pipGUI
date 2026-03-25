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

    struct ConfigureStatusBarFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ConfigureStatusBarFluent);
        bool _enabled;
        uint32_t _bgColor;
        uint8_t _height;
        StatusBarPosition _pos;

        explicit ConfigureStatusBarFluent(GUI *g)
            : detail::FluentLifetime(g), _enabled(true), _bgColor(0x000000), _height(0), _pos(Top)
        {
        }

        ~ConfigureStatusBarFluent() { apply(); }

        ConfigureStatusBarFluent &enabled(bool v = true)
        {
            if (!canMutate())
                return *this;
            _enabled = v;
            return *this;
        }

        ConfigureStatusBarFluent &bgColor(uint32_t v)
        {
            if (!canMutate())
                return *this;
            _bgColor = v;
            return *this;
        }

        ConfigureStatusBarFluent &height(uint8_t v)
        {
            if (!canMutate())
                return *this;
            _height = v;
            return *this;
        }

        ConfigureStatusBarFluent &position(StatusBarPosition v)
        {
            if (!canMutate())
                return *this;
            _pos = v;
            return *this;
        }

        void apply();
    };

    struct SetStatusBarTextFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(SetStatusBarTextFluent);
        String _left;
        String _center;
        String _right;

        explicit SetStatusBarTextFluent(GUI *g)
            : detail::FluentLifetime(g)
        {
        }

        ~SetStatusBarTextFluent() { apply(); }

        SetStatusBarTextFluent &left(const String &v)
        {
            if (!canMutate())
                return *this;
            _left = v;
            return *this;
        }

        SetStatusBarTextFluent &center(const String &v)
        {
            if (!canMutate())
                return *this;
            _center = v;
            return *this;
        }

        SetStatusBarTextFluent &right(const String &v)
        {
            if (!canMutate())
                return *this;
            _right = v;
            return *this;
        }

        void apply();
    };

    struct SetStatusBarIconFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(SetStatusBarIconFluent);
        TextAlign _side;
        IconId _iconId;
        std::optional<uint16_t> _color565;
        uint16_t _sizePx;
        bool _hasSide;
        bool _hasIcon;

        explicit SetStatusBarIconFluent(GUI *g)
            : detail::FluentLifetime(g), _side(Left), _iconId(static_cast<IconId>(0xFFFF)), _sizePx(0), _hasSide(false), _hasIcon(false)
        {
        }

        ~SetStatusBarIconFluent() { apply(); }

        SetStatusBarIconFluent &side(TextAlign v)
        {
            if (!canMutate())
                return *this;
            _side = v;
            _hasSide = true;
            return *this;
        }

        SetStatusBarIconFluent &icon(IconId v)
        {
            if (!canMutate())
                return *this;
            _iconId = v;
            _hasIcon = true;
            return *this;
        }

        SetStatusBarIconFluent &color(int32_t v)
        {
            if (!canMutate())
                return *this;
            detail::assignOptionalColor(_color565, v);
            return *this;
        }

        SetStatusBarIconFluent &size(uint16_t v)
        {
            if (!canMutate())
                return *this;
            _sizePx = v;
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

    struct ConfigGraphScopeFluent : detail::FluentLifetime
    {
        PIPGUI_DEFAULT_FLUENT_MOVE(ConfigGraphScopeFluent);
        uint8_t _screenId = INVALID_SCREEN_ID;
        uint16_t _sampleRateHz = 0;
        uint16_t _timebaseMs = 0;
        uint16_t _visibleSamples = 0;
        bool _touched = false;

        explicit ConfigGraphScopeFluent(GUI *g)
            : detail::FluentLifetime(g)
        {
        }

        ~ConfigGraphScopeFluent() { apply(); }

        ConfigGraphScopeFluent &screen(uint8_t screenId)
        {
            if (!canMutate())
                return *this;
            _screenId = screenId;
            _touched = true;
            return *this;
        }

        ConfigGraphScopeFluent &rate(uint16_t hz)
        {
            if (!canMutate())
                return *this;
            _sampleRateHz = hz;
            _touched = true;
            return *this;
        }

        ConfigGraphScopeFluent &timebase(uint16_t ms)
        {
            if (!canMutate())
                return *this;
            _timebaseMs = ms;
            _touched = true;
            return *this;
        }

        ConfigGraphScopeFluent &visible(uint16_t samples)
        {
            if (!canMutate())
                return *this;
            _visibleSamples = samples;
            _touched = true;
            return *this;
        }

        void apply();
    };

    struct ConfigureListFluent : detail::FluentLifetime
    {
        uint8_t _screenId;
        const ListItemDef *_items;
        uint8_t _itemCount;
        detail::OwnedHeapArray<ListItemDef> _ownedItems;
        uint16_t _cardColor;
        uint16_t _cardActiveColor;
        uint8_t _radius;
        int16_t _cardWidth;
        int16_t _cardHeight;
        ListMode _mode;

        ConfigureListFluent(GUI *g)
            : detail::FluentLifetime(g), _screenId(INVALID_SCREEN_ID), _items(nullptr), _itemCount(0), _ownedItems(detail::resolvePlatform(g)),
              _cardColor(0), _cardActiveColor(0), _radius(0),
              _cardWidth(0), _cardHeight(0), _mode(Cards)
        {
        }

        ConfigureListFluent(ConfigureListFluent &&other) noexcept
            : detail::FluentLifetime(std::move(other)),
              _screenId(other._screenId),
              _items(std::exchange(other._items, nullptr)),
              _itemCount(std::exchange(other._itemCount, 0)),
              _ownedItems(std::move(other._ownedItems)),
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
                _cardColor = other._cardColor;
                _cardActiveColor = other._cardActiveColor;
                _radius = other._radius;
                _cardWidth = other._cardWidth;
                _cardHeight = other._cardHeight;
                _mode = other._mode;
            }
            return *this;
        }

        ConfigureListFluent derive()
        {
            ConfigureListFluent copy(_gui);
            copy._screenId = _screenId;
            copy._itemCount = _itemCount;
            copy._cardColor = _cardColor;
            copy._cardActiveColor = _cardActiveColor;
            copy._radius = _radius;
            copy._cardWidth = _cardWidth;
            copy._cardHeight = _cardHeight;
            copy._mode = _mode;

            if (_ownedItems.data && _itemCount > 0)
            {
                ListItemDef *itemsCopy = copy._ownedItems.copyFromPtr(_ownedItems.data, _itemCount);
                copy._items = itemsCopy;
            }
            else
            {
                copy._items = _items;
            }

            _consumed = true;
            return copy;
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

        ConfigureListFluent &inactive(uint16_t color565)
        {
            if (!canMutate())
                return *this;
            _cardColor = color565;
            return *this;
        }

        ConfigureListFluent &active(uint16_t color565)
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
              _cardColor(0), _cardActiveColor(0), _radius(0), _spacing(8),
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

        ConfigureTileFluent derive()
        {
            ConfigureTileFluent copy(_gui);
            copy._screenId = _screenId;
            copy._itemCount = _itemCount;
            copy._layoutRowCount = _layoutRowCount;
            copy._layoutConfigured = _layoutConfigured;
            copy._cardColor = _cardColor;
            copy._cardActiveColor = _cardActiveColor;
            copy._radius = _radius;
            copy._spacing = _spacing;
            copy._columns = _columns;
            copy._tileWidth = _tileWidth;
            copy._tileHeight = _tileHeight;
            copy._lineGapPx = _lineGapPx;
            copy._contentMode = _contentMode;

            if (_ownedItems.data && _itemCount > 0)
            {
                TileItemDef *itemsCopy = copy._ownedItems.copyFromPtr(_ownedItems.data, _itemCount);
                copy._items = itemsCopy;
            }
            else
            {
                copy._items = _items;
            }

            if (_ownedLayoutRowsSpec.data && _layoutRowCount > 0)
            {
                const char **rowsCopy = copy._ownedLayoutRowsSpec.copyFromPtr(_ownedLayoutRowsSpec.data, _layoutRowCount);
                copy._layoutRowsSpec = rowsCopy;
            }
            else
            {
                copy._layoutRowsSpec = _layoutRowsSpec;
            }

            _consumed = true;
            return copy;
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

        ConfigureTileFluent &inactive(uint32_t color888)
        {
            if (!canMutate())
                return *this;
            _cardColor = color888;
            return *this;
        }

        ConfigureTileFluent &active(uint32_t color888)
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

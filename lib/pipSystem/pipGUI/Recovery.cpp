#include <pipGUI/core/api/pipGUI.h>

namespace
{
    static inline uint16_t panelBgColor(const pipgui::GUI &ui)
    {
        return ui.rgb(48, 48, 48);
    }

    static inline uint16_t highlightColor(const pipgui::GUI &ui)
    {
        return ui.rgb(0, 130, 220);
    }

    static inline uint8_t panelRadius()
    {
        return 14;
    }
}

namespace pipgui
{

void GUI::enableRecovery(Button *prevButton, Button *nextButton, uint32_t holdMs)
{
    enableRecovery(prevButton, nextButton, prevButton, holdMs);
}

void GUI::enableRecovery(Button *prevButton, Button *nextButton, Button *holdButton, uint32_t holdMs)
{
    _recoveryPrev = prevButton;
    _recoveryNext = nextButton;
    _recoveryHold = holdButton;
    _recoveryHoldMs = holdMs ? holdMs : 1;

    _flags.recoveryEnabled = (_recoveryPrev != nullptr && _recoveryNext != nullptr && _recoveryHold != nullptr);
    _flags.recoveryActive = 0;
    _flags.recoveryArmed = 0;
    _recoveryHoldStartMs = 0;

    _flags.bootWasActive = _flags.bootActive;

    if (!_flags.recoveryEnabled)
        return;

    regScreen(_recoveryScreenId, recoveryScreenCallback);
    regScreen(_recoveryActionScreenId, recoveryScreenCallback);
    regScreen(_recoveryExitScreenId, recoveryScreenCallback);
}

void GUI::setRecoveryMenu(std::initializer_list<RecoveryItemDef> items)
{
    pipcore::GuiPlatform *plat = platform();

    uint8_t count = 0;
    for (auto it = items.begin(); it != items.end(); ++it)
        ++count;

    if (count == 0)
    {
        // free existing storage
        if (_recoveryItems)
        {
            detail::guiFree(plat, _recoveryItems);
            _recoveryItems = nullptr;
            _recoveryItemCapacity = 0;
        }
        _recoveryItemCount = 0;
        return;
    }

    if (count > _recoveryItemCapacity)
    {
        // grow capacity (double or at least count)
        uint8_t newCap = _recoveryItemCapacity ? (uint8_t)max<uint8_t>(_recoveryItemCapacity * 2, count) : (uint8_t)max<uint8_t>(4, count);

        RecoveryEntry *newItems = (RecoveryEntry *)detail::guiAlloc(plat, sizeof(RecoveryEntry) * newCap, pipcore::GuiAllocCaps::Default);
        if (!newItems)
            return; // allocation failed, keep old

        // construct/move existing items into new storage
        for (uint8_t i = 0; i < newCap; ++i)
            new (&newItems[i]) RecoveryEntry();

        if (_recoveryItems)
        {
            for (uint8_t i = 0; i < _recoveryItemCount; ++i)
            {
                newItems[i] = std::move(_recoveryItems[i]);
                _recoveryItems[i].~RecoveryEntry();
            }
            detail::guiFree(plat, _recoveryItems);
        }

        _recoveryItems = newItems;
        _recoveryItemCapacity = newCap;
    }

    // copy items; overwrite first count entries
    uint8_t oldCount = _recoveryItemCount;
    uint8_t idx = 0;
    for (auto it = items.begin(); it != items.end() && idx < _recoveryItemCapacity; ++it)
    {
        _recoveryItems[idx].title = it->title ? String(it->title) : String("");
        _recoveryItems[idx].subtitle = it->subtitle ? String(it->subtitle) : String("");
        _recoveryItems[idx].action = it->action;
        _recoveryItems[idx].actionScreen = it->action ? _recoveryActionScreenId : 255;
        ++idx;
    }

    // destroy any leftover old entries if new list is shorter than previous
    if (idx < oldCount)
    {
        for (uint8_t i = idx; i < oldCount; ++i)
            _recoveryItems[i].~RecoveryEntry();
    }

    _recoveryItemCount = idx;

    if (_flags.recoveryActive)
    {
        // temporary list for configureListMenu
        ListMenuItemDef *defs = (ListMenuItemDef *)detail::guiAlloc(plat, sizeof(ListMenuItemDef) * _recoveryItemCount, pipcore::GuiAllocCaps::Default);
        if (defs)
        {
            for (uint8_t i = 0; i < _recoveryItemCount; ++i)
            {
                defs[i].title = _recoveryItems[i].title.c_str();
                defs[i].subtitle = _recoveryItems[i].subtitle.c_str();
                defs[i].targetScreen = _recoveryItems[i].actionScreen;
            }

            if (_recoveryItemCount > 0)
            {
                configureListMenu(_recoveryScreenId, _recoveryExitScreenId, defs, _recoveryItemCount);
                setListMenuStyle(
                    _recoveryScreenId,
                    0,
                    highlightColor(*this),
                    6,
                    10,
                    0,
                    34,
                    0,
                    0,
                    0,
                    Plain);
            }

            detail::guiFree(plat, defs);
        }

        requestRedraw();
    }
}

bool GUI::recoveryEnabled() const
{
    return _flags.recoveryEnabled;
}

bool GUI::recoveryActive() const
{
    return _flags.recoveryActive;
}

void GUI::tickRecovery(uint32_t now)
{
    if (!_flags.recoveryEnabled || !_recoveryHold)
    {
        _flags.bootWasActive = _flags.bootActive;
        return;
    }

    _recoveryHold->update();

    if (!_flags.recoveryArmed && _flags.bootActive)
    {
        _flags.recoveryArmed = 1;
    }

    if (_flags.bootWasActive && !_flags.bootActive)
    {
        _flags.recoveryArmed = 1;
        if (!_recoveryHold->isDown())
            _recoveryHoldStartMs = 0;
    }

    _flags.bootWasActive = _flags.bootActive;

    if (!_flags.recoveryArmed || _flags.recoveryActive)
    {
        if (_flags.recoveryActive && _recoveryPrev && _recoveryNext)
        {
            bool nextPressed = _recoveryNext->wasPressed();
            bool prevPressed = _recoveryPrev->wasPressed();
            bool nextDown = _recoveryNext->isDown();
            bool prevDown = _recoveryPrev->isDown();

            if (_currentScreen == _recoveryScreenId)
                handleListMenuInput(_recoveryScreenId, nextPressed, prevPressed, nextDown, prevDown);
        }
        return;
    }

    if (_recoveryHold->isDown())
    {
        if (_recoveryHoldStartMs == 0)
            _recoveryHoldStartMs = now;

        if ((uint32_t)(now - _recoveryHoldStartMs) >= _recoveryHoldMs)
        {
            _flags.recoveryActive = 1;
            _recoveryHoldStartMs = 0;

            _recoveryReturnScreen = _currentScreen;

            _recoverySavedBgColor = _bgColor;

            _flags.recoverySavedStatusBarEnabled = _flags.statusBarEnabled;
            _recoverySavedStatusBarPos = _statusBarPos;
            _recoverySavedStatusBarHeight = _statusBarHeight;
            _recoverySavedStatusBarBg = _statusBarBg;
            _recoverySavedStatusBarFg = _statusBarFg;

            _recoverySavedStatusLeft = _statusTextLeft;
            _recoverySavedStatusCenter = _statusTextCenter;
            _recoverySavedStatusRight = _statusTextRight;

            _recoverySavedBatteryLevel = _batteryLevel;
            _recoverySavedBatteryStyle = _batteryStyle;

            _statusBarPos = Top;
            _bgColor = 0x0000;

            uint8_t h = _statusBarHeight;
            if (h == 0)
                h = 20;

            configureStatusBar(true, 0x0000, h, Top);
            setStatusBarText("Recovery", "", "");

            if (_batteryLevel >= 0)
                setStatusBarBattery(_batteryLevel, Numeric);
            else
                setStatusBarBattery(-1, Hidden);

            pipcore::GuiPlatform *plat = platform();
            ListMenuItemDef *defs = (ListMenuItemDef *)detail::guiAlloc(plat, sizeof(ListMenuItemDef) * _recoveryItemCount, pipcore::GuiAllocCaps::Default);
            if (defs)
            {
                for (uint8_t i = 0; i < _recoveryItemCount; ++i)
                {
                    defs[i].title = _recoveryItems[i].title.c_str();
                    defs[i].subtitle = _recoveryItems[i].subtitle.c_str();
                    defs[i].targetScreen = _recoveryItems[i].actionScreen;
                }

                if (_recoveryItemCount > 0)
                {
                    configureListMenu(_recoveryScreenId, _recoveryExitScreenId, defs, _recoveryItemCount);
                    setListMenuStyle(
                        _recoveryScreenId,
                        0,
                        highlightColor(*this),
                        6,
                        10,
                        0,
                        34,
                        0,
                        0,
                        0,
                        Plain);
                }

                detail::guiFree(plat, defs);
            }

            setScreen(_recoveryScreenId);
            requestRedraw();
        }
    }
    else
    {
        _recoveryHoldStartMs = 0;
    }
}

void GUI::renderRecoveryScreen()
{
    auto t = getDrawTarget();
    if (!t)
        return;

    t->fillRect(0, 0, _screenWidth, _screenHeight, _bgColor);

    int16_t left = 0, right = _screenWidth, top = 0, bottom = _screenHeight;
    if (_flags.statusBarEnabled && _statusBarHeight > 0)
    {
        if (_statusBarPos == Top)
            top += _statusBarHeight;
        else if (_statusBarPos == Bottom)
            bottom -= _statusBarHeight;
        else if (_statusBarPos == Left)
            left += _statusBarHeight;
        else if (_statusBarPos == Right)
            right -= _statusBarHeight;
    }

    int16_t usableW = right - left;
    int16_t usableH = bottom - top;
    if (usableW <= 0 || usableH <= 0)
        return;

    int16_t pad = 10;
    int16_t px = left + pad;
    int16_t py = top + pad;
    int16_t pw = usableW - pad * 2;
    int16_t ph = usableH - pad * 2;

    if (pw <= 0 || ph <= 0)
        return;

    uint16_t pbg = panelBgColor(*this);
    uint8_t r = panelRadius();

    t->fillSmoothRoundRect(px, py, pw, ph, r, pbg);
    if (ph > (int16_t)r)
        t->fillRect(px, py + r, pw, ph - r, pbg);

    int16_t mx = 6;
    int16_t my = 6;
    int16_t lw = pw - mx * 2;
    int16_t lh = ph - my * 2;
    if (lw > 0 && lh > 0)
        renderListMenu(_recoveryScreenId, px + mx, py + my, lw, lh, pbg);
}

void GUI::recoveryScreenCallback(GUI &ui)
{
    if (ui.currentScreen() == ui._recoveryExitScreenId)
    {
        recoveryExitAction(ui);
        return;
    }

    if (ui.currentScreen() == ui._recoveryActionScreenId)
    {
        ListMenuState *pm = ui.getListMenu(ui._recoveryScreenId);
        if (pm)
        {
            uint8_t idx = pm->selectedIndex;
            if (idx < ui._recoveryItemCount)
            {
                auto &e = ui._recoveryItems[idx];
                if (e.action)
                    e.action(ui);
            }
        }
        ui.setScreen(ui._recoveryScreenId);
        ui.requestRedraw();
        return;
    }

    ui.renderRecoveryScreen();
}

void GUI::recoveryExitAction(GUI &ui)
{
    ui._flags.recoveryActive = 0;
    ui._flags.recoveryArmed = 0;
    ui._recoveryHoldStartMs = 0;

    ui._bgColor = ui._recoverySavedBgColor;

    ui.configureStatusBar(ui._flags.recoverySavedStatusBarEnabled,
                          ui._recoverySavedStatusBarBg,
                          ui._recoverySavedStatusBarHeight,
                          ui._recoverySavedStatusBarPos);

    ui.setStatusBarText(ui._recoverySavedStatusLeft, ui._recoverySavedStatusCenter, ui._recoverySavedStatusRight);
    ui.setStatusBarBattery(ui._recoverySavedBatteryLevel, ui._recoverySavedBatteryStyle);

    ui.setScreen(ui._recoveryReturnScreen);
    ui.requestRedraw();
}

}

#include <pipGUI/Core/pipGUI.hpp>

namespace pipgui
{
    namespace
    {
        constexpr uint32_t kErrorDismissHoldMs = 420;

        static inline uint8_t clampIndex(uint8_t index, uint8_t count) noexcept
        {
            return (count == 0 || index >= count) ? 0 : index;
        }

        static inline void setDotsLayoutState(detail::ErrorState &state, uint8_t prevCount, uint8_t newCount, uint32_t now) noexcept
        {
            const uint8_t prevDotsVisible = (prevCount > 1) ? 255 : 0;
            const uint8_t newDotsVisible = (newCount > 1) ? 255 : 0;
            if (prevDotsVisible != newDotsVisible)
            {
                state.layoutAnimStartMs = now;
                state.layoutFromDotsVisible = prevDotsVisible;
                state.layoutToDotsVisible = newDotsVisible;
            }
            else
            {
                state.layoutAnimStartMs = 0;
                state.layoutFromDotsVisible = newDotsVisible;
                state.layoutToDotsVisible = newDotsVisible;
            }
        }
    }

    bool GUI::ensureErrorCapacity(uint8_t need)
    {
        return detail::ensureCapacity(platform(), _error.entries, _error.capacity, need);
    }

    void GUI::startError(const String &message, const String &code, ErrorType type, const String &buttonText)
    {
        const uint32_t now = nowMs();
        const uint8_t prevCount = _error.count;
        const uint8_t prevCurrent = _error.currentIndex;
        const uint8_t prevNext = _error.nextIndex;
        const bool hadEntries = prevCount > 0;

        uint8_t insertIndex = 0;
        if (!ensureErrorCapacity((uint8_t)(_error.count + 1)))
        {
            if (_error.capacity == 0)
                return;

            for (uint8_t i = 0; i + 1 < _error.capacity; ++i)
                _error.entries[i] = std::move(_error.entries[i + 1]);

            if (hadEntries)
            {
                _error.currentIndex = (prevCurrent > 0) ? (uint8_t)(prevCurrent - 1) : 0;
                _error.nextIndex = (prevNext > 0) ? (uint8_t)(prevNext - 1) : 0;
            }

            insertIndex = (uint8_t)(_error.capacity - 1);
            _error.count = _error.capacity;
        }
        else
        {
            insertIndex = (_error.count < _error.capacity) ? _error.count : (uint8_t)(_error.capacity - 1);
            if (_error.count < _error.capacity)
                ++_error.count;
        }

        _error.entries[insertIndex] = {message, code, type, buttonText};

        setDotsLayoutState(_error, prevCount, _error.count, now);

        _flags.errorActive = 1;
        _flags.needRedraw = 1;

        if (!hadEntries)
        {
            _error.currentIndex = insertIndex;
            _error.nextIndex = insertIndex;
            _error.transitionDir = 0;
            _error.showStartMs = now;
            _error.contentStartMs = now;
            _flags.errorTransition = 0;
            _flags.errorEntering = 1;
            _flags.errorAwaitRelease = 1;
            _flags.errorButtonDown = 0;
            _error.nextHoldStartMs = 0;
            _error.prevHoldStartMs = 0;
            _error.nextLongFired = false;
            _error.prevLongFired = false;
            _error.lastNextDown = false;
            _error.lastPrevDown = false;
        }
        else if (_error.currentIndex != insertIndex)
        {
            _error.nextIndex = insertIndex;
            _error.transitionDir = 1;
            _error.animStartMs = now;
            _flags.errorTransition = 1;
            _flags.errorEntering = 0;
        }

        const detail::ErrorEntry &entry = _error.entries[_flags.errorTransition ? _error.nextIndex : _error.currentIndex];
        _error.buttonState.enabled = (entry.type == ErrorTypeWarning);
        _error.buttonState.pressLevel = 0;
        _error.buttonState.fadeLevel = 255;
        _error.buttonState.prevEnabled = _error.buttonState.enabled;
        _error.buttonState.loading = false;
        _error.buttonState.lastUpdateMs = now;
    }

    void GUI::nextError()
    {
        if (!_flags.errorActive || _flags.errorTransition || _error.count <= 1)
            return;

        _error.nextIndex = (uint8_t)((_error.currentIndex + 1) % _error.count);
        _error.transitionDir = 1;
        _error.animStartMs = nowMs();
        _flags.errorTransition = (_error.nextIndex != _error.currentIndex);
        _flags.errorEntering = 0;
        _flags.needRedraw = 1;
    }

    void GUI::prevError()
    {
        if (!_flags.errorActive || _flags.errorTransition || _error.count <= 1)
            return;

        _error.nextIndex = (_error.currentIndex == 0) ? (uint8_t)(_error.count - 1) : (uint8_t)(_error.currentIndex - 1);
        _error.transitionDir = -1;
        _error.animStartMs = nowMs();
        _flags.errorTransition = (_error.nextIndex != _error.currentIndex);
        _flags.errorEntering = 0;
        _flags.needRedraw = 1;
    }

    bool GUI::errorActive() const noexcept
    {
        return _flags.errorActive;
    }

    void GUI::setErrorButtonsDown(bool nextDown, bool prevDown, bool comboDown)
    {
        if (!_flags.errorActive || _error.count == 0)
            return;
        if (_flags.errorTransition)
            return;

        auto dismissCurrent = [&]()
        {
            const uint32_t dismissNow = nowMs();
            const uint8_t prevCount = _error.count;
            const uint8_t idx = clampIndex(_error.currentIndex, _error.count);
            for (uint8_t i = idx; i + 1 < _error.count; ++i)
                _error.entries[i] = std::move(_error.entries[i + 1]);

            if (_error.count > 0)
                --_error.count;

            _flags.errorTransition = 0;
            _flags.errorEntering = 0;
            _error.transitionDir = 0;
            _error.nextIndex = 0;
            _error.nextHoldStartMs = 0;
            _error.prevHoldStartMs = 0;
            _error.nextLongFired = false;
            _error.prevLongFired = false;
            _error.lastNextDown = false;
            _error.lastPrevDown = false;
            _flags.errorButtonDown = 0;

            if (_error.count == 0)
            {
                _flags.errorActive = 0;
                _error.currentIndex = 0;
                _error.buttonState.enabled = false;
                _error.contentStartMs = dismissNow;
            }
            else
            {
                _error.currentIndex = clampIndex(idx, _error.count);
                _error.nextIndex = _error.currentIndex;
                _error.contentStartMs = dismissNow;
                const detail::ErrorEntry &entry = _error.entries[_error.currentIndex];
                _error.buttonState.enabled = (entry.type == ErrorTypeWarning);
                _error.buttonState.pressLevel = 0;
                _error.buttonState.fadeLevel = 255;
                _error.buttonState.prevEnabled = _error.buttonState.enabled;
                _error.buttonState.loading = false;
                _error.buttonState.lastUpdateMs = dismissNow;
            }

            setDotsLayoutState(_error, prevCount, _error.count, dismissNow);

            _flags.needRedraw = 1;
        };

        const detail::ErrorEntry &entry = _error.entries[clampIndex(_error.currentIndex, _error.count)];
        const bool dismissible = (entry.type == ErrorTypeWarning);

        if (_flags.errorAwaitRelease)
        {
            if (!nextDown && !prevDown && !comboDown)
            {
                _flags.errorAwaitRelease = 0;
                _flags.errorButtonDown = 0;
                _error.lastNextDown = false;
                _error.lastPrevDown = false;
                updateButtonPress(_error.buttonState, false);
            }
            return;
        }

        const uint32_t now = nowMs();
        const bool wasButtonDown = _flags.errorButtonDown;
        _flags.errorButtonDown = dismissible && comboDown;
        updateButtonPress(_error.buttonState, _flags.errorButtonDown);

        if (wasButtonDown && !_flags.errorButtonDown)
        {
            dismissCurrent();
            return;
        }

        if (_flags.errorButtonDown || comboDown)
        {
            _error.nextHoldStartMs = 0;
            _error.prevHoldStartMs = 0;
            _error.nextLongFired = false;
            _error.prevLongFired = false;
            _error.lastNextDown = false;
            _error.lastPrevDown = false;
            return;
        }

        if (nextDown)
        {
            if (!_error.lastNextDown)
            {
                _error.nextHoldStartMs = now;
                _error.nextLongFired = false;
            }
        }
        else
        {
            if (_error.lastNextDown && !_error.nextLongFired && _error.count > 1)
                nextError();
            _error.nextHoldStartMs = 0;
            _error.nextLongFired = false;
        }

        if (prevDown)
        {
            if (!_error.lastPrevDown)
            {
                _error.prevHoldStartMs = now;
                _error.prevLongFired = false;
            }
            else if (dismissible && !_error.prevLongFired &&
                     _error.prevHoldStartMs != 0 &&
                     (now - _error.prevHoldStartMs) >= kErrorDismissHoldMs)
            {
                _error.prevLongFired = true;
                dismissCurrent();
                return;
            }
        }
        else
        {
            if (_error.lastPrevDown && !_error.prevLongFired)
            {
                if (_error.count > 1)
                    prevError();
                else if (dismissible)
                {
                    dismissCurrent();
                    return;
                }
            }
            _error.prevHoldStartMs = 0;
            _error.prevLongFired = false;
        }

        _error.lastNextDown = nextDown;
        _error.lastPrevDown = prevDown;
    }

    void GUI::setErrorButtonDown(bool down)
    {
        setErrorButtonsDown(false, down, false);
    }
}

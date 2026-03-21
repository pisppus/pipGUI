#include <pipGUI/Systems/Screenshots/Internals.hpp>
#include <pipGUI/Core/Internal/ViewModels.hpp>
#include <cstring>

namespace pipgui
{
    bool GUI::startScreenshot()
    {
#if !PIPGUI_SCREENSHOTS
        return false;
#else
        if (_shotStream.active)
            return false;
        startScreenshotStream();
        return _shotStream.active;
#endif
    }

    void GUI::handleScreenshotShortcut(bool comboDown)
    {
#if !PIPGUI_SCREENSHOTS
        (void)comboDown;
        return;
#else
        constexpr uint16_t kScreenshotHoldMs = 300;
        if (comboDown)
        {
            if (_diag.screenshotHoldStartMs == 0)
                _diag.screenshotHoldStartMs = nowMs();

            const uint32_t now = nowMs();
            if (!_diag.screenshotCaptured && (now - _diag.screenshotHoldStartMs) >= kScreenshotHoldMs)
            {
                if (_shotStream.active)
                {
                    _diag.screenshotCaptured = true;
                    return;
                }

                const bool started = startScreenshot();
                if (started)
                {
                    _shotStream.notifyOnComplete = true;
#if (PIPGUI_SCREENSHOT_MODE != 2)
                    captureScreenshotToGallery();
#endif
                }
                else
                {
                    showToastInternal("Screenshot failed", 1800, true,
                                      static_cast<IconId>(psdf_icons::IconCount), 0);
                }
                _diag.screenshotCaptured = true;
            }
        }
        else
        {
            _diag.screenshotHoldStartMs = 0;
            _diag.screenshotCaptured = false;
        }
#endif
    }

    void GUI::resetScreenshotStreamState(pipcore::Platform *plat) noexcept
    {
        if (_shotStream.buffer)
        {
            if (plat)
                detail::free(plat, _shotStream.buffer);
            _shotStream.buffer = nullptr;
        }
#if (PIPGUI_SCREENSHOT_MODE == 2)
        if (_shotStream.file)
            _shotStream.file.close();
        _shotStream.file = {};
        _shotStream.stamp = 0;
        if (_shotStream.path[0] != '\0')
        {
            char tmpPath[64];
            if (screenshots::detail::tmpPathFromFinal(tmpPath, sizeof(tmpPath), _shotStream.path))
                LittleFS.remove(tmpPath);
        }
        _shotStream.path[0] = '\0';
#endif
        _shotStream.width = 0;
        _shotStream.height = 0;
        _shotStream.payloadCrc = 0;
        _shotStream.headerOffset = 0;
        _shotStream.headerReady = false;
        _shotStream.active = false;
        _shotStream.notifyOnComplete = false;
        _shotStream.encPos = 0;
        _shotStream.encPayloadBytes = 0;
        _shotStream.encPrev565 = 0;
        std::memset(_shotStream.encIndex, 0, sizeof(_shotStream.encIndex));
        std::memset(_shotStream.encLiteral565, 0, sizeof(_shotStream.encLiteral565));
        _shotStream.encRun = 0;
        _shotStream.encLiteralLen = 0;
        _shotStream.encOutOff = 0;
        _shotStream.encOutLen = 0;
    }

    void GUI::freeScreenshotStream(pipcore::Platform *plat) noexcept
    {
        resetScreenshotStreamState(plat);
    }
}

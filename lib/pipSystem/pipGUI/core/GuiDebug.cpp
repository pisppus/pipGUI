#include <pipGUI/core/GuiDebug.hpp>

#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/Debug.hpp>

namespace pipgui
{
    bool GuiDebug::_statusBarMetricsEnabled = false;
    uint32_t GuiDebug::_lastStatusBarUpdateMs = 0;

    namespace
    {
        String g_prevLeft;
        String g_prevCenter;
        String g_prevRight;
        bool g_prevSaved = false;

        static constexpr uint32_t kUpdatePeriodMs = 100;
    }

    void GuiDebug::setDirtyRectsEnabled(bool enabled)
    {
        Debug::setDirtyRectEnabled(enabled);
    }

    bool GuiDebug::dirtyRectsEnabled()
    {
        return Debug::dirtyRectEnabled();
    }

    void GuiDebug::setStatusBarMetricsEnabled(GUI &ui, bool enabled)
    {
        if (_statusBarMetricsEnabled == enabled)
            return;

        _statusBarMetricsEnabled = enabled;
        _lastStatusBarUpdateMs = 0;

        if (enabled)
        {
            g_prevLeft = ui.statusBarTextLeft();
            g_prevCenter = ui.statusBarTextCenter();
            g_prevRight = ui.statusBarTextRight();
            g_prevSaved = true;
            Debug::init();
            applyStatusBarMetrics(ui);
        }
        else
        {
            restoreStatusBarText(ui);
            g_prevSaved = false;
        }
    }

    bool GuiDebug::statusBarMetricsEnabled()
    {
        return _statusBarMetricsEnabled;
    }

    void GuiDebug::tick(GUI &ui, uint32_t nowMs)
    {
        if (!_statusBarMetricsEnabled)
            return;

        if ((uint32_t)(nowMs - _lastStatusBarUpdateMs) < kUpdatePeriodMs)
            return;

        _lastStatusBarUpdateMs = nowMs;
        applyStatusBarMetrics(ui);
    }

    void GuiDebug::applyStatusBarMetrics(GUI &ui)
    {
        char buf[48];
        Debug::update();
        Debug::formatStatusBar(buf, sizeof(buf));

        ui.setStatusBarText(String(), String(buf), String());
        ui.updateStatusBarCenter();
    }

    void GuiDebug::restoreStatusBarText(GUI &ui)
    {
        if (!g_prevSaved)
            return;

        ui.setStatusBarText(g_prevLeft, g_prevCenter, g_prevRight);
        ui.updateStatusBarCenter();
    }
}

#pragma once

#include <cstdint>

namespace pipgui
{
    class GUI;

    class GuiDebug
    {
    public:
        static void setDirtyRectsEnabled(bool enabled);
        static bool dirtyRectsEnabled();

        static void setStatusBarMetricsEnabled(GUI &ui, bool enabled);
        static bool statusBarMetricsEnabled();

        static void tick(GUI &ui, uint32_t nowMs);

    private:
        static bool _statusBarMetricsEnabled;
        static uint32_t _lastStatusBarUpdateMs;

        static void applyStatusBarMetrics(GUI &ui);
        static void restoreStatusBarText(GUI &ui);
    };
}

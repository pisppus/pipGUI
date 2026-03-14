#pragma once

#include <Preferences.h>
#include <cstdint>

namespace pipcore::esp32::services
{
    class Prefs
    {
    public:
        ~Prefs();

        bool loadMaxBrightnessPercent(uint8_t &percent);
        bool storeMaxBrightnessPercent(uint8_t percent);

    private:
        bool ensureOpen();

    private:
        Preferences _prefs;
        bool _opened = false;
    };
}

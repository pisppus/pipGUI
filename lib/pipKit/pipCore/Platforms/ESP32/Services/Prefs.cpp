#include <pipCore/Platforms/ESP32/Services/Prefs.hpp>

namespace pipcore::esp32::services
{
    Prefs::~Prefs()
    {
        if (_opened)
            _prefs.end();
    }

    bool Prefs::ensureOpen()
    {
        if (_opened)
            return true;
        if (!_prefs.begin("pipgui", false))
            return false;
        _opened = true;
        return true;
    }

    bool Prefs::loadMaxBrightnessPercent(uint8_t &percent)
    {
        percent = 100;
        if (!ensureOpen())
            return false;

        uint16_t raw = _prefs.getUShort("bmax", 100);
        if (raw > 100)
            raw = 100;
        percent = static_cast<uint8_t>(raw);
        return true;
    }

    bool Prefs::storeMaxBrightnessPercent(uint8_t percent)
    {
        if (percent > 100)
            percent = 100;
        if (!ensureOpen())
            return false;
        _prefs.putUShort("bmax", percent);
        return true;
    }
}

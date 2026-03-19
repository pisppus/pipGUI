#pragma once

#include <pipGUI/Core/Types.hpp>

namespace pipgui
{
    struct GraphArea
    {
        int16_t x = 0;
        int16_t y = 0;
        int16_t w = 0;
        int16_t h = 0;

        int16_t innerX = 0;
        int16_t innerY = 0;
        int16_t innerW = 0;
        int16_t innerH = 0;

        uint32_t bgColor = 0;
        uint32_t gridColor = 0;

        uint8_t gridCellsX = 0;
        uint8_t gridCellsY = 0;

        uint8_t radius = 0;
        GraphDirection direction = LeftToRight;
        float speed = 0.0f;
        bool autoScaleEnabled = false;
        bool autoScaleInitialized = false;
        int16_t autoMin = 0;
        int16_t autoMax = 0;
        uint32_t lastPeakMs = 0;

        uint16_t lineCount = 0;
        uint16_t sampleCapacity = 0;
        int16_t **samples = nullptr;
        uint16_t *sampleCounts = nullptr;
        uint16_t *sampleHead = nullptr;
    };

    struct ListStyle
    {
        uint16_t cardColor = 0;
        uint16_t cardActiveColor = 0;
        uint8_t radius = 0;
        uint8_t spacing = 0;
        int16_t cardWidth = 0;
        int16_t cardHeight = 0;
        uint16_t titleFontPx = 0;
        uint16_t subtitleFontPx = 0;
        uint16_t lineGapPx = 0;
        ListMode mode = Cards;
    };

    struct TileStyle
    {
        uint32_t cardColor = 0;
        uint32_t cardActiveColor = 0;
        uint8_t radius = 0;
        uint8_t spacing = 0;
        uint8_t columns = 0;
        int16_t tileWidth = 0;
        int16_t tileHeight = 0;
        uint16_t lineGapPx = 0;
        TileMode contentMode = TextOnly;
    };

    struct ListState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
            uint16_t iconId = 0xFFFF;

            int16_t titleW = 0;
            int16_t titleH = 0;
            int16_t subW = 0;
            int16_t subH = 0;
            uint16_t cachedTitlePx = 0;
            uint16_t cachedSubPx = 0;
            uint16_t cachedTitleWeight = 0;
            uint16_t cachedSubWeight = 0;
        };

        bool configured = false;
        uint8_t parentScreen = INVALID_SCREEN_ID;
        uint8_t itemCount = 0;
        uint8_t selectedIndex = 0;

        float scrollPos = 0.0f;
        float targetScroll = 0.0f;
        float scrollVel = 0.0f;

        uint32_t nextHoldStartMs = 0;
        uint32_t prevHoldStartMs = 0;
        bool nextLongFired = false;
        bool prevLongFired = false;
        bool lastNextDown = false;
        bool lastPrevDown = false;

        ListStyle style;

        uint8_t scrollbarAlpha = 0;
        uint32_t lastScrollActivityMs = 0;
        uint32_t marqueeStartMs = 0;

        uint8_t capacity = 0;
        Item *items = nullptr;

        uint32_t lastUpdateMs = 0;
    };

    struct TileState
    {
        struct Item
        {
            String title;
            String subtitle;
            uint8_t targetScreen;
            uint16_t iconId = 0xFFFF;
            uint8_t layoutCol = 0;
            uint8_t layoutRow = 0;
            uint8_t layoutColSpan = 1;
            uint8_t layoutRowSpan = 1;

            int16_t titleW = 0;
            int16_t titleH = 0;
            int16_t subW = 0;
            int16_t subH = 0;
            uint16_t cachedTitlePx = 0;
            uint16_t cachedSubPx = 0;
            uint16_t cachedTitleWeight = 0;
            uint16_t cachedSubWeight = 0;
        };

        bool configured = false;
        uint8_t parentScreen = INVALID_SCREEN_ID;
        uint8_t itemCount = 0;
        uint8_t selectedIndex = 0;

        uint32_t nextHoldStartMs = 0;
        uint32_t prevHoldStartMs = 0;
        bool nextLongFired = false;
        bool prevLongFired = false;
        bool lastNextDown = false;
        bool lastPrevDown = false;
        uint32_t marqueeStartMs = 0;

        TileStyle style;

        bool customLayout = false;
        uint8_t layoutCols = 0;
        uint8_t layoutRows = 0;

        Item *items = nullptr;
        uint8_t itemCapacity = 0;
    };
}

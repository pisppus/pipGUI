#pragma once

#include <pipGUI/Core/API/Common.hpp>

namespace pipgui
{
    struct GraphArea
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;

        int16_t innerX;
        int16_t innerY;
        int16_t innerW;
        int16_t innerH;

        uint32_t bgColor;
        uint32_t gridColor;

        uint8_t gridCellsX;
        uint8_t gridCellsY;

        uint8_t radius;
        GraphDirection direction;
        float speed;
        bool autoScaleEnabled;
        bool autoScaleInitialized;
        int16_t autoMin;
        int16_t autoMax;
        uint32_t lastPeakMs;

        uint16_t lineCount;
        uint16_t sampleCapacity;
        int16_t **samples;
        uint16_t *sampleCounts;
        uint16_t *sampleHead;
    };

    struct ListStyle
    {
        uint16_t cardColor;
        uint16_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        int16_t cardWidth;
        int16_t cardHeight;
        uint16_t titleFontPx;
        uint16_t subtitleFontPx;
        uint16_t lineGapPx;
        ListMode mode;
    };

    struct TileStyle
    {
        uint32_t cardColor;
        uint32_t cardActiveColor;
        uint8_t radius;
        uint8_t spacing;
        uint8_t columns;
        int16_t tileWidth;
        int16_t tileHeight;
        uint16_t lineGapPx;
        TileMode contentMode;
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

        bool configured;
        uint8_t parentScreen;
        uint8_t itemCount;
        uint8_t selectedIndex;

        float scrollPos;
        float targetScroll;
        float scrollVel;

        uint32_t nextHoldStartMs;
        uint32_t prevHoldStartMs;
        bool nextLongFired;
        bool prevLongFired;
        bool lastNextDown;
        bool lastPrevDown;

        ListStyle style;

        uint8_t scrollbarAlpha;
        uint32_t lastScrollActivityMs;
        uint32_t marqueeStartMs;

        uint8_t capacity;
        Item *items;

        uint32_t lastUpdateMs;
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

        bool configured;
        uint8_t parentScreen;
        uint8_t itemCount;
        uint8_t selectedIndex;

        uint32_t nextHoldStartMs;
        uint32_t prevHoldStartMs;
        bool nextLongFired;
        bool prevLongFired;
        bool lastNextDown;
        bool lastPrevDown;
        uint32_t marqueeStartMs;

        TileStyle style;

        bool customLayout;
        uint8_t layoutCols;
        uint8_t layoutRows;

        Item *items = nullptr;
        uint8_t itemCapacity = 0;
    };
}

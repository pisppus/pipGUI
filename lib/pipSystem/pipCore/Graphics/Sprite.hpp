#pragma once

#include <stdint.h>
#include <stddef.h>

namespace pipcore
{
    class GuiDisplay;

    class Sprite
    {
    public:
        Sprite() = default;
        ~Sprite();

        Sprite(const Sprite &) = delete;
        Sprite &operator=(const Sprite &) = delete;

        bool createSprite(int16_t w, int16_t h);
        void deleteSprite();

        int16_t width() const { return _w; }
        int16_t height() const { return _h; }

        void *getBuffer() { return _buf; }
        const void *getBuffer() const { return _buf; }

        void fillScreen(uint16_t color565);
        void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565);
        void drawPixel(int16_t x, int16_t y, uint16_t color565);
        void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565);
        void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565);
        void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565);
        void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius, uint16_t color565);
        void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color565);

        void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels565);

        void setClipRect(int16_t x, int16_t y, int16_t w, int16_t h);
        void getClipRect(int32_t *x, int32_t *y, int32_t *w, int32_t *h) const;

        void pushSprite(Sprite *dst, int16_t x, int16_t y) const;

        void writeToDisplay(GuiDisplay &display, int16_t x, int16_t y, int16_t w, int16_t h) const;

        static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
        {
            return (uint16_t)((((uint16_t)(r >> 3)) << 11) | (((uint16_t)(g >> 2)) << 5) | ((uint16_t)(b >> 3)));
        }

    private:
        bool clipTest(int16_t x, int16_t y) const;
        void clipNormalize();
        inline void fillRow(uint16_t *dst, int16_t w, uint16_t v);

        static inline int16_t clampi16(int16_t v, int16_t lo, int16_t hi)
        {
            return (v < lo) ? lo : (v > hi ? hi : v);
        }

    private:
        uint16_t *_buf = nullptr;
        int16_t _w = 0;
        int16_t _h = 0;

        int16_t _clipX = 0;
        int16_t _clipY = 0;
        int16_t _clipW = 0;
        int16_t _clipH = 0;
    };
}

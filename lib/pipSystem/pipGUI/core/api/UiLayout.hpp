#pragma once

#include <cstdint>
#include <cassert>

namespace pipgui
{

    struct UiSize
    {
        int16_t w;
        int16_t h;
    };

    struct UiRect
    {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
    };

    struct UiInsets
    {
        int16_t l;
        int16_t t;
        int16_t r;
        int16_t b;
    };

    enum class UiAlign : uint8_t
    {
        Start,
        Center,
        End
    };

    enum class UiJustify : uint8_t
    {
        Start,
        Center,
        End,
        SpaceBetween,
        SpaceEvenly
    };

    struct UiLayout
    {
        static inline constexpr UiRect inset(const UiRect &rc, int16_t all) noexcept
        {
            return UiRect{(int16_t)(rc.x + all), (int16_t)(rc.y + all), (int16_t)(rc.w - all * 2), (int16_t)(rc.h - all * 2)};
        }

        static inline constexpr UiRect inset(const UiRect &rc, const UiInsets &in) noexcept
        {
            return UiRect{(int16_t)(rc.x + in.l), (int16_t)(rc.y + in.t), (int16_t)(rc.w - in.l - in.r), (int16_t)(rc.h - in.t - in.b)};
        }

        static inline UiRect takeTop(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
        {
            if (h < 0)
                h = 0;
            if (h > rc.h)
                h = rc.h;
            UiRect out{rc.x, rc.y, rc.w, h};
            int16_t shift = (int16_t)(h + gap);
            if (shift > rc.h)
                shift = rc.h;
            rc.y = (int16_t)(rc.y + shift);
            rc.h = (int16_t)(rc.h - shift);
            return out;
        }

        static inline UiRect takeBottom(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
        {
            if (h < 0)
                h = 0;
            if (h > rc.h)
                h = rc.h;
            UiRect out{rc.x, (int16_t)(rc.y + rc.h - h), rc.w, h};
            int16_t shift = (int16_t)(h + gap);
            if (shift > rc.h)
                shift = rc.h;
            rc.h = (int16_t)(rc.h - shift);
            return out;
        }

        static inline UiRect takeLeft(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
        {
            if (w < 0)
                w = 0;
            if (w > rc.w)
                w = rc.w;
            UiRect out{rc.x, rc.y, w, rc.h};
            int16_t shift = (int16_t)(w + gap);
            if (shift > rc.w)
                shift = rc.w;
            rc.x = (int16_t)(rc.x + shift);
            rc.w = (int16_t)(rc.w - shift);
            return out;
        }

        static inline UiRect takeRight(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
        {
            if (w < 0)
                w = 0;
            if (w > rc.w)
                w = rc.w;
            UiRect out{(int16_t)(rc.x + rc.w - w), rc.y, w, rc.h};
            int16_t shift = (int16_t)(w + gap);
            if (shift > rc.w)
                shift = rc.w;
            rc.w = (int16_t)(rc.w - shift);
            return out;
        }

        static inline void flowRow(const UiRect &area,
                                   const UiSize *sizes,
                                   UiRect *out,
                                   uint8_t count,
                                   int16_t gap,
                                   UiJustify justify = UiJustify::Center,
                                   UiAlign align = UiAlign::Center) noexcept
        {
            if (!sizes || !out || count == 0)
                return;

            int32_t totalW = 0;
            for (uint8_t i = 0; i < count; ++i)
                totalW += sizes[i].w;

            int16_t useGap = gap;
            if (useGap < 0)
                useGap = 0;

            int16_t startX = area.x;

            if (justify == UiJustify::SpaceBetween)
            {
                if (count > 1)
                    useGap = (int16_t)((area.w - totalW) / (int32_t)(count - 1));
                else
                    useGap = 0;
                if (useGap < 0)
                    useGap = 0;
                startX = area.x;
            }
            else if (justify == UiJustify::SpaceEvenly)
            {
                useGap = (int16_t)((area.w - totalW) / (int32_t)(count + 1));
                if (useGap < 0)
                    useGap = 0;
                startX = (int16_t)(area.x + useGap);
            }
            else
            {
                int32_t contentW = totalW + (int32_t)useGap * (int32_t)(count > 1 ? (count - 1) : 0);
                if (justify == UiJustify::Center)
                    startX = (int16_t)(area.x + (area.w - contentW) / 2);
                else if (justify == UiJustify::End)
                    startX = (int16_t)(area.x + (area.w - contentW));
                else
                    startX = area.x;
            }

            int16_t x = startX;
            for (uint8_t i = 0; i < count; ++i)
            {
                int16_t yy = area.y;
                if (align == UiAlign::Center)
                    yy = (int16_t)(area.y + (area.h - sizes[i].h) / 2);
                else if (align == UiAlign::End)
                    yy = (int16_t)(area.y + (area.h - sizes[i].h));

                out[i] = UiRect{x, yy, sizes[i].w, sizes[i].h};
                x = (int16_t)(x + sizes[i].w + useGap);
            }
        }

        static inline void flowColumn(const UiRect &area,
                                      const UiSize *sizes,
                                      UiRect *out,
                                      uint8_t count,
                                      int16_t gap,
                                      UiJustify justify = UiJustify::Center,
                                      UiAlign align = UiAlign::Center) noexcept
        {
            if (!sizes || !out || count == 0)
                return;

            int32_t totalH = 0;
            for (uint8_t i = 0; i < count; ++i)
                totalH += sizes[i].h;

            int16_t useGap = gap;
            if (useGap < 0)
                useGap = 0;

            int16_t startY = area.y;

            if (justify == UiJustify::SpaceBetween)
            {
                if (count > 1)
                    useGap = (int16_t)((area.h - totalH) / (int32_t)(count - 1));
                else
                    useGap = 0;
                if (useGap < 0)
                    useGap = 0;
                startY = area.y;
            }
            else if (justify == UiJustify::SpaceEvenly)
            {
                useGap = (int16_t)((area.h - totalH) / (int32_t)(count + 1));
                if (useGap < 0)
                    useGap = 0;
                startY = (int16_t)(area.y + useGap);
            }
            else
            {
                int32_t contentH = totalH + (int32_t)useGap * (int32_t)(count > 1 ? (count - 1) : 0);
                if (justify == UiJustify::Center)
                    startY = (int16_t)(area.y + (area.h - contentH) / 2);
                else if (justify == UiJustify::End)
                    startY = (int16_t)(area.y + (area.h - contentH));
                else
                    startY = area.y;
            }

            int16_t y = startY;
            for (uint8_t i = 0; i < count; ++i)
            {
                int16_t xx = area.x;
                if (align == UiAlign::Center)
                    xx = (int16_t)(area.x + (area.w - sizes[i].w) / 2);
                else if (align == UiAlign::End)
                    xx = (int16_t)(area.x + (area.w - sizes[i].w));

                out[i] = UiRect{xx, y, sizes[i].w, sizes[i].h};
                y = (int16_t)(y + sizes[i].h + useGap);
            }
        }
    };

    template <uint8_t N>
    struct UiFlowRow
    {
        UiRect area;
        int16_t gap;
        UiJustify justify;
        UiAlign align;

        uint8_t count;
        UiSize sizes[N];
        UiRect rects[N];

        explicit UiFlowRow(const UiRect &a,
                           int16_t g = 0,
                           UiJustify j = UiJustify::Center,
                           UiAlign al = UiAlign::Center)
            : area(a), gap(g), justify(j), align(al), count(0)
        {
        }

        UiRect next(int16_t w, int16_t h)
        {
            assert(count < N);
            if (count >= N)
                return UiRect{area.x, area.y, 0, 0};
            if (w < 0)
                w = 0;
            if (h < 0)
                h = 0;
            sizes[count] = UiSize{w, h};
            rects[count] = UiRect{area.x, area.y, w, h};
            ++count;
            return rects[count - 1];
        }

        void finish() noexcept
        {
            UiLayout::flowRow(area, sizes, rects, count, gap, justify, align);
        }

        const UiRect &operator[](uint8_t i) const { return rects[i]; }
    };

    template <uint8_t N>
    struct UiFlowColumn
    {
        UiRect area;
        int16_t gap;
        UiJustify justify;
        UiAlign align;

        uint8_t count;
        UiSize sizes[N];
        UiRect rects[N];

        explicit UiFlowColumn(const UiRect &a,
                              int16_t g = 0,
                              UiJustify j = UiJustify::Center,
                              UiAlign al = UiAlign::Center)
            : area(a), gap(g), justify(j), align(al), count(0)
        {
        }

        UiRect next(int16_t w, int16_t h)
        {
            assert(count < N);
            if (count >= N)
                return UiRect{area.x, area.y, 0, 0};
            if (w < 0)
                w = 0;
            if (h < 0)
                h = 0;
            sizes[count] = UiSize{w, h};
            rects[count] = UiRect{area.x, area.y, w, h};
            ++count;
            return rects[count - 1];
        }

        void finish() noexcept
        {
            UiLayout::flowColumn(area, sizes, rects, count, gap, justify, align);
        }

        const UiRect &operator[](uint8_t i) const { return rects[i]; }
    };

}

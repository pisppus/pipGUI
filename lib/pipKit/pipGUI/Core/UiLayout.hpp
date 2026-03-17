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

    namespace layout
    {
        inline constexpr UiJustify SpaceBetween = UiJustify::SpaceBetween;
        inline constexpr UiJustify SpaceEvenly = UiJustify::SpaceEvenly;
    }

    struct UiLayout
    {
        static inline UiRect inset(const UiRect &rc, int16_t all) noexcept
        {
            const int16_t w = rc.w - all * 2;
            const int16_t h = rc.h - all * 2;
            return UiRect{
                (int16_t)(rc.x + all),
                (int16_t)(rc.y + all),
                (int16_t)(w > 0 ? w : 0),
                (int16_t)(h > 0 ? h : 0)};
        }

        static inline UiRect inset(const UiRect &rc, const UiInsets &in) noexcept
        {
            const int16_t w = rc.w - in.l - in.r;
            const int16_t h = rc.h - in.t - in.b;
            return UiRect{
                (int16_t)(rc.x + in.l),
                (int16_t)(rc.y + in.t),
                (int16_t)(w > 0 ? w : 0),
                (int16_t)(h > 0 ? h : 0)};
        }

        static inline UiRect takeTop(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
        {
            if (h < 0)
                h = 0;
            if (h > rc.h)
                h = rc.h;
            UiRect out{rc.x, rc.y, rc.w, h};
            rc.y = (int16_t)(rc.y + h + gap);
            rc.h = (int16_t)(rc.h - h - gap);
            if (rc.h < 0)
                rc.h = 0;
            return out;
        }

        static inline UiRect takeBottom(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
        {
            if (h < 0)
                h = 0;
            if (h > rc.h)
                h = rc.h;
            UiRect out{rc.x, (int16_t)(rc.y + rc.h - h), rc.w, h};
            rc.h = (int16_t)(rc.h - h - gap);
            if (rc.h < 0)
                rc.h = 0;
            return out;
        }

        static inline UiRect takeLeft(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
        {
            if (w < 0)
                w = 0;
            if (w > rc.w)
                w = rc.w;
            UiRect out{rc.x, rc.y, w, rc.h};
            rc.x = (int16_t)(rc.x + w + gap);
            rc.w = (int16_t)(rc.w - w - gap);
            if (rc.w < 0)
                rc.w = 0;
            return out;
        }

        static inline UiRect takeRight(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
        {
            if (w < 0)
                w = 0;
            if (w > rc.w)
                w = rc.w;
            UiRect out{(int16_t)(rc.x + rc.w - w), rc.y, w, rc.h};
            rc.w = (int16_t)(rc.w - w - gap);
            if (rc.w < 0)
                rc.w = 0;
            return out;
        }

        static inline UiRect placeInside(const UiRect &area, const UiSize &size,
                                         UiJustify hJustify = UiJustify::Center,
                                         UiAlign vAlign = UiAlign::Center) noexcept
        {
            int16_t x = area.x;
            int16_t y = area.y;

            if (hJustify == UiJustify::Center)
                x = (int16_t)(area.x + (area.w - size.w) / 2);
            else if (hJustify == UiJustify::End)
                x = (int16_t)(area.x + (area.w - size.w));

            if (vAlign == UiAlign::Center)
                y = (int16_t)(area.y + (area.h - size.h) / 2);
            else if (vAlign == UiAlign::End)
                y = (int16_t)(area.y + (area.h - size.h));

            return UiRect{x, y, size.w, size.h};
        }

        static inline UiRect centerIn(const UiRect &area, const UiSize &size) noexcept
        {
            return placeInside(area, size, UiJustify::Center, UiAlign::Center);
        }

        static inline UiRect inset(const UiRect &rc, int16_t l, int16_t t, int16_t r, int16_t b) noexcept
        {
            const int16_t w = rc.w - l - r;
            const int16_t h = rc.h - t - b;
            return UiRect{
                (int16_t)(rc.x + l),
                (int16_t)(rc.y + t),
                (int16_t)(w > 0 ? w : 0),
                (int16_t)(h > 0 ? h : 0)};
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

            int16_t totalW = 0;
            for (uint8_t i = 0; i < count; ++i)
                totalW += sizes[i].w;

            int16_t useGap = gap;
            if (useGap < 0)
                useGap = 0;

            int16_t startX = area.x;

            if (justify == UiJustify::SpaceBetween)
            {
                if (count > 1)
                    useGap = (area.w - totalW) / (count - 1);
                else
                    useGap = 0;
                if (useGap < 0)
                    useGap = 0;
            }
            else if (justify == UiJustify::SpaceEvenly)
            {
                useGap = (area.w - totalW) / (count + 1);
                if (useGap < 0)
                    useGap = 0;
                startX = (int16_t)(area.x + useGap);
            }
            else
            {
                int16_t contentW = totalW + useGap * (count > 1 ? (count - 1) : 0);
                if (justify == UiJustify::Center)
                    startX = (int16_t)(area.x + (area.w - contentW) / 2);
                else if (justify == UiJustify::End)
                    startX = (int16_t)(area.x + (area.w - contentW));
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

            int16_t totalH = 0;
            for (uint8_t i = 0; i < count; ++i)
                totalH += sizes[i].h;

            int16_t useGap = gap;
            if (useGap < 0)
                useGap = 0;

            int16_t startY = area.y;

            if (justify == UiJustify::SpaceBetween)
            {
                if (count > 1)
                    useGap = (area.h - totalH) / (count - 1);
                else
                    useGap = 0;
                if (useGap < 0)
                    useGap = 0;
            }
            else if (justify == UiJustify::SpaceEvenly)
            {
                useGap = (area.h - totalH) / (count + 1);
                if (useGap < 0)
                    useGap = 0;
                startY = (int16_t)(area.y + useGap);
            }
            else
            {
                int16_t contentH = totalH + useGap * (count > 1 ? (count - 1) : 0);
                if (justify == UiJustify::Center)
                    startY = (int16_t)(area.y + (area.h - contentH) / 2);
                else if (justify == UiJustify::End)
                    startY = (int16_t)(area.y + (area.h - contentH));
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

    [[nodiscard]] inline UiRect inset(const UiRect &rc, int16_t all) noexcept
    {
        return UiLayout::inset(rc, all);
    }

    [[nodiscard]] inline UiRect inset(const UiRect &rc, const UiInsets &in) noexcept
    {
        return UiLayout::inset(rc, in);
    }

    [[nodiscard]] inline UiRect takeTop(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
    {
        return UiLayout::takeTop(rc, h, gap);
    }

    [[nodiscard]] inline UiRect takeBottom(UiRect &rc, int16_t h, int16_t gap = 0) noexcept
    {
        return UiLayout::takeBottom(rc, h, gap);
    }

    [[nodiscard]] inline UiRect takeLeft(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
    {
        return UiLayout::takeLeft(rc, w, gap);
    }

    [[nodiscard]] inline UiRect takeRight(UiRect &rc, int16_t w, int16_t gap = 0) noexcept
    {
        return UiLayout::takeRight(rc, w, gap);
    }

    [[nodiscard]] inline UiRect placeInside(const UiRect &area, const UiSize &size,
                                            UiJustify hJustify = UiJustify::Center,
                                            UiAlign vAlign = UiAlign::Center) noexcept
    {
        return UiLayout::placeInside(area, size, hJustify, vAlign);
    }

    [[nodiscard]] inline UiRect centerIn(const UiRect &area, const UiSize &size) noexcept
    {
        return UiLayout::centerIn(area, size);
    }

    [[nodiscard]] inline UiRect inset(const UiRect &rc, int16_t l, int16_t t, int16_t r, int16_t b) noexcept
    {
        return UiLayout::inset(rc, l, t, r, b);
    }

    static inline void flowRow(const UiRect &area,
                               const UiSize *sizes,
                               UiRect *out,
                               uint8_t count,
                               int16_t gap,
                               UiJustify justify = UiJustify::Center,
                               UiAlign align = UiAlign::Center) noexcept
    {
        UiLayout::flowRow(area, sizes, out, count, gap, justify, align);
    }

    static inline void flowColumn(const UiRect &area,
                                  const UiSize *sizes,
                                  UiRect *out,
                                  uint8_t count,
                                  int16_t gap,
                                  UiJustify justify = UiJustify::Center,
                                  UiAlign align = UiAlign::Center) noexcept
    {
        UiLayout::flowColumn(area, sizes, out, count, gap, justify, align);
    }

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

        [[nodiscard]] UiRect &next(int16_t w, int16_t h)
        {
            assert(count < N);
            if (count >= N)
            {
                static UiRect empty{};
                empty = UiRect{area.x, area.y, 0, 0};
                return empty;
            }
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

        [[nodiscard]] const UiRect &operator[](uint8_t i) const
        {
            assert(i < count);
            if (i >= count)
            {
                static const UiRect empty{};
                return empty;
            }
            return rects[i];
        }
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

        [[nodiscard]] UiRect &next(int16_t w, int16_t h)
        {
            assert(count < N);
            if (count >= N)
            {
                static UiRect empty{};
                empty = UiRect{area.x, area.y, 0, 0};
                return empty;
            }
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

        [[nodiscard]] const UiRect &operator[](uint8_t i) const
        {
            assert(i < count);
            if (i >= count)
            {
                static const UiRect empty{};
                return empty;
            }
            return rects[i];
        }
    };

}

#include <pipGUI/Core/pipGUI.hpp>
#include <pipGUI/Core/Debug.hpp>

namespace pipgui
{

    namespace
    {
        constexpr int16_t kDirtyMergeGapPx = 5;

        struct RectBounds
        {
            int32_t x1;
            int32_t y1;
            int32_t x2;
            int32_t y2;
        };

        static inline RectBounds getRectBounds(const detail::DirtyRect &rect)
        {
            return {
                rect.x,
                rect.y,
                static_cast<int32_t>(rect.x) + rect.w,
                static_cast<int32_t>(rect.y) + rect.h,
            };
        }

        static inline int32_t rectArea(const detail::DirtyRect &rect)
        {
            return static_cast<int32_t>(rect.w) * rect.h;
        }

        static inline int32_t intervalGap(int32_t a1, int32_t a2, int32_t b1, int32_t b2)
        {
            if (a2 < b1)
                return b1 - a2;
            if (b2 < a1)
                return a1 - b2;
            return 0;
        }

        static inline bool containsDirtyRect(const detail::DirtyRect &outer, const detail::DirtyRect &inner)
        {
            const RectBounds outerBounds = getRectBounds(outer);
            const RectBounds innerBounds = getRectBounds(inner);
            return innerBounds.x1 >= outerBounds.x1 && innerBounds.x2 <= outerBounds.x2 &&
                   innerBounds.y1 >= outerBounds.y1 && innerBounds.y2 <= outerBounds.y2;
        }

        static inline detail::DirtyRect unionDirtyRects(const detail::DirtyRect &a, const detail::DirtyRect &b)
        {
            const RectBounds aBounds = getRectBounds(a);
            const RectBounds bBounds = getRectBounds(b);
            const int32_t x1 = std::min(aBounds.x1, bBounds.x1);
            const int32_t y1 = std::min(aBounds.y1, bBounds.y1);
            const int32_t x2 = std::max(aBounds.x2, bBounds.x2);
            const int32_t y2 = std::max(aBounds.y2, bBounds.y2);
            return {
                static_cast<int16_t>(x1),
                static_cast<int16_t>(y1),
                static_cast<int16_t>(x2 - x1),
                static_cast<int16_t>(y2 - y1),
            };
        }

        static inline bool shouldMergeDirtyRects(const detail::DirtyRect &a, const detail::DirtyRect &b)
        {
            const RectBounds aBounds = getRectBounds(a);
            const RectBounds bBounds = getRectBounds(b);
            const int32_t gapX = intervalGap(aBounds.x1, aBounds.x2, bBounds.x1, bBounds.x2);
            const int32_t gapY = intervalGap(aBounds.y1, aBounds.y2, bBounds.y1, bBounds.y2);

            if (gapX == 0 && gapY == 0)
                return true;

            return (gapX <= kDirtyMergeGapPx && gapY == 0) ||
                   (gapY <= kDirtyMergeGapPx && gapX == 0);
        }

        static void compactDirtyRects(detail::DirtyState &dirty)
        {
            bool merged = false;
            do
            {
                merged = false;
                for (uint8_t i = 0; i < dirty.count && !merged; ++i)
                {
                    for (uint8_t j = i + 1; j < dirty.count; ++j)
                    {
                        if (containsDirtyRect(dirty.rects[i], dirty.rects[j]))
                        {
                            dirty.rects[j] = dirty.rects[--dirty.count];
                            merged = true;
                            break;
                        }

                        if (containsDirtyRect(dirty.rects[j], dirty.rects[i]))
                        {
                            dirty.rects[i] = dirty.rects[j];
                            dirty.rects[j] = dirty.rects[--dirty.count];
                            merged = true;
                            break;
                        }

                        if (!shouldMergeDirtyRects(dirty.rects[i], dirty.rects[j]))
                            continue;

                        dirty.rects[i] = unionDirtyRects(dirty.rects[i], dirty.rects[j]);
                        dirty.rects[j] = dirty.rects[--dirty.count];
                        merged = true;
                        break;
                    }
                }
            } while (merged);
        }

        static uint8_t findBestDirtyMergeTarget(const detail::DirtyState &dirty, const detail::DirtyRect &rect)
        {
            uint8_t bestIndex = 0;
            int32_t bestCost = 0x7FFFFFFF;

            for (uint8_t i = 0; i < dirty.count; ++i)
            {
                const detail::DirtyRect merged = unionDirtyRects(dirty.rects[i], rect);
                const int32_t cost = rectArea(merged) - rectArea(dirty.rects[i]);
                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestIndex = i;
                }
            }

            return bestIndex;
        }
    }

    static inline void applyClipState(pipcore::Sprite *spr,
                                      bool enabled,
                                      int16_t x,
                                      int16_t y,
                                      int16_t w,
                                      int16_t h)
    {
        if (!spr)
            return;

        int32_t curX = 0;
        int32_t curY = 0;
        int32_t curW = 0;
        int32_t curH = 0;
        spr->getClipRect(&curX, &curY, &curW, &curH);

        if (!enabled)
        {
            if (curX == 0 && curY == 0 && curW == spr->width() && curH == spr->height())
                return;
            spr->setClipRect(0, 0, spr->width(), spr->height());
            return;
        }
        if (curX == x && curY == y && curW == w && curH == h)
            return;
        spr->setClipRect(x, y, w, h);
    }

    uint16_t GUI::rgb(uint8_t r, uint8_t g, uint8_t b) const
    {
        return pipcore::Sprite::color565(r, g, b);
    }

    pipcore::Sprite *GUI::getDrawTarget()
    {
        pipcore::Sprite *spr = (_flags.inSpritePass && _flags.spriteEnabled)
                                   ? (_render.activeSprite ? _render.activeSprite : &_render.sprite)
                                   : &_render.sprite;
        applyClipState(spr, _clip.enabled, _clip.x, _clip.y, _clip.w, _clip.h);
        return spr;
    }

    void GUI::applyClip(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_clip.enabled)
        {
            _clip.enabled = true;
            _clip.x = x;
            _clip.y = y;
            _clip.w = w;
            _clip.h = h;
            return;
        }

        const int32_t oldX1 = _clip.x;
        const int32_t oldY1 = _clip.y;
        const int32_t oldX2 = (int32_t)_clip.x + _clip.w;
        const int32_t oldY2 = (int32_t)_clip.y + _clip.h;
        const int32_t newX1 = x;
        const int32_t newY1 = y;
        const int32_t newX2 = (int32_t)x + w;
        const int32_t newY2 = (int32_t)y + h;

        const int32_t clipX1 = std::max(oldX1, newX1);
        const int32_t clipY1 = std::max(oldY1, newY1);
        const int32_t clipX2 = std::min(oldX2, newX2);
        const int32_t clipY2 = std::min(oldY2, newY2);

        _clip.enabled = true;
        _clip.x = (int16_t)clipX1;
        _clip.y = (int16_t)clipY1;
        _clip.w = (int16_t)std::max<int32_t>(0, clipX2 - clipX1);
        _clip.h = (int16_t)std::max<int32_t>(0, clipY2 - clipY1);
    }

    void GUI::clearClip()
    {
        _clip.enabled = false;
        _clip.x = 0;
        _clip.y = 0;
        _clip.w = 0;
        _clip.h = 0;
    }

    int16_t GUI::AutoX(int32_t contentWidth) const
    {
        int16_t availW = _render.screenWidth;

        if (contentWidth > availW)
            availW = (int16_t)contentWidth;
        return (availW - (int16_t)contentWidth) / 2;
    }

    int16_t GUI::AutoY(int32_t contentHeight) const
    {
        int16_t sb = statusBarHeight();
        int16_t top = (_flags.statusBarEnabled && _status.pos == Top) ? sb : 0;
        int16_t availH = _render.screenHeight - ((_flags.statusBarEnabled && (_status.pos == Top || _status.pos == Bottom)) ? sb : 0);

        if (contentHeight > availH)
            availH = (int16_t)contentHeight;
        return top + (availH - (int16_t)contentHeight) / 2;
    }

    void GUI::clear(uint16_t color)
    {
        setBackgroundColorCache(color);

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr || !spr->getBuffer())
            return;

        int32_t clipX = 0;
        int32_t clipY = 0;
        int32_t clipW = 0;
        int32_t clipH = 0;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        if (clipX == 0 && clipY == 0 && clipW == spr->width() && clipH == spr->height())
            spr->fillScreen(color);
        else
            spr->fillRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH, color);

        if (_disp.display && !_flags.inSpritePass)
            invalidateRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_disp.display || !_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        int32_t x2 = static_cast<int32_t>(x) + w;
        int32_t y2 = static_cast<int32_t>(y) + h;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x2 > _render.screenWidth)
            x2 = _render.screenWidth;
        if (y2 > _render.screenHeight)
            y2 = _render.screenHeight;

        w = static_cast<int16_t>(x2 - x);
        h = static_cast<int16_t>(y2 - y);
        if (w <= 0 || h <= 0)
            return;

        const detail::DirtyRect rect = {x, y, w, h};

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            if (containsDirtyRect(_dirty.rects[i], rect))
                return;
        }

        if (_dirty.count < DIRTY_RECT_MAX)
        {
            _dirty.rects[_dirty.count++] = rect;
            compactDirtyRects(_dirty);
            return;
        }

        const uint8_t bestIndex = findBestDirtyMergeTarget(_dirty, rect);
        _dirty.rects[bestIndex] = unionDirtyRects(_dirty.rects[bestIndex], rect);
        compactDirtyRects(_dirty);
    }

    void GUI::flushDirty()
    {
        if (_dirty.count == 0)
            return;
        if (_flags.screenTransition || _flags.errorActive || _flags.notifActive || _flags.toastActive)
        {
            _flags.needRedraw = 1;
            _dirty.count = 0;
            Debug::clearRects();
            return;
        }
        if (!_disp.display || !_flags.spriteEnabled)
        {
            _dirty.count = 0;
            return;
        }

        const int16_t sw = _render.sprite.width();
        const int16_t sh = _render.sprite.height();
        uint16_t *buf = (uint16_t *)_render.sprite.getBuffer();
        const int32_t stride = sw;
        const bool debugDirty = Debug::dirtyRectEnabled();

        const auto clipRect = [&](const detail::DirtyRect &r, int16_t &x0, int16_t &y0, int16_t &w, int16_t &h) -> bool
        {
            if (r.w <= 0 || r.h <= 0)
                return false;

            x0 = r.x;
            y0 = r.y;
            w = r.w;
            h = r.h;

            if (x0 < 0)
            {
                w += x0;
                x0 = 0;
            }
            if (y0 < 0)
            {
                h += y0;
                y0 = 0;
            }
            if (w <= 0 || h <= 0)
                return false;

            if (x0 + w > sw)
                w = (int16_t)(sw - x0);
            if (y0 + h > sh)
                h = (int16_t)(sh - y0);
            return w > 0 && h > 0;
        };

        if (debugDirty)
        {
            for (uint8_t i = 0; i < _dirty.count; ++i)
            {
                int16_t x0 = 0;
                int16_t y0 = 0;
                int16_t w = 0;
                int16_t h = 0;
                if (!clipRect(_dirty.rects[i], x0, y0, w, h))
                    continue;
                Debug::recordDirtyRect(x0, y0, w, h);
            }
        }

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            int16_t x0 = 0;
            int16_t y0 = 0;
            int16_t w = 0;
            int16_t h = 0;
            if (!clipRect(_dirty.rects[i], x0, y0, w, h))
                continue;

            if (debugDirty)
                Debug::drawOverlay(buf, stride, x0, y0, w, h);

            presentSprite(x0, y0, w, h, "present");
        }

        _dirty.count = 0;

        Debug::clearRects();
    }
}

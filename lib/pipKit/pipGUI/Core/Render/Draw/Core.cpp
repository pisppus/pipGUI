#include <pipGUI/core/api/pipGUI.hpp>
#include <pipGUI/core/Debug.hpp>
#include <pipGUI/core/utils/Colors.hpp>
#include <pipCore/Graphics/Sprite.hpp>
#include "Blend.hpp"

namespace pipgui
{

    static inline void applyClip(pipcore::Sprite *spr,
                                 bool enabled,
                                 int16_t x,
                                 int16_t y,
                                 int16_t w,
                                 int16_t h)
    {
        if (!spr)
            return;
        if (!enabled)
        {
            spr->setClipRect(0, 0, spr->width(), spr->height());
            return;
        }
        spr->setClipRect(x, y, w, h);
    }

    uint16_t GUI::rgb(uint8_t r, uint8_t g, uint8_t b) const
    {
        return pipcore::Sprite::color565(r, g, b);
    }

    pipcore::Sprite *GUI::getDrawTarget()
    {
        pipcore::Sprite *spr = (_flags.renderToSprite && _flags.spriteEnabled)
                                   ? (_render.activeSprite ? _render.activeSprite : &_render.sprite)
                                   : &_render.sprite;
        applyClip(spr, _clip.enabled, _clip.x, _clip.y, _clip.w, _clip.h);
        return spr;
    }

    void GUI::setClip(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        _clip.enabled = true;
        _clip.x = x;
        _clip.y = y;
        _clip.w = w;
        _clip.h = h;
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
        int16_t sb = statusBarHeight();
        int16_t left = (_flags.statusBarEnabled && _status.pos == Left) ? sb : 0;
        int16_t availW = _render.screenWidth - ((_flags.statusBarEnabled && (_status.pos == Left || _status.pos == Right)) ? sb : 0);

        if (contentWidth > availW)
            availW = (int16_t)contentWidth;
        return left + (availW - (int16_t)contentWidth) / 2;
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
        _render.bgColor = detail::color565To888(color);

        if (!_flags.spriteEnabled)
            return;

        pipcore::Sprite *spr = getDrawTarget();
        if (!spr)
            return;

        uint16_t *buf = (uint16_t *)spr->getBuffer();
        if (!buf)
            return;

        const int16_t w = spr->width();
        const int16_t h = spr->height();
        if (w <= 0 || h <= 0)
            return;

        int32_t clipX = 0, clipY = 0, clipW = w, clipH = h;
        spr->getClipRect(&clipX, &clipY, &clipW, &clipH);
        if (clipW <= 0 || clipH <= 0)
            return;

        const uint16_t v = pipcore::Sprite::swap16(color);
        for (int32_t yy = 0; yy < clipH; ++yy)
        {
            const int32_t row = (int32_t)(clipY + yy) * (int32_t)w;
            uint16_t *p = buf + row + clipX;
            for (int32_t xx = 0; xx < clipW; ++xx)
                p[xx] = v;
        }

        if (_disp.display && _flags.spriteEnabled && !_flags.renderToSprite)
            invalidateRect((int16_t)clipX, (int16_t)clipY, (int16_t)clipW, (int16_t)clipH);
    }

    void GUI::invalidateRect(int16_t x, int16_t y, int16_t w, int16_t h)
    {
        if (!_disp.display || !_flags.spriteEnabled || w <= 0 || h <= 0)
            return;

        if (Debug::dirtyRectEnabled())
            Debug::recordDirtyRect(x, y, w, h);

        int16_t x2 = x + w;
        int16_t y2 = y + h;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x2 > (int16_t)_render.screenWidth)
            x2 = (int16_t)_render.screenWidth;
        if (y2 > (int16_t)_render.screenHeight)
            y2 = (int16_t)_render.screenHeight;

        w = x2 - x;
        h = y2 - y;
        if (w <= 0 || h <= 0)
            return;

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            DirtyRect &dr = _dirty.rects[i];

            if (x > dr.x + dr.w || x2 < dr.x || y > dr.y + dr.h || y2 < dr.y)
                continue;

            int16_t nx1 = (x < dr.x) ? x : dr.x;
            int16_t ny1 = (y < dr.y) ? y : dr.y;
            int16_t nx2 = (x2 > dr.x + dr.w) ? x2 : (dr.x + dr.w);
            int16_t ny2 = (y2 > dr.y + dr.h) ? y2 : (dr.y + dr.h);

            dr.x = nx1;
            dr.y = ny1;
            dr.w = nx2 - nx1;
            dr.h = ny2 - ny1;

            for (uint8_t j = i + 1; j < _dirty.count;)
            {
                DirtyRect &next = _dirty.rects[j];
                bool overlap = !(dr.x > next.x + next.w || dr.x + dr.w < next.x ||
                                 dr.y > next.y + next.h || dr.y + dr.h < next.y);
                if (!overlap)
                {
                    ++j;
                    continue;
                }
                int16_t nx1 = (dr.x < next.x) ? dr.x : next.x;
                int16_t ny1 = (dr.y < next.y) ? dr.y : next.y;
                int16_t nx2 = (dr.x + dr.w > next.x + next.w) ? (dr.x + dr.w) : (next.x + next.w);
                int16_t ny2 = (dr.y + dr.h > next.y + next.h) ? (dr.y + dr.h) : (next.y + next.h);

                dr.x = nx1;
                dr.y = ny1;
                dr.w = nx2 - nx1;
                dr.h = ny2 - ny1;
                _dirty.rects[j] = _dirty.rects[--_dirty.count];
            }
            return;
        }

        if (_dirty.count < DIRTY_RECT_MAX)
        {
            _dirty.rects[_dirty.count++] = {x, y, w, h};
            return;
        }

        DirtyRect &first = _dirty.rects[0];
        int16_t total_x2 = first.x + first.w;
        int16_t total_y2 = first.y + first.h;

        for (uint8_t i = 1; i < _dirty.count; ++i)
        {
            if (_dirty.rects[i].x < first.x)
                first.x = _dirty.rects[i].x;
            if (_dirty.rects[i].y < first.y)
                first.y = _dirty.rects[i].y;
            int16_t r_x2 = _dirty.rects[i].x + _dirty.rects[i].w;
            int16_t r_y2 = _dirty.rects[i].y + _dirty.rects[i].h;
            if (r_x2 > total_x2)
                total_x2 = r_x2;
            if (r_y2 > total_y2)
                total_y2 = r_y2;
        }

        if (x < first.x)
            first.x = x;
        if (y < first.y)
            first.y = y;
        if (x2 > total_x2)
            total_x2 = x2;
        if (y2 > total_y2)
            total_y2 = y2;

        first.w = total_x2 - first.x;
        first.h = total_y2 - first.y;
        _dirty.count = 1;
    }

    void GUI::flushDirty()
    {
        if (_dirty.count == 0)
            return;
        if (!_disp.display || !_flags.spriteEnabled)
        {
            _dirty.count = 0;
            return;
        }

        const int16_t sw = _render.sprite.width();
        const int16_t sh = _render.sprite.height();
        uint16_t *buf = (uint16_t *)_render.sprite.getBuffer();
        const int32_t stride = sw;

        for (uint8_t i = 0; i < _dirty.count; ++i)
        {
            DirtyRect r = _dirty.rects[i];
            if (r.w <= 0 || r.h <= 0)
                continue;

            int16_t x0 = r.x;
            int16_t y0 = r.y;
            int16_t w = r.w;
            int16_t h = r.h;

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
                continue;

            if (x0 + w > sw)
                w = (int16_t)(sw - x0);
            if (y0 + h > sh)
                h = (int16_t)(sh - y0);
            if (w <= 0 || h <= 0)
                continue;

            if (Debug::dirtyRectEnabled())
            {
                Debug::drawOverlay(buf, stride, x0, y0, w, h);
            }

            _render.sprite.writeToDisplay(*_disp.display, x0, y0, w, h);
        }

        _dirty.count = 0;

        Debug::clearRects();
    }
}

#pragma once

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <new>
#include <utility>
#include <pipCore/Platform.hpp>
#include <pipCore/Platforms/Select.hpp>
#include <pipGUI/Core/API/Common.hpp>

namespace pipgui
{
    namespace detail
    {
        pipcore::Platform *resolvePlatform(GUI *gui) noexcept;

        static inline void *alloc(pipcore::Platform *plat, size_t bytes, pipcore::AllocCaps caps) noexcept
        {
            pipcore::Platform *p = plat ? plat : pipcore::GetPlatform();
            if (!p)
                return nullptr;
            void *ptr = p->alloc(bytes, caps);
            if (ptr)
                return ptr;
            if (caps != pipcore::AllocCaps::Default)
                return p->alloc(bytes, pipcore::AllocCaps::Default);
            return nullptr;
        }

        static inline void free(pipcore::Platform *plat, void *ptr) noexcept
        {
            if (!ptr)
                return;
            pipcore::Platform *p = plat ? plat : pipcore::GetPlatform();
            if (p)
                p->free(ptr);
        }

        static inline bool assignString(String &text, const char *src)
        {
            if (!src)
            {
                text = String();
                return true;
            }

            text = src;
            return text.length() > 0 || src[0] == '\0';
        }

        template <typename T>
        static bool ensureCapacity(pipcore::Platform *plat, T *&arr, uint8_t &capacity, uint8_t need) noexcept
        {
            if (capacity >= need && arr)
                return true;

            if (capacity >= need && !arr)
            {
                T *newArr = static_cast<T *>(alloc(plat, sizeof(T) * capacity, pipcore::AllocCaps::Default));
                if (!newArr)
                    return false;
                for (uint8_t i = 0; i < capacity; ++i)
                    new (&newArr[i]) T();
                arr = newArr;
                return true;
            }

            uint8_t newCap = capacity ? static_cast<uint8_t>(std::max<uint8_t>(capacity * 2, need))
                                      : static_cast<uint8_t>(std::max<uint8_t>(4, need));
            T *newArr = static_cast<T *>(alloc(plat, sizeof(T) * newCap, pipcore::AllocCaps::Default));
            if (!newArr)
                return false;

            for (uint8_t i = 0; i < newCap; ++i)
                new (&newArr[i]) T();

            if (arr)
            {
                for (uint8_t i = 0; i < capacity; ++i)
                {
                    newArr[i] = std::move(arr[i]);
                    arr[i].~T();
                }
                free(plat, arr);
            }

            arr = newArr;
            capacity = newCap;
            return true;
        }

        static inline pipcore::DisplayConfig defaultDisplayConfig() noexcept
        {
            pipcore::DisplayConfig cfg;
            cfg.mosi = -1;
            cfg.sclk = -1;
            cfg.cs = -1;
            cfg.dc = -1;
            cfg.rst = -1;
            cfg.width = 0;
            cfg.height = 0;
            cfg.hz = 80000000;
            cfg.order = 0;
            cfg.invert = true;
            cfg.swap = false;
            cfg.xOffset = 0;
            cfg.yOffset = 0;
            return cfg;
        }

        struct FluentLifetime
        {
        protected:
            GUI *_gui;
            bool _consumed;

            explicit FluentLifetime(GUI *gui) noexcept
                : _gui(gui), _consumed(false) {}

            FluentLifetime(const FluentLifetime &) = delete;
            FluentLifetime &operator=(const FluentLifetime &) = delete;

            FluentLifetime(FluentLifetime &&other) noexcept
                : _gui(other._gui), _consumed(other._consumed)
            {
                other._gui = nullptr;
                other._consumed = true;
            }

            FluentLifetime &operator=(FluentLifetime &&other) noexcept
            {
                if (this != &other)
                {
                    _gui = other._gui;
                    _consumed = other._consumed;
                    other._gui = nullptr;
                    other._consumed = true;
                }
                return *this;
            }

            bool canMutate() const noexcept
            {
                return !_consumed;
            }
        };

        template <typename T>
        struct OwnedHeapArray
        {
            pipcore::Platform *plat = nullptr;
            T *data = nullptr;

            OwnedHeapArray() = default;

            explicit OwnedHeapArray(pipcore::Platform *platform) noexcept
                : plat(platform) {}

            OwnedHeapArray(const OwnedHeapArray &) = delete;
            OwnedHeapArray &operator=(const OwnedHeapArray &) = delete;

            OwnedHeapArray(OwnedHeapArray &&other) noexcept
                : plat(other.plat), data(other.data)
            {
                other.plat = nullptr;
                other.data = nullptr;
            }

            OwnedHeapArray &operator=(OwnedHeapArray &&other) noexcept
            {
                if (this != &other)
                {
                    reset();
                    plat = other.plat;
                    data = other.data;
                    other.plat = nullptr;
                    other.data = nullptr;
                }
                return *this;
            }

            ~OwnedHeapArray()
            {
                reset();
            }

            void bind(pipcore::Platform *platform) noexcept
            {
                plat = platform;
            }

            void reset() noexcept
            {
                if (data)
                {
                    free(plat, data);
                    data = nullptr;
                }
            }

            template <typename U>
            T *copyFrom(std::initializer_list<U> items) noexcept
            {
                reset();
                if (items.size() == 0)
                    return nullptr;

                T *copy = static_cast<T *>(alloc(plat, sizeof(T) * items.size(), pipcore::AllocCaps::Default));
                if (!copy)
                    return nullptr;

                size_t i = 0;
                for (const U &item : items)
                    copy[i++] = item;

                data = copy;
                return data;
            }
        };
    }
}

#pragma once

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <new>
#include <optional>
#include <utility>
#include <pipCore/Platform.hpp>
#include <pipCore/Platforms/Select.hpp>
#include <pipGUI/Core/Types.hpp>

namespace pipgui
{
    namespace detail
    {
        pipcore::Platform *resolvePlatform(GUI *gui) noexcept;

        [[nodiscard]] inline void *alloc(pipcore::Platform *plat, size_t bytes, pipcore::AllocCaps caps) noexcept
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

        inline void free(pipcore::Platform *plat, void *ptr) noexcept
        {
            if (!ptr)
                return;
            pipcore::Platform *p = plat ? plat : pipcore::GetPlatform();
            if (p)
                p->free(ptr);
        }

        [[nodiscard]] inline bool assignString(String &text, const char *src)
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
        [[nodiscard]] static bool ensureCapacity(pipcore::Platform *plat, T *&arr, uint8_t &capacity, uint8_t need) noexcept
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

        [[nodiscard]] inline pipcore::DisplayConfig defaultDisplayConfig() noexcept
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

        inline void assignOptionalColor(std::optional<uint16_t> &slot, int32_t value) noexcept
        {
            if (value < 0)
                slot.reset();
            else
                slot = static_cast<uint16_t>(value);
        }

        [[nodiscard]] constexpr int16_t optionalColor16(const std::optional<uint16_t> &slot) noexcept
        {
            return slot ? static_cast<int16_t>(*slot) : static_cast<int16_t>(-1);
        }

        [[nodiscard]] constexpr int32_t optionalColor32(const std::optional<uint16_t> &slot) noexcept
        {
            return slot ? static_cast<int32_t>(*slot) : -1;
        }

        template <typename T>
        [[nodiscard]] constexpr T valueOr(const std::optional<T> &slot, T fallback) noexcept
        {
            return slot ? *slot : fallback;
        }

        template <bool IsUpdate, typename UpdateFn, typename DrawFn>
        inline void callByMode(UpdateFn &&updateFn, DrawFn &&drawFn)
        {
            if constexpr (IsUpdate)
                std::forward<UpdateFn>(updateFn)();
            else
                std::forward<DrawFn>(drawFn)();
        }

        struct FluentLifetime
        {
        protected:
            GUI *_gui;
            bool _consumed;

            explicit FluentLifetime(GUI *gui) noexcept
                : _gui(gui), _consumed(false) {}

            FluentLifetime(const FluentLifetime &other) noexcept
                : _gui(other._gui), _consumed(false)
            {
            }

            FluentLifetime &operator=(const FluentLifetime &other) noexcept
            {
                if (this != &other)
                {
                    _gui = other._gui;
                    _consumed = false;
                }
                return *this;
            }

            FluentLifetime(FluentLifetime &&other) noexcept
                : _gui(std::exchange(other._gui, nullptr)),
                  _consumed(std::exchange(other._consumed, true))
            {
            }

            FluentLifetime &operator=(FluentLifetime &&other) noexcept
            {
                if (this != &other)
                {
                    _gui = std::exchange(other._gui, nullptr);
                    _consumed = std::exchange(other._consumed, true);
                }
                return *this;
            }

            [[nodiscard]] bool canMutate() const noexcept
            {
                return !_consumed;
            }

            [[nodiscard]] bool beginCommit() noexcept
            {
                if (_consumed || !_gui)
                    return false;
                _consumed = true;
                return true;
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
                : plat(std::exchange(other.plat, nullptr)),
                  data(std::exchange(other.data, nullptr))
            {
            }

            OwnedHeapArray &operator=(OwnedHeapArray &&other) noexcept
            {
                if (this != &other)
                {
                    reset();
                    plat = std::exchange(other.plat, nullptr);
                    data = std::exchange(other.data, nullptr);
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
            [[nodiscard]] T *copyFrom(std::initializer_list<U> items) noexcept
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

            [[nodiscard]] T *copyFromPtr(const T *items, size_t count) noexcept
            {
                reset();
                if (!items || count == 0)
                    return nullptr;

                T *copy = static_cast<T *>(alloc(plat, sizeof(T) * count, pipcore::AllocCaps::Default));
                if (!copy)
                    return nullptr;

                for (size_t i = 0; i < count; ++i)
                    copy[i] = items[i];

                data = copy;
                return data;
            }
        };
    }
}


#pragma once

#include <pipCore/Platform.hpp>

namespace pipcore::esp32::services
{
    class Heap
    {
    public:
        void *alloc(size_t bytes, AllocCaps caps) const noexcept;
        void free(void *ptr) const noexcept;
        [[nodiscard]] uint32_t freeHeapTotal() const noexcept;
        [[nodiscard]] uint32_t freeHeapInternal() const noexcept;
        [[nodiscard]] uint32_t largestFreeBlock() const noexcept;
        [[nodiscard]] uint32_t minFreeHeap() const noexcept;
    };
}

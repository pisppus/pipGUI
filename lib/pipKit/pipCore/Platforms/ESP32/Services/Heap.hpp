#pragma once

#include <pipCore/Platform.hpp>

namespace pipcore::esp32::services
{
    class Heap
    {
    public:
        void *alloc(size_t bytes, AllocCaps caps) const;
        void free(void *ptr) const;
        uint32_t freeHeapTotal() const;
        uint32_t freeHeapInternal() const;
        uint32_t largestFreeBlock() const;
        uint32_t minFreeHeap() const;
    };
}

#include <pipCore/Platforms/ESP32/Services/Heap.hpp>
#include <esp_heap_caps.h>
#include <esp_system.h>

namespace pipcore::esp32::services
{
    void *Heap::alloc(size_t bytes, AllocCaps caps) const noexcept
    {
        if (bytes == 0)
            return nullptr;

        if (caps == AllocCaps::PreferExternal)
        {
            void *ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (ptr)
                return ptr;
        }

        return heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    }

    void Heap::free(void *ptr) const noexcept
    {
        heap_caps_free(ptr);
    }

    uint32_t Heap::freeHeapTotal() const noexcept
    {
        return esp_get_free_heap_size();
    }

    uint32_t Heap::freeHeapInternal() const noexcept
    {
        return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    }

    uint32_t Heap::largestFreeBlock() const noexcept
    {
        return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    }

    uint32_t Heap::minFreeHeap() const noexcept
    {
        return esp_get_minimum_free_heap_size();
    }
}

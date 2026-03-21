#pragma once

#include <pipGUI/Core/pipGUI.hpp>

namespace pipgui::detail
{
#if (PIPGUI_SCREENSHOT_MODE == 2)
    [[nodiscard]] bool decodeScreenshotPayloadTo565(fs::File &f, uint16_t w, uint16_t h, uint16_t *dst, uint32_t payloadSize, uint32_t *outPayloadCrc32) noexcept;
    [[nodiscard]] bool encodeScreenshotPayload565ToFile(fs::File &f, const uint16_t *src565, uint16_t w, uint16_t h, uint32_t &outBytes, uint32_t *outPayloadCrc32) noexcept;
#endif
}

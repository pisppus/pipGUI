#pragma once

#include <cstdint>

namespace pipcore::ota
{
    struct Options
    {
        uint16_t currentVerMajor = 0;
        uint16_t currentVerMinor = 0;
        uint16_t currentVerPatch = 0;
        const char *ed25519PubkeyHex = nullptr;
    };

    enum class Channel : uint8_t
    {
        Stable = 0,
        Beta = 1,
        Custom = 2,
    };

    enum class CheckMode : uint8_t
    {
        NewerOnly = 0,
        AllowDowngrade = 1,
    };

    enum class State : uint8_t
    {
        Idle = 0,
        WifiStarting = 1,
        FetchingManifest = 2,
        UpdateAvailable = 3,
        Downloading = 4,
        Installing = 5,
        Success = 6,
        Error = 7,
        UpToDate = 8,
    };

    enum class Error : uint8_t
    {
        None = 0,
        WifiNotEnabled = 1,
        WifiNotConnected = 2,
        HttpBeginFailed = 3,
        HttpStatusNotOk = 4,
        ManifestTooLarge = 5,
        ManifestParseFailed = 6,
        PayloadSizeMismatch = 7,
        FlashLayoutInvalid = 8,
        RollbackUnavailable = 9,
        UpdateBeginFailed = 10,
        UpdateWriteFailed = 11,
        HashMismatch = 12,
        SignatureMissing = 13,
        SignatureInvalid = 14,
        UpdateEndFailed = 15,
        TimeSyncFailed = 16,
    };

    struct Manifest
    {
        uint16_t verMajor = 0;
        uint16_t verMinor = 0;
        uint16_t verPatch = 0;
        char version[16] = {};
        uint32_t size = 0;
        char url[192] = {};
        uint8_t sha256[32] = {};
        uint8_t sigEd25519[64] = {};
        bool hasSig = false;
    };

    struct Status
    {
        State state = State::Idle;
        Error error = Error::None;
        int httpCode = 0;
        int platformCode = 0;
        uint32_t downloaded = 0;
        uint32_t total = 0;
        uint32_t lastChangeMs = 0;
        int8_t versionCmp = 0;
        Channel channel = Channel::Custom;
        Manifest manifest = {};
    };

    using StatusCallback = void (*)(const Status &st, void *user);

    void markAppValid() noexcept;

    void configure(const char *manifestUrl,
                   const Options &opt,
                   StatusCallback cb = nullptr,
                   void *user = nullptr) noexcept;

    void configureChannels(const char *stableUrl,
                           const char *betaUrl,
                           Channel initial,
                           const Options &opt,
                           StatusCallback cb = nullptr,
                           void *user = nullptr) noexcept;

    void setChannel(Channel ch) noexcept;
    [[nodiscard]] Channel channel() noexcept;

    void requestCheck() noexcept;
    void requestCheck(CheckMode mode) noexcept;

    void requestInstall() noexcept;
    void requestRollback() noexcept;
    void cancel() noexcept;
    void service() noexcept;

    [[nodiscard]] const Status &status() noexcept;
}

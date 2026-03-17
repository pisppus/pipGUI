#pragma once

#include <pipCore/Platforms/ESP32/Services/Wifi.hpp>
#include <pipCore/Systems/Update/Ota.hpp>

#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#include <mbedtls/sha256.h>

namespace pipcore::esp32::services
{
    class Ota
    {
    public:
        Ota() = default;

        void bindWifi(Wifi *wifi) noexcept { _wifi = wifi; }

        void markAppValid() noexcept;

        void configure(const char *manifestUrl,
                       const pipcore::ota::Options &opt,
                       pipcore::ota::StatusCallback cb,
                       void *user) noexcept;

        void configureChannels(const char *stableUrl,
                               const char *betaUrl,
                               pipcore::ota::Channel initial,
                               const pipcore::ota::Options &opt,
                               pipcore::ota::StatusCallback cb,
                               void *user) noexcept;

        void setChannel(pipcore::ota::Channel ch) noexcept;
        [[nodiscard]] pipcore::ota::Channel channel() const noexcept { return _st.channel; }

        void requestCheck(pipcore::ota::CheckMode mode) noexcept;
        void requestInstall() noexcept;
        void requestRollback() noexcept;
        void cancel() noexcept;

        void service(uint32_t nowMs) noexcept;

        [[nodiscard]] const pipcore::ota::Status &status() const noexcept { return _st; }

    private:
        void publish() noexcept;
        void setState(pipcore::ota::State s, uint32_t nowMs) noexcept;
        void setError(pipcore::ota::Error e, uint32_t nowMs, int httpCode = 0, int platformCode = 0) noexcept;

        void wifiAcquire() noexcept;
        void wifiRelease() noexcept;
        [[nodiscard]] bool wifiReady() const noexcept;

        [[nodiscard]] bool beginHttp(const char *url, bool forManifest) noexcept;
        void stopHttp() noexcept;
        [[nodiscard]] bool readHttpBody() noexcept;
        [[nodiscard]] bool parseManifest(uint32_t nowMs) noexcept;
        [[nodiscard]] bool verifyManifestSignature(uint32_t nowMs) noexcept;
        [[nodiscard]] bool beginFirmwareDownload(uint32_t nowMs) noexcept;
        [[nodiscard]] bool stepFirmwareDownload(uint32_t nowMs) noexcept;

        [[nodiscard]] bool parseVersion(const char *version, uint16_t &maj, uint16_t &min, uint16_t &pat) noexcept;
        [[nodiscard]] int compareVersion(uint16_t aMaj, uint16_t aMin, uint16_t aPat,
                                         uint16_t bMaj, uint16_t bMin, uint16_t bPat) noexcept;

        Wifi *_wifi = nullptr;

        pipcore::ota::Status _st = {};
        pipcore::ota::Options _opt = {};
        pipcore::ota::StatusCallback _cb = nullptr;
        void *_cbUser = nullptr;

        char _manifestUrl[192] = {};
        char _manifestUrlStable[192] = {};
        char _manifestUrlBeta[192] = {};

        bool _wantCheck = false;
        pipcore::ota::CheckMode _checkMode = pipcore::ota::CheckMode::NewerOnly;
        bool _wantInstall = false;
        bool _wantRollback = false;
        bool _wantCancel = false;

        bool _wifiOwned = false;
        bool _wifiNextDownload = false;

        uint8_t _pubkey[32] = {};
        bool _pubkeyOk = false;

        static constexpr size_t kMaxManifestBuf = 4096;
        uint8_t _manifestBuf[kMaxManifestBuf + 1] = {};
        size_t _manifestLen = 0;
        size_t _manifestLimit = 0;

        struct HttpState
        {
            HTTPClient http;
            WiFiClientSecure client;
            bool active = false;
            bool forManifest = false;
            uint32_t bodyRead = 0;
            uint32_t bodyTotal = 0;

            mbedtls_sha256_context sha = {};
            bool shaInit = false;
            bool updateStarted = false;

            bool timeSyncStarted = false;
            uint32_t timeSyncStartMs = 0;
        };

        HttpState _http = {};

        bool _bootInit = false;
        bool _bootPendingVerify = false;
        bool _bootConfirmed = false;
        uint32_t _bootStartMs = 0;
    };
}

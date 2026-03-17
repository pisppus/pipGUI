#include <pipCore/Platforms/ESP32/Services/Ota.hpp>

#include <Arduino.h>
#include <esp_ota_ops.h>

#include <cstdio>
#include <cstring>
#include <ctime>

#if __has_include(<sodium.h>)
#include <sodium.h>
#define PIPCORE_OTA_HAVE_SODIUM 1
#else
#define PIPCORE_OTA_HAVE_SODIUM 0
#endif

namespace pipcore::esp32::services
{
    namespace
    {
        constexpr const char *kSigPrefix = "pipgui-ota-manifest-v2";
        constexpr size_t kManifestLimitBytes = 2048;
        constexpr time_t kMinValidUnixTime = 1700000000; // 2023-11-14

        static constexpr const char kRootCaLetsEncryptBundle[] = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----

-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----)";

        [[nodiscard]] bool isHex(char c) noexcept
        {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }

        [[nodiscard]] uint8_t hexNibble(char c) noexcept
        {
            if (c >= '0' && c <= '9')
                return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f')
                return static_cast<uint8_t>(10 + (c - 'a'));
            if (c >= 'A' && c <= 'F')
                return static_cast<uint8_t>(10 + (c - 'A'));
            return 0;
        }

        [[nodiscard]] bool parseHexBytes(const char *s, size_t hexLen, uint8_t *out, size_t outLen) noexcept
        {
            if (!s || !out)
                return false;
            if (hexLen != outLen * 2u)
                return false;
            for (size_t i = 0; i < hexLen; i += 2)
            {
                const char a = s[i];
                const char b = s[i + 1];
                if (!isHex(a) || !isHex(b))
                    return false;
                out[i / 2] = static_cast<uint8_t>((hexNibble(a) << 4) | hexNibble(b));
            }
            return true;
        }

        [[nodiscard]] const char *skipWs(const char *p) noexcept
        {
            while (p && *p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
                ++p;
            return p;
        }

        [[nodiscard]] bool jsonFindKey(const char *json, const char *key, const char *&outAfterColon) noexcept
        {
            outAfterColon = nullptr;
            if (!json || !key || !key[0])
                return false;

            char needle[64];
            const int n = std::snprintf(needle, sizeof(needle), "\"%s\"", key);
            if (n <= 0 || static_cast<size_t>(n) >= sizeof(needle))
                return false;

            const char *p = std::strstr(json, needle);
            if (!p)
                return false;
            p += n;
            p = skipWs(p);
            if (!p || *p != ':')
                return false;
            ++p;
            outAfterColon = skipWs(p);
            return outAfterColon != nullptr;
        }

        [[nodiscard]] bool jsonReadU32(const char *p, uint32_t &out, const char *&outAfter) noexcept
        {
            outAfter = nullptr;
            p = skipWs(p);
            if (!p || !*p)
                return false;

            uint64_t v = 0;
            bool any = false;
            while (*p >= '0' && *p <= '9')
            {
                any = true;
                v = (v * 10u) + static_cast<uint64_t>(*p - '0');
                if (v > 0xFFFFFFFFu)
                    return false;
                ++p;
            }
            if (!any)
                return false;
            out = static_cast<uint32_t>(v);
            outAfter = p;
            return true;
        }

        [[nodiscard]] bool jsonReadString(const char *p, char *out, size_t outCap, const char *&outAfter) noexcept
        {
            outAfter = nullptr;
            if (!out || outCap == 0)
                return false;
            p = skipWs(p);
            if (!p || *p != '"')
                return false;
            ++p;

            size_t n = 0;
            while (*p)
            {
                const char c = *p++;
                if (c == '"')
                {
                    if (n >= outCap)
                        return false;
                    out[n] = '\0';
                    outAfter = p;
                    return true;
                }
                if (c == '\\' && *p)
                {
                    const char esc = *p++;
                    if (n + 1 >= outCap)
                        return false;
                    out[n++] = esc;
                    continue;
                }
                if (n + 1 >= outCap)
                    return false;
                out[n++] = c;
            }
            return false;
        }

        [[nodiscard]] bool appendQueryTs(const char *base, uint32_t ts, char *out, size_t outCap) noexcept
        {
            if (!base || !out || outCap == 0)
                return false;
            const size_t baseLen = std::strlen(base);
            const char sep = std::strchr(base, '?') ? '&' : '?';
            const int n = std::snprintf(out, outCap, "%s%c%s=%lu", base, sep, "ts", (unsigned long)ts);
            return n > 0 && static_cast<size_t>(n) < outCap && static_cast<size_t>(n) >= baseLen + 3;
        }

        [[nodiscard]] bool timeIsValid() noexcept
        {
            const time_t now = time(nullptr);
            return now >= kMinValidUnixTime;
        }
    }

    void Ota::publish() noexcept
    {
        if (_cb)
            _cb(_st, _cbUser);
    }

    void Ota::setState(pipcore::ota::State s, uint32_t nowMs) noexcept
    {
        if (_st.state == s)
            return;
        _st.state = s;
        _st.lastChangeMs = nowMs;
        publish();
    }

    void Ota::stopHttp() noexcept
    {
        if (_http.active)
        {
            _http.http.end();
            _http.active = false;
        }
        _http.forManifest = false;
        _http.bodyRead = 0;
        _http.bodyTotal = 0;

        if (_http.updateStarted)
        {
            Update.abort();
            _http.updateStarted = false;
        }
        if (_http.shaInit)
        {
            mbedtls_sha256_free(&_http.sha);
            _http.shaInit = false;
        }

        _http.timeSyncStarted = false;
        _http.timeSyncStartMs = 0;
    }

    void Ota::setError(pipcore::ota::Error e, uint32_t nowMs, int httpCode, int platformCode) noexcept
    {
        stopHttp();
        wifiRelease();
        _st.error = e;
        _st.httpCode = httpCode;
        _st.platformCode = platformCode;
        setState(pipcore::ota::State::Error, nowMs);
    }

    void Ota::wifiAcquire() noexcept
    {
        if (_wifiOwned || !_wifi)
            return;
        if (_wifi->state() != pipcore::net::WifiState::Off)
            return;
        _wifi->request(true);
        _wifiOwned = true;
    }

    void Ota::wifiRelease() noexcept
    {
        if (!_wifiOwned || !_wifi)
            return;
        _wifi->request(false);
        _wifiOwned = false;
    }

    bool Ota::wifiReady() const noexcept
    {
        if (!_wifi)
            return false;
        return _wifi->state() == pipcore::net::WifiState::Connected;
    }

    void Ota::markAppValid() noexcept
    {
        esp_ota_mark_app_valid_cancel_rollback();
    }

    void Ota::configure(const char *manifestUrl,
                        const pipcore::ota::Options &opt,
                        pipcore::ota::StatusCallback cb,
                        void *user) noexcept
    {
        stopHttp();
        wifiRelease();

        _opt = opt;
        _cb = cb;
        _cbUser = user;
        _st = {};
        _st.channel = pipcore::ota::Channel::Custom;
        _st.platformCode = 0;

        if (manifestUrl)
        {
            std::strncpy(_manifestUrl, manifestUrl, sizeof(_manifestUrl) - 1);
            _manifestUrl[sizeof(_manifestUrl) - 1] = '\0';
        }
        else
        {
            _manifestUrl[0] = '\0';
        }

        _manifestUrlStable[0] = '\0';
        _manifestUrlBeta[0] = '\0';

        _manifestLimit = kManifestLimitBytes;
        if (_manifestLimit == 0 || _manifestLimit > kMaxManifestBuf)
            _manifestLimit = kMaxManifestBuf;

        _pubkeyOk = false;
        std::memset(_pubkey, 0, sizeof(_pubkey));
        if (_opt.ed25519PubkeyHex && _opt.ed25519PubkeyHex[0])
        {
            const size_t hexLen = std::strlen(_opt.ed25519PubkeyHex);
            _pubkeyOk = parseHexBytes(_opt.ed25519PubkeyHex, hexLen, _pubkey, sizeof(_pubkey));
        }

        publish();
    }

    void Ota::configureChannels(const char *stableUrl,
                                const char *betaUrl,
                                pipcore::ota::Channel initial,
                                const pipcore::ota::Options &opt,
                                pipcore::ota::StatusCallback cb,
                                void *user) noexcept
    {
        stopHttp();
        wifiRelease();

        _opt = opt;
        _cb = cb;
        _cbUser = user;
        _st = {};
        _st.channel = initial;
        _st.platformCode = 0;

        if (stableUrl)
        {
            std::strncpy(_manifestUrlStable, stableUrl, sizeof(_manifestUrlStable) - 1);
            _manifestUrlStable[sizeof(_manifestUrlStable) - 1] = '\0';
        }
        else
        {
            _manifestUrlStable[0] = '\0';
        }

        if (betaUrl)
        {
            std::strncpy(_manifestUrlBeta, betaUrl, sizeof(_manifestUrlBeta) - 1);
            _manifestUrlBeta[sizeof(_manifestUrlBeta) - 1] = '\0';
        }
        else
        {
            _manifestUrlBeta[0] = '\0';
        }

        _manifestLimit = kManifestLimitBytes;
        if (_manifestLimit == 0 || _manifestLimit > kMaxManifestBuf)
            _manifestLimit = kMaxManifestBuf;

        setChannel(initial);

        _pubkeyOk = false;
        std::memset(_pubkey, 0, sizeof(_pubkey));
        if (_opt.ed25519PubkeyHex && _opt.ed25519PubkeyHex[0])
        {
            const size_t hexLen = std::strlen(_opt.ed25519PubkeyHex);
            _pubkeyOk = parseHexBytes(_opt.ed25519PubkeyHex, hexLen, _pubkey, sizeof(_pubkey));
        }

        publish();
    }

    void Ota::setChannel(pipcore::ota::Channel ch) noexcept
    {
        _st.channel = ch;
        const char *src = nullptr;
        switch (ch)
        {
        case pipcore::ota::Channel::Stable:
            src = _manifestUrlStable;
            break;
        case pipcore::ota::Channel::Beta:
            src = _manifestUrlBeta;
            break;
        case pipcore::ota::Channel::Custom:
        default:
            src = _manifestUrl;
            break;
        }

        if (src && src[0])
        {
            std::strncpy(_manifestUrl, src, sizeof(_manifestUrl) - 1);
            _manifestUrl[sizeof(_manifestUrl) - 1] = '\0';
        }

        publish();
    }

    void Ota::requestCheck(pipcore::ota::CheckMode mode) noexcept
    {
        _wantCheck = true;
        _checkMode = mode;
        _wantInstall = false;
        _wantCancel = false;
        _wifiNextDownload = false;
    }

    void Ota::requestInstall() noexcept
    {
        _wantInstall = true;
        _wantCancel = false;
        _wifiNextDownload = true;
    }

    void Ota::requestRollback() noexcept
    {
        _wantRollback = true;
        _wantCancel = false;
    }

    void Ota::cancel() noexcept
    {
        _wantCancel = true;
        _wantCheck = false;
        _wantInstall = false;
        _wantRollback = false;
        _wifiNextDownload = false;
    }

    int Ota::compareVersion(uint16_t aMaj, uint16_t aMin, uint16_t aPat,
                            uint16_t bMaj, uint16_t bMin, uint16_t bPat) noexcept
    {
        if (aMaj != bMaj)
            return (aMaj > bMaj) ? 1 : -1;
        if (aMin != bMin)
            return (aMin > bMin) ? 1 : -1;
        if (aPat != bPat)
            return (aPat > bPat) ? 1 : -1;
        return 0;
    }

    bool Ota::parseVersion(const char *version, uint16_t &maj, uint16_t &min, uint16_t &pat) noexcept
    {
        maj = min = pat = 0;
        if (!version || !version[0])
            return false;
        const char *p = version;
        uint32_t parts[3] = {0, 0, 0};
        for (int i = 0; i < 3; ++i)
        {
            if (!*p)
                return false;
            uint64_t v = 0;
            bool any = false;
            while (*p >= '0' && *p <= '9')
            {
                any = true;
                v = (v * 10u) + static_cast<uint64_t>(*p - '0');
                if (v > 100000u)
                    return false;
                ++p;
            }
            if (!any)
                return false;
            parts[i] = static_cast<uint32_t>(v);
            if (i < 2)
            {
                if (*p != '.')
                    return false;
                ++p;
            }
        }
        if (*p != '\0')
            return false;
        if (parts[0] >= 10000u || parts[1] >= 1000u || parts[2] >= 1000u)
            return false;
        maj = static_cast<uint16_t>(parts[0]);
        min = static_cast<uint16_t>(parts[1]);
        pat = static_cast<uint16_t>(parts[2]);
        return true;
    }

    bool Ota::beginHttp(const char *url, bool forManifest) noexcept
    {
        stopHttp();
        if (!url || !url[0])
            return false;

        _http.client.setCACert(kRootCaLetsEncryptBundle);
        _http.client.setTimeout(5000);
        _http.http.setReuse(false);
        _http.http.setConnectTimeout(5000);
        _http.http.setTimeout(5000);
        if (!_http.http.begin(_http.client, url))
        {
            _st.platformCode = 0;
            return false;
        }

        const int code = _http.http.GET();
        _st.httpCode = code;
        if (code != 200)
        {
            _st.platformCode = 0;
            if (code <= 0)
            {
                char tmp[128];
                const int tlsErr = _http.client.lastError(tmp, sizeof(tmp));
                _st.platformCode = (tlsErr != 0) ? tlsErr : code;
            }
            _http.http.end();
            return false;
        }

        _http.active = true;
        _http.forManifest = forManifest;
        _st.platformCode = 0;
        const int total = _http.http.getSize();
        _http.bodyTotal = total > 0 ? static_cast<uint32_t>(total) : 0;
        _http.bodyRead = 0;
        if (forManifest)
        {
            _st.downloaded = 0;
            _st.total = 0;
        }
        return true;
    }

    bool Ota::readHttpBody() noexcept
    {
        if (!_http.active || !_http.forManifest)
            return false;

        WiFiClient *stream = _http.http.getStreamPtr();
        if (!stream)
            return false;

        size_t budget = 1024;
        while (budget > 0)
        {
            const int avail = stream->available();
            if (avail <= 0)
                break;

            const size_t space = (_manifestLimit > _manifestLen) ? (_manifestLimit - _manifestLen) : 0;
            if (space == 0)
            {
                _manifestLen = _manifestLimit + 1;
                return true;
            }

            size_t chunk = static_cast<size_t>(avail);
            if (chunk > 256)
                chunk = 256;
            if (chunk > space)
                chunk = space;
            if (chunk > budget)
                chunk = budget;

            const int n = stream->read(reinterpret_cast<uint8_t *>(_manifestBuf + _manifestLen), chunk);
            if (n <= 0)
                break;
            _manifestLen += static_cast<size_t>(n);
            _http.bodyRead += static_cast<uint32_t>(n);
            budget -= static_cast<size_t>(n);
        }

        if (_http.bodyTotal > 0 && _http.bodyRead >= _http.bodyTotal)
            return true;

        if (_http.bodyTotal == 0 && !_http.http.connected() && stream->available() <= 0)
            return true;

        return false;
    }

    bool Ota::parseManifest(uint32_t nowMs) noexcept
    {
        if (_manifestLen == 0)
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (_manifestLen > _manifestLimit)
        {
            setError(pipcore::ota::Error::ManifestTooLarge, nowMs);
            return false;
        }

        _manifestBuf[_manifestLen] = '\0';
        const char *json = reinterpret_cast<const char *>(_manifestBuf);

        pipcore::ota::Manifest m = {};

        const char *after = nullptr;

        if (!jsonFindKey(json, "version", after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (!jsonReadString(after, m.version, sizeof(m.version), after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (!parseVersion(m.version, m.verMajor, m.verMinor, m.verPatch))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }

        if (!jsonFindKey(json, "size", after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (!jsonReadU32(after, m.size, after) || m.size == 0)
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }

        char shaHex[80] = {};
        if (!jsonFindKey(json, "sha256", after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (!jsonReadString(after, shaHex, sizeof(shaHex), after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        const size_t shaLen = std::strlen(shaHex);
        if (!parseHexBytes(shaHex, shaLen, m.sha256, sizeof(m.sha256)))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }

        if (!jsonFindKey(json, "url", after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }
        if (!jsonReadString(after, m.url, sizeof(m.url), after))
        {
            setError(pipcore::ota::Error::ManifestParseFailed, nowMs);
            return false;
        }

        char sigHex[160] = {};
        if (jsonFindKey(json, "sig_ed25519", after))
        {
            if (jsonReadString(after, sigHex, sizeof(sigHex), after))
            {
                const size_t sigLen = std::strlen(sigHex);
                if (parseHexBytes(sigHex, sigLen, m.sigEd25519, sizeof(m.sigEd25519)))
                    m.hasSig = true;
            }
        }

        _st.manifest = m;

        if (!m.hasSig)
        {
            setError(pipcore::ota::Error::SignatureMissing, nowMs);
            return false;
        }
        if (!verifyManifestSignature(nowMs))
            return false;

        const int cmp = compareVersion(m.verMajor, m.verMinor, m.verPatch,
                                       _opt.currentVerMajor, _opt.currentVerMinor, _opt.currentVerPatch);
        _st.versionCmp = static_cast<int8_t>(cmp);

        if (cmp == 0)
        {
            _st.error = pipcore::ota::Error::None;
            _st.httpCode = 0;
            _st.platformCode = 0;
            setState(pipcore::ota::State::UpToDate, nowMs);
            return true;
        }
        if (cmp < 0 && _checkMode == pipcore::ota::CheckMode::NewerOnly)
        {
            _st.error = pipcore::ota::Error::None;
            _st.httpCode = 0;
            _st.platformCode = 0;
            setState(pipcore::ota::State::UpToDate, nowMs);
            return true;
        }

        setState(pipcore::ota::State::UpdateAvailable, nowMs);
        return true;
    }

    bool Ota::verifyManifestSignature(uint32_t nowMs) noexcept
    {
        if (!_pubkeyOk)
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }

#if !PIPCORE_OTA_HAVE_SODIUM
        setError(pipcore::ota::Error::SignatureInvalid, nowMs);
        return false;
#else
        if (sodium_init() < 0)
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }

        const pipcore::ota::Manifest &m = _st.manifest;

        uint8_t payload[512];
        size_t off = 0;

        const size_t prefLen = std::strlen(kSigPrefix);
        if (off + prefLen > sizeof(payload))
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }
        std::memcpy(payload + off, kSigPrefix, prefLen);
        off += prefLen;

        const size_t verLen = std::strlen(m.version);
        if (verLen == 0 || off + verLen + 1 > sizeof(payload))
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }
        std::memcpy(payload + off, m.version, verLen);
        off += verLen;
        payload[off++] = 0;

        if (off + 4 + sizeof(m.sha256) > sizeof(payload))
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }
        payload[off++] = static_cast<uint8_t>((m.size >> 24) & 0xFF);
        payload[off++] = static_cast<uint8_t>((m.size >> 16) & 0xFF);
        payload[off++] = static_cast<uint8_t>((m.size >> 8) & 0xFF);
        payload[off++] = static_cast<uint8_t>((m.size) & 0xFF);

        std::memcpy(payload + off, m.sha256, sizeof(m.sha256));
        off += sizeof(m.sha256);

        const size_t urlLen = std::strlen(m.url);
        if (urlLen == 0 || off + urlLen > sizeof(payload))
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }
        std::memcpy(payload + off, m.url, urlLen);
        off += urlLen;

        if (crypto_sign_verify_detached(m.sigEd25519, payload, off, _pubkey) != 0)
        {
            setError(pipcore::ota::Error::SignatureInvalid, nowMs);
            return false;
        }
        return true;
#endif
    }

    bool Ota::beginFirmwareDownload(uint32_t nowMs) noexcept
    {
        const pipcore::ota::Manifest &m = _st.manifest;

        const esp_partition_t *part = esp_ota_get_next_update_partition(nullptr);
        if (!part)
        {
            setError(pipcore::ota::Error::FlashLayoutInvalid, nowMs);
            return false;
        }
        const uint32_t flashSize = ESP.getFlashChipSize();
        const uint64_t end = static_cast<uint64_t>(part->address) + static_cast<uint64_t>(part->size);
        if (end > flashSize)
        {
            setError(pipcore::ota::Error::FlashLayoutInvalid, nowMs);
            return false;
        }
        if (part->size < m.size)
        {
            setError(pipcore::ota::Error::FlashLayoutInvalid, nowMs);
            return false;
        }

        if (!beginHttp(m.url, false))
        {
            setError(pipcore::ota::Error::HttpStatusNotOk, nowMs, _st.httpCode);
            return false;
        }

        _st.downloaded = 0;
        _st.total = m.size;

        if (!_http.shaInit)
        {
            mbedtls_sha256_init(&_http.sha);
            if (mbedtls_sha256_starts_ret(&_http.sha, 0) != 0)
            {
                setError(pipcore::ota::Error::UpdateBeginFailed, nowMs);
                return false;
            }
            _http.shaInit = true;
        }

        if (!Update.begin(m.size))
        {
            setError(pipcore::ota::Error::UpdateBeginFailed, nowMs);
            return false;
        }
        _http.updateStarted = true;
        setState(pipcore::ota::State::Downloading, nowMs);
        return true;
    }

    bool Ota::stepFirmwareDownload(uint32_t nowMs) noexcept
    {
        if (!_http.active || _http.forManifest)
        {
            setError(pipcore::ota::Error::UpdateWriteFailed, nowMs);
            return false;
        }
        WiFiClient *stream = _http.http.getStreamPtr();
        if (!stream)
        {
            setError(pipcore::ota::Error::UpdateWriteFailed, nowMs);
            return false;
        }

        uint8_t buf[1024];
        size_t budget = 8 * 1024;
        while (budget > 0)
        {
            const int avail = stream->available();
            if (avail <= 0)
                break;

            size_t chunk = static_cast<size_t>(avail);
            if (chunk > sizeof(buf))
                chunk = sizeof(buf);
            const uint32_t remain = (_st.total > _st.downloaded) ? (_st.total - _st.downloaded) : 0;
            if (remain == 0)
                break;
            if (chunk > remain)
                chunk = remain;
            if (chunk > budget)
                chunk = budget;

            const int n = stream->read(buf, chunk);
            if (n <= 0)
                break;

            if (mbedtls_sha256_update_ret(&_http.sha, buf, static_cast<size_t>(n)) != 0)
            {
                setError(pipcore::ota::Error::UpdateWriteFailed, nowMs);
                return false;
            }

            const size_t wrote = Update.write(buf, static_cast<size_t>(n));
            if (wrote != static_cast<size_t>(n))
            {
                setError(pipcore::ota::Error::UpdateWriteFailed, nowMs);
                return false;
            }

            _st.downloaded += static_cast<uint32_t>(n);
            budget -= static_cast<size_t>(n);
            if (_st.downloaded > _st.total)
            {
                setError(pipcore::ota::Error::PayloadSizeMismatch, nowMs);
                return false;
            }
        }

        if (_st.downloaded < _st.total)
        {
            if (!_http.http.connected() || !stream->connected())
            {
                setError(pipcore::ota::Error::PayloadSizeMismatch, nowMs);
                return false;
            }
            return true;
        }

        uint8_t hash[32] = {};
        if (mbedtls_sha256_finish_ret(&_http.sha, hash) != 0)
        {
            setError(pipcore::ota::Error::HashMismatch, nowMs);
            return false;
        }
        mbedtls_sha256_free(&_http.sha);
        _http.shaInit = false;

        if (std::memcmp(hash, _st.manifest.sha256, sizeof(hash)) != 0)
        {
            setError(pipcore::ota::Error::HashMismatch, nowMs);
            return false;
        }

        setState(pipcore::ota::State::Installing, nowMs);

        if (!Update.end(false))
        {
            const int updErr = (int)Update.getError();
            if (updErr == UPDATE_ERROR_ACTIVATE)
            {
                const esp_partition_t *part = esp_ota_get_next_update_partition(nullptr);
                const esp_err_t err = part ? esp_ota_set_boot_partition(part) : ESP_ERR_INVALID_STATE;
                if (err == ESP_OK)
                {
                    _http.updateStarted = false;
                    stopHttp();
                    wifiRelease();
                    setState(pipcore::ota::State::Success, nowMs);
                    return true;
                }
                setError(pipcore::ota::Error::UpdateEndFailed, nowMs, 0, (int)err);
                return false;
            }

            setError(pipcore::ota::Error::UpdateEndFailed, nowMs, 0, updErr);
            return false;
        }
        _http.updateStarted = false;

        stopHttp();
        wifiRelease();
        setState(pipcore::ota::State::Success, nowMs);
        return true;
    }

    void Ota::service(uint32_t nowMs) noexcept
    {
        if (!_bootInit)
        {
            _bootInit = true;
            const esp_partition_t *running = esp_ota_get_running_partition();
            esp_ota_img_states_t st = ESP_OTA_IMG_INVALID;
            if (running && esp_ota_get_state_partition(running, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY)
            {
                _bootPendingVerify = true;
                _bootConfirmed = false;
                _bootStartMs = nowMs;
            }
        }

        // Reduce "forgot to call markAppValid()" footgun:
        // if we booted into a PENDING_VERIFY app and survived long enough, confirm it automatically.
        if (_bootPendingVerify && !_bootConfirmed)
        {
            // Give the app time for its own self-checks / init before confirming.
            constexpr uint32_t kConfirmAfterMs = 12'000;
            if (static_cast<uint32_t>(nowMs - _bootStartMs) >= kConfirmAfterMs)
            {
                esp_ota_mark_app_valid_cancel_rollback();
                _bootConfirmed = true;
                _bootPendingVerify = false;
            }
        }

        if (_wantCancel)
        {
            _wantCancel = false;
            stopHttp();
            wifiRelease();
            _st.error = pipcore::ota::Error::None;
            _st.httpCode = 0;
            _st.platformCode = 0;
            _st.downloaded = 0;
            _st.total = 0;
            setState(pipcore::ota::State::Idle, nowMs);
            return;
        }

        if (_wantRollback)
        {
            _wantRollback = false;

            const esp_partition_t *running = esp_ota_get_running_partition();
            const esp_partition_t *target = esp_ota_get_next_update_partition(running);
            if (!target)
            {
                setError(pipcore::ota::Error::RollbackUnavailable, nowMs);
                return;
            }
            if (running && target == running)
            {
                setError(pipcore::ota::Error::RollbackUnavailable, nowMs);
                return;
            }

            esp_ota_img_states_t st = ESP_OTA_IMG_INVALID;
            if (esp_ota_get_state_partition(target, &st) != ESP_OK || st != ESP_OTA_IMG_VALID)
            {
                setError(pipcore::ota::Error::RollbackUnavailable, nowMs);
                return;
            }
            if (esp_ota_set_boot_partition(target) != ESP_OK)
            {
                setError(pipcore::ota::Error::RollbackUnavailable, nowMs);
                return;
            }
            ESP.restart();
            return;
        }

        if (_wantCheck &&
            _st.state != pipcore::ota::State::Idle &&
            _st.state != pipcore::ota::State::Downloading &&
            _st.state != pipcore::ota::State::Installing)
        {
            stopHttp();
            wifiRelease();
            _st.error = pipcore::ota::Error::None;
            _st.httpCode = 0;
            _st.platformCode = 0;
            _st.downloaded = 0;
            _st.total = 0;
            _manifestLen = 0;
            setState(pipcore::ota::State::Idle, nowMs);
        }

        if (_st.state == pipcore::ota::State::Error)
            return;

        if (_st.state == pipcore::ota::State::Idle)
        {
            if (_wantCheck)
            {
                _wantCheck = false;
                _wifiNextDownload = false;
                _st.error = pipcore::ota::Error::None;
                _st.httpCode = 0;
                _st.platformCode = 0;
                _st.downloaded = 0;
                _st.total = 0;
                _manifestLen = 0;
                _http.timeSyncStarted = false;
                _http.timeSyncStartMs = 0;
                wifiAcquire();
                setState(pipcore::ota::State::WifiStarting, nowMs);
            }
            return;
        }

        if (_st.state == pipcore::ota::State::WifiStarting)
        {
            wifiAcquire();
            if (!_wifi)
            {
                setError(pipcore::ota::Error::WifiNotEnabled, nowMs);
                return;
            }

            if (_wifi->state() == pipcore::net::WifiState::Failed)
            {
                setError(pipcore::ota::Error::WifiNotConnected, nowMs);
                return;
            }
            if (!wifiReady())
                return;

            if (!timeIsValid())
            {
                if (!_http.timeSyncStarted)
                {
                    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
                    _http.timeSyncStarted = true;
                    _http.timeSyncStartMs = nowMs;
                    return;
                }

                if (static_cast<uint32_t>(nowMs - _http.timeSyncStartMs) > 10'000)
                {
                    setError(pipcore::ota::Error::TimeSyncFailed, nowMs);
                    return;
                }
                return;
            }

            if (_wifiNextDownload)
            {
                _wifiNextDownload = false;
                (void)beginFirmwareDownload(nowMs);
                return;
            }

            setState(pipcore::ota::State::FetchingManifest, nowMs);
            _manifestLen = 0;

            char url[256] = {};
            if (!appendQueryTs(_manifestUrl, nowMs, url, sizeof(url)))
            {
                setError(pipcore::ota::Error::HttpBeginFailed, nowMs);
                return;
            }
            if (!beginHttp(url, true))
            {
                if (_st.httpCode == 404)
                {
                    wifiRelease();
                    _st.error = pipcore::ota::Error::None;
                    _st.httpCode = 0;
                    _st.platformCode = 0;
                    _st.manifest = {};
                    _st.versionCmp = 0;
                    setState(pipcore::ota::State::UpToDate, nowMs);
                    return;
                }
                setError(pipcore::ota::Error::HttpStatusNotOk, nowMs, _st.httpCode);
                return;
            }
            return;
        }

        if (_st.state == pipcore::ota::State::FetchingManifest)
        {
            if (!_http.active)
            {
                setError(pipcore::ota::Error::HttpBeginFailed, nowMs);
                return;
            }

            const bool done = readHttpBody();
            if (!done)
                return;

            stopHttp();
            if (!parseManifest(nowMs))
                return;

            wifiRelease();
            return;
        }

        if (_st.state == pipcore::ota::State::UpdateAvailable)
        {
            if (_wantInstall)
            {
                _wantInstall = false;
                _wifiNextDownload = true;
                _http.timeSyncStarted = false;
                _http.timeSyncStartMs = 0;
                wifiAcquire();
                if (wifiReady())
                {
                    _wifiNextDownload = false;
                    (void)beginFirmwareDownload(nowMs);
                    return;
                }
                setState(pipcore::ota::State::WifiStarting, nowMs);
            }
            return;
        }

        if (_st.state == pipcore::ota::State::Downloading || _st.state == pipcore::ota::State::Installing)
        {
            (void)stepFirmwareDownload(nowMs);
            return;
        }
    }
}

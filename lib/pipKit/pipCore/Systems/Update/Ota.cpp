#include <pipCore/Systems/Update/Ota.hpp>

#include <pipCore/Platforms/Select.hpp>

namespace pipcore::ota
{
    void markAppValid() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaMarkAppValid();
    }

    void configure(const char *manifestUrl, const Options &opt, StatusCallback cb, void *user) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaConfigure(manifestUrl, opt, cb, user);
    }

    void configureChannels(const char *stableUrl,
                           const char *betaUrl,
                           Channel initial,
                           const Options &opt,
                           StatusCallback cb,
                           void *user) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaConfigureChannels(stableUrl, betaUrl, initial, opt, cb, user);
    }

    void setChannel(Channel ch) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaSetChannel(ch);
    }

    Channel channel() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            return p->otaChannel();
        return Channel::Custom;
    }

    void requestCheck() noexcept
    {
        requestCheck(CheckMode::NewerOnly);
    }

    void requestCheck(CheckMode mode) noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaRequestCheck(mode);
    }

    void requestInstall() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaRequestInstall();
    }

    void requestRollback() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaRequestRollback();
    }

    void cancel() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaCancel();
    }

    void service() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            p->otaService();
    }

    const Status &status() noexcept
    {
        if (pipcore::Platform *p = pipcore::GetPlatform())
            return p->otaStatus();
        static Status st = {};
        return st;
    }
}

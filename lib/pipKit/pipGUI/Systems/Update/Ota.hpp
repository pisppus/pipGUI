#pragma once

#include <pipGUI/Core/Common.hpp>
#include <pipCore/Systems/Update/Ota.hpp>

namespace pipgui
{
    using OtaChannel = pipcore::ota::Channel;
    using OtaCheckMode = pipcore::ota::CheckMode;
    using OtaState = pipcore::ota::State;
    using OtaError = pipcore::ota::Error;
    using OtaManifest = pipcore::ota::Manifest;
    using OtaStatus = pipcore::ota::Status;
    using OtaStatusCallback = pipcore::ota::StatusCallback;
}

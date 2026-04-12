/*
██████╗ ███████╗██████╗ ██╗██████╗ ██╗  ██╗███████╗██████╗  █████╗ ██╗     ███╗   ███╗ █████╗ ███╗   ██╗ █████╗  ██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗
██╔══██╗██╔════╝██╔══██╗██║██╔══██╗██║  ██║██╔════╝██╔══██╗██╔══██╗██║     ████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝ ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██████╔╝█████╗  ██████╔╝██║██████╔╝███████║█████╗  ██████╔╝███████║██║     ██╔████╔██║███████║██╔██╗ ██║███████║██║  ███╗█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔═══╝ ██╔══╝  ██╔══██╗██║██╔═══╝ ██╔══██║██╔══╝  ██╔══██╗██╔══██║██║     ██║╚██╔╝██║██╔══██║██║╚██╗██║██╔══██║██║   ██║██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║     ███████╗██║  ██║██║██║     ██║  ██║███████╗██║  ██║██║  ██║███████╗██║ ╚═╝ ██║██║  ██║██║ ╚████║██║  ██║╚██████╔╝███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "peripheralManager.h"
#include "../settings/globalSettings.h"
#include <algorithm>

namespace {

int clampAverageWindowSeconds(int value)
{
    return std::clamp(value, 1, 30);
}

int clampPresenceAbsentTimeoutSeconds(int value)
{
    return std::clamp(value, 1, 120);
}

MmWavePresenceConfig loadPresenceConfigFromSettings()
{
    MmWavePresenceConfig config;
    config.detectionDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS));
    config.movingDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS));
    config.stationaryDistanceAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS));
    config.stationaryEnergyAverageWindowSecs = clampAverageWindowSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS));
    return config;
}

int loadPresenceAbsentTimeoutSecondsFromSettings()
{
    return clampPresenceAbsentTimeoutSeconds(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::PRESENCE_ABSENT_TIMEOUT_SECS));
}

bool loadPresenceDetectionEnabledFromSettings()
{
    return GlobalSettings::getSettingOfType<bool>(
        GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED);
}

const char* presenceStateToString(MmWavePresenceState state)
{
    switch (state) {
    case MmWavePresenceState::Present:
        return "present";
    case MmWavePresenceState::Absent:
        return "absent";
    case MmWavePresenceState::Unknown:
    default:
        return "unknown";
    }
}

} // namespace

DelayedPresenceTracker::DelayedPresenceTracker(int absentTimeoutSecs)
    : absentTimeoutSecs_(clampAbsentTimeoutSecs(absentTimeoutSecs))
{
}

void DelayedPresenceTracker::reset()
{
    immediateState_ = MmWavePresenceState::Unknown;
    delayedState_ = MmWavePresenceState::Unknown;
    absentStateStartedAt_.reset();
}

void DelayedPresenceTracker::setAbsentTimeoutSecs(
    int absentTimeoutSecs,
    std::chrono::steady_clock::time_point now)
{
    absentTimeoutSecs_ = clampAbsentTimeoutSecs(absentTimeoutSecs);
    reconcileDelayedState(now);
}

int DelayedPresenceTracker::absentTimeoutSecs() const
{
    return absentTimeoutSecs_;
}

PresenceStatusSnapshot DelayedPresenceTracker::updateImmediateState(
    MmWavePresenceState immediateState,
    std::chrono::steady_clock::time_point now)
{
    immediateState_ = immediateState;
    switch (immediateState_) {
    case MmWavePresenceState::Present:
        delayedState_ = MmWavePresenceState::Present;
        absentStateStartedAt_.reset();
        break;
    case MmWavePresenceState::Absent:
        if (!absentStateStartedAt_.has_value()) {
            absentStateStartedAt_ = now;
        }
        reconcileDelayedState(now);
        break;
    case MmWavePresenceState::Unknown:
    default:
        absentStateStartedAt_.reset();
        break;
    }

    return snapshot(now);
}

PresenceStatusSnapshot DelayedPresenceTracker::snapshot(std::chrono::steady_clock::time_point now)
{
    reconcileDelayedState(now);
    return {
        .immediateState = immediateState_,
        .delayedState = delayedState_,
        .absentTimeoutSecs = absentTimeoutSecs_,
    };
}

int DelayedPresenceTracker::clampAbsentTimeoutSecs(int value)
{
    return clampPresenceAbsentTimeoutSeconds(value);
}

void DelayedPresenceTracker::reconcileDelayedState(std::chrono::steady_clock::time_point now)
{
    if (immediateState_ != MmWavePresenceState::Absent || !absentStateStartedAt_.has_value()) {
        return;
    }

    const auto timeout = std::chrono::seconds(absentTimeoutSecs_);
    if ((now - *absentStateStartedAt_) >= timeout) {
        delayedState_ = MmWavePresenceState::Absent;
    }
}

PeripheralManager::PeripheralManager()
    : PeripheralManager(std::make_unique<mmWave>())
{
}

PeripheralManager::PeripheralManager(
    std::unique_ptr<mmWave> mmWaveSensorOverride,
    bool registerSettingCallbacks)
    : mmWaveSensor(std::move(mmWaveSensorOverride))
    , delayedPresenceTracker_(loadPresenceAbsentTimeoutSecondsFromSettings())
    , presenceDetectionEnabled_(loadPresenceDetectionEnabledFromSettings())
{
    syncPresenceConfigFromSettings();
    syncPresenceAbsentTimeoutFromSettings();
    syncPresenceDetectionEnabledFromSettings();

    if (mmWaveSensor) {
        mmWaveSensor->setPresenceUpdateCallback([this](const MmWavePresenceDecision& decision) {
            handleImmediatePresenceDecision(decision);
        });
        handleImmediatePresenceDecision(mmWaveSensor->getPresenceDecision());
    }

    if (registerSettingCallbacks) {
        const auto registerConfigCallback = [this](GlobalSettings::SettingType settingType) {
            GlobalSettings::setSettingCB(settingType, [this]() {
                syncPresenceConfigFromSettings();
            });
        };

        registerConfigCallback(GlobalSettings::SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS);
        registerConfigCallback(GlobalSettings::SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS);
        registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS);
        registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS);
        GlobalSettings::setSettingCB(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, [this]() {
            syncPresenceDetectionEnabledFromSettings();
        });
        GlobalSettings::setSettingCB(GlobalSettings::SettingType::PRESENCE_ABSENT_TIMEOUT_SECS, [this]() {
            syncPresenceAbsentTimeoutFromSettings();
        });
    }
}

PeripheralManager::~PeripheralManager()
{
    if (mmWaveSensor) {
        mmWaveSensor->setPresenceUpdateCallback({});
    }
}

HttpEndPointData_t PeripheralManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    data.push_back({
        PUBLIC_ENDPOINT | GET_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            const PresenceStatusSnapshot status = getPresenceStatus();
            nlohmann::json response = {
                { "immediateState", presenceStateToString(status.immediateState) },
                { "delayedState", presenceStateToString(status.delayedState) },
                { "immediatePresent", status.immediateState == MmWavePresenceState::Present },
                { "delayedPresent", status.delayedState == MmWavePresenceState::Present },
                { "absentTimeoutSecs", status.absentTimeoutSecs }
            };
            res.status = 200;
            res.set_content(response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "status",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get immediate and delayed presence state"
    });
    return data;
}

void PeripheralManager::syncPresenceConfigFromSettings()
{
    if (!mmWaveSensor) {
        return;
    }

    mmWaveSensor->setPresenceConfig(loadPresenceConfigFromSettings());
}

void PeripheralManager::syncPresenceDetectionEnabledFromSettings()
{
    const bool enabled = loadPresenceDetectionEnabledFromSettings();
    {
        std::lock_guard<std::mutex> lock(presenceStatusMutex);
        presenceDetectionEnabled_ = enabled;
        delayedPresenceTracker_.reset();
    }

    if (mmWaveSensor) {
        mmWaveSensor->setPresenceDetectionEnabled(enabled);
    }
}

void PeripheralManager::syncPresenceAbsentTimeoutFromSettings()
{
    std::lock_guard<std::mutex> lock(presenceStatusMutex);
    delayedPresenceTracker_.setAbsentTimeoutSecs(
        loadPresenceAbsentTimeoutSecondsFromSettings(),
        std::chrono::steady_clock::now());
}

void PeripheralManager::handleImmediatePresenceDecision(const MmWavePresenceDecision& decision)
{
    const auto now = decision.timestamp == std::chrono::steady_clock::time_point {}
        ? std::chrono::steady_clock::now()
        : decision.timestamp;

    std::lock_guard<std::mutex> lock(presenceStatusMutex);
    if (!presenceDetectionEnabled_) {
        return;
    }
    delayedPresenceTracker_.updateImmediateState(decision.state, now);
}

bool PeripheralManager::isMmWavePresent()
{
    return getImmediatePresenceState() == MmWavePresenceState::Present;
}

MmWavePresenceState PeripheralManager::getImmediatePresenceState()
{
    return getPresenceStatus().immediateState;
}

MmWavePresenceState PeripheralManager::getDelayedPresenceState()
{
    return getPresenceStatus().delayedState;
}

PresenceStatusSnapshot PeripheralManager::getPresenceStatus()
{
    {
        std::lock_guard<std::mutex> lock(presenceStatusMutex);
        if (!presenceDetectionEnabled_) {
            return delayedPresenceTracker_.snapshot(std::chrono::steady_clock::now());
        }
    }

    if (mmWaveSensor) {
        handleImmediatePresenceDecision(mmWaveSensor->getPresenceDecision());
    }

    std::lock_guard<std::mutex> lock(presenceStatusMutex);
    return delayedPresenceTracker_.snapshot(std::chrono::steady_clock::now());
}

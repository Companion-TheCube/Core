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
#include <cmath>

namespace {

constexpr int kDefaultFanControlPollIntervalMs = 2000;
constexpr double kDefaultFanControlHysteresisC = 2.0;
constexpr uint8_t kDefaultFanControlFailsafePercent = 100;

struct FanCurvePoint {
    double temperatureC = 0.0;
    uint8_t dutyPercent = 0;
};

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

bool loadFanControlEnabledFromSettings()
{
    return GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::FAN_CONTROL_ENABLED);
}

int loadFanControlPollIntervalMsFromSettings()
{
    return std::clamp(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS),
        250,
        10000);
}

double loadFanControlHysteresisCFromSettings()
{
    return std::clamp(
        GlobalSettings::getSettingOfType<double>(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C),
        0.0,
        10.0);
}

uint8_t loadFanControlFailsafePercentFromSettings()
{
    return static_cast<uint8_t>(std::clamp(
        GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT),
        0,
        100));
}

std::vector<FanCurvePoint> defaultFanControlCurvePoints()
{
    return {
        { 35.0, 25 },
        { 50.0, 40 },
        { 65.0, 70 },
        { 80.0, 100 },
    };
}

std::vector<FanCurvePoint> loadFanControlCurvePointsFromSettings()
{
    const auto curveJson = GlobalSettings::getSetting(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS);
    if (!curveJson.is_array() || curveJson.size() < 2) {
        return defaultFanControlCurvePoints();
    }

    std::vector<FanCurvePoint> points;
    points.reserve(curveJson.size());
    for (const auto& point : curveJson) {
        if (!point.is_array() || point.size() != 2 || !point[0].is_number() || !point[1].is_number()) {
            return defaultFanControlCurvePoints();
        }
        points.push_back({
            .temperatureC = std::clamp(point[0].get<double>(), 0.0, 120.0),
            .dutyPercent = static_cast<uint8_t>(std::clamp(point[1].get<int>(), 0, 100))
        });
    }

    if (points.size() < 2) {
        return defaultFanControlCurvePoints();
    }

    std::sort(points.begin(), points.end(), [](const FanCurvePoint& left, const FanCurvePoint& right) {
        return left.temperatureC < right.temperatureC;
    });
    return points;
}

uint8_t interpolateFanDutyPercent(double cpuTemperatureC, const std::vector<FanCurvePoint>& curvePoints)
{
    if (curvePoints.empty()) {
        return 0;
    }
    if (cpuTemperatureC <= curvePoints.front().temperatureC) {
        return curvePoints.front().dutyPercent;
    }
    if (cpuTemperatureC >= curvePoints.back().temperatureC) {
        return curvePoints.back().dutyPercent;
    }

    for (size_t pointIndex = 1; pointIndex < curvePoints.size(); ++pointIndex) {
        const auto& previous = curvePoints[pointIndex - 1];
        const auto& next = curvePoints[pointIndex];
        if (cpuTemperatureC > next.temperatureC) {
            continue;
        }

        if (next.temperatureC <= previous.temperatureC) {
            return next.dutyPercent;
        }

        const double fraction = (cpuTemperatureC - previous.temperatureC) / (next.temperatureC - previous.temperatureC);
        const double interpolated = static_cast<double>(previous.dutyPercent)
            + fraction * static_cast<double>(next.dutyPercent - previous.dutyPercent);
        return static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::lround(interpolated)), 0, 100));
    }

    return curvePoints.back().dutyPercent;
}

std::unique_ptr<FanController> createFanControllerFromConfig()
{
    const std::string devicePath = Config::get("FAN_CONTROLLER_I2C_DEVICE", "");
    const std::string addressString = Config::get("FAN_CONTROLLER_I2C_ADDRESS", "");
    const bool tenBitAddress = Config::getBool("FAN_CONTROLLER_I2C_10BIT", false);

    if (!Config::getBool("HARDWARE_I2C_ENABLED", true)) {
        CubeLog::info("PeripheralManager: HARDWARE_I2C_ENABLED=0, leaving fan control disabled.");
        return nullptr;
    }

    if (devicePath.empty() || addressString.empty()) {
        CubeLog::info("PeripheralManager: fan controller config missing, leaving fan control disabled.");
        return nullptr;
    }

    try {
        const unsigned long rawAddress = std::stoul(addressString, nullptr, 0);
        const unsigned long maxAddress = tenBitAddress ? 0x03FFUL : 0x007FUL;
        if (rawAddress > maxAddress) {
            CubeLog::warning("PeripheralManager: fan controller I2C address out of range: " + addressString);
            return nullptr;
        }

        auto bus = std::make_shared<PiI2CBus>();
        if (!bus->setI2cDevicePath(devicePath)) {
            CubeLog::warning("PeripheralManager: invalid fan controller I2C device path: " + devicePath);
            return nullptr;
        }

        return std::make_unique<FanController>(bus, static_cast<uint16_t>(rawAddress), tenBitAddress);
    } catch (...) {
        CubeLog::warning("PeripheralManager: failed to parse fan controller I2C address: " + addressString);
        return nullptr;
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
    : PeripheralManager(
        std::move(mmWaveSensorOverride),
        nullptr,
        {},
        {},
        registerSettingCallbacks,
        true)
{
}

PeripheralManager::PeripheralManager(
    std::unique_ptr<mmWave> mmWaveSensorOverride,
    std::unique_ptr<FanController> fanControllerOverride,
    TemperatureReader cpuTemperatureReader,
    TemperatureReader systemTemperatureReader,
    bool registerSettingCallbacks,
    bool startThermalControlLoopThread)
    : mmWaveSensor(std::move(mmWaveSensorOverride))
    , fanController_(fanControllerOverride ? std::move(fanControllerOverride) : createFanControllerFromConfig())
    , cpuTemperatureReader_(cpuTemperatureReader ? std::move(cpuTemperatureReader) : TemperatureReader([]() {
        return HardwareInfo::cpuTemperatureCelsius();
    }))
    , systemTemperatureReader_(systemTemperatureReader ? std::move(systemTemperatureReader) : TemperatureReader([]() {
        return HardwareInfo::systemTemperatureCelsius();
    }))
    , delayedPresenceTracker_(loadPresenceAbsentTimeoutSecondsFromSettings())
    , presenceDetectionEnabled_(loadPresenceDetectionEnabledFromSettings())
{
    thermalStatus_.controlEnabled = loadFanControlEnabledFromSettings();
    thermalStatus_.fanConfigured = fanController_ && fanController_->isConfigured();

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
        registerSettingsCallbacks();
    }

    if (startThermalControlLoopThread) {
        startThermalControlLoop();
    }
}

PeripheralManager::~PeripheralManager()
{
    if (settingsCallbackGate_) {
        settingsCallbackGate_->owner = nullptr;
        settingsCallbackGate_.reset();
    }

    if (thermalControlThread_.joinable()) {
        thermalControlThread_.request_stop();
        {
            std::lock_guard<std::mutex> lock(thermalControlWakeMutex_);
            thermalControlWakeRequested_ = true;
        }
        thermalControlWakeCv_.notify_all();
    }

    if (mmWaveSensor) {
        mmWaveSensor->setPresenceUpdateCallback({});
    }
}

void PeripheralManager::registerSettingsCallbacks()
{
    settingsCallbackGate_ = std::make_shared<SettingsCallbackGate>();
    settingsCallbackGate_->owner = this;
    const std::weak_ptr<SettingsCallbackGate> weakGate = settingsCallbackGate_;

    const auto makeGuardedCallback = [weakGate](auto callback) {
        return [weakGate, callback]() mutable {
            if (auto gate = weakGate.lock(); gate && gate->owner) {
                callback(*gate->owner);
            }
        };
    };

    const auto registerConfigCallback = [&makeGuardedCallback](GlobalSettings::SettingType settingType) {
        GlobalSettings::setSettingCB(settingType, makeGuardedCallback([](PeripheralManager& owner) {
            owner.syncPresenceConfigFromSettings();
        }));
    };

    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_DETECTION_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_MOVING_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_DISTANCE_AVERAGE_WINDOW_SECS);
    registerConfigCallback(GlobalSettings::SettingType::MMWAVE_STATIONARY_ENERGY_AVERAGE_WINDOW_SECS);
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, makeGuardedCallback([](PeripheralManager& owner) {
        owner.syncPresenceDetectionEnabledFromSettings();
    }));
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::PRESENCE_ABSENT_TIMEOUT_SECS, makeGuardedCallback([](PeripheralManager& owner) {
        owner.syncPresenceAbsentTimeoutFromSettings();
    }));

    const auto registerThermalCallback = [&makeGuardedCallback](GlobalSettings::SettingType settingType) {
        GlobalSettings::setSettingCB(settingType, makeGuardedCallback([](PeripheralManager& owner) {
            owner.requestThermalControlWake();
        }));
    };

    registerThermalCallback(GlobalSettings::SettingType::FAN_CONTROL_ENABLED);
    registerThermalCallback(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS);
    registerThermalCallback(GlobalSettings::SettingType::FAN_CONTROL_HYSTERESIS_C);
    registerThermalCallback(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT);
    registerThermalCallback(GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS);
}

void PeripheralManager::startThermalControlLoop()
{
    if (thermalControlThread_.joinable()) {
        return;
    }

    thermalControlThread_ = std::jthread([this](std::stop_token stopToken) {
        thermalControlLoop(stopToken);
    });
}

void PeripheralManager::thermalControlLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        runThermalControlIteration();

        std::unique_lock<std::mutex> lock(thermalControlWakeMutex_);
        thermalControlWakeCv_.wait_for(
            lock,
            std::chrono::milliseconds(loadFanControlPollIntervalMsFromSettings()),
            [this, &stopToken]() {
                return stopToken.stop_requested() || thermalControlWakeRequested_;
            });
        thermalControlWakeRequested_ = false;
    }
}

void PeripheralManager::requestThermalControlWake()
{
    {
        std::lock_guard<std::mutex> lock(thermalControlWakeMutex_);
        thermalControlWakeRequested_ = true;
    }
    thermalControlWakeCv_.notify_all();
}

void PeripheralManager::setThermalStatusSnapshot(
    const ThermalStatusSnapshot& status,
    std::optional<double> appliedDutyTemperatureC)
{
    std::lock_guard<std::mutex> lock(thermalStatusMutex_);
    thermalStatus_ = status;
    if (appliedDutyTemperatureC.has_value()) {
        lastAppliedDutyTemperatureC_ = appliedDutyTemperatureC;
    } else if (!status.controlEnabled || !status.fanConfigured || status.failsafeActive || status.appliedDutyPercent == 0) {
        lastAppliedDutyTemperatureC_.reset();
    }
}

std::expected<void, I2CError> PeripheralManager::applyFanDutyPercent(uint8_t dutyPercent)
{
    if (!fanController_ || !fanController_->isConfigured()) {
        return std::unexpected(I2CError::NOT_INITIALIZED);
    }

    dutyPercent = static_cast<uint8_t>(std::min<uint8_t>(dutyPercent, 100));
    if (dutyPercent == 0) {
        if (auto result = fanController_->setManualDutyPercent(0); !result) {
            return result;
        }
        if (auto result = fanController_->setControlMode(FanControlMode::Disabled); !result) {
            return result;
        }
        return fanController_->setEnabled(false);
    }

    if (auto result = fanController_->setEnabled(true); !result) {
        return result;
    }
    if (auto result = fanController_->setControlMode(FanControlMode::ManualDuty); !result) {
        return result;
    }
    return fanController_->setManualDutyPercent(dutyPercent);
}

void PeripheralManager::runThermalControlIteration()
{
    const bool controlEnabled = loadFanControlEnabledFromSettings();
    const auto curvePoints = loadFanControlCurvePointsFromSettings();
    const double hysteresisC = loadFanControlHysteresisCFromSettings();
    const uint8_t failsafePercent = loadFanControlFailsafePercentFromSettings();
    const bool fanConfigured = fanController_ && fanController_->isConfigured();
    const auto cpuTemperatureC = cpuTemperatureReader_ ? cpuTemperatureReader_() : std::nullopt;
    const auto systemTemperatureC = systemTemperatureReader_ ? systemTemperatureReader_() : std::nullopt;

    uint8_t appliedDutyPercent = 0;
    std::optional<double> lastAppliedDutyTemperatureC;
    {
        std::lock_guard<std::mutex> lock(thermalStatusMutex_);
        appliedDutyPercent = thermalStatus_.appliedDutyPercent;
        lastAppliedDutyTemperatureC = lastAppliedDutyTemperatureC_;
    }

    ThermalStatusSnapshot nextStatus;
    nextStatus.controlEnabled = controlEnabled;
    nextStatus.fanConfigured = fanConfigured;
    nextStatus.failsafeActive = false;
    nextStatus.cpuTemperatureC = cpuTemperatureC;
    nextStatus.systemTemperatureC = systemTemperatureC;
    nextStatus.appliedDutyPercent = appliedDutyPercent;

    if (!fanConfigured) {
        setThermalStatusSnapshot(nextStatus);
        return;
    }

    if (!controlEnabled) {
        if (appliedDutyPercent != 0) {
            if (auto disableResult = applyFanDutyPercent(0); !disableResult) {
                CubeLog::warning("PeripheralManager: failed to disable fan controller while fan control is disabled.");
            }
        }
        nextStatus.appliedDutyPercent = 0;
        setThermalStatusSnapshot(nextStatus, std::nullopt);
        return;
    }

    if (!cpuTemperatureC.has_value()) {
        nextStatus.failsafeActive = true;
        if (auto failsafeResult = applyFanDutyPercent(failsafePercent); !failsafeResult) {
            CubeLog::warning("PeripheralManager: failed to apply fan failsafe duty after CPU temperature read failure.");
        } else {
            nextStatus.appliedDutyPercent = failsafePercent;
        }
        setThermalStatusSnapshot(nextStatus, std::nullopt);
        return;
    }

    const uint8_t targetDutyPercent = interpolateFanDutyPercent(*cpuTemperatureC, curvePoints);
    bool shouldApplyTarget = targetDutyPercent > appliedDutyPercent;
    if (!shouldApplyTarget && targetDutyPercent < appliedDutyPercent) {
        shouldApplyTarget = !lastAppliedDutyTemperatureC.has_value()
            || *cpuTemperatureC <= (*lastAppliedDutyTemperatureC - hysteresisC);
    }

    if (!shouldApplyTarget) {
        setThermalStatusSnapshot(nextStatus);
        return;
    }

    if (auto applyResult = applyFanDutyPercent(targetDutyPercent); !applyResult) {
        CubeLog::warning("PeripheralManager: failed to apply target fan duty, entering failsafe.");
        nextStatus.failsafeActive = true;
        if (auto failsafeResult = applyFanDutyPercent(failsafePercent); !failsafeResult) {
            CubeLog::warning("PeripheralManager: failed to apply fan failsafe duty after target-duty failure.");
        } else {
            nextStatus.appliedDutyPercent = failsafePercent;
        }
        setThermalStatusSnapshot(nextStatus, std::nullopt);
        return;
    }

    nextStatus.appliedDutyPercent = targetDutyPercent;
    setThermalStatusSnapshot(nextStatus, cpuTemperatureC);
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

ThermalStatusSnapshot PeripheralManager::getThermalStatus() const
{
    std::lock_guard<std::mutex> lock(thermalStatusMutex_);
    return thermalStatus_;
}

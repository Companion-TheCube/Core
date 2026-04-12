#include "../../src/hardware/peripheralManager.h"
#include "../../src/settings/globalSettings.h"
#include <gtest/gtest.h>

#include <thread>

namespace {

using Clock = std::chrono::steady_clock;

class FakeFanController final : public FanController {
public:
    FakeFanController()
        : FanController(nullptr, 0x2A)
    {
    }

    bool isConfigured() const override
    {
        return configured;
    }

    std::expected<void, I2CError> setEnabled(bool enabledValue) const override
    {
        if (commandError.has_value()) {
            return std::unexpected(*commandError);
        }
        enabled = enabledValue;
        return { };
    }

    std::expected<void, I2CError> setControlMode(FanControlMode modeValue) const override
    {
        if (commandError.has_value()) {
            return std::unexpected(*commandError);
        }
        mode = modeValue;
        return { };
    }

    std::expected<void, I2CError> setManualDutyPercent(uint8_t dutyPercentValue) const override
    {
        if (commandError.has_value()) {
            return std::unexpected(*commandError);
        }
        dutyPercent = dutyPercentValue;
        dutyHistory.push_back(dutyPercentValue);
        return { };
    }

    std::expected<FanStatus, I2CError> readStatus() const override
    {
        if (commandError.has_value()) {
            return std::unexpected(*commandError);
        }

        FanStatus status;
        status.enabled = enabled;
        status.mode = mode;
        status.targetDutyPercent = dutyPercent;
        status.protocolVersion = 1;
        if (tachRpm.has_value()) {
            status.tachRpm = tachRpm;
            status.statusFlags |= 0x04;
        }
        return status;
    }

    mutable bool configured = true;
    mutable bool enabled = false;
    mutable FanControlMode mode = FanControlMode::Disabled;
    mutable uint8_t dutyPercent = 0;
    mutable std::vector<uint8_t> dutyHistory;
    mutable std::optional<uint16_t> tachRpm;
    mutable std::optional<I2CError> commandError;
};

class TestPeripheralManager : public PeripheralManager {
public:
    using PeripheralManager::handleImmediatePresenceDecision;
    using PeripheralManager::runThermalControlIteration;
    using PeripheralManager::syncPresenceDetectionEnabledFromSettings;

    TestPeripheralManager(
        std::unique_ptr<mmWave> mmWaveSensorOverride,
        std::unique_ptr<FanController> fanControllerOverride = nullptr,
        TemperatureReader cpuTemperatureReader = {},
        TemperatureReader systemTemperatureReader = {},
        bool registerSettingCallbacks = false,
        bool startThermalControlLoopThread = false)
        : PeripheralManager(
            std::move(mmWaveSensorOverride),
            std::move(fanControllerOverride),
            std::move(cpuTemperatureReader),
            std::move(systemTemperatureReader),
            registerSettingCallbacks,
            startThermalControlLoopThread)
    {
    }
};

TEST(DelayedPresenceTrackerTest, ImmediatePresentMakesDelayedPresentImmediately)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    const PresenceStatusSnapshot status = tracker.updateImmediateState(
        MmWavePresenceState::Present,
        now);

    EXPECT_EQ(status.immediateState, MmWavePresenceState::Present);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);
    EXPECT_EQ(status.absentTimeoutSecs, 15);
}

TEST(DelayedPresenceTrackerTest, KeepsDelayedPresentUntilAbsentTimeoutExpires)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    tracker.updateImmediateState(MmWavePresenceState::Present, now);
    PresenceStatusSnapshot status = tracker.updateImmediateState(
        MmWavePresenceState::Absent,
        now + std::chrono::seconds(1));

    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);

    status = tracker.snapshot(now + std::chrono::seconds(15));
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);

    status = tracker.snapshot(now + std::chrono::seconds(16));
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Absent);
}

TEST(DelayedPresenceTrackerTest, UnknownStateHoldsDelayedStateWithoutAdvancingTimeout)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    tracker.updateImmediateState(MmWavePresenceState::Present, now);
    tracker.updateImmediateState(MmWavePresenceState::Absent, now + std::chrono::seconds(1));
    tracker.updateImmediateState(MmWavePresenceState::Unknown, now + std::chrono::seconds(6));

    const PresenceStatusSnapshot status = tracker.snapshot(now + std::chrono::seconds(30));

    EXPECT_EQ(status.immediateState, MmWavePresenceState::Unknown);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);
}

TEST(DelayedPresenceTrackerTest, ColdStartAbsentTransitionsAfterTimeout)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    PresenceStatusSnapshot status = tracker.updateImmediateState(
        MmWavePresenceState::Absent,
        now);

    EXPECT_EQ(status.immediateState, MmWavePresenceState::Absent);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Unknown);

    status = tracker.snapshot(now + std::chrono::seconds(14));
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Unknown);

    status = tracker.snapshot(now + std::chrono::seconds(15));
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Absent);
}

TEST(DelayedPresenceTrackerTest, UpdatingTimeoutAppliesToCurrentAbsentWindow)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    tracker.updateImmediateState(MmWavePresenceState::Present, now);
    tracker.updateImmediateState(MmWavePresenceState::Absent, now + std::chrono::seconds(1));
    tracker.setAbsentTimeoutSecs(5, now + std::chrono::seconds(7));

    const PresenceStatusSnapshot status = tracker.snapshot(now + std::chrono::seconds(7));

    EXPECT_EQ(status.immediateState, MmWavePresenceState::Absent);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Absent);
    EXPECT_EQ(status.absentTimeoutSecs, 5);
}

TEST(DelayedPresenceTrackerTest, ResetClearsPresenceStates)
{
    DelayedPresenceTracker tracker(15);
    const auto now = Clock::now();

    tracker.updateImmediateState(MmWavePresenceState::Present, now);
    tracker.reset();

    const PresenceStatusSnapshot status = tracker.snapshot(now + std::chrono::seconds(1));
    EXPECT_EQ(status.immediateState, MmWavePresenceState::Unknown);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Unknown);
    EXPECT_EQ(status.absentTimeoutSecs, 15);
}

TEST(PeripheralManagerTest, StatusEndpointReturnsImmediateAndDelayedPresenceFields)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, true);
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_ABSENT_TIMEOUT_SECS, 21);

    TestPeripheralManager manager(nullptr);
    const auto endpoints = manager.getHttpEndpointData();

    ASSERT_EQ(endpoints.size(), 1u);
    EXPECT_EQ(std::get<2>(endpoints[0]), "status");
    EXPECT_EQ(
        std::get<0>(endpoints[0]),
        PUBLIC_ENDPOINT | GET_ENDPOINT);

    httplib::Request req;
    httplib::Response res;

    const auto error = std::get<1>(endpoints[0])(req, res);

    EXPECT_EQ(error.errorType, EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR);
    EXPECT_EQ(res.status, 200);

    const auto body = nlohmann::json::parse(res.body);
    EXPECT_EQ(body.value("immediateState", std::string()), "unknown");
    EXPECT_EQ(body.value("delayedState", std::string()), "unknown");
    EXPECT_FALSE(body.value("immediatePresent", true));
    EXPECT_FALSE(body.value("delayedPresent", true));
    EXPECT_EQ(body.value("absentTimeoutSecs", 0), 21);
}

TEST(PeripheralManagerTest, PresenceSettingDisableResetsStateAndIgnoresUpdates)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, true);

    TestPeripheralManager manager(nullptr);
    const auto now = Clock::now();

    MmWavePresenceDecision presentDecision;
    presentDecision.state = MmWavePresenceState::Present;
    presentDecision.timestamp = now;
    manager.handleImmediatePresenceDecision(presentDecision);

    auto status = manager.getPresenceStatus();
    EXPECT_EQ(status.immediateState, MmWavePresenceState::Present);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);

    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, false);
    manager.syncPresenceDetectionEnabledFromSettings();

    status = manager.getPresenceStatus();
    EXPECT_EQ(status.immediateState, MmWavePresenceState::Unknown);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Unknown);

    MmWavePresenceDecision secondPresentDecision;
    secondPresentDecision.state = MmWavePresenceState::Present;
    secondPresentDecision.timestamp = now + std::chrono::seconds(1);
    manager.handleImmediatePresenceDecision(secondPresentDecision);

    status = manager.getPresenceStatus();
    EXPECT_EQ(status.immediateState, MmWavePresenceState::Unknown);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Unknown);
}

TEST(PeripheralManagerTest, PresenceSettingEnableAllowsUpdatesAgain)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, false);

    TestPeripheralManager manager(nullptr);
    const auto now = Clock::now();

    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, true);
    manager.syncPresenceDetectionEnabledFromSettings();

    MmWavePresenceDecision presentDecision;
    presentDecision.state = MmWavePresenceState::Present;
    presentDecision.timestamp = now;
    manager.handleImmediatePresenceDecision(presentDecision);

    const auto status = manager.getPresenceStatus();
    EXPECT_EQ(status.immediateState, MmWavePresenceState::Present);
    EXPECT_EQ(status.delayedState, MmWavePresenceState::Present);
}

TEST(PeripheralManagerThermalTest, InterpolatesDutyFromCurve)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(
        GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS,
        nlohmann::json::array({
            nlohmann::json::array({ 50.0, 40 }),
            nlohmann::json::array({ 65.0, 70 }),
        }));

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();
    std::optional<double> cpuTemperatureC = 57.5;
    std::optional<double> systemTemperatureC = 58.0;

    TestPeripheralManager manager(
        nullptr,
        std::move(fanController),
        [&cpuTemperatureC]() { return cpuTemperatureC; },
        [&systemTemperatureC]() { return systemTemperatureC; });

    manager.runThermalControlIteration();

    ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 55);

    const auto status = manager.getThermalStatus();
    EXPECT_EQ(status.appliedDutyPercent, 55);
    ASSERT_TRUE(status.cpuTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.cpuTemperatureC, 57.5);
    ASSERT_TRUE(status.systemTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.systemTemperatureC, 58.0);
}

TEST(PeripheralManagerThermalTest, DecreasesDutyOnlyAfterHysteresisMargin)
{
    GlobalSettings defaults;

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();
    std::optional<double> cpuTemperatureC = 65.0;

    TestPeripheralManager manager(
        nullptr,
        std::move(fanController),
        [&cpuTemperatureC]() { return cpuTemperatureC; },
        []() { return std::optional<double>(60.0); });

    manager.runThermalControlIteration();
    ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 70);

    cpuTemperatureC = 64.0;
    manager.runThermalControlIteration();
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 70);

    cpuTemperatureC = 62.0;
    manager.runThermalControlIteration();
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 64);
}

TEST(PeripheralManagerThermalTest, MissingCpuTemperatureTriggersFailsafeDuty)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_FAILSAFE_PERCENT, 90);

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();

    TestPeripheralManager manager(
        nullptr,
        std::move(fanController),
        []() { return std::optional<double> {}; },
        []() { return std::optional<double>(61.0); });

    manager.runThermalControlIteration();

    ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 90);

    const auto status = manager.getThermalStatus();
    EXPECT_TRUE(status.failsafeActive);
    EXPECT_EQ(status.appliedDutyPercent, 90);
    EXPECT_FALSE(status.cpuTemperatureC.has_value());
    ASSERT_TRUE(status.systemTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.systemTemperatureC, 61.0);
}

TEST(PeripheralManagerThermalTest, DisabledControlTurnsFanOff)
{
    GlobalSettings defaults;

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();
    std::optional<double> cpuTemperatureC = 65.0;

    TestPeripheralManager manager(
        nullptr,
        std::move(fanController),
        [&cpuTemperatureC]() { return cpuTemperatureC; },
        []() { return std::optional<double>(60.0); });

    manager.runThermalControlIteration();
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 70);

    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_ENABLED, false);
    manager.runThermalControlIteration();

    EXPECT_FALSE(fanControllerPtr->enabled);
    EXPECT_EQ(fanControllerPtr->mode, FanControlMode::Disabled);
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 0);

    const auto status = manager.getThermalStatus();
    EXPECT_FALSE(status.controlEnabled);
    EXPECT_EQ(status.appliedDutyPercent, 0);
}

TEST(PeripheralManagerThermalTest, MissingFanConfigurationLeavesThermalStatusReadable)
{
    GlobalSettings defaults;
    std::optional<double> cpuTemperatureC = 67.0;
    std::optional<double> systemTemperatureC = 65.0;

    TestPeripheralManager manager(
        nullptr,
        nullptr,
        [&cpuTemperatureC]() { return cpuTemperatureC; },
        [&systemTemperatureC]() { return systemTemperatureC; });

    manager.runThermalControlIteration();

    const auto status = manager.getThermalStatus();
    EXPECT_TRUE(status.controlEnabled);
    EXPECT_FALSE(status.fanConfigured);
    EXPECT_FALSE(status.failsafeActive);
    ASSERT_TRUE(status.cpuTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.cpuTemperatureC, 67.0);
    ASSERT_TRUE(status.systemTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.systemTemperatureC, 65.0);
    EXPECT_EQ(status.appliedDutyPercent, 0);
}

TEST(PeripheralManagerThermalTest, CommandFailureMarksFailsafeActive)
{
    GlobalSettings defaults;

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();
    fanControllerPtr->commandError = I2CError::IO_FAILED;

    TestPeripheralManager manager(
        nullptr,
        std::move(fanController),
        []() { return std::optional<double>(72.0); },
        []() { return std::optional<double>(70.0); });

    manager.runThermalControlIteration();

    const auto status = manager.getThermalStatus();
    EXPECT_TRUE(status.failsafeActive);
    EXPECT_EQ(status.appliedDutyPercent, 0);
}

TEST(PeripheralManagerThermalTest, RegisteredThermalSettingCallbacksWakeLoopAndApplyNewCurve)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::FAN_CONTROL_POLL_INTERVAL_MS, 10000);

    auto fanController = std::make_unique<FakeFanController>();
    auto* fanControllerPtr = fanController.get();
    std::optional<double> cpuTemperatureC = 50.0;

    {
        TestPeripheralManager manager(
            nullptr,
            std::move(fanController),
            [&cpuTemperatureC]() { return cpuTemperatureC; },
            []() { return std::optional<double>(48.0); },
            true,
            true);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
        EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 40);

        GlobalSettings::setSetting(
            GlobalSettings::SettingType::FAN_CONTROL_CURVE_POINTS,
            nlohmann::json::array({
                nlohmann::json::array({ 35.0, 60 }),
                nlohmann::json::array({ 80.0, 60 }),
            }));

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 60);
    }
}

} // namespace

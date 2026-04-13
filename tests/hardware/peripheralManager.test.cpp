#include "../../src/hardware/interactionEvents.h"
#include "../../src/hardware/peripheralManager.h"
#include "../../src/settings/globalSettings.h"
#include <gtest/gtest.h>

#include <deque>
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

class FakeAccelerometer final : public Bmi270Accelerometer {
public:
    FakeAccelerometer()
        : Bmi270Accelerometer(nullptr, 0x68)
    {
    }

    bool isConfigured() const override
    {
        return configured;
    }

    bool isInitialized() const override
    {
        return initialized;
    }

    bool isAvailable() const override
    {
        return available;
    }

    std::expected<void, Bmi270Error> initialize() override
    {
        initCalls++;
        if (initError.has_value()) {
            initialized = false;
            available = false;
            return std::unexpected(*initError);
        }
        initialized = true;
        available = true;
        return { };
    }

    std::expected<void, Bmi270Error> configureInteractionDetection(const Bmi270InteractionConfig& config) override
    {
        configuredInteraction = config;
        if (configError.has_value()) {
            return std::unexpected(*configError);
        }
        return { };
    }

    std::expected<Bmi270InteractionStatus, Bmi270Error> pollInteractionStatus() override
    {
        pollCalls++;
        if (pollError.has_value()) {
            return std::unexpected(*pollError);
        }
        if (!queuedStatuses.empty()) {
            latestStatus = queuedStatuses.front();
            queuedStatuses.pop_front();
        }
        return latestStatus;
    }

    bool configured = true;
    mutable bool initialized = false;
    mutable bool available = true;
    std::optional<Bmi270Error> initError;
    std::optional<Bmi270Error> configError;
    std::optional<Bmi270Error> pollError;
    mutable size_t initCalls = 0;
    mutable size_t pollCalls = 0;
    mutable std::deque<Bmi270InteractionStatus> queuedStatuses;
    mutable Bmi270InteractionStatus latestStatus;
    mutable Bmi270InteractionConfig configuredInteraction;
};

class TestPeripheralManager : public PeripheralManager {
public:
    using PeripheralManager::handleImmediatePresenceDecision;
    using PeripheralManager::runInteractionControlIteration;
    using PeripheralManager::runThermalControlIteration;
    using PeripheralManager::syncPresenceDetectionEnabledFromSettings;

    TestPeripheralManager(
        std::unique_ptr<mmWave> mmWaveSensorOverride,
        std::unique_ptr<FanController> fanControllerOverride = nullptr,
        std::unique_ptr<Bmi270Accelerometer> accelerometerOverride = nullptr,
        TemperatureReader cpuTemperatureReader = {},
        TemperatureReader systemTemperatureReader = {},
        MonotonicNowReader monotonicNowReader = {},
        EpochMsReader epochMsReader = {},
        bool registerSettingCallbacks = false,
        bool startThermalControlLoopThread = false,
        bool startInteractionControlLoopThread = false)
        : PeripheralManager(
            std::move(mmWaveSensorOverride),
            std::move(fanControllerOverride),
            std::move(accelerometerOverride),
            std::move(cpuTemperatureReader),
            std::move(systemTemperatureReader),
            std::move(monotonicNowReader),
            std::move(epochMsReader),
            registerSettingCallbacks,
            startThermalControlLoopThread,
            startInteractionControlLoopThread)
    {
    }
};

Bmi270AccelerationSample sample(float xG, float yG, float zG)
{
    return Bmi270AccelerationSample { xG, yG, zG };
}

Bmi270InteractionStatus interactionStatus(
    const Bmi270AccelerationSample& accelSample,
    bool tapDetected = false,
    bool motionDetected = false,
    bool noMotionDetected = false)
{
    Bmi270InteractionStatus status;
    status.latestSample = accelSample;
    status.interruptStatus.tapDetected = tapDetected;
    status.interruptStatus.motionDetected = motionDetected;
    status.interruptStatus.noMotionDetected = noMotionDetected;
    return status;
}

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
        nullptr,
        [&cpuTemperatureC]() { return cpuTemperatureC; },
        [&systemTemperatureC]() { return systemTemperatureC; });

    manager.runThermalControlIteration();

    ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 55);

    const auto status = manager.getThermalStatus();
    EXPECT_EQ(status.appliedDutyPercent, 55);
    ASSERT_TRUE(status.cpuTemperatureC.has_value());
    EXPECT_DOUBLE_EQ(*status.cpuTemperatureC, 57.5);
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
        nullptr,
        []() { return std::optional<double> {}; },
        []() { return std::optional<double>(61.0); });

    manager.runThermalControlIteration();

    ASSERT_FALSE(fanControllerPtr->dutyHistory.empty());
    EXPECT_EQ(fanControllerPtr->dutyHistory.back(), 90);

    const auto status = manager.getThermalStatus();
    EXPECT_TRUE(status.failsafeActive);
    EXPECT_EQ(status.appliedDutyPercent, 90);
}

TEST(PeripheralManagerInteractionTest, TapEventUpdatesSnapshotAndPublishesBusEvent)
{
    GlobalSettings defaults;

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    auto* accelerometerPtr = accelerometer.get();
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.6f, 0.0f, 1.3f), true, true, false));

    auto nowMono = Clock::now();
    uint64_t nowEpochMs = 1000;

    TestPeripheralManager manager(
        nullptr,
        nullptr,
        std::move(accelerometer),
        {},
        {},
        [&nowMono]() { return nowMono; },
        [&nowEpochMs]() { return nowEpochMs; });

    std::vector<InteractionEvent> seenEvents;
    const auto handle = InteractionEvents::subscribe([&seenEvents](const InteractionEvent& event) {
        seenEvents.push_back(event);
    });

    manager.runInteractionControlIteration();
    nowMono += std::chrono::milliseconds(200);
    nowEpochMs += 200;
    manager.runInteractionControlIteration();

    InteractionEvents::unsubscribe(handle);

    const auto status = manager.getInteractionStatus();
    EXPECT_TRUE(status.available);
    EXPECT_TRUE(status.initialized);
    ASSERT_TRUE(status.lastTapAtEpochMs.has_value());
    EXPECT_EQ(*status.lastTapAtEpochMs, 1200u);
    EXPECT_EQ(status.lastEventSequence, 1u);

    ASSERT_EQ(seenEvents.size(), 1u);
    EXPECT_EQ(seenEvents[0].type, InteractionEventType::Tap);
    EXPECT_EQ(seenEvents[0].sequence, 1u);
}

TEST(PeripheralManagerInteractionTest, LiftTransitionsFromDeskToLiftedAndBack)
{
    GlobalSettings defaults;

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    auto* accelerometerPtr = accelerometer.get();
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.7f, 0.0f, 0.6f), false, true, false));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.9f, 0.0f, 0.2f), false, true, false));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));
    accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));

    auto nowMono = Clock::now();
    uint64_t nowEpochMs = 1000;

    TestPeripheralManager manager(
        nullptr,
        nullptr,
        std::move(accelerometer),
        {},
        {},
        [&nowMono]() { return nowMono; },
        [&nowEpochMs]() { return nowEpochMs; });

    std::vector<InteractionEvent> seenEvents;
    const auto handle = InteractionEvents::subscribe([&seenEvents](const InteractionEvent& event) {
        seenEvents.push_back(event);
    });

    manager.runInteractionControlIteration();
    nowMono += std::chrono::milliseconds(600);
    nowEpochMs += 600;
    manager.runInteractionControlIteration();

    EXPECT_EQ(manager.getInteractionStatus().liftState, Bmi270LiftState::OnDesk);

    nowMono += std::chrono::milliseconds(100);
    nowEpochMs += 100;
    manager.runInteractionControlIteration();

    nowMono += std::chrono::milliseconds(200);
    nowEpochMs += 200;
    manager.runInteractionControlIteration();

    EXPECT_EQ(manager.getInteractionStatus().liftState, Bmi270LiftState::Lifted);

    nowMono += std::chrono::milliseconds(300);
    nowEpochMs += 300;
    manager.runInteractionControlIteration();

    nowMono += std::chrono::milliseconds(600);
    nowEpochMs += 600;
    manager.runInteractionControlIteration();

    InteractionEvents::unsubscribe(handle);

    const auto status = manager.getInteractionStatus();
    EXPECT_EQ(status.liftState, Bmi270LiftState::OnDesk);
    ASSERT_TRUE(status.lastLiftChangeAtEpochMs.has_value());
    EXPECT_EQ(*status.lastLiftChangeAtEpochMs, 2800u);

    ASSERT_EQ(seenEvents.size(), 2u);
    EXPECT_EQ(seenEvents[0].type, InteractionEventType::LiftStarted);
    EXPECT_EQ(seenEvents[1].type, InteractionEventType::LiftEnded);
}

TEST(PeripheralManagerInteractionTest, EventHistoryReportsTruncationWhenCallerFallsBehind)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_EVENT_HISTORY_SIZE, 32);

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    auto* accelerometerPtr = accelerometer.get();
    for (int i = 0; i < 40; ++i) {
        accelerometerPtr->queuedStatuses.push_back(interactionStatus(sample(0.7f, 0.0f, 1.2f), true, true, false));
    }

    auto nowMono = Clock::now();
    uint64_t nowEpochMs = 1000;

    TestPeripheralManager manager(
        nullptr,
        nullptr,
        std::move(accelerometer),
        {},
        {},
        [&nowMono]() { return nowMono; },
        [&nowEpochMs]() { return nowEpochMs; });

    for (int i = 0; i < 40; ++i) {
        manager.runInteractionControlIteration();
        nowMono += std::chrono::milliseconds(200);
        nowEpochMs += 200;
    }

    const auto page = manager.getInteractionEvents(0, 10);
    EXPECT_TRUE(page.historyTruncated);
    EXPECT_EQ(page.events.size(), 10u);
    EXPECT_EQ(page.nextSequence, 41u);
    EXPECT_GT(page.events.front().sequence, 1u);
}

TEST(PeripheralManagerInteractionTest, RegisteredInteractionCallbacksWakeLoopAndApplyDisable)
{
    GlobalSettings defaults;
    GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_POLL_INTERVAL_MS, 500);

    auto accelerometer = std::make_unique<FakeAccelerometer>();
    accelerometer->queuedStatuses.push_back(interactionStatus(sample(0.0f, 0.0f, 1.0f), false, false, true));

    {
        TestPeripheralManager manager(
            nullptr,
            nullptr,
            std::move(accelerometer),
            {},
            {},
            {},
            {},
            true,
            false,
            true);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_TRUE(manager.getInteractionStatus().enabled);

        GlobalSettings::setSetting(GlobalSettings::SettingType::INTERACTION_DETECTION_ENABLED, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        EXPECT_FALSE(manager.getInteractionStatus().enabled);
    }
}

} // namespace

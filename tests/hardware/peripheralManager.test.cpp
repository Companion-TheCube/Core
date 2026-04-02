#include "../../src/hardware/peripheralManager.h"
#include "../../src/settings/globalSettings.h"
#include <gtest/gtest.h>

namespace {

using Clock = std::chrono::steady_clock;

class TestPeripheralManager : public PeripheralManager {
public:
    using PeripheralManager::PeripheralManager;
    using PeripheralManager::handleImmediatePresenceDecision;
    using PeripheralManager::syncPresenceDetectionEnabledFromSettings;
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
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, true);
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_ABSENT_TIMEOUT_SECS, 21);

    TestPeripheralManager manager(nullptr, false);
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
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, true);

    TestPeripheralManager manager(nullptr, false);
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
    GlobalSettings::setSetting(GlobalSettings::SettingType::PRESENCE_DETECTION_ENABLED, false);

    TestPeripheralManager manager(nullptr, false);
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

} // namespace

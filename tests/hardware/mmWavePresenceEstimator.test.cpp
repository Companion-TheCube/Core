#include "../../src/hardware/mmWave.h"
#include "../../src/hardware/mmWavePresenceEstimator.h"
#include <gtest/gtest.h>

namespace {

using Clock = std::chrono::steady_clock;

MmWaveReading makeReading(
    uint8_t targetState,
    uint16_t movingDistance,
    uint16_t stationaryDistance,
    uint16_t detectionDistance,
    uint8_t stationaryEnergy = 85,
    uint8_t movingEnergy = 0)
{
    MmWaveReading reading;
    reading.targetState = targetState;
    reading.movingTargetDistance = movingDistance;
    reading.movingTargetEnergy = movingEnergy;
    reading.stationaryTargetDistance = stationaryDistance;
    reading.stationaryTargetEnergy = stationaryEnergy;
    reading.detectionDistance = detectionDistance;
    return reading;
}

} // namespace

TEST(MmWavePresenceEstimatorTest, MarksPresentWhenAllRelevantAveragesAreBelowPresentThreshold)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 90, 95, 92, 85, 40),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Present);
    EXPECT_TRUE(estimator.isPresent());
}

TEST(MmWavePresenceEstimatorTest, MarksAbsentWhenAllRelevantAveragesExceedAbsentThresholds)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 210, 205, 160, 85, 30),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Absent);
    EXPECT_FALSE(estimator.isPresent());
}

TEST(MmWavePresenceEstimatorTest, HoldsPresentStateInsideHysteresisBand)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    estimator.update(makeReading(0x03u, 80, 85, 90, 85, 20), now);
    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 120, 130, 110, 85, 20),
        now + std::chrono::seconds(1));

    EXPECT_EQ(decision.state, MmWavePresenceState::Present);
    EXPECT_TRUE(estimator.isPresent());
}

TEST(MmWavePresenceEstimatorTest, IgnoresStationaryDistanceWhenLatestStateIsMovingOnly)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x01u, 210, 80, 160, 85, 35),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Absent);
    EXPECT_FALSE(estimator.isPresent());
}

TEST(MmWavePresenceEstimatorTest, IgnoresMovingDistanceWhenLatestStateIsStationaryOnly)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x02u, 250, 95, 90, 85, 20),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Present);
    EXPECT_TRUE(estimator.isPresent());
}

TEST(MmWavePresenceEstimatorTest, ReducesAbsentThresholdsWhenLatestStateIsNone)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x00u, 0, 0, 132, 85, 0),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Absent);
    EXPECT_FLOAT_EQ(decision.absentDetectionDistanceThresholdCm, 127.5f);
}

TEST(MmWavePresenceEstimatorTest, ReducesAbsentThresholdsFurtherWhenStationaryEnergyAverageIsLow)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 182, 184, 138, 60, 10),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Absent);
    EXPECT_FLOAT_EQ(decision.absentDetectionDistanceThresholdCm, 135.0f);
    EXPECT_FLOAT_EQ(decision.absentMovingDistanceThresholdCm, 180.0f);
    EXPECT_FLOAT_EQ(decision.absentStationaryDistanceThresholdCm, 180.0f);
}

TEST(MmWavePresenceEstimatorTest, UsesIndependentAverageWindowsForEachMetric)
{
    MmWavePresenceConfig config;
    config.detectionDistanceAverageWindowSecs = 2;
    config.movingDistanceAverageWindowSecs = 10;
    config.stationaryDistanceAverageWindowSecs = 10;
    config.stationaryEnergyAverageWindowSecs = 10;

    MmWavePresenceEstimator estimator(config);
    const auto now = Clock::now();

    estimator.update(makeReading(0x03u, 100, 100, 200, 80, 10), now - std::chrono::seconds(8));
    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 200, 200, 100, 80, 10),
        now);

    ASSERT_TRUE(decision.detectionDistanceAverageCm.has_value());
    ASSERT_TRUE(decision.movingTargetDistanceAverageCm.has_value());
    ASSERT_TRUE(decision.stationaryTargetDistanceAverageCm.has_value());
    EXPECT_FLOAT_EQ(*decision.detectionDistanceAverageCm, 100.0f);
    EXPECT_FLOAT_EQ(*decision.movingTargetDistanceAverageCm, 150.0f);
    EXPECT_FLOAT_EQ(*decision.stationaryTargetDistanceAverageCm, 150.0f);
}

TEST(MmWavePresenceEstimatorTest, SetConfigPreservesStateAndRecomputesUsingExistingBuffer)
{
    MmWavePresenceConfig config;
    config.detectionDistanceAverageWindowSecs = 10;
    config.movingDistanceAverageWindowSecs = 10;
    config.stationaryDistanceAverageWindowSecs = 10;
    config.stationaryEnergyAverageWindowSecs = 10;

    MmWavePresenceEstimator estimator(config);
    const auto now = Clock::now();

    estimator.update(makeReading(0x03u, 80, 80, 80, 85, 10), now - std::chrono::seconds(8));
    MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 110, 110, 110, 85, 10),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Present);

    MmWavePresenceConfig updatedConfig;
    updatedConfig.detectionDistanceAverageWindowSecs = 2;
    updatedConfig.movingDistanceAverageWindowSecs = 2;
    updatedConfig.stationaryDistanceAverageWindowSecs = 2;
    updatedConfig.stationaryEnergyAverageWindowSecs = 2;
    estimator.setConfig(updatedConfig);

    decision = estimator.decision();
    ASSERT_TRUE(decision.detectionDistanceAverageCm.has_value());
    ASSERT_TRUE(decision.movingTargetDistanceAverageCm.has_value());
    ASSERT_TRUE(decision.stationaryTargetDistanceAverageCm.has_value());
    EXPECT_FLOAT_EQ(*decision.detectionDistanceAverageCm, 110.0f);
    EXPECT_FLOAT_EQ(*decision.movingTargetDistanceAverageCm, 110.0f);
    EXPECT_FLOAT_EQ(*decision.stationaryTargetDistanceAverageCm, 110.0f);
    EXPECT_EQ(decision.state, MmWavePresenceState::Present);
}

TEST(MmWavePresenceEstimatorTest, DiscardsSamplesOutsideLargestConfiguredWindow)
{
    MmWavePresenceEstimator estimator;
    const auto now = Clock::now();

    estimator.update(makeReading(0x03u, 210, 205, 160, 85, 15), now - std::chrono::seconds(20));
    const MmWavePresenceDecision decision = estimator.update(
        makeReading(0x03u, 90, 95, 92, 85, 15),
        now);

    EXPECT_EQ(decision.state, MmWavePresenceState::Present);
    ASSERT_TRUE(decision.detectionDistanceAverageCm.has_value());
    EXPECT_FLOAT_EQ(*decision.detectionDistanceAverageCm, 92.0f);
}

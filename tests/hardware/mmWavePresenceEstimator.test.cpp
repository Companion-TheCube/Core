#include "../../src/hardware/mmWave.h"
#include "../../src/hardware/mmWavePresenceEstimator.h"
#include <gtest/gtest.h>

namespace {
MmWaveReading stationaryDeskReading()
{
    MmWaveReading reading;
    reading.targetState = 0x02u;
    reading.stationaryTargetDistance = 90;
    reading.stationaryTargetEnergy = 70;
    reading.detectionDistance = 90;
    return reading;
}

MmWaveReading emptyDeskReading()
{
    MmWaveReading reading;
    reading.targetState = 0x00u;
    reading.detectionDistance = 90;
    return reading;
}
}

TEST(MmWavePresenceEstimatorTest, SeatedStationaryDeskCaseBecomesOccupied)
{
    MmWavePresenceEstimator estimator;
    const auto now = std::chrono::steady_clock::now();

    const float confidence = estimator.update(stationaryDeskReading(), now);

    EXPECT_GT(confidence, 0.9f);
    EXPECT_TRUE(estimator.isOccupied());
}

TEST(MmWavePresenceEstimatorTest, BriefLeanOutDoesNotVacateImmediately)
{
    MmWavePresenceEstimator estimator;
    const auto now = std::chrono::steady_clock::now();

    estimator.update(stationaryDeskReading(), now);
    const float confidenceAfterLeanOut = estimator.update(
        emptyDeskReading(), now + std::chrono::milliseconds(500));

    EXPECT_GT(confidenceAfterLeanOut, 0.8f);
    EXPECT_TRUE(estimator.isOccupied());
}

TEST(MmWavePresenceEstimatorTest, MotionOutsideDeskZoneStaysBelowOccupancy)
{
    MmWavePresenceEstimator estimator;
    MmWaveReading hallwayMotion;
    hallwayMotion.targetState = 0x01u;
    hallwayMotion.movingTargetDistance = 240;
    hallwayMotion.movingTargetEnergy = 100;
    hallwayMotion.detectionDistance = 240;

    const auto now = std::chrono::steady_clock::now();
    estimator.update(hallwayMotion, now);
    estimator.update(hallwayMotion, now + std::chrono::seconds(1));

    EXPECT_LT(estimator.confidence(), 0.1f);
    EXPECT_FALSE(estimator.isOccupied());
}

TEST(MmWavePresenceEstimatorTest, HysteresisDoesNotFlapNearThresholds)
{
    MmWavePresenceEstimator estimator;
    const auto now = std::chrono::steady_clock::now();

    MmWaveReading mediumEvidence;
    mediumEvidence.targetState = 0x01u;
    mediumEvidence.movingTargetDistance = 90;
    mediumEvidence.movingTargetEnergy = 40;
    mediumEvidence.detectionDistance = 90;

    estimator.update(stationaryDeskReading(), now);
    estimator.update(mediumEvidence, now + std::chrono::seconds(1));
    EXPECT_TRUE(estimator.isOccupied());

    estimator.reset();
    estimator.update(mediumEvidence, now);
    estimator.update(mediumEvidence, now + std::chrono::seconds(1));
    EXPECT_FALSE(estimator.isOccupied());
}

TEST(MmWavePresenceEstimatorTest, EmptyDeskDecaysToVacant)
{
    MmWavePresenceEstimator estimator;
    const auto now = std::chrono::steady_clock::now();

    estimator.update(stationaryDeskReading(), now);
    for (int second = 1; second <= 20; ++second) {
        estimator.update(emptyDeskReading(), now + std::chrono::seconds(second));
    }

    EXPECT_LT(estimator.confidence(), 0.35f);
    EXPECT_FALSE(estimator.isOccupied());
}

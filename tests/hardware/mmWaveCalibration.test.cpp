#include "../../src/hardware/mmWaveCalibration.h"
#include <gtest/gtest.h>

TEST(MmWaveCalibrationTest, DerivesDeskParametersAndGatesFromSamples)
{
    MmWaveCalibrationSamples samples;

    for (int i = 0; i < 5; ++i) {
        MmWaveReading empty;
        empty.movingTargetEnergy = 5;
        empty.stationaryTargetEnergy = 4;
        samples.emptySamples.push_back(empty);
    }

    for (int i = 0; i < 5; ++i) {
        MmWaveReading moving;
        moving.targetState = 0x01u;
        moving.movingTargetEnergy = 30;
        moving.movingTargetDistance = 150;
        samples.movingSamples.push_back(moving);
    }

    for (int i = 0; i < 3; ++i) {
        MmWaveReading resting;
        resting.targetState = 0x02u;
        resting.stationaryTargetEnergy = 40;
        resting.stationaryTargetDistance = static_cast<uint16_t>(85 + (i * 5));
        resting.detectionDistance = resting.stationaryTargetDistance;
        samples.restingSamples.push_back(resting);
    }

    const MmWaveCalibrationResult result = analyzeMmWaveCalibrationSamples(samples);

    EXPECT_EQ(result.maxMovingGate, 2);
    EXPECT_EQ(result.maxRestingGate, 2);
    EXPECT_FLOAT_EQ(result.presenceParams.movingFloor, 5.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.stationaryFloor, 4.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.movingSat, 30.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.stationarySat, 40.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.deskDistanceCm, 90.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.innerRadiusCm, 30.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.outerRadiusCm, 90.0f);
}

TEST(MmWaveCalibrationTest, EmptyPhaseInfluencesFloorsAndSaturation)
{
    MmWaveCalibrationSamples samples;

    for (int i = 0; i < 4; ++i) {
        MmWaveReading empty;
        empty.movingTargetEnergy = 25;
        empty.stationaryTargetEnergy = 20;
        samples.emptySamples.push_back(empty);
    }

    for (int i = 0; i < 4; ++i) {
        MmWaveReading moving;
        moving.targetState = 0x01u;
        moving.movingTargetEnergy = 30;
        moving.movingTargetDistance = 75;
        samples.movingSamples.push_back(moving);
    }

    for (int i = 0; i < 4; ++i) {
        MmWaveReading resting;
        resting.targetState = 0x02u;
        resting.stationaryTargetEnergy = 26;
        resting.stationaryTargetDistance = 90;
        resting.detectionDistance = 90;
        samples.restingSamples.push_back(resting);
    }

    const MmWaveCalibrationResult result = analyzeMmWaveCalibrationSamples(samples);

    EXPECT_FLOAT_EQ(result.presenceParams.movingFloor, 25.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.stationaryFloor, 20.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.movingSat, 35.0f);
    EXPECT_FLOAT_EQ(result.presenceParams.stationarySat, 30.0f);
}

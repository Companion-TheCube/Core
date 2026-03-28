#pragma once

#include "mmWave.h"
#include "mmWavePresenceEstimator.h"
#include <vector>

struct MmWaveCalibrationSamples {
    std::vector<MmWaveReading> emptySamples;
    std::vector<MmWaveReading> movingSamples;
    std::vector<MmWaveReading> restingSamples;
};

struct MmWaveCalibrationResult {
    int maxMovingGate = 5;
    int maxRestingGate = 5;
    int motionSensitivity = 50;
    int restingSensitivity = 50;
    MmWavePresenceParams presenceParams {};
};

MmWaveCalibrationResult analyzeMmWaveCalibrationSamples(const MmWaveCalibrationSamples& samples);

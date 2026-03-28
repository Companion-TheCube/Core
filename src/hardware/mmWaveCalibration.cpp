#include "mmWaveCalibration.h"
#include <algorithm>
#include <cmath>

namespace {
template <typename T>
float percentileValue(std::vector<T> values, float percentile)
{
    if (values.empty()) {
        return 0.0f;
    }

    std::sort(values.begin(), values.end());
    const float clampedPercentile = std::clamp(percentile, 0.0f, 100.0f);
    const float index = (clampedPercentile / 100.0f) * static_cast<float>(values.size() - 1);
    const size_t lowerIndex = static_cast<size_t>(std::floor(index));
    const size_t upperIndex = static_cast<size_t>(std::ceil(index));
    const float fraction = index - static_cast<float>(lowerIndex);

    const float lower = static_cast<float>(values[lowerIndex]);
    const float upper = static_cast<float>(values[upperIndex]);
    return lower + (fraction * (upper - lower));
}

int chooseGateFromDistanceCm(float distanceCm)
{
    if (distanceCm <= 0.0f) {
        return 5;
    }
    return std::clamp(static_cast<int>(std::ceil(distanceCm / 75.0f)), 1, 8);
}
}

MmWaveCalibrationResult analyzeMmWaveCalibrationSamples(const MmWaveCalibrationSamples& samples)
{
    MmWaveCalibrationResult result;

    std::vector<uint8_t> emptyMovingEnergy;
    std::vector<uint8_t> emptyStationaryEnergy;
    std::vector<uint8_t> movingEnergy;
    std::vector<uint8_t> stationaryEnergy;
    std::vector<uint16_t> movingDistances;
    std::vector<uint16_t> stationaryDistances;
    std::vector<uint16_t> seatedDistances;

    emptyMovingEnergy.reserve(samples.emptySamples.size());
    emptyStationaryEnergy.reserve(samples.emptySamples.size());
    movingEnergy.reserve(samples.movingSamples.size());
    stationaryEnergy.reserve(samples.restingSamples.size());
    movingDistances.reserve(samples.movingSamples.size());
    stationaryDistances.reserve(samples.restingSamples.size());
    seatedDistances.reserve(samples.restingSamples.size());

    for (const MmWaveReading& reading : samples.emptySamples) {
        emptyMovingEnergy.push_back(reading.movingTargetEnergy);
        emptyStationaryEnergy.push_back(reading.stationaryTargetEnergy);
    }

    for (const MmWaveReading& reading : samples.movingSamples) {
        if ((reading.targetState & 0x01u) != 0u) {
            movingEnergy.push_back(reading.movingTargetEnergy);
            if (reading.movingTargetDistance > 0) {
                movingDistances.push_back(reading.movingTargetDistance);
            }
        }
    }

    for (const MmWaveReading& reading : samples.restingSamples) {
        if ((reading.targetState & 0x02u) != 0u) {
            stationaryEnergy.push_back(reading.stationaryTargetEnergy);
            if (reading.stationaryTargetDistance > 0) {
                stationaryDistances.push_back(reading.stationaryTargetDistance);
                seatedDistances.push_back(reading.stationaryTargetDistance);
            }
        } else if (reading.detectionDistance > 0) {
            seatedDistances.push_back(reading.detectionDistance);
        }
    }

    result.presenceParams.movingFloor = percentileValue(emptyMovingEnergy, 95.0f);
    result.presenceParams.stationaryFloor = percentileValue(emptyStationaryEnergy, 95.0f);

    const float movingSat = percentileValue(movingEnergy, 85.0f);
    const float stationarySat = percentileValue(stationaryEnergy, 85.0f);

    result.presenceParams.movingSat = std::clamp(
        std::max(movingSat, result.presenceParams.movingFloor + 10.0f), 20.0f, 100.0f);
    result.presenceParams.stationarySat = std::clamp(
        std::max(stationarySat, result.presenceParams.stationaryFloor + 10.0f), 20.0f, 100.0f);

    const float deskDistanceCm = percentileValue(seatedDistances, 50.0f);
    result.presenceParams.deskDistanceCm = (deskDistanceCm > 0.0f) ? deskDistanceCm : 90.0f;

    std::vector<float> seatedDeltas;
    seatedDeltas.reserve(seatedDistances.size());
    for (uint16_t sampleDistance : seatedDistances) {
        if (sampleDistance > 0) {
            seatedDeltas.push_back(std::fabs(static_cast<float>(sampleDistance) - result.presenceParams.deskDistanceCm));
        }
    }

    result.presenceParams.innerRadiusCm = std::clamp(
        std::max(30.0f, percentileValue(seatedDeltas, 90.0f)), 30.0f, 40.0f);
    result.presenceParams.outerRadiusCm = 90.0f;
    result.presenceParams.tauAttackMs = 500.0f;
    result.presenceParams.tauReleaseMs = 8000.0f;
    result.presenceParams.occupiedThreshold = 0.65f;
    result.presenceParams.vacantThreshold = 0.35f;

    result.maxMovingGate = chooseGateFromDistanceCm(percentileValue(movingDistances, 95.0f));
    result.maxRestingGate = chooseGateFromDistanceCm(percentileValue(stationaryDistances, 95.0f));
    result.motionSensitivity = 50;
    result.restingSensitivity = 50;

    return result;
}

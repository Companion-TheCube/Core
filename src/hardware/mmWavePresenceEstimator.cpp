#include "mmWavePresenceEstimator.h"
#include "mmWave.h"
#include <algorithm>
#include <cmath>

MmWavePresenceEstimator::MmWavePresenceEstimator(const MmWavePresenceParams& params)
    : params_(params)
{
}

void MmWavePresenceEstimator::setParams(const MmWavePresenceParams& params)
{
    params_ = params;
    reset();
}

const MmWavePresenceParams& MmWavePresenceEstimator::getParams() const
{
    return params_;
}

void MmWavePresenceEstimator::reset()
{
    confidence_ = 0.0f;
    occupied_ = false;
    initialized_ = false;
    lastUpdate_ = {};
}

float MmWavePresenceEstimator::update(const MmWaveReading& reading, std::chrono::steady_clock::time_point now)
{
    const bool hasMoving = (reading.targetState & 0x01u) != 0u;
    const bool hasStationary = (reading.targetState & 0x02u) != 0u;

    const float movingStrength = normalize(static_cast<float>(reading.movingTargetEnergy), params_.movingFloor, params_.movingSat);
    const float stationaryStrength = normalize(static_cast<float>(reading.stationaryTargetEnergy), params_.stationaryFloor, params_.stationarySat);

    const float movingComponent = (0.75f * movingStrength) + (0.25f * (hasMoving ? 1.0f : 0.0f));
    const float stationaryComponent = (0.85f * stationaryStrength) + (0.15f * (hasStationary ? 1.0f : 0.0f));
    const float combinedEvidence = 1.0f - ((1.0f - movingComponent) * (1.0f - stationaryComponent));

    const float chosenDistanceCm = chooseDistanceCm(reading);
    const float zoneScore = computeZoneScore(chosenDistanceCm);
    const float rawPresence = clamp01(zoneScore * combinedEvidence);

    if (!initialized_) {
        confidence_ = rawPresence;
        occupied_ = (confidence_ >= params_.occupiedThreshold);
        initialized_ = true;
        lastUpdate_ = now;
        return confidence_;
    }

    const float dtSeconds = std::max(0.0f,
        std::chrono::duration<float>(now - lastUpdate_).count());
    lastUpdate_ = now;

    const float tauAttackSeconds = std::max(params_.tauAttackMs, 1.0f) / 1000.0f;
    const float tauReleaseSeconds = std::max(params_.tauReleaseMs, 1.0f) / 1000.0f;
    const float alphaAttack = 1.0f - std::exp(-dtSeconds / tauAttackSeconds);
    const float alphaRelease = 1.0f - std::exp(-dtSeconds / tauReleaseSeconds);
    const float alpha = (rawPresence > confidence_) ? alphaAttack : alphaRelease;

    confidence_ += alpha * (rawPresence - confidence_);
    confidence_ = clamp01(confidence_);

    if (!occupied_ && confidence_ >= params_.occupiedThreshold) {
        occupied_ = true;
    } else if (occupied_ && confidence_ <= params_.vacantThreshold) {
        occupied_ = false;
    }

    return confidence_;
}

float MmWavePresenceEstimator::confidence() const
{
    return confidence_;
}

bool MmWavePresenceEstimator::isOccupied() const
{
    return occupied_;
}

bool MmWavePresenceEstimator::isInitialized() const
{
    return initialized_;
}

float MmWavePresenceEstimator::clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float MmWavePresenceEstimator::normalize(float value, float floor, float saturation)
{
    if (saturation <= floor) {
        return 0.0f;
    }
    return clamp01((value - floor) / (saturation - floor));
}

float MmWavePresenceEstimator::chooseDistanceCm(const MmWaveReading& reading) const
{
    const bool hasMoving = (reading.targetState & 0x01u) != 0u;
    const bool hasStationary = (reading.targetState & 0x02u) != 0u;

    if (hasStationary && reading.stationaryTargetDistance > 0) {
        return static_cast<float>(reading.stationaryTargetDistance);
    }
    if (hasMoving && reading.movingTargetDistance > 0) {
        return static_cast<float>(reading.movingTargetDistance);
    }
    return static_cast<float>(reading.detectionDistance);
}

float MmWavePresenceEstimator::computeZoneScore(float distanceCm) const
{
    const float delta = std::fabs(distanceCm - params_.deskDistanceCm);

    if (delta <= params_.innerRadiusCm) {
        return 1.0f;
    }
    if (delta >= params_.outerRadiusCm) {
        return 0.0f;
    }
    if (params_.outerRadiusCm <= params_.innerRadiusCm) {
        return 0.0f;
    }

    const float t = (delta - params_.innerRadiusCm) / (params_.outerRadiusCm - params_.innerRadiusCm);
    return clamp01(1.0f - t);
}

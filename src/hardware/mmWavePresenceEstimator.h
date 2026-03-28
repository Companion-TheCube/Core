#pragma once

#include <chrono>

struct MmWaveReading;

struct MmWavePresenceParams {
    float deskDistanceCm = 90.0f;
    float innerRadiusCm = 35.0f;
    float outerRadiusCm = 90.0f;

    float movingFloor = 10.0f;
    float movingSat = 80.0f;
    float stationaryFloor = 8.0f;
    float stationarySat = 70.0f;

    float tauAttackMs = 500.0f;
    float tauReleaseMs = 8000.0f;

    float occupiedThreshold = 0.65f;
    float vacantThreshold = 0.35f;
};

class MmWavePresenceEstimator {
public:
    explicit MmWavePresenceEstimator(const MmWavePresenceParams& params = {});

    void setParams(const MmWavePresenceParams& params);
    const MmWavePresenceParams& getParams() const;

    void reset();
    float update(const MmWaveReading& reading, std::chrono::steady_clock::time_point now);

    float confidence() const;
    bool isOccupied() const;
    bool isInitialized() const;

private:
    static float clamp01(float value);
    static float normalize(float value, float floor, float saturation);

    float chooseDistanceCm(const MmWaveReading& reading) const;
    float computeZoneScore(float distanceCm) const;

    MmWavePresenceParams params_;
    float confidence_ = 0.0f;
    bool occupied_ = false;
    bool initialized_ = false;
    std::chrono::steady_clock::time_point lastUpdate_ {};
};

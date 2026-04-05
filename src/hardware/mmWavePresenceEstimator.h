#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

struct MmWaveReading;

struct MmWavePresenceConfig {
    int detectionDistanceAverageWindowSecs = 10;
    int movingDistanceAverageWindowSecs = 10;
    int stationaryDistanceAverageWindowSecs = 10;
    int stationaryEnergyAverageWindowSecs = 10;
};

enum class MmWavePresenceState : uint8_t {
    Unknown = 0,
    Present,
    Absent
};

struct MmWavePresenceDecision {
    MmWavePresenceState state = MmWavePresenceState::Unknown;
    std::chrono::steady_clock::time_point timestamp {};
    std::optional<float> detectionDistanceAverageCm;
    std::optional<float> movingTargetDistanceAverageCm;
    std::optional<float> stationaryTargetDistanceAverageCm;
    std::optional<float> stationaryTargetEnergyAverage;
    size_t detectionDistanceAverageSampleCount = 0;
    size_t movingTargetDistanceAverageSampleCount = 0;
    size_t stationaryTargetDistanceAverageSampleCount = 0;
    size_t stationaryTargetEnergyAverageSampleCount = 0;
    std::optional<uint8_t> latestTargetState;
    float absentDetectionDistanceThresholdCm = 150.0f;
    float absentMovingDistanceThresholdCm = 200.0f;
    float absentStationaryDistanceThresholdCm = 200.0f;
};

class MmWavePresenceEstimator {
public:
    explicit MmWavePresenceEstimator(const MmWavePresenceConfig& config = {});

    void setConfig(const MmWavePresenceConfig& config);
    const MmWavePresenceConfig& getConfig() const;

    void reset();
    MmWavePresenceDecision update(const MmWaveReading& reading, std::chrono::steady_clock::time_point now);

    MmWavePresenceState state() const;
    MmWavePresenceDecision decision() const;
    bool isPresent() const;

private:
    struct BufferedReading {
        uint8_t targetState = 0;
        uint16_t movingTargetDistance = 0;
        uint8_t movingTargetEnergy = 0;
        uint16_t stationaryTargetDistance = 0;
        uint8_t stationaryTargetEnergy = 0;
        uint16_t detectionDistance = 0;
        std::chrono::steady_clock::time_point timestamp {};
    };

    static int clampWindowSeconds(int value);

    void pruneSamples(std::chrono::steady_clock::time_point latestTimestamp);
    void recomputeDecision(std::chrono::steady_clock::time_point latestTimestamp);
    int maxAverageWindowSeconds() const;

    MmWavePresenceConfig config_ {};
    std::deque<BufferedReading> samples_;
    MmWavePresenceState state_ = MmWavePresenceState::Unknown;
    MmWavePresenceDecision lastDecision_ {};
};

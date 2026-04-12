/*
███╗   ███╗███╗   ███╗██╗    ██╗ █████╗ ██╗   ██╗███████╗██████╗ ██████╗ ███████╗███████╗███████╗███╗   ██╗ ██████╗███████╗███████╗███████╗████████╗██╗███╗   ███╗ █████╗ ████████╗ ██████╗ ██████╗     ██████╗██████╗ ██████╗
████╗ ████║████╗ ████║██║    ██║██╔══██╗██║   ██║██╔════╝██╔══██╗██╔══██╗██╔════╝██╔════╝██╔════╝████╗  ██║██╔════╝██╔════╝██╔════╝██╔════╝╚══██╔══╝██║████╗ ████║██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██╔████╔██║██╔████╔██║██║ █╗ ██║███████║██║   ██║█████╗  ██████╔╝██████╔╝█████╗  ███████╗█████╗  ██╔██╗ ██║██║     █████╗  █████╗  ███████╗   ██║   ██║██╔████╔██║███████║   ██║   ██║   ██║██████╔╝   ██║     ██████╔╝██████╔╝
██║╚██╔╝██║██║╚██╔╝██║██║███╗██║██╔══██║╚██╗ ██╔╝██╔══╝  ██╔═══╝ ██╔══██╗██╔══╝  ╚════██║██╔══╝  ██║╚██╗██║██║     ██╔══╝  ██╔══╝  ╚════██║   ██║   ██║██║╚██╔╝██║██╔══██║   ██║   ██║   ██║██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║ ╚═╝ ██║██║ ╚═╝ ██║╚███╔███╔╝██║  ██║ ╚████╔╝ ███████╗██║     ██║  ██║███████╗███████║███████╗██║ ╚████║╚██████╗███████╗███████╗███████║   ██║   ██║██║ ╚═╝ ██║██║  ██║   ██║   ╚██████╔╝██║  ██║██╗╚██████╗██║     ██║
╚═╝     ╚═╝╚═╝     ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═══╝ ╚═════╝╚══════╝╚══════╝╚══════╝   ╚═╝   ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "mmWavePresenceEstimator.h"
#include "mmWave.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace {

constexpr int MMWAVE_MIN_AVERAGE_WINDOW_SECS = 1;
constexpr int MMWAVE_MAX_AVERAGE_WINDOW_SECS = 30;

constexpr float MMWAVE_PRESENT_DISTANCE_CM = 100.0f;
constexpr float MMWAVE_BASE_ABSENT_DETECTION_DISTANCE_CM = 150.0f;
constexpr float MMWAVE_BASE_ABSENT_MOVING_DISTANCE_CM = 200.0f;
constexpr float MMWAVE_BASE_ABSENT_STATIONARY_DISTANCE_CM = 200.0f;
constexpr float MMWAVE_TARGET_STATE_NONE_ABSENT_REDUCTION = 0.15f;
constexpr float MMWAVE_LOW_STATIONARY_ENERGY_ABSENT_REDUCTION = 0.10f;
constexpr float MMWAVE_LOW_STATIONARY_ENERGY_THRESHOLD = 75.0f;
constexpr float MMWAVE_MIN_ABSENT_THRESHOLD_MULTIPLIER = 0.5f;

struct AveragedMetric {
    std::optional<float> average;
    size_t sampleCount = 0;
};

template <typename Accessor>
AveragedMetric averageMetric(
    const auto& samples,
    std::chrono::steady_clock::time_point latestTimestamp,
    int windowSeconds,
    Accessor accessor)
{
    if (samples.empty()) {
        return {};
    }

    const auto cutoff = latestTimestamp - std::chrono::seconds(windowSeconds);
    float total = 0.0f;
    size_t count = 0;

    for (const auto& sample : samples) {
        if (sample.timestamp < cutoff) {
            continue;
        }

        total += accessor(sample);
        ++count;
    }

    if (count == 0) {
        return {};
    }

    return {
        .average = total / static_cast<float>(count),
        .sampleCount = count
    };
}

struct MetricComparison {
    std::optional<float> average;
    float absentThreshold = 0.0f;
    float presentThreshold = MMWAVE_PRESENT_DISTANCE_CM;
};

MmWavePresenceConfig normalizeConfig(const MmWavePresenceConfig& config)
{
    MmWavePresenceConfig normalized = config;
    normalized.detectionDistanceAverageWindowSecs = std::clamp(
        normalized.detectionDistanceAverageWindowSecs,
        MMWAVE_MIN_AVERAGE_WINDOW_SECS,
        MMWAVE_MAX_AVERAGE_WINDOW_SECS);
    normalized.movingDistanceAverageWindowSecs = std::clamp(
        normalized.movingDistanceAverageWindowSecs,
        MMWAVE_MIN_AVERAGE_WINDOW_SECS,
        MMWAVE_MAX_AVERAGE_WINDOW_SECS);
    normalized.stationaryDistanceAverageWindowSecs = std::clamp(
        normalized.stationaryDistanceAverageWindowSecs,
        MMWAVE_MIN_AVERAGE_WINDOW_SECS,
        MMWAVE_MAX_AVERAGE_WINDOW_SECS);
    normalized.stationaryEnergyAverageWindowSecs = std::clamp(
        normalized.stationaryEnergyAverageWindowSecs,
        MMWAVE_MIN_AVERAGE_WINDOW_SECS,
        MMWAVE_MAX_AVERAGE_WINDOW_SECS);
    return normalized;
}

} // namespace

MmWavePresenceEstimator::MmWavePresenceEstimator(const MmWavePresenceConfig& config)
    : config_(normalizeConfig(config))
{
}

void MmWavePresenceEstimator::setConfig(const MmWavePresenceConfig& config)
{
    config_ = normalizeConfig(config);

    if (!samples_.empty()) {
        const auto latestTimestamp = samples_.back().timestamp;
        pruneSamples(latestTimestamp);
        recomputeDecision(latestTimestamp);
    }
}

const MmWavePresenceConfig& MmWavePresenceEstimator::getConfig() const
{
    return config_;
}

void MmWavePresenceEstimator::reset()
{
    samples_.clear();
    state_ = MmWavePresenceState::Unknown;
    lastDecision_ = {};
}

MmWavePresenceDecision MmWavePresenceEstimator::update(
    const MmWaveReading& reading,
    std::chrono::steady_clock::time_point now)
{
    samples_.push_back({
        reading.targetState,
        reading.movingTargetDistance,
        reading.movingTargetEnergy,
        reading.stationaryTargetDistance,
        reading.stationaryTargetEnergy,
        reading.detectionDistance,
        now
    });
    pruneSamples(now);
    recomputeDecision(now);
    return lastDecision_;
}

MmWavePresenceState MmWavePresenceEstimator::state() const
{
    return state_;
}

MmWavePresenceDecision MmWavePresenceEstimator::decision() const
{
    return lastDecision_;
}

bool MmWavePresenceEstimator::isPresent() const
{
    return state_ == MmWavePresenceState::Present;
}

int MmWavePresenceEstimator::clampWindowSeconds(int value)
{
    return std::clamp(value, MMWAVE_MIN_AVERAGE_WINDOW_SECS, MMWAVE_MAX_AVERAGE_WINDOW_SECS);
}

void MmWavePresenceEstimator::pruneSamples(std::chrono::steady_clock::time_point latestTimestamp)
{
    if (samples_.empty()) {
        return;
    }

    const auto cutoff = latestTimestamp - std::chrono::seconds(maxAverageWindowSeconds());
    while (!samples_.empty() && samples_.front().timestamp < cutoff) {
        samples_.pop_front();
    }
}

void MmWavePresenceEstimator::recomputeDecision(std::chrono::steady_clock::time_point latestTimestamp)
{
    MmWavePresenceDecision nextDecision;
    nextDecision.state = state_;
    nextDecision.timestamp = latestTimestamp;

    if (samples_.empty()) {
        lastDecision_ = nextDecision;
        return;
    }

    const BufferedReading& latestReading = samples_.back();
    const uint8_t latestTargetState = latestReading.targetState & 0x03u;
    nextDecision.latestTargetState = latestTargetState;

    const AveragedMetric detectionDistanceAverage = averageMetric(
        samples_,
        latestTimestamp,
        config_.detectionDistanceAverageWindowSecs,
        [](const BufferedReading& reading) {
            return static_cast<float>(reading.detectionDistance);
        });
    nextDecision.detectionDistanceAverageCm = detectionDistanceAverage.average;
    nextDecision.detectionDistanceAverageSampleCount = detectionDistanceAverage.sampleCount;

    const AveragedMetric movingTargetDistanceAverage = averageMetric(
        samples_,
        latestTimestamp,
        config_.movingDistanceAverageWindowSecs,
        [](const BufferedReading& reading) {
            return static_cast<float>(reading.movingTargetDistance);
        });
    nextDecision.movingTargetDistanceAverageCm = movingTargetDistanceAverage.average;
    nextDecision.movingTargetDistanceAverageSampleCount = movingTargetDistanceAverage.sampleCount;

    const AveragedMetric stationaryTargetDistanceAverage = averageMetric(
        samples_,
        latestTimestamp,
        config_.stationaryDistanceAverageWindowSecs,
        [](const BufferedReading& reading) {
            return static_cast<float>(reading.stationaryTargetDistance);
        });
    nextDecision.stationaryTargetDistanceAverageCm = stationaryTargetDistanceAverage.average;
    nextDecision.stationaryTargetDistanceAverageSampleCount = stationaryTargetDistanceAverage.sampleCount;

    const AveragedMetric stationaryTargetEnergyAverage = averageMetric(
        samples_,
        latestTimestamp,
        config_.stationaryEnergyAverageWindowSecs,
        [](const BufferedReading& reading) {
            return static_cast<float>(reading.stationaryTargetEnergy);
        });
    nextDecision.stationaryTargetEnergyAverage = stationaryTargetEnergyAverage.average;
    nextDecision.stationaryTargetEnergyAverageSampleCount = stationaryTargetEnergyAverage.sampleCount;

    float reduction = 0.0f;
    if (latestTargetState == 0u) {
        reduction += MMWAVE_TARGET_STATE_NONE_ABSENT_REDUCTION;
    }
    if (nextDecision.stationaryTargetEnergyAverage.has_value()
        && *nextDecision.stationaryTargetEnergyAverage < MMWAVE_LOW_STATIONARY_ENERGY_THRESHOLD) {
        reduction += MMWAVE_LOW_STATIONARY_ENERGY_ABSENT_REDUCTION;
    }

    const float multiplier = std::max(MMWAVE_MIN_ABSENT_THRESHOLD_MULTIPLIER, 1.0f - reduction);
    nextDecision.absentDetectionDistanceThresholdCm = MMWAVE_BASE_ABSENT_DETECTION_DISTANCE_CM * multiplier;
    nextDecision.absentMovingDistanceThresholdCm = MMWAVE_BASE_ABSENT_MOVING_DISTANCE_CM * multiplier;
    nextDecision.absentStationaryDistanceThresholdCm = MMWAVE_BASE_ABSENT_STATIONARY_DISTANCE_CM * multiplier;

    std::vector<MetricComparison> activeMetrics;
    activeMetrics.reserve(3);
    activeMetrics.push_back({
        nextDecision.detectionDistanceAverageCm,
        nextDecision.absentDetectionDistanceThresholdCm,
        MMWAVE_PRESENT_DISTANCE_CM
    });

    if (latestTargetState == 1u || latestTargetState == 3u) {
        activeMetrics.push_back({
            nextDecision.movingTargetDistanceAverageCm,
            nextDecision.absentMovingDistanceThresholdCm,
            MMWAVE_PRESENT_DISTANCE_CM
        });
    }

    if (latestTargetState == 2u || latestTargetState == 3u) {
        activeMetrics.push_back({
            nextDecision.stationaryTargetDistanceAverageCm,
            nextDecision.absentStationaryDistanceThresholdCm,
            MMWAVE_PRESENT_DISTANCE_CM
        });
    }

    const bool hasAllAverages = std::all_of(
        activeMetrics.begin(),
        activeMetrics.end(),
        [](const MetricComparison& metric) {
            return metric.average.has_value();
        });

    if (hasAllAverages
        && std::all_of(
            activeMetrics.begin(),
            activeMetrics.end(),
            [](const MetricComparison& metric) {
                return metric.average.has_value() && *metric.average < metric.presentThreshold;
            })) {
        state_ = MmWavePresenceState::Present;
    } else if (hasAllAverages
        && std::all_of(
            activeMetrics.begin(),
            activeMetrics.end(),
            [](const MetricComparison& metric) {
                return metric.average.has_value() && *metric.average > metric.absentThreshold;
            })) {
        state_ = MmWavePresenceState::Absent;
    }

    nextDecision.state = state_;
    lastDecision_ = nextDecision;
}

int MmWavePresenceEstimator::maxAverageWindowSeconds() const
{
    return std::max({
        clampWindowSeconds(config_.detectionDistanceAverageWindowSecs),
        clampWindowSeconds(config_.movingDistanceAverageWindowSecs),
        clampWindowSeconds(config_.stationaryDistanceAverageWindowSecs),
        clampWindowSeconds(config_.stationaryEnergyAverageWindowSecs)
    });
}

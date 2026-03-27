#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace audio {

struct SileroVadChunkResult {
    bool speechObserved = false;
    bool speechStarted = false;
    bool speechEnded = false;
    bool segmentActive = false;
    float maxSpeechProbability = 0.0f;
    size_t windowsEvaluated = 0;
};

class SileroVad {
public:
    SileroVad();
    ~SileroVad();

    bool available() const;
    const std::string& failureReason() const;
    void reset();
    SileroVadChunkResult analyzePcm16(std::span<const int16_t> samples);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void initialize();
};

} // namespace audio

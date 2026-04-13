/*
███████╗██╗██╗     ███████╗██████╗  ██████╗ ██╗   ██╗ █████╗ ██████╗     ██████╗██████╗ ██████╗
██╔════╝██║██║     ██╔════╝██╔══██╗██╔═══██╗██║   ██║██╔══██╗██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
███████╗██║██║     █████╗  ██████╔╝██║   ██║██║   ██║███████║██║  ██║   ██║     ██████╔╝██████╔╝
╚════██║██║██║     ██╔══╝  ██╔══██╗██║   ██║╚██╗ ██╔╝██╔══██║██║  ██║   ██║     ██╔═══╝ ██╔═══╝
███████║██║███████╗███████╗██║  ██║╚██████╔╝ ╚████╔╝ ██║  ██║██████╔╝██╗╚██████╗██║     ██║
╚══════╝╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝   ╚═══╝  ╚═╝  ╚═╝╚═════╝ ╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "sileroVad.h"

#include "audio/constants.h"
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <sstream>
#include <type_traits>
#include <vector>

namespace audio {

namespace fs = std::filesystem;

namespace {

template <typename T>
T parseNumericConfig(const std::string& key, T fallback)
{
    try {
        const std::string raw = Config::get(key, "");
        if (raw.empty()) return fallback;
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(std::stod(raw));
        } else {
            return static_cast<T>(std::stoll(raw));
        }
    } catch (...) {
        return fallback;
    }
}

bool parseBoolConfig(const std::string& key, bool fallback)
{
    std::string raw = Config::get(key, "");
    if (raw.empty()) return fallback;
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (raw == "1" || raw == "true" || raw == "yes" || raw == "on") return true;
    if (raw == "0" || raw == "false" || raw == "no" || raw == "off") return false;
    return fallback;
}

std::string formatFloat(float value, int precision = 2)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(precision);
    oss << value;
    return oss.str();
}

std::optional<fs::path> resolveModelPathFromConfig(const std::string& configuredPath)
{
    std::vector<fs::path> candidates;
    if (!configuredPath.empty()) {
        const fs::path configured(configuredPath);
        candidates.push_back(configured);
        if (!configured.is_absolute()) {
            candidates.push_back(fs::current_path() / configured);
            candidates.push_back(fs::current_path() / ".." / configured);
            candidates.push_back(fs::current_path() / "build" / "bin" / configured);
        }
    } else {
        candidates.push_back(fs::path("data") / "silero_vad.onnx");
        candidates.push_back(fs::current_path() / "data" / "silero_vad.onnx");
        candidates.push_back(fs::current_path() / ".." / "data" / "silero_vad.onnx");
        candidates.push_back(fs::current_path() / "build" / "bin" / "data" / "silero_vad.onnx");
    }

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (candidate.empty()) continue;
        if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec)) {
            return fs::absolute(candidate, ec);
        }
    }
    return std::nullopt;
}

} // namespace

struct SileroVad::Impl {
    Ort::Env env { ORT_LOGGING_LEVEL_WARNING, "SileroVad" };
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memoryInfo { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU) };
    std::array<const char*, 3> inputNames { "input", "state", "sr" };
    std::array<const char*, 2> outputNames { "output", "stateN" };
    std::array<int64_t, 2> inputNodeDims { 1, 0 };
    std::array<int64_t, 3> stateNodeDims { 2, 1, 128 };
    std::array<int64_t, 1> srNodeDims { 1 };

    fs::path modelPath;
    std::string failureReason;

    float threshold = 0.5f;
    float releaseThreshold = 0.35f;
    int sampleRate = static_cast<int>(audio::SAMPLE_RATE);
    int windowSizeMs = 32;
    int windowSizeSamples = 512;
    int contextSamples = 64;
    int minSilenceDurationMs = 160;
    int minSilenceSamples = 2560;

    bool ready = false;
    bool triggered = false;
    int currentSample = 0;
    int tempEndSample = 0;

    std::vector<float> state;
    std::vector<int64_t> sampleRateTensor;
    std::vector<float> context;
    std::vector<float> inputBuffer;
    std::vector<float> pendingSamples;
};

SileroVad::SileroVad()
    : impl_(std::make_unique<Impl>())
{
    initialize();
}

SileroVad::~SileroVad() = default;

bool SileroVad::available() const
{
    return impl_ && impl_->ready;
}

const std::string& SileroVad::failureReason() const
{
    static const std::string empty;
    if (!impl_) return empty;
    return impl_->failureReason;
}

void SileroVad::reset()
{
    if (!impl_) return;

    impl_->triggered = false;
    impl_->currentSample = 0;
    impl_->tempEndSample = 0;
    impl_->pendingSamples.clear();
    std::fill(impl_->state.begin(), impl_->state.end(), 0.0f);
    std::fill(impl_->context.begin(), impl_->context.end(), 0.0f);
}

void SileroVad::initialize()
{
    if (!impl_) return;

    if (!parseBoolConfig("SILERO_VAD_ENABLED", true)) {
        impl_->failureReason = "disabled_by_config";
        CubeLog::info("SileroVad: disabled by config, using fallback speech detection");
        return;
    }

    const auto resolvedModelPath = resolveModelPathFromConfig(Config::get("SILERO_VAD_MODEL_PATH", ""));
    if (!resolvedModelPath.has_value()) {
        impl_->failureReason = "model_not_found";
        CubeLog::warning("SileroVad: model file not found, using fallback speech detection");
        return;
    }

    impl_->modelPath = *resolvedModelPath;
    impl_->threshold = parseNumericConfig<float>("SILERO_VAD_THRESHOLD", 0.5f);
    const float releaseDelta = parseNumericConfig<float>("SILERO_VAD_RELEASE_DELTA", 0.15f);
    impl_->releaseThreshold = std::max(0.0f, impl_->threshold - releaseDelta);
    impl_->windowSizeMs = parseNumericConfig<int>("SILERO_VAD_WINDOW_MS", 32);
    impl_->minSilenceDurationMs = parseNumericConfig<int>("SILERO_VAD_MIN_SILENCE_MS", 160);
    impl_->sampleRate = static_cast<int>(audio::SAMPLE_RATE);
    impl_->windowSizeSamples = std::max(1, (impl_->sampleRate * impl_->windowSizeMs) / 1000);
    impl_->contextSamples = std::max(1, impl_->windowSizeSamples / 8);
    impl_->minSilenceSamples = std::max(impl_->windowSizeSamples, (impl_->sampleRate * impl_->minSilenceDurationMs) / 1000);
    impl_->state.assign(2 * 1 * 128, 0.0f);
    impl_->sampleRateTensor = { impl_->sampleRate };
    impl_->context.assign(static_cast<size_t>(impl_->contextSamples), 0.0f);
    impl_->inputBuffer.assign(static_cast<size_t>(impl_->contextSamples + impl_->windowSizeSamples), 0.0f);
    impl_->pendingSamples.clear();
    impl_->inputNodeDims[1] = static_cast<int64_t>(impl_->inputBuffer.size());

    try {
        impl_->sessionOptions.SetIntraOpNumThreads(parseNumericConfig<int>("SILERO_VAD_INTRA_THREADS", 1));
        impl_->sessionOptions.SetInterOpNumThreads(parseNumericConfig<int>("SILERO_VAD_INTER_THREADS", 1));
        impl_->sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef _WIN32
        const std::wstring wideModelPath = impl_->modelPath.wstring();
        impl_->session = std::make_unique<Ort::Session>(impl_->env, wideModelPath.c_str(), impl_->sessionOptions);
#else
        impl_->session = std::make_unique<Ort::Session>(impl_->env, impl_->modelPath.string().c_str(), impl_->sessionOptions);
#endif
        impl_->ready = true;
        reset();
        CubeLog::info(
            "SileroVad: loaded model path=" + impl_->modelPath.string()
            + ", threshold=" + formatFloat(impl_->threshold)
            + ", windowMs=" + std::to_string(impl_->windowSizeMs)
            + ", minSilenceMs=" + std::to_string(impl_->minSilenceDurationMs));
    } catch (const Ort::Exception& ex) {
        impl_->failureReason = ex.what();
        impl_->ready = false;
        CubeLog::warning("SileroVad: failed to initialize model, using fallback speech detection error=" + impl_->failureReason);
    } catch (const std::exception& ex) {
        impl_->failureReason = ex.what();
        impl_->ready = false;
        CubeLog::warning("SileroVad: failed to initialize model, using fallback speech detection error=" + impl_->failureReason);
    }
}

SileroVadChunkResult SileroVad::analyzePcm16(std::span<const int16_t> samples)
{
    SileroVadChunkResult result;
    if (!impl_) return result;

    result.segmentActive = impl_->triggered;
    if (!impl_->ready || !impl_->session || samples.empty()) {
        return result;
    }

    impl_->pendingSamples.reserve(impl_->pendingSamples.size() + samples.size());
    for (const int16_t sample : samples) {
        impl_->pendingSamples.push_back(static_cast<float>(sample));
    }

    size_t offset = 0;
    try {
        while ((impl_->pendingSamples.size() - offset) >= static_cast<size_t>(impl_->windowSizeSamples)) {
            const float* windowData = impl_->pendingSamples.data() + offset;
            std::copy(impl_->context.begin(), impl_->context.end(), impl_->inputBuffer.begin());
            std::copy(windowData, windowData + impl_->windowSizeSamples, impl_->inputBuffer.begin() + impl_->contextSamples);

            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                impl_->memoryInfo,
                impl_->inputBuffer.data(),
                impl_->inputBuffer.size(),
                impl_->inputNodeDims.data(),
                impl_->inputNodeDims.size());
            Ort::Value stateTensor = Ort::Value::CreateTensor<float>(
                impl_->memoryInfo,
                impl_->state.data(),
                impl_->state.size(),
                impl_->stateNodeDims.data(),
                impl_->stateNodeDims.size());
            Ort::Value srTensor = Ort::Value::CreateTensor<int64_t>(
                impl_->memoryInfo,
                impl_->sampleRateTensor.data(),
                impl_->sampleRateTensor.size(),
                impl_->srNodeDims.data(),
                impl_->srNodeDims.size());

            std::array<Ort::Value, 3> ortInputs = {
                std::move(inputTensor),
                std::move(stateTensor),
                std::move(srTensor)
            };

            auto ortOutputs = impl_->session->Run(
                Ort::RunOptions { nullptr },
                impl_->inputNames.data(),
                ortInputs.data(),
                ortInputs.size(),
                impl_->outputNames.data(),
                impl_->outputNames.size());

            const float speechProbability = ortOutputs[0].GetTensorData<float>()[0];
            result.maxSpeechProbability = std::max(result.maxSpeechProbability, speechProbability);
            ++result.windowsEvaluated;
            impl_->currentSample += impl_->windowSizeSamples;

            const float* nextState = ortOutputs[1].GetTensorData<float>();
            std::copy(nextState, nextState + impl_->state.size(), impl_->state.begin());
            std::copy(
                impl_->inputBuffer.end() - impl_->contextSamples,
                impl_->inputBuffer.end(),
                impl_->context.begin());

            if (speechProbability >= impl_->threshold && impl_->tempEndSample != 0) {
                impl_->tempEndSample = 0;
            }

            if (speechProbability >= impl_->threshold) {
                result.speechObserved = true;
                if (!impl_->triggered) {
                    impl_->triggered = true;
                    result.speechStarted = true;
                }
            } else if (impl_->triggered && speechProbability < impl_->releaseThreshold) {
                if (impl_->tempEndSample == 0) {
                    impl_->tempEndSample = impl_->currentSample;
                }
                if ((impl_->currentSample - impl_->tempEndSample) >= impl_->minSilenceSamples) {
                    impl_->triggered = false;
                    impl_->tempEndSample = 0;
                    result.speechEnded = true;
                }
            }

            result.segmentActive = impl_->triggered;
            offset += static_cast<size_t>(impl_->windowSizeSamples);
        }
    } catch (const Ort::Exception& ex) {
        impl_->failureReason = ex.what();
        impl_->ready = false;
        CubeLog::warning("SileroVad: inference failed, using fallback speech detection error=" + impl_->failureReason);
        result = {};
    }

    if (offset > 0) {
        impl_->pendingSamples.erase(impl_->pendingSamples.begin(), impl_->pendingSamples.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    result.segmentActive = impl_->triggered;
    return result;
}

} // namespace audio

/*
███████╗██████╗ ███████╗███████╗ ██████╗██╗  ██╗██╗███╗   ██╗    ██████╗██████╗ ██████╗ 
██╔════╝██╔══██╗██╔════╝██╔════╝██╔════╝██║  ██║██║████╗  ██║   ██╔════╝██╔══██╗██╔══██╗
███████╗██████╔╝█████╗  █████╗  ██║     ███████║██║██╔██╗ ██║   ██║     ██████╔╝██████╔╝
╚════██║██╔═══╝ ██╔══╝  ██╔══╝  ██║     ██╔══██║██║██║╚██╗██║   ██║     ██╔═══╝ ██╔═══╝ 
███████║██║     ███████╗███████╗╚██████╗██║  ██║██║██║ ╚████║██╗╚██████╗██║     ██║     
╚══════╝╚═╝     ╚══════╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

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

/*

In this file we refactor SpeechIn into a small orchestrator that wires together
three focused classes:
  - AudioCapture: RtAudio input -> PCM blocks
  - WakeWordClient: streams PCM to openwakeword socket and signals wake events
  - AudioRouter: pre-trigger ring buffer and fan-out to registered queues

*/

#include "speechIn.h"
#include "audioRouter.h"
#include "audioCapture.h"
#include "micAutoLevel.h"
#include "wakeWordClient.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <algorithm>
#include <cctype>
#include <type_traits>

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

} // namespace

std::unordered_map<unsigned int, std::function<void()>> SpeechIn::wakeWordDetectionCallbacks;
std::atomic<unsigned int> SpeechIn::handle = 0;
std::unordered_map<size_t, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>> SpeechIn::registeredWakeAudioQueues;
std::mutex SpeechIn::registeredQueuesMutex;
std::unordered_map<size_t, std::weak_ptr<ThreadSafeQueue<std::vector<int16_t>>>> SpeechIn::registeredPreTriggerAudioQueues;

SpeechIn::~SpeechIn()
{
    stop();
}

SpeechIn::SpeechIn() = default;

void SpeechIn::start()
{
    CubeLog::info("SpeechIn: start requested");
    // Build components
    wwClient = std::make_unique<WakeWordClient>();
    router = std::make_unique<AudioRouter>();
    router->setWakeWordClient(std::shared_ptr<WakeWordClient>(wwClient.get(), [](WakeWordClient*){}));
    router->setProviders(
        []() { return SpeechIn::snapshotWakeTargets(); },
        []() { return SpeechIn::snapshotPreTriggerTargets(); }
    );
    CubeLog::debug("SpeechIn: components created and wired");
    // When wake is detected, update router and notify subscribers
    wwClient->setOnWake([this]() {
        if (router) router->onWakeDetected();
        // Dispatch callbacks asynchronously
        std::vector<std::function<void()>> callbacks;
        callbacks.reserve(SpeechIn::wakeWordDetectionCallbacks.size());
        for (const auto& kv : SpeechIn::wakeWordDetectionCallbacks)
            callbacks.push_back(kv.second);
        std::thread([cb = std::move(callbacks)]() {
            for (const auto& fn : cb) {
                try { fn(); }
                catch (...) { CubeLog::error("Wake callback exception"); }
            }
        }).detach();
    });

    micRestoreOnStop = false;
    micAutoLevelPending = false;
    micAutoLevelPendingPeak = 0;
    micAutoLevelPendingClipRatio = 0.0;
    micLevelMonitor.reset();
    micVolumeController.reset();

    const bool micAutoLevelEnabled = parseBoolConfig("MIC_AUTO_LEVEL_ENABLED", true);
    if (micAutoLevelEnabled) {
        MicSourceVolumeControllerConfig volumeConfig;
        volumeConfig.stepPercent = parseNumericConfig<int>("MIC_AUTO_LEVEL_STEP_PERCENT", 5);
        volumeConfig.minPercent = parseNumericConfig<int>("MIC_AUTO_LEVEL_MIN_PERCENT", 20);

        auto controller = std::make_unique<MicSourceVolumeController>(volumeConfig);
        if (controller->initialize()) {
            micRestoreOnStop = parseBoolConfig("MIC_AUTO_LEVEL_RESTORE_ON_STOP", true);

            MicInputLevelConfig monitorConfig;
            monitorConfig.sampleRate = audio::SAMPLE_RATE;
            monitorConfig.windowMs = parseNumericConfig<unsigned int>("MIC_AUTO_LEVEL_WINDOW_MS", 1000);
            monitorConfig.peakTrigger = parseNumericConfig<int>("MIC_AUTO_LEVEL_PEAK_TRIGGER", 32000);
            monitorConfig.clipRatioTrigger = parseNumericConfig<double>("MIC_AUTO_LEVEL_CLIP_RATIO_TRIGGER", 0.001);
            monitorConfig.consecutiveHotWindows = parseNumericConfig<unsigned int>("MIC_AUTO_LEVEL_CONSECUTIVE_HOT_WINDOWS", 2);
            monitorConfig.cooldownMs = parseNumericConfig<unsigned int>("MIC_AUTO_LEVEL_COOLDOWN_MS", 3000);

            micVolumeController = std::move(controller);
            micLevelMonitor = std::make_unique<MicInputLevelMonitor>(
                monitorConfig,
                [this](const MicInputLevelHotWindow& hotWindow) {
                    {
                        std::lock_guard<std::mutex> lock(micAutoLevelMutex);
                        micAutoLevelPending = true;
                        micAutoLevelPendingPeak = hotWindow.peak;
                        micAutoLevelPendingClipRatio = hotWindow.clipRatio;
                    }
                    micAutoLevelCv.notify_one();
                });

            micAutoLevelWorker = std::jthread([this](std::stop_token stopToken) {
                while (!stopToken.stop_requested()) {
                    std::unique_lock<std::mutex> lock(micAutoLevelMutex);
                    micAutoLevelCv.wait(lock, [this, &stopToken] {
                        return stopToken.stop_requested() || micAutoLevelPending;
                    });
                    if (stopToken.stop_requested()) break;

                    const int peak = micAutoLevelPendingPeak;
                    const double clipRatio = micAutoLevelPendingClipRatio;
                    micAutoLevelPending = false;
                    lock.unlock();

                    CubeLog::info(
                        "Mic auto level: hot input detected, peak=" + std::to_string(peak) +
                        ", clipRatio=" + std::to_string(clipRatio));
                    if (micVolumeController) {
                        micVolumeController->lowerStep();
                    }
                }
            });
            CubeLog::info("SpeechIn: mic auto level enabled");
        } else {
            CubeLog::warning("SpeechIn: mic auto level unavailable, continuing without automatic input adjustment");
        }
    }

    // capture to router ingest
    capture = std::make_unique<AudioCapture>(
        router->ingestQueue(),
        [this](std::span<const int16_t> samples) {
            if (micLevelMonitor) {
                micLevelMonitor->observe(samples);
            }
        });
    // Start order: router -> client -> capture
    router->start();
    wwClient->start();
    capture->start();
    CubeLog::info("SpeechIn: started");
}

void SpeechIn::stop()
{
    CubeLog::info("SpeechIn: stop requested");
    if (capture) {
        capture->stop();
        capture.reset();
    }
    if (micAutoLevelWorker.joinable()) {
        micAutoLevelWorker.request_stop();
        micAutoLevelCv.notify_all();
        micAutoLevelWorker = std::jthread();
    }
    if (micRestoreOnStop && micVolumeController) {
        micVolumeController->restoreBaseline();
    }
    micLevelMonitor.reset();
    micVolumeController.reset();
    if (router) {
        router->stop();
        router.reset();
    }
    if (wwClient) {
        wwClient->stop();
        wwClient.reset();
    }
    CubeLog::info("SpeechIn: stopped");
}

// Snapshot helpers for router
std::vector<std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>> SpeechIn::snapshotWakeTargets()
{
    std::vector<std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>> out;
    std::lock_guard<std::mutex> lg(SpeechIn::registeredQueuesMutex);
    out.reserve(SpeechIn::registeredWakeAudioQueues.size());
    for (auto it = SpeechIn::registeredWakeAudioQueues.begin(); it != SpeechIn::registeredWakeAudioQueues.end();) {
        if (auto sp = it->second.lock()) {
            out.push_back(std::move(sp));
            ++it;
        } else {
            it = SpeechIn::registeredWakeAudioQueues.erase(it);
        }
    }
    return out;
}

std::vector<std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>> SpeechIn::snapshotPreTriggerTargets()
{
    std::vector<std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>> out;
    std::lock_guard<std::mutex> lg(SpeechIn::registeredQueuesMutex);
    out.reserve(SpeechIn::registeredPreTriggerAudioQueues.size());
    for (auto it = SpeechIn::registeredPreTriggerAudioQueues.begin(); it != SpeechIn::registeredPreTriggerAudioQueues.end();) {
        if (auto sp = it->second.lock()) {
            out.push_back(std::move(sp));
            ++it;
        } else {
            it = SpeechIn::registeredPreTriggerAudioQueues.erase(it);
        }
    }
    return out;
}

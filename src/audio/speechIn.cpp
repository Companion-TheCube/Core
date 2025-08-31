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
#include "wakeWordClient.h"
#ifndef LOGGER_H
#include <logger.h>
#endif

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
    // Build components
    wwClient = std::make_unique<WakeWordClient>();
    router = std::make_unique<AudioRouter>();
    router->setWakeWordClient(std::shared_ptr<WakeWordClient>(wwClient.get(), [](WakeWordClient*){}));
    router->setProviders(
        []() { return SpeechIn::snapshotWakeTargets(); },
        []() { return SpeechIn::snapshotPreTriggerTargets(); }
    );
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
    // capture to router ingest
    capture = std::make_unique<AudioCapture>(router->ingestQueue());
    // Start order: router -> client -> capture
    router->start();
    wwClient->start();
    capture->start();
}

void SpeechIn::stop()
{
    if (capture) capture->stop();
    if (wwClient) wwClient->stop();
    if (router) router->stop();
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

/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

#include "threadsafeQueue.h"
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class WakeWordClient;

// AudioRouter: central hub for audio ingestion, pre-trigger buffering,
// and fan-out after wake detection.
class AudioRouter {
public:
    using QueuePtr = std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>;
    using TargetsProvider = std::function<std::vector<QueuePtr>()>;

    AudioRouter();
    ~AudioRouter();

    void setWakeWordClient(const std::shared_ptr<WakeWordClient>& client);
    void setProviders(TargetsProvider wakeTargets, TargetsProvider preTargets);

    void start();
    void stop();

    // Queue that AudioCapture should push into
    QueuePtr ingestQueue() const { return ingest_; }

    // Wake event from detector
    void onWakeDetected();

private:
    QueuePtr ingest_ { std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>() };
    std::jthread thread_;
    std::atomic<bool> stop_{ false };
    std::shared_ptr<WakeWordClient> wakeClient_;
    TargetsProvider wakeTargetsProvider_;
    TargetsProvider preTargetsProvider_;

    // pre-trigger ring buffer (blocks)
    std::deque<std::vector<int16_t>> ring_;
    size_t maxBlocks_ { 0 };
    std::mutex mtx_;

    // fan-out state after wake
    bool streaming_ { false };
    std::vector<QueuePtr> currentWakeTargets_;

    void run(std::stop_token st);
};


/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

#include "RtAudio.h"
#include "threadsafeQueue.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <thread>

// AudioCapture: configures RtAudio to capture mono 16 kHz int16 audio
// and pushes frames into a provided ThreadSafeQueue.
class AudioCapture {
public:
    using AudioObserver = std::function<void(std::span<const int16_t>)>;

    explicit AudioCapture(
        std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> targetQueue,
        AudioObserver observer = {});
    ~AudioCapture();

    void start();
    void stop();

private:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> queue_;
    AudioObserver observer_;
    std::jthread thread_;
    std::atomic<bool> stop_{false};
    void run(std::stop_token st);
};

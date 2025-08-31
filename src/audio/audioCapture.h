/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

#include "RtAudio.h"
#include "threadsafeQueue.h"
#include <cstdint>
#include <memory>
#include <thread>

// AudioCapture: configures RtAudio to capture mono 16 kHz int16 audio
// and pushes frames into a provided ThreadSafeQueue.
class AudioCapture {
public:
    explicit AudioCapture(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> targetQueue);
    ~AudioCapture();

    void start();
    void stop();

private:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> queue_;
    std::jthread thread_;
    std::atomic<bool> stop_{false};
    void run(std::stop_token st);
};


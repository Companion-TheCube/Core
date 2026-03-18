/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/
#pragma once

#include "threadsafeQueue.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// WakeWordClient: maintains a connection to the openwakeword unix socket,
// writes audio blocks, and invokes onWake callback when detection messages
// are received.
class WakeWordClient {
public:
    using OnWake = std::function<void()>;
    WakeWordClient();
    ~WakeWordClient();

    void setOnWake(OnWake cb);
    void start();
    void stop();

    // Enqueue an audio block to be sent to the detector
    void submit(const std::vector<int16_t>& block);

private:
    std::jthread thread_;
    std::atomic<bool> stop_{ false };
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> txQueue_ { std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>() };
    OnWake onWake_;
    std::chrono::time_point<std::chrono::steady_clock> lastWake_ { std::chrono::steady_clock::now() };
    void run(std::stop_token st);
};


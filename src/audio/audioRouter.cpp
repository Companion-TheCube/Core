/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "audioRouter.h"
#include "wakeWordClient.h"
#include "speechIn.h" // for SAMPLE_RATE, ROUTER_FIFO_SIZE
#ifndef LOGGER_H
#include <logger.h>
#endif

AudioRouter::AudioRouter()
{
    maxBlocks_ = (5 * SAMPLE_RATE + ROUTER_FIFO_SIZE - 1) / ROUTER_FIFO_SIZE;
}

AudioRouter::~AudioRouter() { stop(); }

void AudioRouter::setWakeWordClient(const std::shared_ptr<WakeWordClient>& client)
{
    wakeClient_ = client;
}

void AudioRouter::setProviders(TargetsProvider wakeTargets, TargetsProvider preTargets)

{
    wakeTargetsProvider_ = std::move(wakeTargets);
    preTargetsProvider_ = std::move(preTargets);
}

void AudioRouter::start()
{
    stop_.store(false);
    thread_ = std::jthread([this](std::stop_token st) { this->run(st); });
}

void AudioRouter::stop()
{
    stop_.store(true);
    if (thread_.joinable()) thread_.request_stop();
}

void AudioRouter::onWakeDetected()
{
    std::lock_guard<std::mutex> lk(mtx_);
    // Snapshot current wake targets
    if (wakeTargetsProvider_) currentWakeTargets_ = wakeTargetsProvider_();
    // Fan-out pre-trigger buffers to queues that requested them
    if (preTargetsProvider_) {
        auto preTargets = preTargetsProvider_();
        for (const auto& block : ring_) {
            for (auto& q : preTargets) if (q) q->push(block);
        }
    }
    streaming_ = true;
}

void AudioRouter::run(std::stop_token st)
{
    while (!st.stop_requested() && !stop_.load()) {
        auto opt = ingest_->pop();
        if (!opt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        const auto& block = *opt;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            // update ring
            ring_.push_back(block);
            while (ring_.size() > maxBlocks_) ring_.pop_front();
            // forward to wakeword client
            if (wakeClient_) wakeClient_->submit(block);
            // fan-out live audio if streaming
            if (streaming_ && !currentWakeTargets_.empty()) {
                for (auto& q : currentWakeTargets_) if (q) q->push(block);
            }
        }
    }
}


/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "audioRouter.h"
#include "wakeWordClient.h"
#include "audio/constants.h"
#include <chrono>
#ifndef LOGGER_H
#include <logger.h>
#endif

AudioRouter::AudioRouter()
{
    maxBlocks_ = (audio::PRE_TRIGGER_SECONDS * audio::SAMPLE_RATE + audio::ROUTER_FIFO_FRAMES - 1) / audio::ROUTER_FIFO_FRAMES;
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
        CubeLog::info("AudioRouter wake: ringBlocks=" + std::to_string(ring_.size()) + 
                      ", preTargets=" + std::to_string(preTargets.size()) +
                      ", wakeTargets=" + std::to_string(currentWakeTargets_.size()));
        for (const auto& block : ring_) {
            for (auto& q : preTargets) if (q) q->push(block);
        }
    }
    streaming_ = true;
}

void AudioRouter::run(std::stop_token st)
{
    using clock = std::chrono::steady_clock;
    auto lastLog = clock::now();
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
            // periodic health log
            auto now = clock::now();
            if (now - lastLog > std::chrono::seconds(5)) {
                lastLog = now;
                size_t ingSize = ingest_->size();
                CubeLog::debug("AudioRouter health: ringBlocks=" + std::to_string(ring_.size()) +
                               ", ingestQ=" + std::to_string(ingSize) +
                               ", streaming=" + std::string(streaming_ ? "true" : "false") +
                               ", wakeTargets=" + std::to_string(currentWakeTargets_.size()));
            }
        }
    }
}

/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗███████╗███████╗███████╗███████╗██╗ ██████╗ ███╗   ██╗   ██╗  ██╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔════╝██╔════╝██╔════╝██╔════╝██║██╔═══██╗████╗  ██║   ██║  ██║
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║███████╗█████╗  ███████╗███████╗██║██║   ██║██╔██╗ ██║   ███████║
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║╚════██║██╔══╝  ╚════██║╚════██║██║██║   ██║██║╚██╗██║   ██╔══██║
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║███████║███████╗███████║███████║██║╚██████╔╝██║ ╚████║██╗██║  ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚══════╝╚══════╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef EVENT_DAEMON_SESSION_H
#define EVENT_DAEMON_SESSION_H

#include "../apiEventBroker.h"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class EventDaemonSession {
public:
    static constexpr size_t kDefaultMaxQueuedFrames = 128;

    EventDaemonSession(
        int clientFd,
        std::optional<ApiEventBroker::SourceSet> sources,
        uint64_t sinceSequence,
        uint64_t cutoverSequence,
        size_t maxQueuedFrames = kDefaultMaxQueuedFrames);
    ~EventDaemonSession();

    void start();
    void stop();

    bool matchesSource(const std::string& source) const;
    bool isPending() const;
    bool isClosed() const;
    uint64_t cutoverSequence() const;
    uint64_t lastQueuedSequence() const;
    uint64_t lastSentSequence() const;

    bool queueHelloAck(uint64_t nextSequence, bool historyTruncated);
    bool queueReplayEvent(const ApiEvent& event);
    bool queueLiveEvent(const ApiEvent& event);
    bool bufferPendingLiveEvent(const ApiEvent& event);
    bool queueTerminalError(const std::string& code, const std::string& message, uint64_t nextSequence);
    void finishReplay();

private:
    struct QueuedFrame {
        std::string payload;
        std::optional<uint64_t> deliveredSequence;
        bool terminal = false;
    };

    bool queueFrameLocked(const QueuedFrame& frame);
    void armResyncRequiredLocked(const std::string& message);
    void writerLoop(std::stop_token stopToken);
    bool sendAll(const std::string& payload);

    int clientFd_ = -1;
    std::optional<ApiEventBroker::SourceSet> sources_;
    uint64_t cutoverSequence_ = 0;

    mutable std::mutex mutex_;
    std::condition_variable_any cv_;
    std::deque<QueuedFrame> outboundFrames_;
    std::vector<QueuedFrame> pendingLiveFrames_;
    std::jthread writerThread_;
    bool pending_ = true;
    bool closed_ = false;
    uint64_t lastQueuedSequence_ = 0;
    uint64_t lastSentSequence_ = 0;
    size_t maxQueuedFrames_ = kDefaultMaxQueuedFrames;
};

#endif

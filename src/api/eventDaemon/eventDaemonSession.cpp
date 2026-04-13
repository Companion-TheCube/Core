/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗███████╗███████╗███████╗███████╗██╗ ██████╗ ███╗   ██╗    ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔════╝██╔════╝██╔════╝██╔════╝██║██╔═══██╗████╗  ██║   ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║███████╗█████╗  ███████╗███████╗██║██║   ██║██╔██╗ ██║   ██║     ██████╔╝██████╔╝
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║╚════██║██╔══╝  ╚════██║╚════██║██║██║   ██║██║╚██╗██║   ██║     ██╔═══╝ ██╔═══╝
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║███████║███████╗███████║███████║██║╚██████╔╝██║ ╚████║██╗╚██████╗██║     ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚══════╝╚══════╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "eventDaemonSession.h"

#include "eventDaemonProtocol.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

constexpr auto kHeartbeatInterval = std::chrono::seconds(1);

bool configureClientSocket(int fd)
{
    timeval sendTimeout {};
    sendTimeout.tv_sec = 1;
    sendTimeout.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) != 0) {
        CubeLog::warning("EventDaemonSession: failed to set send timeout on client socket.");
    }
    return true;
}

} // namespace

EventDaemonSession::EventDaemonSession(
    int clientFd,
    std::optional<ApiEventBroker::SourceSet> sources,
    uint64_t sinceSequence,
    uint64_t cutoverSequence,
    size_t maxQueuedFrames)
    : clientFd_(clientFd)
    , sources_(std::move(sources))
    , cutoverSequence_(cutoverSequence)
    , lastQueuedSequence_(sinceSequence)
    , lastSentSequence_(sinceSequence)
    , maxQueuedFrames_(std::max<size_t>(1, maxQueuedFrames))
{
    if (clientFd_ >= 0) {
        configureClientSocket(clientFd_);
    }
}

EventDaemonSession::~EventDaemonSession()
{
    stop();
}

void EventDaemonSession::start()
{
    if (writerThread_.joinable()) {
        return;
    }

    writerThread_ = std::jthread([this](std::stop_token stopToken) {
        writerLoop(stopToken);
    });
}

void EventDaemonSession::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            if (writerThread_.joinable()) {
                writerThread_.request_stop();
                writerThread_.join();
            }
            return;
        }
        closed_ = true;
    }

    cv_.notify_all();

    if (writerThread_.joinable()) {
        writerThread_.request_stop();
    }

    if (clientFd_ >= 0) {
        shutdown(clientFd_, SHUT_RDWR);
    }

    if (writerThread_.joinable()) {
        writerThread_.join();
    }

    if (clientFd_ >= 0) {
        close(clientFd_);
        clientFd_ = -1;
    }
}

bool EventDaemonSession::matchesSource(const std::string& source) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return !sources_.has_value() || sources_->contains(source);
}

bool EventDaemonSession::isPending() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_;
}

bool EventDaemonSession::isClosed() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

uint64_t EventDaemonSession::cutoverSequence() const
{
    return cutoverSequence_;
}

uint64_t EventDaemonSession::lastQueuedSequence() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return lastQueuedSequence_;
}

uint64_t EventDaemonSession::lastSentSequence() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return lastSentSequence_;
}

bool EventDaemonSession::queueHelloAck(uint64_t nextSequence, bool historyTruncated)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queueFrameLocked(QueuedFrame {
        .payload = EventDaemonProtocol::serializeHelloAck(nextSequence, historyTruncated),
        .deliveredSequence = std::nullopt,
        .terminal = false,
    });
}

bool EventDaemonSession::queueReplayEvent(const ApiEvent& event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (event.sequence <= lastQueuedSequence_) {
        return true;
    }

    if (!queueFrameLocked(QueuedFrame {
            .payload = EventDaemonProtocol::serializeEvent(event),
            .deliveredSequence = event.sequence,
            .terminal = false,
        })) {
        return false;
    }

    lastQueuedSequence_ = event.sequence;
    return true;
}

bool EventDaemonSession::queueLiveEvent(const ApiEvent& event)
{
    return queueReplayEvent(event);
}

bool EventDaemonSession::bufferPendingLiveEvent(const ApiEvent& event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }
    if (event.sequence <= cutoverSequence_ || event.sequence <= lastQueuedSequence_) {
        return true;
    }
    if (pendingLiveFrames_.size() >= maxQueuedFrames_) {
        armResyncRequiredLocked("Event daemon session overflowed while buffering live events.");
        return false;
    }

    pendingLiveFrames_.push_back(QueuedFrame {
        .payload = EventDaemonProtocol::serializeEvent(event),
        .deliveredSequence = event.sequence,
        .terminal = false,
    });
    lastQueuedSequence_ = event.sequence;
    return true;
}

bool EventDaemonSession::queueTerminalError(const std::string& code, const std::string& message, uint64_t nextSequence)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return false;
    }

    outboundFrames_.clear();
    pendingLiveFrames_.clear();
    const bool queued = queueFrameLocked(QueuedFrame {
        .payload = EventDaemonProtocol::serializeError(code, message, nextSequence),
        .deliveredSequence = std::nullopt,
        .terminal = true,
    });
    closed_ = !queued;
    return queued;
}

void EventDaemonSession::finishReplay()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return;
    }

    pending_ = false;
    for (const auto& frame : pendingLiveFrames_) {
        if (!queueFrameLocked(frame)) {
            break;
        }
    }
    pendingLiveFrames_.clear();
}

bool EventDaemonSession::queueFrameLocked(const QueuedFrame& frame)
{
    if (closed_) {
        return false;
    }
    if (outboundFrames_.size() >= maxQueuedFrames_) {
        armResyncRequiredLocked("Event daemon session overflowed while queueing outbound frames.");
        return false;
    }

    outboundFrames_.push_back(frame);
    cv_.notify_all();
    return true;
}

void EventDaemonSession::armResyncRequiredLocked(const std::string& message)
{
    outboundFrames_.clear();
    pendingLiveFrames_.clear();
    outboundFrames_.push_back(QueuedFrame {
        .payload = EventDaemonProtocol::serializeError("resync_required", message, lastSentSequence_),
        .deliveredSequence = std::nullopt,
        .terminal = true,
    });
    cv_.notify_all();
}

void EventDaemonSession::writerLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        QueuedFrame frame;
        bool sendHeartbeat = false;
        uint64_t heartbeatSequence = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, kHeartbeatInterval, [this, &stopToken]() {
                return stopToken.stop_requested() || closed_ || !outboundFrames_.empty();
            });

            if (stopToken.stop_requested() || closed_) {
                break;
            }

            if (outboundFrames_.empty()) {
                sendHeartbeat = true;
                heartbeatSequence = lastSentSequence_;
            } else {
                frame = outboundFrames_.front();
                outboundFrames_.pop_front();
            }
        }

        const bool sent = sendHeartbeat
            ? sendAll(EventDaemonProtocol::serializeHeartbeat(heartbeatSequence))
            : sendAll(frame.payload);
        if (!sent) {
            CubeLog::warning("EventDaemonSession: failed to send frame to connected app.");
            break;
        }

        if (!sendHeartbeat && frame.deliveredSequence.has_value()) {
            std::lock_guard<std::mutex> lock(mutex_);
            lastSentSequence_ = *frame.deliveredSequence;
        }

        if (!sendHeartbeat && frame.terminal) {
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }

    if (clientFd_ >= 0) {
        shutdown(clientFd_, SHUT_RDWR);
        close(clientFd_);
        clientFd_ = -1;
    }
}

bool EventDaemonSession::sendAll(const std::string& payload)
{
    if (clientFd_ < 0) {
        return false;
    }

    size_t totalSent = 0;
    while (totalSent < payload.size()) {
        const ssize_t sent = send(
            clientFd_,
            payload.data() + totalSent,
            payload.size() - totalSent,
            MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (sent == 0) {
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }

    return true;
}

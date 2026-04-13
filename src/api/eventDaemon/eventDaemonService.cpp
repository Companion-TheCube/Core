/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗███████╗███████╗██████╗ ██╗   ██╗██╗ ██████╗███████╗    ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║   ██║██║██╔════╝██╔════╝   ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║███████╗█████╗  ██████╔╝██║   ██║██║██║     █████╗     ██║     ██████╔╝██████╔╝
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██║██║     ██╔══╝     ██║     ██╔═══╝ ██╔═══╝
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║███████║███████╗██║  ██║ ╚████╔╝ ██║╚██████╗███████╗██╗╚██████╗██║     ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚═╝ ╚═════╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "eventDaemonService.h"

#include "eventDaemonProtocol.h"
#include "eventDaemonSession.h"

#include "../authentication.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

constexpr auto kDispatchPollInterval = std::chrono::milliseconds(1000);
constexpr auto kHelloReadTimeout = std::chrono::seconds(3);
constexpr size_t kReplayBatchSize = 128;
constexpr size_t kDispatchBatchSize = 128;
constexpr size_t kMaximumHelloFrameBytes = 64 * 1024;

bool sendAll(int fd, const std::string& payload)
{
    size_t totalSent = 0;
    while (totalSent < payload.size()) {
        const ssize_t sent = send(fd, payload.data() + totalSent, payload.size() - totalSent, MSG_NOSIGNAL);
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

void closeSocketQuietly(int fd)
{
    if (fd < 0) {
        return;
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

std::expected<std::string, std::string> readLineWithTimeout(int fd, std::chrono::milliseconds timeout)
{
    timeval readTimeout {};
    readTimeout.tv_sec = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(timeout).count());
    readTimeout.tv_usec = static_cast<long>((timeout - std::chrono::seconds(readTimeout.tv_sec)).count() * 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &readTimeout, sizeof(readTimeout)) != 0) {
        return std::unexpected("Failed to configure hello read timeout.");
    }

    std::string line;
    line.reserve(256);
    char ch = '\0';
    while (line.size() < kMaximumHelloFrameBytes) {
        const ssize_t bytesRead = recv(fd, &ch, 1, 0);
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return std::unexpected("Timed out waiting for hello frame.");
            }
            return std::unexpected("Failed to read hello frame.");
        }
        if (bytesRead == 0) {
            return std::unexpected("Connection closed before hello frame was received.");
        }
        if (ch == '\n') {
            return line;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }

    return std::unexpected("Hello frame exceeded maximum size.");
}

bool makeUnixSocketAddress(const std::string& path, sockaddr_un& addr)
{
    if (path.empty() || path.size() >= sizeof(addr.sun_path)) {
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    return true;
}

void sendErrorAndClose(int fd, const std::string& code, const std::string& message, uint64_t nextSequence)
{
    sendAll(fd, EventDaemonProtocol::serializeError(code, message, nextSequence));
    closeSocketQuietly(fd);
}

} // namespace

EventDaemonService::EventDaemonService(
    std::shared_ptr<ApiEventBroker> broker,
    std::string socketPath,
    size_t maxQueuedFramesPerSession)
    : broker_(std::move(broker))
    , socketPath_(std::move(socketPath))
    , maxQueuedFramesPerSession_(std::max<size_t>(1, maxQueuedFramesPerSession))
{
}

EventDaemonService::~EventDaemonService()
{
    stop();
}

bool EventDaemonService::start()
{
    if (started_.exchange(true)) {
        return true;
    }

    if (!broker_) {
        CubeLog::error("EventDaemonService: cannot start without an event broker.");
        started_ = false;
        return false;
    }

    std::error_code ec;
    const std::filesystem::path socketPath(socketPath_);
    const auto socketDir = socketPath.parent_path();
    if (!socketDir.empty()) {
        std::filesystem::create_directories(socketDir, ec);
        if (ec) {
            CubeLog::error("EventDaemonService: failed to create socket directory: " + ec.message());
            started_ = false;
            return false;
        }
    }

    std::filesystem::remove(socketPath, ec);

    listenFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        CubeLog::error("EventDaemonService: failed to create AF_UNIX socket.");
        started_ = false;
        return false;
    }

    sockaddr_un addr {};
    if (!makeUnixSocketAddress(socketPath_, addr)) {
        CubeLog::error("EventDaemonService: invalid daemon socket path.");
        closeSocketQuietly(listenFd_);
        listenFd_ = -1;
        started_ = false;
        return false;
    }

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        CubeLog::error("EventDaemonService: failed to bind daemon socket.");
        closeSocketQuietly(listenFd_);
        listenFd_ = -1;
        started_ = false;
        return false;
    }

    if (listen(listenFd_, 64) != 0) {
        CubeLog::error("EventDaemonService: failed to listen on daemon socket.");
        closeSocketQuietly(listenFd_);
        listenFd_ = -1;
        started_ = false;
        return false;
    }

    dispatchCursor_ = broker_->latestSequence();

    listenerThread_ = std::jthread([this](std::stop_token stopToken) {
        listenerLoop(stopToken);
    });
    dispatchThread_ = std::jthread([this](std::stop_token stopToken) {
        dispatchLoop(stopToken);
    });

    CubeLog::info("EventDaemonService: listening on " + socketPath_);
    return true;
}

void EventDaemonService::stop()
{
    if (!started_.exchange(false)) {
        return;
    }

    if (listenerThread_.joinable()) {
        listenerThread_.request_stop();
    }
    if (dispatchThread_.joinable()) {
        dispatchThread_.request_stop();
    }

    if (listenFd_ >= 0) {
        closeSocketQuietly(listenFd_);
        listenFd_ = -1;
    }

    if (listenerThread_.joinable()) {
        listenerThread_.join();
    }
    if (dispatchThread_.joinable()) {
        dispatchThread_.join();
    }

    std::vector<std::shared_ptr<EventDaemonSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions.swap(sessions_);
    }
    for (const auto& session : sessions) {
        if (session) {
            session->stop();
        }
    }

    std::error_code ec;
    std::filesystem::remove(socketPath_, ec);
}

std::string EventDaemonService::socketPath() const
{
    return socketPath_;
}

size_t EventDaemonService::sessionCount() const
{
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    return sessions_.size();
}

void EventDaemonService::listenerLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        removeClosedSessions();

        if (listenFd_ < 0) {
            return;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenFd_, &readSet);
        timeval timeout {};
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;

        const int ready = select(listenFd_ + 1, &readSet, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (ready == 0 || !FD_ISSET(listenFd_, &readSet)) {
            continue;
        }

        const int clientFd = accept(listenFd_, nullptr, nullptr);
        if (clientFd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            CubeLog::warning("EventDaemonService: accept() failed.");
            continue;
        }

        handleAcceptedClient(clientFd);
    }
}

void EventDaemonService::dispatchLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested()) {
        const auto page = broker_->waitForEvents(
            dispatchCursor_.load(),
            std::nullopt,
            kDispatchBatchSize,
            kDispatchPollInterval);

        if (page.historyTruncated && page.events.empty()) {
            dispatchCursor_ = broker_->latestSequence();
            removeClosedSessions();
            continue;
        }

        if (!page.events.empty()) {
            std::vector<std::shared_ptr<EventDaemonSession>> sessionsSnapshot;
            {
                std::lock_guard<std::mutex> lock(sessionsMutex_);
                sessionsSnapshot = sessions_;
            }

            for (const auto& event : page.events) {
                for (const auto& session : sessionsSnapshot) {
                    if (!session || session->isClosed() || !session->matchesSource(event.source)) {
                        continue;
                    }

                    if (session->isPending()) {
                        if (event.sequence <= session->cutoverSequence()) {
                            continue;
                        }
                        session->bufferPendingLiveEvent(event);
                        continue;
                    }

                    if (event.sequence <= session->lastQueuedSequence()) {
                        continue;
                    }
                    session->queueLiveEvent(event);
                }

                dispatchCursor_ = event.sequence;
            }
        }

        removeClosedSessions();
    }
}

void EventDaemonService::handleAcceptedClient(int clientFd)
{
    const auto helloLine = readLineWithTimeout(clientFd, std::chrono::duration_cast<std::chrono::milliseconds>(kHelloReadTimeout));
    if (!helloLine) {
        sendErrorAndClose(clientFd, "invalid_request", helloLine.error(), 0);
        return;
    }

    const auto hello = EventDaemonProtocol::parseHelloFrame(*helloLine);
    if (!hello) {
        sendErrorAndClose(clientFd, hello.error().code, hello.error().message, 0);
        return;
    }

    if (hello->sources.has_value()) {
        for (const auto& source : *hello->sources) {
            if (!broker_->hasSource(source)) {
                sendErrorAndClose(clientFd, "unknown_source", "Unknown source '" + source + "'.", hello->sinceSequence);
                return;
            }
        }
    }

    if (!CubeAuth::isAuthorizedApp(hello->appAuthId, kConnectGrantEndpoint)) {
        const std::string authError = CubeAuth::getLastError();
        sendErrorAndClose(
            clientFd,
            "not_authorized",
            authError.empty() ? "App is not authorized to connect to the event daemon." : authError,
            hello->sinceSequence);
        return;
    }

    const uint64_t cutoverSequence = broker_->latestSequence();
    const auto preview = broker_->waitForEvents(
        hello->sinceSequence,
        hello->sources,
        1,
        std::chrono::milliseconds::zero());

    auto session = std::make_shared<EventDaemonSession>(
        clientFd,
        hello->sources,
        hello->sinceSequence,
        cutoverSequence,
        maxQueuedFramesPerSession_);
    session->start();
    session->queueHelloAck(hello->sinceSequence, preview.historyTruncated);

    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_.push_back(session);
    }

    replaySessionUpToCutover(session, hello->sinceSequence, hello->sources, cutoverSequence);
    session->finishReplay();
}

void EventDaemonService::replaySessionUpToCutover(
    const std::shared_ptr<EventDaemonSession>& session,
    uint64_t sinceSequence,
    const std::optional<ApiEventBroker::SourceSet>& sources,
    uint64_t cutoverSequence)
{
    uint64_t cursor = sinceSequence;
    while (!session->isClosed()) {
        const auto page = broker_->waitForEvents(
            cursor,
            sources,
            kReplayBatchSize,
            std::chrono::milliseconds::zero());
        if (page.events.empty()) {
            break;
        }

        bool advanced = false;
        for (const auto& event : page.events) {
            if (event.sequence > cutoverSequence) {
                return;
            }
            if (!session->queueReplayEvent(event)) {
                return;
            }
            cursor = event.sequence;
            advanced = true;
        }

        if (!advanced) {
            break;
        }
    }
}

void EventDaemonService::removeClosedSessions()
{
    std::vector<std::shared_ptr<EventDaemonSession>> removedSessions;
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = std::remove_if(sessions_.begin(), sessions_.end(), [&](const auto& session) {
            return !session || session->isClosed();
        });
        removedSessions.assign(it, sessions_.end());
        sessions_.erase(it, sessions_.end());
    }

    for (const auto& session : removedSessions) {
        if (session) {
            session->stop();
        }
    }
}

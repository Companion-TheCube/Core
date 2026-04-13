/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗███████╗███████╗██████╗ ██╗   ██╗██╗ ██████╗███████╗   ██╗  ██╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔════╝██╔════╝██╔══██╗██║   ██║██║██╔════╝██╔════╝   ██║  ██║
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║███████╗█████╗  ██████╔╝██║   ██║██║██║     █████╗     ███████║
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██║██║     ██╔══╝     ██╔══██║
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║███████║███████╗██║  ██║ ╚████╔╝ ██║╚██████╗███████╗██╗██║  ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚═╝ ╚═════╝╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef EVENT_DAEMON_SERVICE_H
#define EVENT_DAEMON_SERVICE_H

#include "../apiEventBroker.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class EventDaemonSession;

class EventDaemonService {
public:
    static constexpr const char* kConnectGrantEndpoint = "Events-daemon-connect";

    EventDaemonService(
        std::shared_ptr<ApiEventBroker> broker,
        std::string socketPath,
        size_t maxQueuedFramesPerSession = 128);
    ~EventDaemonService();

    bool start();
    void stop();

    std::string socketPath() const;
    size_t sessionCount() const;

private:
    void listenerLoop(std::stop_token stopToken);
    void dispatchLoop(std::stop_token stopToken);
    void handleAcceptedClient(int clientFd);
    void replaySessionUpToCutover(
        const std::shared_ptr<EventDaemonSession>& session,
        uint64_t sinceSequence,
        const std::optional<ApiEventBroker::SourceSet>& sources,
        uint64_t cutoverSequence);
    void removeClosedSessions();

    std::shared_ptr<ApiEventBroker> broker_;
    std::string socketPath_;
    int listenFd_ = -1;
    std::jthread listenerThread_;
    std::jthread dispatchThread_;
    std::atomic<uint64_t> dispatchCursor_ { 0 };
    std::atomic<bool> started_ { false };

    mutable std::mutex sessionsMutex_;
    std::vector<std::shared_ptr<EventDaemonSession>> sessions_;
    size_t maxQueuedFramesPerSession_ = 128;
};

#endif

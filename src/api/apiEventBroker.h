/*
 █████╗ ██████╗ ██╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗ ██████╗  ██████╗ ██╗  ██╗███████╗██████╗    ██╗  ██╗
██╔══██╗██╔══██╗██║██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔═══██╗██║ ██╔╝██╔════╝██╔══██╗   ██║  ██║
███████║██████╔╝██║█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██████╔╝██████╔╝██║   ██║█████╔╝ █████╗  ██████╔╝   ███████║
██╔══██║██╔═══╝ ██║██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██╗██╔══██╗██║   ██║██╔═██╗ ██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║██║     ██║███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║╚██████╔╝██║  ██╗███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef API_EVENT_BROKER_H
#define API_EVENT_BROKER_H

#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

struct ApiEvent {
    uint64_t sequence = 0;
    uint64_t occurredAtEpochMs = 0;
    std::string source;
    std::string event;
    nlohmann::json payload = nlohmann::json::object();
};

struct ApiEventPage {
    std::vector<ApiEvent> events;
    uint64_t nextSequence = 0;
    bool historyTruncated = false;
    bool timedOut = false;
};

class ApiEventBroker {
public:
    using SourceSet = std::unordered_set<std::string>;

    explicit ApiEventBroker(size_t maxHistory = 512);

    void registerSource(const std::string& source);
    bool hasSource(const std::string& source) const;
    std::vector<std::string> listSources() const;
    uint64_t latestSequence() const;
    uint64_t oldestSequence() const;

    ApiEvent publish(
        const std::string& source,
        const std::string& event,
        nlohmann::json payload,
        uint64_t occurredAtEpochMs);

    ApiEventPage waitForEvents(
        uint64_t sinceSequence,
        const std::optional<SourceSet>& sources,
        size_t limit,
        std::chrono::milliseconds waitDuration) const;

private:
    ApiEventPage collectPageLocked(
        uint64_t sinceSequence,
        const std::optional<SourceSet>& sources,
        size_t limit) const;

    size_t maxHistory_ = 512;
    mutable std::mutex mutex_;
    mutable std::condition_variable eventsAvailableCv_;
    std::deque<ApiEvent> history_;
    uint64_t nextSequence_ = 1;
    SourceSet knownSources_;
};

#endif

/*
 █████╗ ██████╗ ██╗███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗ ██████╗  ██████╗ ██╗  ██╗███████╗██████╗     ██████╗██████╗ ██████╗
██╔══██╗██╔══██╗██║██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔═══██╗██║ ██╔╝██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
███████║██████╔╝██║█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██████╔╝██████╔╝██║   ██║█████╔╝ █████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██║██╔═══╝ ██║██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██╗██╔══██╗██║   ██║██╔═██╗ ██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║  ██║██║     ██║███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║╚██████╔╝██║  ██╗███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "apiEventBroker.h"

#include <algorithm>

namespace {

bool eventMatchesSources(const ApiEvent& event, const std::optional<ApiEventBroker::SourceSet>& sources)
{
    return !sources.has_value() || sources->contains(event.source);
}

} // namespace

ApiEventBroker::ApiEventBroker(size_t maxHistory)
    : maxHistory_(std::max<size_t>(1, maxHistory))
{
}

void ApiEventBroker::registerSource(const std::string& source)
{
    if (source.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    knownSources_.insert(source);
}

bool ApiEventBroker::hasSource(const std::string& source) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return knownSources_.contains(source);
}

std::vector<std::string> ApiEventBroker::listSources() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return std::vector<std::string>(knownSources_.begin(), knownSources_.end());
}

uint64_t ApiEventBroker::latestSequence() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) {
        return 0;
    }
    return history_.back().sequence;
}

uint64_t ApiEventBroker::oldestSequence() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) {
        return 0;
    }
    return history_.front().sequence;
}

ApiEvent ApiEventBroker::publish(
    const std::string& source,
    const std::string& event,
    nlohmann::json payload,
    uint64_t occurredAtEpochMs)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!source.empty()) {
        knownSources_.insert(source);
    }

    ApiEvent brokerEvent;
    brokerEvent.sequence = nextSequence_++;
    brokerEvent.occurredAtEpochMs = occurredAtEpochMs;
    brokerEvent.source = source;
    brokerEvent.event = event;
    brokerEvent.payload = std::move(payload);

    history_.push_back(brokerEvent);
    while (history_.size() > maxHistory_) {
        history_.pop_front();
    }

    eventsAvailableCv_.notify_all();
    return brokerEvent;
}

ApiEventPage ApiEventBroker::waitForEvents(
    uint64_t sinceSequence,
    const std::optional<SourceSet>& sources,
    size_t limit,
    std::chrono::milliseconds waitDuration) const
{
    if (limit == 0) {
        limit = 1;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    ApiEventPage page = collectPageLocked(sinceSequence, sources, limit);
    if (!page.events.empty() || waitDuration <= std::chrono::milliseconds::zero()) {
        return page;
    }

    const bool sawEvent = eventsAvailableCv_.wait_for(lock, waitDuration, [&]() {
        ApiEventPage candidate = collectPageLocked(sinceSequence, sources, limit);
        return !candidate.events.empty() || candidate.historyTruncated;
    });

    page = collectPageLocked(sinceSequence, sources, limit);
    if (!sawEvent && page.events.empty()) {
        page.timedOut = true;
    }
    return page;
}

ApiEventPage ApiEventBroker::collectPageLocked(
    uint64_t sinceSequence,
    const std::optional<SourceSet>& sources,
    size_t limit) const
{
    ApiEventPage page;
    page.nextSequence = sinceSequence;

    if (history_.empty()) {
        return page;
    }

    const uint64_t oldestSequence = history_.front().sequence;
    const uint64_t latestSequence = history_.back().sequence;
    uint64_t effectiveSinceSequence = sinceSequence;
    if (effectiveSinceSequence + 1 < oldestSequence) {
        page.historyTruncated = true;
        effectiveSinceSequence = oldestSequence - 1;
    }

    for (const auto& event : history_) {
        if (event.sequence <= effectiveSinceSequence) {
            continue;
        }
        if (!eventMatchesSources(event, sources)) {
            continue;
        }
        page.events.push_back(event);
        if (page.events.size() >= limit) {
            break;
        }
    }

    if (!page.events.empty()) {
        page.nextSequence = page.events.back().sequence;
    } else if (page.historyTruncated) {
        page.nextSequence = latestSequence;
    }

    return page;
}

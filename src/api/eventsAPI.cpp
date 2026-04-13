/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗███████╗ █████╗ ██████╗ ██╗    ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██║   ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████╗███████║██████╔╝██║   ██║     ██████╔╝██████╔╝
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ╚════██║██╔══██║██╔═══╝ ██║   ██║     ██╔═══╝ ██╔═══╝
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ███████║██║  ██║██║     ██║██╗╚██████╗██║     ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "eventsAPI.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>

namespace {

constexpr size_t kDefaultEventLimit = 50;
constexpr size_t kMinimumEventLimit = 1;
constexpr size_t kMaximumEventLimit = 256;
constexpr int kDefaultWaitMs = 25000;
constexpr int kMaximumWaitMs = 30000;

std::string trimAsciiWhitespace(std::string value)
{
    const auto notSpace = [](unsigned char ch) {
        return !std::isspace(ch);
    };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

size_t parseLimit(const httplib::Request& req)
{
    const auto limitIt = req.params.find("limit");
    if (limitIt == req.params.end()) {
        return kDefaultEventLimit;
    }

    try {
        return static_cast<size_t>(std::clamp(
            std::stoi(limitIt->second),
            static_cast<int>(kMinimumEventLimit),
            static_cast<int>(kMaximumEventLimit)));
    } catch (...) {
        return kDefaultEventLimit;
    }
}

uint64_t parseSinceSequence(const httplib::Request& req)
{
    const auto sinceIt = req.params.find("since_seq");
    if (sinceIt == req.params.end()) {
        return 0;
    }

    try {
        return std::stoull(sinceIt->second);
    } catch (...) {
        return 0;
    }
}

int parseWaitMs(const httplib::Request& req)
{
    const auto waitIt = req.params.find("wait_ms");
    if (waitIt == req.params.end()) {
        return kDefaultWaitMs;
    }

    try {
        return std::clamp(std::stoi(waitIt->second), 0, kMaximumWaitMs);
    } catch (...) {
        return kDefaultWaitMs;
    }
}

std::optional<ApiEventBroker::SourceSet> parseSources(const httplib::Request& req)
{
    const auto sourcesIt = req.params.find("sources");
    if (sourcesIt == req.params.end()) {
        return std::nullopt;
    }

    ApiEventBroker::SourceSet sources;
    size_t offset = 0;
    const std::string rawSources = sourcesIt->second;
    while (offset <= rawSources.size()) {
        const size_t comma = rawSources.find(',', offset);
        const size_t tokenEnd = comma == std::string::npos ? rawSources.size() : comma;
        std::string token = trimAsciiWhitespace(rawSources.substr(offset, tokenEnd - offset));
        if (!token.empty()) {
            sources.insert(std::move(token));
        }
        if (comma == std::string::npos) {
            break;
        }
        offset = comma + 1;
    }

    return sources;
}

nlohmann::json apiEventToJson(const ApiEvent& event)
{
    return nlohmann::json {
        { "sequence", event.sequence },
        { "occurredAtEpochMs", event.occurredAtEpochMs },
        { "source", event.source },
        { "event", event.event },
        { "payload", event.payload },
    };
}

bool allSourcesKnown(const ApiEventBroker& broker, const ApiEventBroker::SourceSet& sources, std::string& unknownSourceOut)
{
    for (const auto& source : sources) {
        if (!broker.hasSource(source)) {
            unknownSourceOut = source;
            return false;
        }
    }
    return true;
}

} // namespace

EventsAPI::EventsAPI(std::shared_ptr<API> api)
    : api_(std::move(api))
{
}

HttpEndPointData_t EventsAPI::getHttpEndpointData()
{
    HttpEndPointData_t data;
    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto broker = api_ ? api_->getEventBroker() : nullptr;
            if (!broker) {
                res.status = 500;
                res.set_content(R"({"error":"event_broker_unavailable"})", "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Event broker unavailable");
            }

            const auto sources = parseSources(req);
            if (sources.has_value()) {
                std::string unknownSource;
                if (!allSourcesKnown(*broker, *sources, unknownSource)) {
                    res.status = 400;
                    res.set_content(
                        nlohmann::json {
                            { "error", "unknown_source" },
                            { "source", unknownSource },
                        }
                            .dump(),
                        "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Unknown event source");
                }
            }

            const auto page = broker->waitForEvents(
                parseSinceSequence(req),
                sources,
                parseLimit(req),
                std::chrono::milliseconds(parseWaitMs(req)));

            nlohmann::json events = nlohmann::json::array();
            for (const auto& event : page.events) {
                events.push_back(apiEventToJson(event));
            }

            res.status = 200;
            res.set_content(
                nlohmann::json {
                    { "events", events },
                    { "nextSequence", page.nextSequence },
                    { "historyTruncated", page.historyTruncated },
                    { "timedOut", page.timedOut },
                }
                    .dump(),
                "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "wait",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Wait for canonical API events with source filtering and replay"
    });
    return data;
}

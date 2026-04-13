/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗██████╗ ██████╗  ██████╗ ████████╗ ██████╗  ██████╗ ██████╗ ██╗         ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔══██╗██╔══██╗██╔═══██╗╚══██╔══╝██╔═══██╗██╔════╝██╔═══██╗██║        ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║██████╔╝██████╔╝██║   ██║   ██║   ██║   ██║██║     ██║   ██║██║        ██║     ██████╔╝██████╔╝
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║██╔═══╝ ██╔══██╗██║   ██║   ██║   ██║   ██║██║     ██║   ██║██║        ██║     ██╔═══╝ ██╔═══╝
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║██║     ██║  ██║╚██████╔╝   ██║   ╚██████╔╝╚██████╗╚██████╔╝███████╗██╗╚██████╗██║     ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝  ╚═╝ ╚═════╝    ╚═╝    ╚═════╝  ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "eventDaemonProtocol.h"

#include <nlohmann/json.hpp>

namespace {

std::expected<EventDaemonHello, EventDaemonProtocolError> invalidRequest(const std::string& message)
{
    return std::unexpected(EventDaemonProtocolError {
        .code = "invalid_request",
        .message = message,
    });
}

nlohmann::json canonicalEventToJson(const ApiEvent& event)
{
    return nlohmann::json {
        { "sequence", event.sequence },
        { "occurredAtEpochMs", event.occurredAtEpochMs },
        { "source", event.source },
        { "event", event.event },
        { "payload", event.payload },
    };
}

} // namespace

std::expected<EventDaemonHello, EventDaemonProtocolError> EventDaemonProtocol::parseHelloFrame(const std::string& line)
{
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(line);
    } catch (const std::exception& e) {
        return invalidRequest(std::string("Malformed JSON: ") + e.what());
    }

    if (!payload.is_object()) {
        return invalidRequest("Hello frame must be a JSON object.");
    }
    if (payload.value("type", "") != "hello") {
        return invalidRequest("First frame must have type 'hello'.");
    }
    if (!payload.contains("appAuthId") || !payload["appAuthId"].is_string() || payload["appAuthId"].get<std::string>().empty()) {
        return invalidRequest("Hello frame must include non-empty string field 'appAuthId'.");
    }
    if (payload.contains("sinceSeq") && !payload["sinceSeq"].is_number_unsigned()) {
        return invalidRequest("Field 'sinceSeq' must be an unsigned integer.");
    }
    if (payload.contains("sources") && !payload["sources"].is_array()) {
        return invalidRequest("Field 'sources' must be an array of strings.");
    }

    EventDaemonHello hello;
    hello.appAuthId = payload["appAuthId"].get<std::string>();
    hello.sinceSequence = payload.value("sinceSeq", 0ULL);

    if (payload.contains("sources")) {
        ApiEventBroker::SourceSet sources;
        for (const auto& source : payload["sources"]) {
            if (!source.is_string() || source.get<std::string>().empty()) {
                return invalidRequest("Field 'sources' must contain only non-empty strings.");
            }
            sources.insert(source.get<std::string>());
        }
        hello.sources = std::move(sources);
    }

    return hello;
}

std::string EventDaemonProtocol::serializeHelloAck(uint64_t nextSequence, bool historyTruncated)
{
    return nlohmann::json {
        { "type", "hello_ack" },
        { "nextSequence", nextSequence },
        { "historyTruncated", historyTruncated },
    }
        .dump()
        + "\n";
}

std::string EventDaemonProtocol::serializeEvent(const ApiEvent& event)
{
    return nlohmann::json {
        { "type", "event" },
        { "event", canonicalEventToJson(event) },
    }
        .dump()
        + "\n";
}

std::string EventDaemonProtocol::serializeHeartbeat(uint64_t nextSequence)
{
    return nlohmann::json {
        { "type", "heartbeat" },
        { "nextSequence", nextSequence },
    }
        .dump()
        + "\n";
}

std::string EventDaemonProtocol::serializeError(
    const std::string& code,
    const std::string& message,
    uint64_t nextSequence)
{
    return nlohmann::json {
        { "type", "error" },
        { "code", code },
        { "message", message },
        { "nextSequence", nextSequence },
    }
        .dump()
        + "\n";
}

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
    const nlohmann::json payload = nlohmann::json::parse(line, nullptr, false);
    if (payload.is_discarded()) {
        return invalidRequest("Malformed JSON.");
    }

    if (!payload.is_object()) {
        return invalidRequest("Hello frame must be a JSON object.");
    }

    const auto typeIt = payload.find("type");
    if (typeIt == payload.end() || !typeIt->is_string() || typeIt->get_ref<const std::string&>() != "hello") {
        return invalidRequest("First frame must have type 'hello'.");
    }

    const auto appAuthIdIt = payload.find("appAuthId");
    if (appAuthIdIt == payload.end() || !appAuthIdIt->is_string() || appAuthIdIt->get_ref<const std::string&>().empty()) {
        return invalidRequest("Hello frame must include non-empty string field 'appAuthId'.");
    }

    const auto sinceSeqIt = payload.find("sinceSeq");
    if (sinceSeqIt != payload.end() && !sinceSeqIt->is_number_unsigned()) {
        return invalidRequest("Field 'sinceSeq' must be an unsigned integer.");
    }

    const auto sourcesIt = payload.find("sources");
    if (sourcesIt != payload.end() && !sourcesIt->is_array()) {
        return invalidRequest("Field 'sources' must be an array of strings.");
    }

    EventDaemonHello hello;
    hello.appAuthId = appAuthIdIt->get_ref<const std::string&>();
    hello.sinceSequence = sinceSeqIt != payload.end() ? sinceSeqIt->get<uint64_t>() : 0ULL;

    if (sourcesIt != payload.end()) {
        ApiEventBroker::SourceSet sources;
        for (const auto& source : *sourcesIt) {
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

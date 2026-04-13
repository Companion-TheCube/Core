/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██████╗  █████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗██████╗ ██████╗  ██████╗ ████████╗ ██████╗  ██████╗ ██████╗ ██╗        ██╗  ██╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔══██╗██╔══██╗██╔═══██╗╚══██╔══╝██╔═══██╗██╔════╝██╔═══██╗██║        ██║  ██║
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ██║  ██║███████║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║██████╔╝██████╔╝██║   ██║   ██║   ██║   ██║██║     ██║   ██║██║        ███████║
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██║  ██║██╔══██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║██╔═══╝ ██╔══██╗██║   ██║   ██║   ██║   ██║██║     ██║   ██║██║        ██╔══██║
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██████╔╝██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║██║     ██║  ██║╚██████╔╝   ██║   ╚██████╔╝╚██████╗╚██████╔╝███████╗██╗██║  ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝  ╚═╝ ╚═════╝    ╚═╝    ╚═════╝  ╚═════╝ ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once
#ifndef EVENT_DAEMON_PROTOCOL_H
#define EVENT_DAEMON_PROTOCOL_H

#include "../apiEventBroker.h"

#include <expected>
#include <optional>
#include <string>

struct EventDaemonHello {
    std::string appAuthId;
    uint64_t sinceSequence = 0;
    std::optional<ApiEventBroker::SourceSet> sources;
};

struct EventDaemonProtocolError {
    std::string code;
    std::string message;
};

class EventDaemonProtocol {
public:
    static std::expected<EventDaemonHello, EventDaemonProtocolError> parseHelloFrame(const std::string& line);

    static std::string serializeHelloAck(uint64_t nextSequence, bool historyTruncated);
    static std::string serializeEvent(const ApiEvent& event);
    static std::string serializeHeartbeat(uint64_t nextSequence);
    static std::string serializeError(
        const std::string& code,
        const std::string& message,
        uint64_t nextSequence);
};

#endif

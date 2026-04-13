/*
██╗███╗   ██╗████████╗███████╗██████╗  █████╗  ██████╗████████╗██╗ ██████╗ ███╗   ██╗     █████╗ ██████╗ ██╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║    ██╔══██╗██╔══██╗██║
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝███████║██║        ██║   ██║██║   ██║██╔██╗ ██║    ███████║██████╔╝██║
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══██║██║        ██║   ██║██║   ██║██║╚██╗██║    ██╔══██║██╔═══╝ ██║
██║██║ ╚████║   ██║   ███████╗██║  ██║██║  ██║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║    ██║  ██║██║     ██║
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝    ╚═╝  ╚═╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC
*/

#include "interactionAPI.h"
#include "interactionEvents.h"

#include <algorithm>

namespace {

nlohmann::json accelerationSampleToJson(const std::optional<Bmi270AccelerationSample>& sample)
{
    if (!sample.has_value()) {
        return nullptr;
    }

    return nlohmann::json {
        { "xG", sample->xG },
        { "yG", sample->yG },
        { "zG", sample->zG },
    };
}

nlohmann::json interactionEventToJson(const InteractionEvent& event)
{
    return nlohmann::json {
        { "sequence", event.sequence },
        { "type", interactionEventTypeToString(event.type) },
        { "occurredAtEpochMs", event.occurredAtEpochMs },
        { "liftStateAfter", interactionLiftStateToString(event.liftStateAfter) },
        { "sample", accelerationSampleToJson(event.sample) },
    };
}

size_t clampRequestedLimit(const httplib::Request& req)
{
    const auto limitIt = req.params.find("limit");
    if (limitIt == req.params.end()) {
        return 50;
    }

    try {
        return static_cast<size_t>(std::clamp(std::stoi(limitIt->second), 1, 256));
    } catch (...) {
        return 50;
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

} // namespace

InteractionAPI::InteractionAPI(std::shared_ptr<PeripheralManager> peripheralManager)
    : peripheralManager_(std::move(peripheralManager))
{
}

HttpEndPointData_t InteractionAPI::getHttpEndpointData()
{
    HttpEndPointData_t data;

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            (void)req;
            if (!peripheralManager_) {
                res.status = 500;
                res.set_content(R"({"error":"peripheral_manager_unavailable"})", "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Peripheral manager unavailable");
            }

            const auto status = peripheralManager_->getInteractionStatus();
            nlohmann::json response = {
                { "available", status.available },
                { "enabled", status.enabled },
                { "initialized", status.initialized },
                { "liftState", interactionLiftStateToString(status.liftState) },
                { "latestSample", accelerationSampleToJson(status.latestSample) },
                { "lastTapAtEpochMs", status.lastTapAtEpochMs.has_value() ? nlohmann::json(*status.lastTapAtEpochMs) : nlohmann::json(nullptr) },
                { "lastLiftChangeAtEpochMs", status.lastLiftChangeAtEpochMs.has_value() ? nlohmann::json(*status.lastLiftChangeAtEpochMs) : nlohmann::json(nullptr) },
                { "lastEventSequence", status.lastEventSequence },
            };
            res.status = 200;
            res.set_content(response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "status",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get the current tap and lift interaction status"
    });

    data.push_back({
        PRIVATE_ENDPOINT | GET_ENDPOINT,
        [this](const httplib::Request& req, httplib::Response& res) {
            if (!peripheralManager_) {
                res.status = 500;
                res.set_content(R"({"error":"peripheral_manager_unavailable"})", "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Peripheral manager unavailable");
            }

            const auto page = peripheralManager_->getInteractionEvents(
                parseSinceSequence(req),
                clampRequestedLimit(req));

            nlohmann::json events = nlohmann::json::array();
            for (const auto& event : page.events) {
                events.push_back(interactionEventToJson(event));
            }

            nlohmann::json response = {
                { "events", events },
                { "nextSequence", page.nextSequence },
                { "historyTruncated", page.historyTruncated },
            };
            res.status = 200;
            res.set_content(response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "");
        },
        "events",
        nlohmann::json({ { "type", "object" }, { "properties", nlohmann::json::object() } }),
        "Get recent tap and lift interaction events"
    });

    return data;
}

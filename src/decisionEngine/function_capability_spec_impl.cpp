#include "functionRegistry.h"

namespace DecisionEngine {

CapabilitySpec::CapabilitySpec()
    : name(""), description(""), timeoutMs(2000), retryLimit(3), lastCalled(TimePoint::min()), enabled(true), type("core"), entry("")
{
}

CapabilitySpec::CapabilitySpec(const CapabilitySpec& other)
    : name(other.name), description(other.description), action(other.action), timeoutMs(other.timeoutMs),
      retryLimit(other.retryLimit), lastCalled(other.lastCalled), enabled(other.enabled), type(other.type), entry(other.entry), parameters(other.parameters)
{
}

CapabilitySpec& CapabilitySpec::operator=(const CapabilitySpec& other)
{
    if (this != &other) {
        name = other.name;
        description = other.description;
        action = other.action;
        timeoutMs = other.timeoutMs;
        retryLimit = other.retryLimit;
        lastCalled = other.lastCalled;
        enabled = other.enabled;
        type = other.type;
        entry = other.entry;
        parameters = other.parameters;
    }
    return *this;
}

CapabilitySpec::CapabilitySpec(CapabilitySpec&& other) noexcept
    : name(std::move(other.name)), description(std::move(other.description)), action(std::move(other.action)),
      timeoutMs(other.timeoutMs), retryLimit(other.retryLimit), lastCalled(std::move(other.lastCalled)),
      enabled(other.enabled), type(std::move(other.type)), entry(std::move(other.entry)), parameters(std::move(other.parameters))
{
}

CapabilitySpec& CapabilitySpec::operator=(CapabilitySpec&& other)
{
    if (this != &other) {
        name = std::move(other.name);
        description = std::move(other.description);
        action = std::move(other.action);
        timeoutMs = other.timeoutMs;
        retryLimit = other.retryLimit;
        lastCalled = std::move(other.lastCalled);
        enabled = other.enabled;
        type = std::move(other.type);
        entry = std::move(other.entry);
        parameters = std::move(other.parameters);
    }
    return *this;
}

CapabilitySpec::~CapabilitySpec() { }

nlohmann::json CapabilitySpec::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["timeoutMs"] = timeoutMs;
    j["type"] = type;
    j["entry"] = entry;
    j["parameters"] = nlohmann::json::array();
    for (const auto& param : parameters) {
        nlohmann::json paramJson;
        paramJson["name"] = param.name;
        paramJson["type"] = param.type;
        paramJson["description"] = param.description;
        paramJson["required"] = param.required;
        j["parameters"].push_back(paramJson);
    }
    return j;
}

} // namespace DecisionEngine


#include "functionRegistry.h"

namespace DecisionEngine {

// FunctionSpec implementations
FunctionSpec::FunctionSpec()
    : name(""), appName(""), version(""), description(""), humanReadableName(""),
      timeoutMs(4000), rateLimit(0), retryLimit(3), lastCalled(TimePoint::min()), enabled(true)
{
}

FunctionSpec::FunctionSpec(const FunctionSpec& other)
    : name(other.name), appName(other.appName), version(other.version),
      description(other.description), humanReadableName(other.humanReadableName),
      parameters(other.parameters), onComplete(other.onComplete), timeoutMs(other.timeoutMs),
      rateLimit(other.rateLimit), retryLimit(other.retryLimit), lastCalled(other.lastCalled),
      enabled(other.enabled)
{
}

FunctionSpec& FunctionSpec::operator=(const FunctionSpec& other)
{
    if (this != &other) {
        name = other.name;
        appName = other.appName;
        version = other.version;
        description = other.description;
        humanReadableName = other.humanReadableName;
        parameters = other.parameters;
        onComplete = other.onComplete;
        timeoutMs = other.timeoutMs;
        rateLimit = other.rateLimit;
        retryLimit = other.retryLimit;
        lastCalled = other.lastCalled;
        enabled = other.enabled;
    }
    return *this;
}

FunctionSpec::FunctionSpec(FunctionSpec&& other) noexcept
    : name(std::move(other.name)), appName(std::move(other.appName)), version(std::move(other.version)),
      description(std::move(other.description)), humanReadableName(std::move(other.humanReadableName)),
      parameters(std::move(other.parameters)), onComplete(std::move(other.onComplete)), timeoutMs(other.timeoutMs),
      rateLimit(other.rateLimit), retryLimit(other.retryLimit), lastCalled(std::move(other.lastCalled)),
      enabled(other.enabled)
{
}

FunctionSpec& FunctionSpec::operator=(FunctionSpec&& other)
{
    if (this != &other) {
        name = std::move(other.name);
        appName = std::move(other.appName);
        version = std::move(other.version);
        description = std::move(other.description);
        humanReadableName = std::move(other.humanReadableName);
        parameters = std::move(other.parameters);
        onComplete = std::move(other.onComplete);
        timeoutMs = other.timeoutMs;
        rateLimit = other.rateLimit;
        retryLimit = other.retryLimit;
        lastCalled = std::move(other.lastCalled);
        enabled = other.enabled;
    }
    return *this;
}

nlohmann::json FunctionSpec::toJson() const
{
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["parameters"] = nlohmann::json::array();
    for (const auto& param : parameters) {
        nlohmann::json paramJson;
        paramJson["name"] = param.name;
        paramJson["type"] = param.type;
        paramJson["description"] = param.description;
        paramJson["required"] = param.required;
        j["parameters"].push_back(paramJson);
    }
    j["timeoutMs"] = timeoutMs;
    return j;
}

// CapabilitySpec implementations
// CapabilitySpec implementations moved to function_capability_spec_impl.cpp

} // namespace DecisionEngine

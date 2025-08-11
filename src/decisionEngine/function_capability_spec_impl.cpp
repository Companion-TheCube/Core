/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗         ██████╗ █████╗ ██████╗  █████╗ ██████╗ ██╗██╗     ██╗████████╗██╗   ██╗     ███████╗██████╗ ███████╗ ██████╗        ██╗███╗   ███╗██████╗ ██╗         ██████╗██████╗ ██████╗ 
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║        ██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔══██╗██║██║     ██║╚══██╔══╝╚██╗ ██╔╝     ██╔════╝██╔══██╗██╔════╝██╔════╝        ██║████╗ ████║██╔══██╗██║        ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║        ██║     ███████║██████╔╝███████║██████╔╝██║██║     ██║   ██║    ╚████╔╝      ███████╗██████╔╝█████╗  ██║             ██║██╔████╔██║██████╔╝██║        ██║     ██████╔╝██████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║        ██║     ██╔══██║██╔═══╝ ██╔══██║██╔══██╗██║██║     ██║   ██║     ╚██╔╝       ╚════██║██╔═══╝ ██╔══╝  ██║             ██║██║╚██╔╝██║██╔═══╝ ██║        ██║     ██╔═══╝ ██╔═══╝ 
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████╗╚██████╗██║  ██║██║     ██║  ██║██████╔╝██║███████╗██║   ██║      ██║███████╗███████║██║     ███████╗╚██████╗███████╗██║██║ ╚═╝ ██║██║     ███████╗██╗╚██████╗██║     ██║     
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═════╝ ╚═╝╚══════╝╚═╝   ╚═╝      ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝ ╚═════╝╚══════╝╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


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

///////////////////////////////////////////////////////////////////////////////////////////

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


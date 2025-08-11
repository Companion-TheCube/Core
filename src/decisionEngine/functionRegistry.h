/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗██╗  ██╗
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██║  ██║
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ███████║
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██╔══██║
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗██║  ██║
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝╚═╝  ╚═╝
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

// Function Registry: structured catalogue of callable functions for tool-use
// by LLMs and internal components.
//
// Goals
// - Allow apps to register discoverable functions with typed parameters
// - Provide a JSON catalogue suitable for LLM tool schemas (OpenAI-style)
// - Offer fast lookup and execution routing by function name
#pragma once
#include <chrono>
#include <functional>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "./../apps/appsManager.h"
#ifndef API_H
#include "../api/api.h"
#endif
#include "../threadsafeQueue.h"

namespace DecisionEngine {
using TimePoint = std::chrono::system_clock::time_point;

// Parameter specification (name, type, description, required)
struct ParamSpec {
    std::string name;
    // type must be one of: "string", "number", "boolean", "json"
    std::string type;
    std::string description;
    bool required { true };
};

// Function specification and callback
// Functions are data sources within apps that can be called by LLMs or other components.
struct FunctionSpec {
    FunctionSpec();
    FunctionSpec(const FunctionSpec&);
    FunctionSpec& operator=(const FunctionSpec&);
    FunctionSpec(FunctionSpec&&) noexcept;
    FunctionSpec& operator=(FunctionSpec&&);

    ~FunctionSpec() = default;
    std::string name;
    std::string appName;
    std::string version;
    std::string description;
    std::string humanReadableName; // Optional, for better UX
    std::vector<ParamSpec> parameters;
    std::function<nlohmann::json(const nlohmann::json& args)> callback;
    uint32_t timeoutMs { 4000 };
    uint32_t rateLimit { 0 }; // 0 means no rate limit
    int retryLimit { 3 }; // Number of retries on failure
    TimePoint lastCalled { TimePoint::min() }; // For rate limiting
    std::mutex mutex; // For thread safety
    bool enabled { true }; // Whether the function is enabled by default
    // Serialize into OpenAI-style JSON for the LLM
    nlohmann::json toJson() const;
};

// Capability specification
// Capabilities are actions that can be performed by the CORE such as playing a sound, sending a notification, etc.
// They are not functions in the traditional sense, but rather actions that can be triggered by the
// decision engine based on user input or other triggers.
struct CapabilitySpec {
    std::string name; // Unique name for the capability
    std::string description; // Brief description of what the capability does
    std::function<void(const nlohmann::json& args)> action; // Action to perform, takes JSON args
    uint32_t timeoutMs { 2000 }; // Timeout for the action
    int retryLimit { 3 }; // Number of retries on failure
    TimePoint lastCalled { TimePoint::min() }; // For rate limiting
    std::mutex mutex; // For thread safety
    bool enabled { true }; // Whether the capability is enabled by default
    nlohmann::json toJson() const;
};

// Singleton registry: stores FunctionSpec by name and exposes a JSON catalogue
class FunctionRegistry: public AutoRegisterAPI<FunctionRegistry> {
public:
    FunctionRegistry();
    ~FunctionRegistry();

    bool registerFunc(const FunctionSpec& spec);
    const FunctionSpec* find(const std::string& name) const;
    std::vector<nlohmann::json> catalogueJson() const;

    constexpr std::string getInterfaceName() const override { return "FunctionRegistry"; }
    HttpEndPointData_t getHttpEndpointData() override;

private:
    std::unordered_map<std::string, FunctionSpec> funcs_;
    std::unordered_map<std::string, CapabilitySpec> capabilities_;
    mutable std::mutex mutex_;
};

// Function runner: executes a function by name with given parameters.
// This class is responsible for executing the function and handling any errors that may occur during execution.
// It will also handle retries and rate limiting.
// The function runner will also handle the emotional score ranges and matching params/phrases.
// It will check if the function is enabled based on the emotional score ranges and matching params/phrases.
// If the function is not enabled, it will return an error.
class FunctionRunner{
    // We'll need a thread pool to execute functions asynchronously
    
};

}

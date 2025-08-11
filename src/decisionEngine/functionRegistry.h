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
#include <future>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include "./../apps/appsManager.h"
#ifndef API_H
#include "../api/api.h"
#endif
#include "../threadsafeQueue.h"
#include "jsonrpccxx/client.hpp"
#include "asio.hpp"

namespace DecisionEngine {
using TimePoint = std::chrono::system_clock::time_point;

class FunctionRunner;

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
    // Function names must be unique across the registry.
    std::string name;
    std::string appName;
    std::string version;
    std::string description;
    std::string humanReadableName; // Optional, for better UX
    std::vector<ParamSpec> parameters;
    // Optional completion callback: invoked to deliver async results back to the originator.
    // This is used when the function cannot return data synchronously and needs to push
    // results asynchronously (e.g., streaming or callback-based RPC).
    std::function<void(const nlohmann::json& result)> onComplete;
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
    CapabilitySpec();
    CapabilitySpec(const CapabilitySpec&);
    CapabilitySpec& operator=(const CapabilitySpec&);
    CapabilitySpec(CapabilitySpec&&) noexcept;
    CapabilitySpec& operator=(CapabilitySpec&&);
    ~CapabilitySpec();
    std::string name; // Unique name for the capability
    std::string description; // Brief description of what the capability does
    std::function<void(const nlohmann::json& args)> action; // Action to perform, takes JSON args
    uint32_t timeoutMs { 2000 }; // Timeout for the action
    int retryLimit { 3 }; // Number of retries on failure
    TimePoint lastCalled { TimePoint::min() }; // For rate limiting
    std::mutex mutex; // For thread safety
    bool enabled { true }; // Whether the capability is enabled by default
    // Optional type: "core", "rpc", "plugin"
    std::string type { "core" };
    // Optional entry: for rpc this can be the app_id, for plugin the plugin path
    std::string entry;
    std::vector<ParamSpec> parameters;
    nlohmann::json toJson() const;
};


// Function runner: executes a function by name with given parameters.
// This class is responsible for executing the function and handling any errors that may occur during execution.
// It will also handle retries and rate limiting.
// If the function is not enabled, it will return an error.
class FunctionRunner {
    // We'll need a thread pool to execute functions asynchronously
public:
    // A generic task: the worker will call `work()` and then `onComplete(result)` if provided.
    struct Task {
        std::function<nlohmann::json()> work;
        std::function<void(const nlohmann::json&)> onComplete;
        uint32_t timeoutMs { 0 };
        std::string name; // optional, for logging
        // Retry and rate limit settings (populated by the enqueuer)
        int retryLimit { 0 };
        uint32_t rateLimitMs { 0 }; // minimum milliseconds between calls for this name; 0 = no limit
        int attempt { 0 };
    };

    FunctionRunner();
    ~FunctionRunner();

    // Start the worker pool with `numThreads` workers. Safe to call multiple
    // times; subsequent calls will be no-ops while running.
    void start(size_t numThreads = std::thread::hardware_concurrency());

    // Stop workers and join threads. Blocks until all workers exit.
    void stop();

    // Enqueue a task. Returns immediately.
    void enqueue(Task&& task);

    // Convenience helpers for function/capability calls:
    // - `work` should perform the actual call and return a JSON result.
    void enqueueFunctionCall(const std::string& functionName,
        std::function<nlohmann::json()> work,
        std::function<void(const nlohmann::json&)> onComplete = nullptr,
        uint32_t timeoutMs = 0);

    void enqueueCapabilityCall(const std::string& capabilityName,
        std::function<nlohmann::json()> work,
        std::function<void(const nlohmann::json&)> onComplete = nullptr,
        uint32_t timeoutMs = 0);

private:
    void workerLoop();

    ThreadSafeQueue<Task> queue_ { DEFAULT_QUEUE_SIZE };
    std::vector<std::thread> workers_;
    std::atomic<bool> running_ { false };
    std::mutex startStopMutex_;
    size_t numThreads_ { 0 };
    // Track last-called timestamps for rate limiting per function/capability name
    std::unordered_map<std::string, TimePoint> lastCalledMap_;
    std::mutex lastCalledMutex_;
};


// Stores FunctionSpec by name and exposes a JSON catalogue
class FunctionRegistry : public AutoRegisterAPI<FunctionRegistry> {
public:
    FunctionRegistry();
    ~FunctionRegistry();

    bool registerFunc(const FunctionSpec& spec);
    bool registerCapability(const CapabilitySpec& spec);
    const CapabilitySpec* findCapability(const std::string& name) const;
    // Load capability manifests from a set of directories. If `paths` is empty,
    // defaults to `data/capabilities` and `apps/*/capabilities`.
    void loadCapabilityManifests(const std::vector<std::string>& paths = {});
    const FunctionSpec* find(const std::string& name) const;
    std::vector<nlohmann::json> catalogueJson() const;

    // Asynchronous helpers: enqueue a function or capability call.
    // `onComplete` will be invoked on the worker thread after the call completes.
    void runFunctionAsync(const std::string& functionName,
                          const nlohmann::json& args,
                          std::function<void(const nlohmann::json&)> onComplete = nullptr);

    void runCapabilityAsync(const std::string& capabilityName,
                            const nlohmann::json& args,
                            std::function<void(const nlohmann::json&)> onComplete = nullptr);

    constexpr std::string getInterfaceName() const override { return "FunctionRegistry"; }
    HttpEndPointData_t getHttpEndpointData() override;

    // Public test helper: perform RPC using the spec. In production code this
    // is used indirectly via runFunctionAsync; exposing it here simplifies
    // unit testing of the RPC path.
    nlohmann::json performFunctionRpcPublic(const FunctionSpec& spec, const nlohmann::json& args) { return performFunctionRpc(spec, args); }

private:
    std::unordered_map<std::string, FunctionSpec> funcs_;
    std::unordered_map<std::string, CapabilitySpec> capabilities_;
    mutable std::mutex mutex_;
    // Worker pool for executing functions/capabilities
    FunctionRunner runner_;
    // Perform the function RPC. Implemented in the cpp file. This is where
    // JSON-RPC / asio integration should live; currently it's a stub that
    // returns an error JSON and logs a message.
    nlohmann::json performFunctionRpc(const FunctionSpec& spec, const nlohmann::json& args);
};

}

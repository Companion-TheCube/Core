/*
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗██████╗ ███████╗ ██████╗ ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗██████╗ ██████╗ 
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔══██╗██╔════╝██╔════╝ ██║██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║██████╔╝█████╗  ██║  ███╗██║███████╗   ██║   ██████╔╝ ╚████╔╝ ██║     ██████╔╝██████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║██╔══██╗██╔══╝  ██║   ██║██║╚════██║   ██║   ██╔══██╗  ╚██╔╝  ██║     ██╔═══╝ ██╔═══╝ 
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║██║  ██║███████╗╚██████╔╝██║███████║   ██║   ██║  ██║   ██║██╗╚██████╗██║     ██║     
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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


/*

The function registry provides a way to register and manage functions, which are data sources that are provided by apps.
Functions can be registered with a name, action, parameters, and other metadata.

It also provides HTTP endpoints for interacting with the function registry, allowing other applications to register, unregister, and query functions.
The decision engine can execute functions based on user input or other triggers, and it can also manage scheduled tasks that execute functions at 
specified times or intervals.

Functions are registered with the following information:
- Function name
- Action to execute
- Parameters for the action. These can be static or can be loaded from a data source.
- Brief description of the function
- Response string that can include placeholders for parameters
- Emotional score ranges for the function to be enabled or disabled based on the user's emotional state
- Matching params for Spacy
- Matching phrases for other matching methods

Functions are called via RPC. We'll use a simple JSON-RPC style interface for this. Each app will get a unix socket in the root of its directory 
that it should use for RPC. 

*/

#include "functionRegistry.h"
// Asio and json-rpc-cxx
#include <asio.hpp>
#include <jsonrpccxx/client.hpp>
#include <jsonrpccxx/iclientconnector.hpp>
#include <memory>
#include <atomic>
#include <filesystem>

namespace DecisionEngine {

namespace FunctionUtils {
    // Helper to convert a JSON object to a FunctionSpec
    FunctionSpec specFromJson(const nlohmann::json& j);
    CapabilitySpec capabilityFromJson(const nlohmann::json& j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FunctionRunner implementation (stubbed)
FunctionRunner::FunctionRunner()
{
}

FunctionRunner::~FunctionRunner()
{
    stop();
}

void FunctionRunner::start(size_t numThreads)
{
    std::lock_guard<std::mutex> guard(startStopMutex_);
    if (running_)
        return;
    if (numThreads == 0)
        numThreads = 1;
    numThreads_ = numThreads;
    running_ = true;
    workers_.reserve(numThreads_);
    for (size_t i = 0; i < numThreads_; ++i) {
        workers_.emplace_back([this]() { this->workerLoop(); });
    }
}

void FunctionRunner::stop()
{
    std::lock_guard<std::mutex> guard(startStopMutex_);
    if (!running_)
        return;
    running_ = false;
    // push empty tasks to wake workers
    for (size_t i = 0; i < workers_.size(); ++i) {
        Task t;
        t.work = []() { return nlohmann::json(); };
        queue_.push(t);
    }
    for (auto& th : workers_) {
        if (th.joinable())
            th.join();
    }
    workers_.clear();
}

void FunctionRunner::enqueue(Task&& task)
{
    queue_.push(std::move(task));
}

void FunctionRunner::enqueueFunctionCall(const std::string& functionName,
                                        std::function<nlohmann::json()> work,
                                        std::function<void(const nlohmann::json&)> onComplete,
                                        uint32_t timeoutMs)
{
    Task t;
    t.name = functionName;
    t.work = std::move(work);
    t.onComplete = std::move(onComplete);
    t.timeoutMs = timeoutMs;
    enqueue(std::move(t));
}

void FunctionRunner::enqueueCapabilityCall(const std::string& capabilityName,
                                          std::function<nlohmann::json()> work,
                                          std::function<void(const nlohmann::json&)> onComplete,
                                          uint32_t timeoutMs)
{
    Task t;
    t.name = capabilityName;
    t.work = std::move(work);
    t.onComplete = std::move(onComplete);
    t.timeoutMs = timeoutMs;
    enqueue(std::move(t));
}

void FunctionRunner::workerLoop()
{
    const uint32_t baseBackoffMs = 100;
    while (running_) {
        auto opt = queue_.pop();
        if (!opt.has_value())
            continue;
        Task task = std::move(opt.value());
        if (!running_)
            break;

        // Rate limiting: if rateLimitMs > 0, ensure enough time has passed since last call
        if (task.rateLimitMs > 0) {
            TimePoint last;
            {
                std::lock_guard<std::mutex> lock(lastCalledMutex_);
                auto it = lastCalledMap_.find(task.name);
                if (it != lastCalledMap_.end()) last = it->second;
            }
            if (last != TimePoint::min()) {
                auto now = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
                if (elapsed < (int)task.rateLimitMs) {
                    uint32_t waitMs = task.rateLimitMs - static_cast<uint32_t>(elapsed);
                    if (task.timeoutMs > 0 && waitMs >= task.timeoutMs) {
                        // Not enough time before timeout: reply with rate_limited error
                        if (task.onComplete) {
                            try { task.onComplete(nlohmann::json({{"error", "rate_limited"}})); } catch(...){}
                        }
                        continue;
                    }
                    // Requeue the task to the tail so other tasks can proceed.
                    // We avoid busy-waiting by sleeping a short amount here before
                    // continuing; the task has been requeued so another worker may
                    // pick it later.
                    queue_.push(task);
                    continue; // take next task
                }
            }
        }

        nlohmann::json lastError = nlohmann::json();
        bool success = false;
        int totalAttempts = std::max(1, task.retryLimit + 1);
        for (int attempt = 1; attempt <= totalAttempts && running_; ++attempt) {
            task.attempt = attempt;
            try {
                if (task.timeoutMs > 0) {
                    // Run the work asynchronously and wait for timeout
                    auto fut = std::async(std::launch::async, task.work);
                    if (fut.wait_for(std::chrono::milliseconds(task.timeoutMs)) == std::future_status::ready) {
                        nlohmann::json result = fut.get();
                        // Treat presence of "error" key as failure
                        if (result.is_object() && result.contains("error")) {
                            lastError = result;
                        } else {
                            // success
                            // record last-called
                            {
                                std::lock_guard<std::mutex> lock(lastCalledMutex_);
                                lastCalledMap_[task.name] = std::chrono::system_clock::now();
                            }
                            if (task.onComplete) {
                                try { task.onComplete(result); } catch(...){}
                            }
                            success = true;
                            break;
                        }
                    } else {
                        // timeout
                        lastError = nlohmann::json({{"error", "timeout"}, {"attempt", attempt}});
                    }
                } else {
                    // no timeout specified: execute directly
                    nlohmann::json result = task.work();
                    if (result.is_object() && result.contains("error")) {
                        lastError = result;
                    } else {
                        std::lock_guard<std::mutex> lock(lastCalledMutex_);
                        lastCalledMap_[task.name] = std::chrono::system_clock::now();
                        if (task.onComplete) {
                            try { task.onComplete(result); } catch(...){}
                        }
                        success = true;
                        break;
                    }
                }
            } catch (const std::exception& e) {
                lastError = nlohmann::json({{"error", std::string("exception: ") + e.what()}, {"attempt", attempt}});
            } catch (...) {
                lastError = nlohmann::json({{"error", "unknown_exception"}, {"attempt", attempt}});
            }

            // If we get here, we had an error. If not the last attempt, backoff then retry.
            if (attempt < totalAttempts) {
                uint32_t backoff = baseBackoffMs * (1u << (attempt - 1));
                // don't exceed timeoutMs if provided
                if (task.timeoutMs > 0 && backoff > task.timeoutMs) backoff = task.timeoutMs;
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            }
        }

        if (!success) {
            if (task.onComplete) {
                try { task.onComplete(lastError.is_null() ? nlohmann::json({{"error", "failed"}}) : lastError); } catch(...){}
            }
        }
        // If we reach here, the task has been processed (success or failure).
        // We should sleep a bit to avoid busy-waiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

CapabilitySpec::~CapabilitySpec()
{
    // Nothing to do
}

nlohmann::json CapabilitySpec::toJson() const
{
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

FunctionRegistry::FunctionRegistry()
{
    // Register the FunctionRegistry with the API builder
    this->registerInterface();
    // Start the function runner with default number of threads
    try {
        runner_.start();
    } catch (...) {
        CubeLog::error("Failed to start FunctionRunner");
    }
    // Load capability manifests from default locations
    try {
        this->loadCapabilityManifests();
    } catch (const std::exception& e) {
        CubeLog::error(std::string("Failed to load capability manifests: ") + e.what());
    }
}

FunctionRegistry::~FunctionRegistry()
{
    // Stop the runner to ensure clean shutdown
    try {
        runner_.stop();
    } catch (...) {
        CubeLog::error("Exception while stopping FunctionRunner");
    }
}

bool FunctionRegistry::registerCapability(const CapabilitySpec& spec)
{
    if (spec.name.empty()) {
        CubeLog::error("Capability name cannot be empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    capabilities_[spec.name] = spec;
    CubeLog::info("Registered capability: " + spec.name);
    return true;
}

const CapabilitySpec* FunctionRegistry::findCapability(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = capabilities_.find(name);
    if (it != capabilities_.end()) return &it->second;
    return nullptr;
}

void FunctionRegistry::loadCapabilityManifests(const std::vector<std::string>& paths)
{
    std::vector<std::string> searchPaths = paths;
    if (searchPaths.empty()) {
        searchPaths.push_back("data/capabilities");
        // apps/*/capabilities
        searchPaths.push_back("apps");
    }
    for (const auto& p : searchPaths) {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) {
            // If path is apps, we iterate subdirs
            if (p == "apps") {
                if (!std::filesystem::exists(p)) continue;
                for (auto& dir : std::filesystem::directory_iterator(p)) {
                    if (!dir.is_directory()) continue;
                    auto capDir = dir.path() / "capabilities";
                    if (std::filesystem::exists(capDir)) {
                        for (auto& f : std::filesystem::directory_iterator(capDir)) {
                            if (f.path().extension() == ".json") {
                                try {
                                    std::ifstream ifs(f.path());
                                    nlohmann::json j = nlohmann::json::parse(ifs);
                                    CapabilitySpec spec = FunctionUtils::capabilityFromJson(j);
                                    // If entry not set, default to dir name (app id)
                                    if (spec.name.empty()) continue;
                                    if (spec.action == nullptr && spec.type == "rpc" && spec.entry.empty()) {
                                        spec.entry = dir.path().filename().string();
                                    }
                                    registerCapability(spec);
                                } catch (const std::exception& e) {
                                    CubeLog::error(std::string("Failed to load capability manifest: ") + e.what());
                                }
                            }
                        }
                    }
                }
            }
            continue;
        }
        // If p is data/capabilities or other directory
        if (std::filesystem::is_directory(p)) {
            for (auto& f : std::filesystem::directory_iterator(p)) {
                if (f.path().extension() == ".json") {
                    try {
                        std::ifstream ifs(f.path());
                        nlohmann::json j = nlohmann::json::parse(ifs);
                        CapabilitySpec spec = FunctionUtils::capabilityFromJson(j);
                        registerCapability(spec);
                    } catch (const std::exception& e) {
                        CubeLog::error(std::string("Failed to load capability manifest: ") + e.what());
                    }
                }
            }
        }
    }
    // TODO: add all the built in capabilities here. each one should have a json manifest in data/capabilities
    // and the action should be set to a lambda that implements the capability.
    // Register a small built-in capability as example
    CapabilitySpec ping;
    ping.name = "core.ping";
    ping.description = "Simple ping capability";
    ping.action = [](const nlohmann::json& args){ CubeLog::info("core.ping invoked"); };
    ping.enabled = true;
    registerCapability(ping);
}

bool FunctionRegistry::registerFunc(const FunctionSpec& spec)
{
    // first we verify that the function name is valid. Must start with a version indicator like "v1." and then be a valid identifier.
    // Valid identifiers can only contain alphanumeric characters and underscores, and cannot start with a number.
    // The next part of the name must be the app name, which must be a valid identifier.
    // The last part of the name must be the function name, which must also be a valid identifier.
    // Example: "v1.my_app.my_function" is valid, but "v1.1myapp.my_function" is not valid.
    // The version must be a valid semantic version, e.g. "v1_0_0", "v2_1_3", etc. Version indicates the minimum version of the CORE
    // that the app requires.
    // valid examples: "v1_0_0.my_app.my_function", "v2_1_3.my_app.my_function"
    if(spec.name.empty()){
        CubeLog::error("Function name cannot be empty");
        return false;
    }
    if (!std::regex_match(spec.name, std::regex("^v[0-9_]+\\.[a-zA-Z_][a-zA-Z0-9_]\\.[a-zA-Z_][a-zA-Z0-9_]*$"))) {
        // first we check that the version is valid
        if(!std::regex_match(spec.name, std::regex("^v[0-9_]+\\.[a-zA-Z_\\.0-9]*$"))) {
            CubeLog::error("Function name must start with a valid version indicator like 'v1.0.0.my_app.my_function'");
            return false;
        }
        // then we check that the app name is valid
        // get the app name from the function name
        std::string appName = spec.name.substr(spec.name.find('.') + 1, spec.name.find('.', spec.name.find('.') + 1) - spec.name.find('.') - 1);
        // then check that the app name is a valid identifier
        if (!std::regex_match(appName, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            CubeLog::error("App name '" + appName + "' in function name '" + spec.name + "' is not a valid identifier. It must start with a letter or underscore and can only contain alphanumeric characters and underscores.");
            return false;
        }
        // then check that the app name is in the DB
        auto appNames = AppsManager::getAppNames_static();
        if (std::find(appNames.begin(), appNames.end(), appName) == appNames.end()) {
            CubeLog::error("App '" + appName + "' is not registered. Please register the app before registering functions.");
            return false;
        }
        // then we check that the function name is valid
        std::string functionName = spec.name.substr(spec.name.find_last_of('.') + 1);
        if (!std::regex_match(functionName, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            CubeLog::error("Function name '" + functionName + "' in function name '" + spec.name + "' is not a valid identifier. It must start with a letter or underscore and can only contain alphanumeric characters and underscores.");
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    funcs_[spec.name] = spec;
    CubeLog::info("Registered function: " + spec.name);
    return true;
}

void FunctionRegistry::runFunctionAsync(const std::string& functionName,
                                        const nlohmann::json& args,
                                        std::function<void(const nlohmann::json&)> onComplete)
{
    FunctionSpec spec;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = funcs_.find(functionName);
        if (it == funcs_.end()) {
            if (onComplete) {
                onComplete(nlohmann::json({{"error", "function_not_found"}}));
            }
            return;
        }
        spec = it->second; // copy
    }

    if (!spec.enabled) {
        if (onComplete) onComplete(nlohmann::json({{"error", "function_disabled"}}));
        return;
    }

    // Prepare a task. The actual function invocation (e.g., JSON-RPC) is not
    // implemented here; the `work` callable should perform the RPC and return
    // a JSON result. For now we use a placeholder that returns an error
    // indicating the function is unimplemented. After the work completes we
    // call both the function's registered `onComplete` (if any) and the
    // caller-provided `onComplete`.
    FunctionRunner::Task t;
    t.name = functionName;
    t.timeoutMs = spec.timeoutMs;
    t.retryLimit = spec.retryLimit;
    t.rateLimitMs = spec.rateLimit; // interpret spec.rateLimit as minimum ms between calls
    t.attempt = 0;
    // Build work callable that performs the RPC via FunctionRegistry::performFunctionRpc
    t.work = [this, spec, args]() -> nlohmann::json {
        try {
            return this->performFunctionRpc(spec, args);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("performFunctionRpc exception: ") + e.what());
            return nlohmann::json({{"error", std::string("exception: ") + e.what()}});
        } catch (...) {
            CubeLog::error("performFunctionRpc unknown exception");
            return nlohmann::json({{"error", "unknown_exception"}});
        }
    };
    // chain the spec's onComplete and the caller's onComplete
    auto specOnComplete = spec.onComplete;
    t.onComplete = [specOnComplete, onComplete](const nlohmann::json& result) {
        try {
            if (specOnComplete) specOnComplete(result);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("spec onComplete exception: ") + e.what());
        } catch (...) {
            CubeLog::error("spec onComplete unknown exception");
        }
        try {
            if (onComplete) onComplete(result);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("caller onComplete exception: ") + e.what());
        } catch (...) {
            CubeLog::error("caller onComplete unknown exception");
        }
    };
    runner_.enqueue(std::move(t));
}

void FunctionRegistry::runCapabilityAsync(const std::string& capabilityName,
                                          const nlohmann::json& args,
                                          std::function<void(const nlohmann::json&)> onComplete)
{
    CapabilitySpec cap;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = capabilities_.find(capabilityName);
        if (it == capabilities_.end()) {
            if (onComplete) {
                onComplete(nlohmann::json({{"error", "capability_not_found"}}));
            }
            return;
        }
        cap = it->second; // copy
    }

    if (!cap.enabled) {
        if (onComplete) onComplete(nlohmann::json({{"error", "capability_disabled"}}));
        return;
    }

    auto action = cap.action;
    FunctionRunner::Task t;
    t.name = capabilityName;
    t.timeoutMs = cap.timeoutMs;
    t.retryLimit = cap.retryLimit;
    t.rateLimitMs = 0;
    t.work = [action, args]() -> nlohmann::json {
        try {
            if (action) {
                action(args);
                return nlohmann::json({{"status", "ok"}});
            }
            return nlohmann::json({{"error", "no_action"}});
        } catch (const std::exception& e) {
            return nlohmann::json({{"error", std::string("action_exception: ") + e.what()}});
        } catch (...) {
            return nlohmann::json({{"error", "action_unknown_exception"}});
        }
    };
    t.onComplete = onComplete;
    runner_.enqueue(std::move(t));
}

const FunctionSpec* FunctionRegistry::find(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<nlohmann::json> FunctionRegistry::catalogueJson() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<nlohmann::json> catalogue;
    for (const auto& [name, spec] : funcs_) {
        catalogue.push_back(spec.toJson());
    }
    return catalogue;
}

// Stub for performing the RPC call to the app that implements the function.
// TODO: implement actual JSON-RPC via asio here. For now, return an error
// JSON and log so callers receive a predictable response.
nlohmann::json FunctionRegistry::performFunctionRpc(const FunctionSpec& spec, const nlohmann::json& args)
{
    CubeLog::info("performFunctionRpc called for: " + spec.name + " with args: " + args.dump());

    // Lookup socket location in apps DB. Use app_name first, then app_id.
    std::string socketPath;
    try {
        if (CubeDB::getDBManager() == nullptr) {
            CubeLog::error("CubeDB manager not available");
            return nlohmann::json({{"error", "db_not_ready"}});
        }
        Database* db = CubeDB::getDBManager()->getDatabase("apps");
        if (!db) {
            CubeLog::error("Apps database not available");
            return nlohmann::json({{"error", "db_not_available"}});
        }
        if (db->columnExists("apps", "socket_location")) {
            // Try app_name
            auto rows = db->selectData("apps", {"socket_location"}, "app_name = '" + spec.appName + "'");
            if (rows.size() > 0 && rows[0].size() > 0 && !rows[0][0].empty()) {
                socketPath = rows[0][0];
            } else {
                // Try app_id
                rows = db->selectData("apps", {"socket_location"}, "app_id = '" + spec.appName + "'");
                if (rows.size() > 0 && rows[0].size() > 0 && !rows[0][0].empty()) {
                    socketPath = rows[0][0];
                }
            }
        }
    } catch (const std::exception& e) {
        CubeLog::error(std::string("DB lookup failed: ") + e.what());
        return nlohmann::json({{"error", "db_lookup_failed"}});
    }

    if (socketPath.empty()) {
        CubeLog::error("No socket_location found for app: " + spec.appName);
        return nlohmann::json({{"error", "socket_not_found"}});
    }

    // Ensure RPC io_context and threads are running
    static std::shared_ptr<asio::io_context> rpc_io;
    static std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> rpc_guard;
    static std::vector<std::thread> rpc_threads;
    static std::mutex rpc_init_mutex;
    {
        std::lock_guard<std::mutex> guard(rpc_init_mutex);
        if (!rpc_io) {
            rpc_io = std::make_shared<asio::io_context>();
            rpc_guard = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(rpc_io->get_executor());
            // start a small thread pool for RPC IO
            unsigned int n = std::max(1u, std::min(4u, std::thread::hardware_concurrency() / 2));
            for (unsigned int i = 0; i < n; ++i) {
                rpc_threads.emplace_back([](){
                    try { rpc_io->run(); } catch(...) {}
                });
            }
        }
    }

    // Build JSON-RPC request via jsonrpccxx client, executed on rpc_io.
    std::shared_ptr<std::promise<std::string>> prom = std::make_shared<std::promise<std::string>>();
    std::future<std::string> fut = prom->get_future();
    auto done = std::make_shared<std::atomic<bool>>(false);

    // Connector implementation using httplib
    struct HttplibConnector : public jsonrpccxx::IClientConnector {
        std::string socketPath;
        HttplibConnector(const std::string& p) : socketPath(p) {}
        std::string Send(const std::string &request) override {
            // Use cpp-httplib to POST request to the unix socket
            try {
                httplib::Client cli(socketPath.c_str(), 0);
                cli.set_address_family(AF_UNIX);
                cli.set_read_timeout(5, 0);
                auto res = cli.Post("/", request, "application/json");
                if (res) {
                    return res->body;
                } else {
                    throw std::runtime_error("HTTP request failed");
                }
            } catch (const std::exception& e) {
                throw;
            }
        }
    };

    // Parse method name: use last token after '.' if present
    std::string method = spec.name;
    auto pos = spec.name.find_last_of('.');
    if (pos != std::string::npos) method = spec.name.substr(pos + 1);

    // Prepare params: if args is object, use named params; otherwise use positional param.
    jsonrpccxx::named_parameter namedParams;
    jsonrpccxx::positional_parameter posParams;
    bool useNamed = args.is_object();
    if (useNamed) {
        for (auto it = args.begin(); it != args.end(); ++it) {
            namedParams[it.key()] = it.value();
        }
    } else if (!args.is_null()) {
        posParams.push_back(args);
    }

    // Post work to rpc_io
    asio::post(*rpc_io, [prom, done, socketPath, method, useNamed, namedParams, posParams, &spec]() mutable {
        try {
            HttplibConnector connector(socketPath);
            jsonrpccxx::JsonRpcClient client(connector, jsonrpccxx::version::v2);
            nlohmann::json result;
            try {
                if (useNamed) {
                    result = client.CallMethodNamed<nlohmann::json>(1, method, namedParams);
                } else {
                    result = client.CallMethod<nlohmann::json>(1, method, posParams);
                }
                if (!done->exchange(true)) {
                    prom->set_value(result.dump());
                }
            } catch (const std::exception& e) {
                if (!done->exchange(true)) {
                    nlohmann::json err = nlohmann::json::object();
                    err["error"] = std::string("rpc_exception: ") + e.what();
                    prom->set_value(err.dump());
                }
            }
        } catch (const std::exception& e) {
            if (!done->exchange(true)) {
                nlohmann::json err = nlohmann::json::object();
                err["error"] = std::string("connector_exception: ") + e.what();
                prom->set_value(err.dump());
            }
        }
    });

    // Setup an asio timer for timeout handling on rpc_io
    uint32_t timeoutMs = spec.timeoutMs > 0 ? spec.timeoutMs : 4000u;
    asio::steady_timer timer(*rpc_io, std::chrono::milliseconds(timeoutMs));
    timer.async_wait([prom, done](const asio::error_code& ec) {
        if (ec) return; // cancelled
        if (!done->exchange(true)) {
            try {
                nlohmann::json err = nlohmann::json::object();
                err["error"] = "timeout";
                prom->set_value(err.dump());
            } catch (...) {}
        }
    });

    // Wait for result (the timer and RPC execution run on rpc_io threads)
    std::string raw;
    try {
        raw = fut.get();
    } catch (const std::exception& e) {
        CubeLog::error(std::string("performFunctionRpc future exception: ") + e.what());
        return nlohmann::json({{"error", "future_exception"}});
    }

    // Cancel the timer if still pending
    asio::post(*rpc_io, [&timer]() {
        asio::error_code ec;
        timer.cancel();
    });

    try {
        return nlohmann::json::parse(raw);
    } catch (...) {
        nlohmann::json j;
        j["result"] = raw;
        return j;
    }
}

HttpEndPointData_t FunctionRegistry::getHttpEndpointData()
{
    HttpEndPointData_t endpoints;
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Register a function
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error& e) {
                res.status = 400;
                res.set_content("Invalid JSON: " + std::string(e.what()), "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Invalid JSON format");
            }

            FunctionSpec spec = FunctionUtils::specFromJson(j);
            if (registerFunc(spec)) {
                res.status = 200;
                res.set_content("Function registered successfully", "text/plain");
            } else {
                res.status = 400;
                res.set_content("Failed to register function", "text/plain");
            }
            res.set_header("Content-Type", "application/json");
            res.body = j.dump();
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Register a function", {}, "Register a new function with the function registry" });
    endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Get function catalogue
            nlohmann::json catalogue = catalogueJson();
            res.status = 200;
            res.set_content(catalogue.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Get function catalogue", {}, "Get the list of registered functions" });
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Find a function by name
            std::string functionName = req.get_param_value("name");
            if (functionName.empty()) {
                res.status = 400;
                res.set_content("Function name is required", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Function name is required");
            }
            const FunctionSpec* func = find(functionName);
            if (func) {
                nlohmann::json j = func->toJson();
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Find a function by name", {}, "Find a registered function by its name" });
    endpoints.push_back({ PRIVATE_ENDPOINT | POST_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Unregister a function
            std::string functionName = req.get_param_value("name");
            if (functionName.empty()) {
                res.status = 400;
                res.set_content("Function name is required", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Function name is required");
            }
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = funcs_.find(functionName);
            if (it != funcs_.end()) {
                funcs_.erase(it);
                res.status = 200;
                res.set_content("Function unregistered successfully", "text/plain");
            } else {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Unregister a function", {}, "Unregister a function from the function registry" });
    endpoints.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res){
            // Get the list of all registered functions
            nlohmann::json catalogue = catalogueJson();
            res.status = 200;
            res.set_content(catalogue.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR,"");
        },
        "Get all registered functions", {}, "Get the list of all registered functions" });
    return endpoints;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace FunctionUtils {
    // Helper to convert a JSON object to a FunctionSpec
    FunctionSpec specFromJson(const nlohmann::json& j)
    {
        FunctionSpec spec;
        spec.name = j.at("name").get<std::string>();
        spec.description = j.value("description", "");
        spec.timeoutMs = j.value("timeout_ms", 4000);
    for (const auto& param : j.at("parameters")) {
        ParamSpec p;
        p.name = param.at("name").get<std::string>();
        p.type = param.at("type").get<std::string>();
        p.description = param.value("description", "");
        p.required = param.value("required", true);
        spec.parameters.push_back(p);
    }
        // If a callback field is provided in JSON, we still cannot synthesize an
        // onComplete function here — onComplete is intended to be set by the
        // registering code to receive async results. Provide a default that logs.
        spec.onComplete = [j](const nlohmann::json& result) {
            CubeLog::error("Function onComplete not implemented for: " + j.at("name").get<std::string>());
            CubeLog::debug("Result: " + result.dump());
        };
        spec.appName = j.value("app_name", "");
        spec.version = j.value("version", "1.0");
        spec.humanReadableName = j.value("human_readable_name", "");
        spec.rateLimit = j.value("rate_limit", 0);
        spec.retryLimit = j.value("retry_limit", 3);
        spec.lastCalled = TimePoint::min();
    spec.enabled = j.value("enabled", true);
    return spec;
}
    CapabilitySpec capabilityFromJson(const nlohmann::json& j)
    {
        CapabilitySpec spec;
        spec.name = j.value("name", "");
        spec.description = j.value("description", "");
        spec.timeoutMs = j.value("timeout_ms", 2000);
        spec.retryLimit = j.value("retry_limit", 0);
        spec.enabled = j.value("enabled", true);
        spec.type = j.value("type", "core");
        spec.entry = j.value("entry", "");
        if (j.contains("parameters")) {
            for (const auto& param : j.at("parameters")) {
                ParamSpec p;
                p.name = param.at("name").get<std::string>();
                p.type = param.at("type").get<std::string>();
                p.description = param.value("description", "");
                p.required = param.value("required", true);
                spec.parameters.push_back(p);
            }
        }
        // action to be bound by registry depending on type
        spec.action = nullptr;
        return spec;
    }
}

}

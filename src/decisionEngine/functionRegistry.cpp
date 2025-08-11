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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

FunctionRegistry::FunctionRegistry()
{
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
                                // Bind known core capabilities to CORE implementations
                                if (spec.type == "core" && spec.name == "core.play_sound") {
                                    spec.action = [spec](const nlohmann::json& args) {
                                        try {
                                            std::string file = args.value("file", "");
                                            double volume = args.value("volume", 1.0);
                                            CubeLog::info("core.play_sound invoked. file=" + file + ", volume=" + std::to_string(volume));
                                            // Basic behavior: enable audio output. Real file playback
                                            // is TODO in AudioOutput. For now, ensure audio is started
                                            // and unmute.
                                            // AudioOutput::setSound(true);
                                            // AudioOutput::start();
                                            // TODO: actually load and play `file` at specified `volume`.
                                        } catch (const std::exception& e) {
                                            CubeLog::error(std::string("core.play_sound action exception: ") + e.what());
                                        }
                                    };
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
                        // Bind known core capabilities
                        if (spec.type == "core" && spec.name == "core.play_sound") {
                            spec.action = [spec](const nlohmann::json& args) {
                                try {
                                    std::string file = args.value("file", "");
                                    double volume = args.value("volume", 1.0);
                                    CubeLog::info("core.play_sound invoked. file=" + file + ", volume=" + std::to_string(volume));
                                    // AudioOutput::setSound(true);
                                    // AudioOutput::start();
                                    // TODO: actually load and play `file` at specified `volume`.
                                } catch (const std::exception& e) {
                                    CubeLog::error(std::string("core.play_sound action exception: ") + e.what());
                                }
                            };
                        }
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
        // If the DB did not contain a socket_location, allow spec.appName to
        // be a direct socket path (useful for testing and quick overrides).
        if (!spec.appName.empty() && (spec.appName.find('/') != std::string::npos || spec.appName.rfind('.', 0) == 0 || spec.appName.rfind("./",0)==0)) {
            socketPath = spec.appName;
        }
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

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

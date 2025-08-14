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
Overview:

This module implements a lightweight function & capability registry used by the
decision engine. It provides:

- Storage and lookup for CapabilitySpec and FunctionSpec objects (thread-safe
  via an internal mutex).
- Loading capability manifests from disk (default: `data/capabilities` and
  per-app `apps/ * /capabilities`) and registering them with optional built-in
  actions.
- A small set of HTTP endpoints to register, unregister, list, and query
  functions.
- Facilities to execute registered functions or capabilities asynchronously
  via a FunctionRunner task queue (`runFunctionAsync`, `runCapabilityAsync`).

Function execution / RPC:

Functions backed by external apps are invoked over a JSON-RPC style interface
using a unix socket. `performFunctionRpc` resolves the socket location by
looking up `socket_location` in the `apps` database (falling back to treating
`spec.appName` as a direct socket path if it looks like one). RPC work is
posted into a shared `asio::io_context` with a small thread pool; a
promise/future pair and an asio timer implement timeouts. The JSON-RPC client
uses a small connector that posts JSON over a unix-domain HTTP socket.

Safety and behavior notes:

- Capability and function registration methods validate input; `registerFunc`
  performs structured validation (version token, app identifier, function
  identifier) and returns early on invalid input.
- Tasks in the FunctionRunner carry timeout, retry and rate-limit metadata and
  chain the spec's `onComplete` with the caller `onComplete` safely (each is
  invoked inside try/catch to avoid throwing on user callbacks).
- Built-in example capabilities are registered (e.g. `core.ping` and a stub
  `core.play_sound`) but playback is left as a TODO.

Helpers:

Several small helpers are used to keep the implementation linear and clear:
`processCapabilityFile`, `lookupSocketPathFromDB`, and
`ensureRpcIoInitialized` encapsulate file parsing, DB lookup, and RPC IO
initialization respectively.

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

namespace {
    // RPC IO objects for performFunctionRpc
    std::shared_ptr<asio::io_context> rpc_io;
    std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> rpc_guard;
    std::vector<std::thread> rpc_threads;
    std::mutex rpc_init_mutex;

    // Ensure the RPC io_context and threads are initialized.
    void ensureRpcIoInitialized() {
        std::lock_guard<std::mutex> guard(rpc_init_mutex);
        if (!rpc_io) {
            rpc_io = std::make_shared<asio::io_context>();
            rpc_guard = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(rpc_io->get_executor());
            unsigned int n = std::max(1u, std::min(4u, std::thread::hardware_concurrency() / 2));
            for (unsigned int i = 0; i < n; ++i) {
                rpc_threads.emplace_back([](){
                    try { rpc_io->run(); } catch(...) {}
                });
            }
        }
    }

    // Perform a JSON-RPC request over a unix-domain HTTP socket. This helper
    // posts the work to the shared `rpc_io` and returns the parsed JSON
    // response (or an error object on failure).
    nlohmann::json sendJsonRpcRequest(const std::string& socketPath,
                                      const std::string& method,
                                      const nlohmann::json& args,
                                      uint32_t timeoutMs)
    {
        ensureRpcIoInitialized();

        std::shared_ptr<std::promise<std::string>> prom = std::make_shared<std::promise<std::string>>();
        std::future<std::string> fut = prom->get_future();
        auto done = std::make_shared<std::atomic<bool>>(false);

        struct HttplibConnector : public jsonrpccxx::IClientConnector {
            std::string socketPath;
            HttplibConnector(const std::string& p) : socketPath(p) {}
            std::string Send(const std::string &request) override {
                try {
                    httplib::Client cli(socketPath.c_str(), 0);
                    cli.set_address_family(AF_UNIX);
                    cli.set_read_timeout(5, 0);
                    auto res = cli.Post("/", request, "application/json");
                    if (!res) throw std::runtime_error("HTTP request failed");
                    return res->body;
                } catch (...) {
                    throw;
                }
            }
        };

        // Prepare parameters
        bool useNamed = args.is_object();
        jsonrpccxx::named_parameter namedParams;
        jsonrpccxx::positional_parameter posParams;
        if (useNamed) {
            for (auto it = args.begin(); it != args.end(); ++it) namedParams[it.key()] = it.value();
        } else if (!args.is_null()) {
            // if args is an array, forward elements; otherwise use single positional param
            if (args.is_array()) {
                for (auto &el : args) posParams.push_back(el);
            } else {
                posParams.push_back(args);
            }
        }

        // Post RPC work
        asio::post(*rpc_io, [prom, done, socketPath, method, useNamed, namedParams, posParams]() mutable {
            try {
                HttplibConnector connector(socketPath);
                jsonrpccxx::JsonRpcClient client(connector, jsonrpccxx::version::v2);
                nlohmann::json result;
                try {
                    if (!useNamed) result = client.CallMethod<nlohmann::json>(1, method, posParams);
                    else result = client.CallMethodNamed<nlohmann::json>(1, method, namedParams);
                    if (!done->exchange(true)) prom->set_value(result.dump());
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

        // Timer
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

        std::string raw;
        try {
            raw = fut.get();
        } catch (const std::exception& e) {
            CubeLog::error(std::string("sendJsonRpcRequest future exception: ") + e.what());
            return nlohmann::json({{"error", "future_exception"}});
        }

        // cancel timer
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

    // Lookup socket path for an app using the apps DB, falling back to treating
    // spec.appName as a direct socket path if it looks like one.
    std::string lookupSocketPathFromDB(const FunctionSpec& spec) {
        std::string socketPath;
        try {
            if (CubeDB::getDBManager() == nullptr) {
                CubeLog::error("CubeDB manager not available");
                return {};
            }
            Database* db = CubeDB::getDBManager()->getDatabase("apps");
            if (!db) {
                CubeLog::error("Apps database not available");
                return {};
            }
            if (!db->columnExists("apps", "socket_location")) {
                CubeLog::error("Apps database does not have socket_location column");
            } else {
                auto rows = db->selectData("apps", {"socket_location"}, "app_name = '" + spec.appName + "'");
                if (rows.size() > 0 && rows[0].size() > 0 && !rows[0][0].empty()) {
                    socketPath = rows[0][0];
                }
                if (socketPath.empty()) {
                    rows = db->selectData("apps", {"socket_location"}, "app_id = '" + spec.appName + "'");
                    if (rows.size() > 0 && rows[0].size() > 0 && !rows[0][0].empty()) {
                        socketPath = rows[0][0];
                    }
                }
            }
        } catch (const std::exception& e) {
            CubeLog::error(std::string("DB lookup failed: ") + e.what());
            return {};
        }

        if (socketPath.empty()) {
            // If no DB entry, only accept spec.appName when it looks like a path
            if (spec.appName.empty()) return socketPath;
            bool looksLikePath = (spec.appName.find('/') != std::string::npos || spec.appName.rfind('.', 0) == 0 || spec.appName.rfind("./",0)==0);
            if (!looksLikePath) return socketPath;
            // TODO: Treating `spec.appName` as a direct socket path is a
            // convenience for testing and overrides. Confirm this behavior is
            // acceptable for production and consider validating/normalizing
            // the path (and permissions) before use.
            socketPath = spec.appName;
        }
        return socketPath;
    }

    // Process a capability JSON file and register it on the given registry.
    void processCapabilityFile(FunctionRegistry* registry, const std::filesystem::path& filePath, const std::filesystem::path& appDir = {}) {
        try {
            std::ifstream ifs(filePath);
            nlohmann::json j = nlohmann::json::parse(ifs);
            CapabilitySpec spec = FunctionUtils::capabilityFromJson(j);
            if (spec.name.empty()) return;
            if (spec.action == nullptr && spec.type == "rpc" && spec.entry.empty() && !appDir.empty()) {
                // TODO: Defaulting an RPC capability's `entry` to the app
                // directory name assumes the directory name == app identifier
                // stored elsewhere (DB, manifests). Confirm this mapping is
                // valid in all deployment scenarios, or prefer an explicit
                // `entry` field in the manifest.
                spec.entry = appDir.filename().string();
            }
            // If this is a core capability, perform per-name branching using
            // simple if/else checks. This keeps adding new core capability
            // handlers localized to this function without requiring an enum or
            // other cross-file changes.
            if (spec.type == "core") {
                if (spec.name == "core.play_sound") {
                    spec.action = [spec](const nlohmann::json& args) {
                        try {
                            std::string file = args.value("file", "");
                            double volume = args.value("volume", 1.0);
                            CubeLog::info("core.play_sound invoked. file=" + file + ", volume=" + std::to_string(volume));
                            // TODO: Hook into AudioOutput to actually play `file`.
                        } catch (const std::exception& e) {
                            CubeLog::error(std::string("core.play_sound action exception: ") + e.what());
                        }
                    };
                } else if (spec.name == "core.nfc.read") {
                    spec.action = [](const nlohmann::json& args) {
                        CubeLog::info("core.nfc.read invoked");
                        // TODO: implement NFC read behavior
                    };
                } else if (spec.name == "core.nfc.write") {
                    spec.action = [](const nlohmann::json& args) {
                        CubeLog::info("core.nfc.write invoked");
                        // TODO: implement NFC write behavior
                    };
                } else if (spec.name == "core.ui.draw") {
                    spec.action = [](const nlohmann::json& args) {
                        CubeLog::info("core.ui.draw invoked");
                        // TODO: implement UI draw behavior (compose primitives)
                    };
                } else if (spec.name == "core.audio.record") {
                    spec.action = [](const nlohmann::json& args) {
                        CubeLog::info("core.audio.record invoked");
                        // TODO: implement audio recording
                    };
                } else {
                    // Unknown core capability: leave action as-is (manifest may
                    // provide an RPC-based implementation) and log for
                    // visibility.
                    CubeLog::info("Unrecognized core capability: " + spec.name);
                }
            }

            // If this capability is RPC-backed, synthesize an action that will
            // call the implementing app over JSON-RPC using the shared helper.
            // The capability's `entry` is expected to be the app id or a direct
            // socket path (the latter is supported for testing/overrides).
            if (spec.type == "rpc") {
                // If action already provided by manifest, do not overwrite.
                if (!spec.action) {
                    // capture by value so the spec copy survives registration
                    spec.action = [spec](const nlohmann::json& args) {
                        try {
                            // Resolve socket path: try DB lookup using entry as appName
                            FunctionSpec tmp;
                            tmp.appName = spec.entry;
                            std::string socket = lookupSocketPathFromDB(tmp);
                            if (socket.empty()) {
                                // Allow entry to be a direct socket path
                                if (!spec.entry.empty() && (spec.entry.find('/') != std::string::npos || spec.entry.rfind('.', 0) == 0 || spec.entry.rfind("./",0)==0)) {
                                    socket = spec.entry;
                                }
                            }
                            if (socket.empty()) {
                                CubeLog::error("RPC capability '" + spec.name + "' has no socket_location for entry='" + spec.entry + "'");
                                return;
                            }
                            // method is last token after '.'
                            std::string method = spec.name;
                            auto p = spec.name.find_last_of('.');
                            if (p != std::string::npos) method = spec.name.substr(p + 1);
                            uint32_t timeout = spec.timeoutMs > 0 ? spec.timeoutMs : 2000u;
                            auto res = sendJsonRpcRequest(socket, method, args, timeout);
                            if (res.is_object() && res.contains("error")) {
                                CubeLog::error(std::string("RPC capability call failed: ") + res.dump());
                            }
                        } catch (const std::exception& e) {
                            CubeLog::error(std::string("RPC capability action exception: ") + e.what());
                        } catch (...) {
                            CubeLog::error("RPC capability action unknown exception");
                        }
                    };
                }
            }
            registry->registerCapability(spec);
        } catch (const std::exception& e) {
            CubeLog::error(std::string("Failed to load capability manifest: ") + e.what());
        }
    }
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
        // Special-case apps directory: iterate each app and load its capabilities
        if (p == "apps") {
            if (!std::filesystem::exists(p, ec)) continue;
            for (auto& dir : std::filesystem::directory_iterator(p)) {
                if (!dir.is_directory()) continue;
                auto capDir = dir.path() / "capabilities";
                if (!std::filesystem::exists(capDir)) continue;
                for (auto& f : std::filesystem::directory_iterator(capDir)) {
                    if (f.path().extension() != ".json") continue;
                    processCapabilityFile(this, f.path(), dir.path());
                }
            }
            continue;
        }

        // For other directories, skip early if missing or not a directory
        if (!std::filesystem::exists(p, ec)) continue;
        if (!std::filesystem::is_directory(p)) continue;
        for (auto& f : std::filesystem::directory_iterator(p)) {
            if (f.path().extension() != ".json") continue;
            processCapabilityFile(this, f.path());
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
    if (spec.name.empty()) {
        CubeLog::error("Function name cannot be empty");
        return false;
    }

    // If the full canonical form matches, accept it. Otherwise validate parts.
    const std::regex fullRegex("^v[0-9_]+\\.[a-zA-Z_][a-zA-Z0-9_]*\\.[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!std::regex_match(spec.name, fullRegex)) {
        // Validate version prefix exists
        const std::regex versionRegex("^v[0-9_]+\\.[a-zA-Z_\\.0-9]*$");
        if (!std::regex_match(spec.name, versionRegex)) {
            CubeLog::error("Function name must start with a valid version indicator like 'v1.0.0.my_app.my_function'");
            return false;
        }

        // Extract app name (the token between the first and second '.')
        auto firstDot = spec.name.find('.');
        auto secondDot = spec.name.find('.', firstDot + 1);
        if (firstDot == std::string::npos || secondDot == std::string::npos) {
            CubeLog::error("Function name '" + spec.name + "' is malformed");
            return false;
        }
        std::string appName = spec.name.substr(firstDot + 1, secondDot - firstDot - 1);

        // Validate app name identifier
        const std::regex identRegex("^[a-zA-Z_][a-zA-Z0-9_]*$");
        if (!std::regex_match(appName, identRegex)) {
            CubeLog::error("App name '" + appName + "' in function name '" + spec.name + "' is not a valid identifier. It must start with a letter or underscore and can only contain alphanumeric characters and underscores.");
            return false;
        }

        // Ensure app is registered
        auto appNames = AppsManager::getAppNames_static();
        if (std::find(appNames.begin(), appNames.end(), appName) == appNames.end()) {
            CubeLog::error("App '" + appName + "' is not registered. Please register the app before registering functions.");
            return false;
        }

        // Validate function name token (last token)
        std::string functionName = spec.name.substr(spec.name.find_last_of('.') + 1);
        if (!std::regex_match(functionName, identRegex)) {
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
    // TODO: If FunctionSpec supports a local `action` (local/provider-side
    // implementation), prefer invoking that instead of always performing an
    // RPC. Functions are primarily data sources (provided by apps), but the
    // spec may also include local actions for built-in or core-provided
    // behaviors.
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

    // Lookup socket path and ensure RPC IO is initialized
    std::string socketPath = lookupSocketPathFromDB(spec);
    if (socketPath.empty()) {
        CubeLog::error("No socket_location found for app: " + spec.appName);
        return nlohmann::json({{"error", "socket_not_found"}});
    }
    ensureRpcIoInitialized();

    // Resolve method name and delegate to helper that performs the JSON-RPC
    // request and returns a parsed JSON result.
    std::string method = spec.name;
    auto pos = spec.name.find_last_of('.');
    if (pos != std::string::npos) method = spec.name.substr(pos + 1);
    uint32_t timeoutMs = spec.timeoutMs > 0 ? spec.timeoutMs : 4000u;
    return sendJsonRpcRequest(socketPath, method, args, timeoutMs);
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
            if (!registerFunc(spec)) {
                res.status = 400;
                res.set_content("Failed to register function", "text/plain");
                res.set_header("Content-Type", "application/json");
                res.body = j.dump();
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_PARAMS, "Failed to register function");
            }
            res.status = 200;
            res.set_content("Function registered successfully", "text/plain");
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
            if (!func) {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            nlohmann::json j = func->toJson();
            res.status = 200;
            res.set_content(j.dump(), "application/json");
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
            if (it == funcs_.end()) {
                res.status = 404;
                res.set_content("Function not found", "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Function not found");
            }
            funcs_.erase(it);
            res.status = 200;
            res.set_content("Function unregistered successfully", "text/plain");
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

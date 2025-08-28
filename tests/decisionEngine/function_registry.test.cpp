// Unit tests for FunctionRegistry, FunctionRunner, FunctionSpec, CapabilitySpec
// Uses GoogleTest

#include <gtest/gtest.h>
#include <future>
#include "../src/decisionEngine/functionRegistry.h"
#include <filesystem>

using namespace DecisionEngine;

TEST(FunctionSpec, ToJsonContainsFields) {
    FunctionSpec f;
    f.name = "v1_test.app.my_function";
    f.description = "Test function";
    ParamSpec p;
    p.name = "param1"; p.type = "string"; p.description = "a param"; p.required = true;
    f.parameters.push_back(p);
    auto j = f.toJson();
    EXPECT_EQ(j["name"].get<std::string>(), f.name);
    EXPECT_EQ(j["description"].get<std::string>(), f.description);
    ASSERT_TRUE(j.contains("parameters"));
    EXPECT_EQ(j["parameters"].size(), 1);
}

// Additional edge/behavior tests moved from edge file

// TEST(FunctionRegistry, RegisterFunc_InvalidNames) {
//     FunctionRegistry registry;
//     FunctionSpec invalid1;
//     invalid1.name = ""; // empty
//     EXPECT_FALSE(registry.registerFunc(invalid1));

//     FunctionSpec invalid2;
//     invalid2.name = "no_version.myapp.fn"; // missing version prefix
//     EXPECT_FALSE(registry.registerFunc(invalid2));

//     FunctionSpec valid;
//     valid.name = "v1_my.app.valid_fn";
//     valid.description = "A valid function";
//     EXPECT_TRUE(registry.registerFunc(valid));
// }

TEST(FunctionRunner, RetryThenSucceed) {
    FunctionRunner runner;
    runner.start(2);
    std::promise<nlohmann::json> prom;
    auto fut = prom.get_future();
    std::atomic<int> calls{0};
    FunctionRunner::Task t;
    t.name = "retry.test";
    t.retryLimit = 2;
    t.timeoutMs = 0;
    t.work = [&calls]() -> nlohmann::json {
        int c = ++calls;
        if (c < 2) {
            return nlohmann::json({{"error", "temporary"}});
        }
        return nlohmann::json({{"result", "ok"}});
    };
    t.onComplete = [&prom](const nlohmann::json& res){ prom.set_value(res); };
    runner.enqueue(std::move(t));
    auto status = fut.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_TRUE(res.contains("result"));
    EXPECT_EQ(res["result"].get<std::string>(), "ok");
    EXPECT_GE(calls.load(), 2);
    runner.stop();
}

// TEST(FunctionRegistry, LoadCapabilityManifests_CustomPaths) {
//     // Create temporary manifest directories under tests/tmp_caps
//     std::filesystem::path base = "tests/tmp_caps";
//     std::filesystem::remove_all(base);
//     auto dataDir = base / "data/capabilities";
//     auto appDir = base / "apps/my_app/capabilities";
//     std::filesystem::create_directories(dataDir);
//     std::filesystem::create_directories(appDir);

//     // Write a data manifest
//     nlohmann::json dataMan = {
//         {"name", "sample.data_cap"},
//         {"description", "From data dir"},
//         {"type", "core"},
//         {"parameters", nlohmann::json::array()}
//     };
//     std::ofstream(dataDir / "sample.data_cap.json") << dataMan.dump(2);

//     // Write an app manifest without entry - loader should set entry to app dir name
//     nlohmann::json appMan = {
//         {"name", "sample.app_cap"},
//         {"description", "From app dir"},
//         {"type", "rpc"},
//         {"parameters", nlohmann::json::array()}
//     };
//     std::ofstream(appDir / "sample.app_cap.json") << appMan.dump(2);

//     FunctionRegistry registry;
//     registry.loadCapabilityManifests({ (base / "data/capabilities").string(), (base / "apps").string() });

//     const CapabilitySpec* c1 = registry.findCapability("sample.data_cap");
//     // data/core manifest should be ignored by processCapabilityFile (core
//     // capabilities are registered in code), so c1 should not exist.
//     EXPECT_EQ(c1, nullptr);

//     const CapabilitySpec* c2 = registry.findCapability("sample.app_cap");
//     ASSERT_NE(c2, nullptr);
//     EXPECT_EQ(c2->type, "rpc");
//     // RPC-backed capability should synthesize an action when registered
//     EXPECT_TRUE(bool(c2->action));
//     // entry should be set to the app directory name (my_app)
//     EXPECT_EQ(c2->entry, "my_app");

//     // Cleanup
//     std::filesystem::remove_all(base);
// }

// TEST(FunctionRegistry, PerformFunctionRpc_EndToEndUnixSocket) {
//     // Create temporary directory and socket path
//     std::filesystem::path base = "tests/tmp_rpc";
//     std::filesystem::remove_all(base);
//     std::filesystem::create_directories(base);
//     auto socketPath = (base / "test_rpc.sock").string();

//     // Create a simple httplib server bound to AF_UNIX that echoes JSON-RPC
//     httplib::Server svr;
//     svr.set_address_family(AF_UNIX);
//     svr.Post("/", [&](const httplib::Request& req, httplib::Response& res){
//         try {
//             auto j = nlohmann::json::parse(req.body);
//             nlohmann::json resp;
//             resp["jsonrpc"] = "2.0";
//             resp["id"] = j.value("id", 1);
//             nlohmann::json result;
//             result["method"] = j.value("method", "");
//             result["params"] = j.value("params", nlohmann::json::object());
//             resp["result"] = result;
//             res.set_content(resp.dump(), "application/json");
//         } catch (...) {
//             res.status = 500;
//             res.set_content("{}", "application/json");
//         }
//     });

//     std::thread serverThread([&](){ svr.listen(socketPath.c_str(), 80); });
//     // small sleep to allow server to start and create socket
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));

//     FunctionRegistry registry;
//     FunctionSpec spec;
//     spec.name = "v1_0_0.testapp.echo";
//     // Put socket path directly into appName so performFunctionRpcPublic will use it
//     spec.appName = socketPath;
//     spec.timeoutMs = 2000;

//     nlohmann::json args = { {"msg","hello"} };
//     nlohmann::json res = registry.performFunctionRpcPublic(spec, args);

//     EXPECT_TRUE(res.is_object());
//     if (res.contains("result")) {
//         EXPECT_EQ(res["result"]["method"].get<std::string>(), "echo");
//         EXPECT_TRUE(res["result"]["params"].is_object());
//     } else {
//         // If server failed, output response for debugging and fail
//         FAIL() << "RPC response unexpected: " << res.dump();
//     }

//     svr.stop();
//     if (serverThread.joinable()) serverThread.join();
//     std::filesystem::remove_all(base);
// }

// TEST(FunctionRegistry, RpcCapabilitySocketUnavailableFlag) {
//     FunctionRegistry registry;
//     CapabilitySpec cap;
//     cap.name = "test.rpc_cap";
//     cap.type = "rpc";
//     cap.entry = "/tmp/nonexistent_socket.sock"; // not present
//     registry.registerCapability(cap);

//     // Find capability and ensure action was synthesized
//     const CapabilitySpec* c = registry.findCapability("test.rpc_cap");
//     ASSERT_NE(c, nullptr);
//     EXPECT_TRUE(bool(c->action));

//     // Invoke it: since socket doesn't exist, action should mark socketUnavailable
//     std::promise<nlohmann::json> prom;
//     auto fut = prom.get_future();
//     registry.runCapabilityAsync("test.rpc_cap", nlohmann::json::object(), [&prom](const nlohmann::json& res){ prom.set_value(res); });
//     auto status = fut.wait_for(std::chrono::seconds(1));
//     EXPECT_EQ(status, std::future_status::ready);
//     auto res = fut.get();
//     EXPECT_TRUE(res.is_object());
//     // Action itself does not return a structured error, but the registry should
//     // mark the capability as having unavailable socket.
//     const CapabilitySpec* c2 = registry.findCapability("test.rpc_cap");
//     ASSERT_NE(c2, nullptr);
//     EXPECT_TRUE(c2->socketUnavailable);
// }


// TEST(CapabilitySpec, ToJsonContainsFields) {
//     CapabilitySpec c;
//     c.name = "core.test_cap";
//     c.type = "core";
//     c.entry = "";
//     ParamSpec p; p.name = "file"; p.type = "string"; p.description = "file path"; p.required = true;
//     c.parameters.push_back(p);
//     auto j = c.toJson();
//     EXPECT_EQ(j["name"].get<std::string>(), c.name);
//     EXPECT_EQ(j["type"].get<std::string>(), c.type);
//     ASSERT_TRUE(j.contains("parameters"));
//     EXPECT_EQ(j["parameters"].size(), 1);
// }

// TEST(FunctionRunner, ExecutesTaskAndCallsOnComplete) {
//     FunctionRunner runner;
//     runner.start(2);
//     std::promise<nlohmann::json> prom;
//     auto fut = prom.get_future();
//     FunctionRunner::Task t;
//     t.name = "test.exec";
//     t.work = []() -> nlohmann::json { return nlohmann::json({{"value", 42}}); };
//     t.onComplete = [&prom](const nlohmann::json& res){ prom.set_value(res); };
//     runner.enqueue(std::move(t));
//     auto status = fut.wait_for(std::chrono::seconds(1));
//     EXPECT_EQ(status, std::future_status::ready);
//     auto res = fut.get();
//     EXPECT_TRUE(res.contains("value"));
//     EXPECT_EQ(res["value"].get<int>(), 42);
//     runner.stop();
// }

// TEST(FunctionRunner, TimeoutAndRetryProducesError) {
//     FunctionRunner runner;
//     runner.start(1);
//     std::promise<nlohmann::json> prom;
//     auto fut = prom.get_future();
//     std::atomic<int> attempts{0};
//     FunctionRunner::Task t;
//     t.name = "test.timeout";
//     t.timeoutMs = 20; // short timeout
//     t.retryLimit = 1; // one retry
//     t.work = [&attempts]() -> nlohmann::json {
//         attempts.fetch_add(1);
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         return nlohmann::json({{"ok", true}});
//     };
//     t.onComplete = [&prom](const nlohmann::json& res){ prom.set_value(res); };
//     runner.enqueue(std::move(t));
//     auto status = fut.wait_for(std::chrono::seconds(2));
//     EXPECT_EQ(status, std::future_status::ready);
//     auto res = fut.get();
//     EXPECT_TRUE(res.is_object());
//     EXPECT_TRUE(res.contains("error"));
//     runner.stop();
// }

// TEST(FunctionRegistry, RegisterAndRunCapability) {
//     auto registry = std::make_shared<FunctionRegistry>();
//     CapabilitySpec cap;
//     cap.name = "test.ping";
//     cap.type = "core";
//     cap.action = [](const nlohmann::json& args){
//         CubeLog::info("ping action called");
//     };
//     registry->registerCapability(cap);

//     std::promise<nlohmann::json> prom;
//     auto fut = prom.get_future();
//     nlohmann::json args = { {"msg", "hello"} };
//     registry->runCapabilityAsync("test.ping", args, [&prom](const nlohmann::json& res){ prom.set_value(res); });
//     auto status = fut.wait_for(std::chrono::seconds(1));
//     EXPECT_EQ(status, std::future_status::ready);
//     auto res = fut.get();
//     EXPECT_TRUE(res.contains("status"));
//     EXPECT_EQ(res["status"].get<std::string>(), "ok");
// }

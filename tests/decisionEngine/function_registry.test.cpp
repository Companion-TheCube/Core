// Unit tests for FunctionRegistry, FunctionRunner, FunctionSpec, CapabilitySpec
// Uses GoogleTest

#include <gtest/gtest.h>
#include <future>
#include "../src/decisionEngine/functionRegistry.h"

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

TEST(CapabilitySpec, ToJsonContainsFields) {
    CapabilitySpec c;
    c.name = "core.test_cap";
    c.type = "core";
    c.entry = "";
    ParamSpec p; p.name = "file"; p.type = "string"; p.description = "file path"; p.required = true;
    c.parameters.push_back(p);
    auto j = c.toJson();
    EXPECT_EQ(j["name"].get<std::string>(), c.name);
    EXPECT_EQ(j["type"].get<std::string>(), c.type);
    ASSERT_TRUE(j.contains("parameters"));
    EXPECT_EQ(j["parameters"].size(), 1);
}

TEST(FunctionRunner, ExecutesTaskAndCallsOnComplete) {
    FunctionRunner runner;
    runner.start(2);
    std::promise<nlohmann::json> prom;
    auto fut = prom.get_future();
    FunctionRunner::Task t;
    t.name = "test.exec";
    t.work = []() -> nlohmann::json { return nlohmann::json({{"value", 42}}); };
    t.onComplete = [&prom](const nlohmann::json& res){ prom.set_value(res); };
    runner.enqueue(std::move(t));
    auto status = fut.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_TRUE(res.contains("value"));
    EXPECT_EQ(res["value"].get<int>(), 42);
    runner.stop();
}

TEST(FunctionRunner, TimeoutAndRetryProducesError) {
    FunctionRunner runner;
    runner.start(1);
    std::promise<nlohmann::json> prom;
    auto fut = prom.get_future();
    std::atomic<int> attempts{0};
    FunctionRunner::Task t;
    t.name = "test.timeout";
    t.timeoutMs = 20; // short timeout
    t.retryLimit = 1; // one retry
    t.work = [&attempts]() -> nlohmann::json {
        attempts.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return nlohmann::json({{"ok", true}});
    };
    t.onComplete = [&prom](const nlohmann::json& res){ prom.set_value(res); };
    runner.enqueue(std::move(t));
    auto status = fut.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_TRUE(res.is_object());
    EXPECT_TRUE(res.contains("error"));
    runner.stop();
}

TEST(FunctionRegistry, RegisterAndRunCapability) {
    auto registry = std::make_shared<FunctionRegistry>();
    CapabilitySpec cap;
    cap.name = "test.ping";
    cap.type = "core";
    cap.action = [](const nlohmann::json& args){
        CubeLog::info("ping action called");
    };
    registry->registerCapability(cap);

    std::promise<nlohmann::json> prom;
    auto fut = prom.get_future();
    nlohmann::json args = { {"msg", "hello"} };
    registry->runCapabilityAsync("test.ping", args, [&prom](const nlohmann::json& res){ prom.set_value(res); });
    auto status = fut.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_TRUE(res.contains("status"));
    EXPECT_EQ(res["status"].get<std::string>(), "ok");
}

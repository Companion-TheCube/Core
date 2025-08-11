// Extended unit tests for FunctionRegistry and FunctionRunner edge cases

#include <gtest/gtest.h>
#include <future>
#include <filesystem>
#include "../src/decisionEngine/functionRegistry.h"

using namespace DecisionEngine;

TEST(FunctionRegistry, RegisterFunc_InvalidNames)
{
    FunctionRegistry registry;
    FunctionSpec invalid1;
    invalid1.name = ""; // empty
    EXPECT_FALSE(registry.registerFunc(invalid1));

    FunctionSpec invalid2;
    invalid2.name = "no_version.myapp.fn"; // missing version prefix
    EXPECT_FALSE(registry.registerFunc(invalid2));

    FunctionSpec valid;
    valid.name = "v1_my.app.valid_fn";
    valid.description = "A valid function";
    EXPECT_TRUE(registry.registerFunc(valid));
}

TEST(FunctionRunner, RetryThenSucceed)
{
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

TEST(FunctionRegistry, LoadCapabilityManifests_CustomPaths)
{
    // Create temporary manifest directories under tests/tmp_caps
    std::filesystem::path base = "tests/tmp_caps";
    std::filesystem::remove_all(base);
    auto dataDir = base / "data/capabilities";
    auto appDir = base / "apps/my_app/capabilities";
    std::filesystem::create_directories(dataDir);
    std::filesystem::create_directories(appDir);

    // Write a data manifest
    nlohmann::json dataMan = {
        {"name", "sample.data_cap"},
        {"description", "From data dir"},
        {"type", "core"},
        {"parameters", nlohmann::json::array()}
    };
    std::ofstream(dataDir / "sample.data_cap.json") << dataMan.dump(2);

    // Write an app manifest without entry - loader should set entry to app dir name
    nlohmann::json appMan = {
        {"name", "sample.app_cap"},
        {"description", "From app dir"},
        {"type", "rpc"},
        {"parameters", nlohmann::json::array()}
    };
    std::ofstream(appDir / "sample.app_cap.json") << appMan.dump(2);

    FunctionRegistry registry;
    registry.loadCapabilityManifests({ (base / "data/capabilities").string(), (base / "apps").string() });

    const CapabilitySpec* c1 = registry.findCapability("sample.data_cap");
    ASSERT_NE(c1, nullptr);
    EXPECT_EQ(c1->type, "core");

    const CapabilitySpec* c2 = registry.findCapability("sample.app_cap");
    ASSERT_NE(c2, nullptr);
    EXPECT_EQ(c2->type, "rpc");
    // entry should be set to the app directory name (my_app)
    EXPECT_EQ(c2->entry, "my_app");

    // Cleanup
    std::filesystem::remove_all(base);
}

TEST(FunctionSpec, CopyAndMoveSemantics)
{
    FunctionSpec f;
    f.name = "v1.test.app.fn";
    f.description = "desc";
    ParamSpec p; p.name = "a"; p.type = "string"; p.description = ""; p.required = true;
    f.parameters.push_back(p);

    FunctionSpec copy = f;
    EXPECT_EQ(copy.name, f.name);
    EXPECT_EQ(copy.parameters.size(), 1);

    FunctionSpec moved = std::move(f);
    EXPECT_EQ(moved.name, copy.name);
    EXPECT_EQ(moved.parameters.size(), 1);
}


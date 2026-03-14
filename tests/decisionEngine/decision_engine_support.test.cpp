#include <gtest/gtest.h>

#include "../src/decisionEngine/functionRegistry.h"
#include "../src/decisionEngine/intentRegistry.h"
#include "../src/decisionEngine/scheduler.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace DecisionEngine;

TEST(FunctionRegistry, RunCapabilityAsyncReturnsActionJson)
{
    FunctionRegistry registry;

    CapabilitySpec cap;
    cap.name = "test.return_json";
    cap.action = [](const nlohmann::json& args) {
        return nlohmann::json({
            { "echo", args },
            { "ok", true }
        });
    };
    ASSERT_TRUE(registry.registerCapability(cap));

    std::promise<nlohmann::json> promise;
    auto future = promise.get_future();
    registry.runCapabilityAsync("test.return_json", nlohmann::json({ { "value", 42 } }), [&promise](const nlohmann::json& result) mutable {
        promise.set_value(result);
    });

    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    const auto result = future.get();
    ASSERT_TRUE(result.contains("ok"));
    EXPECT_TRUE(result["ok"].get<bool>());
    ASSERT_TRUE(result.contains("echo"));
    EXPECT_EQ(result["echo"]["value"].get<int>(), 42);
}

TEST(FunctionRegistry, BuiltInCoreCapabilitiesPresent)
{
    FunctionRegistry registry;

    EXPECT_NE(registry.findCapability("core.ping"), nullptr);
    EXPECT_NE(registry.findCapability("core.get_time"), nullptr);
    EXPECT_NE(registry.findCapability("apps.list_installed"), nullptr);
    EXPECT_NE(registry.findCapability("audio.toggle_sound"), nullptr);
    EXPECT_NE(registry.findCapability("audio.set_sound"), nullptr);
}

TEST(IntentRegistry, RegistersV1BuiltInIntents)
{
    IntentRegistry registry;
    const auto names = registry.getIntentNames();

    EXPECT_NE(std::find(names.begin(), names.end(), "core.ping"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.get_time"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "apps.list_installed"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.toggle_sound"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.set_sound_on"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.set_sound_off"), names.end());
}

TEST(Scheduler, OneShotTaskFiresOnce)
{
    std::atomic<int> executions { 0 };
    auto intent = std::make_shared<Intent>("test.scheduler.one_shot", [&executions](const Parameters&, Intent) {
        ++executions;
    });

    Scheduler scheduler;
    scheduler.addTask(ScheduledTask(intent, std::chrono::system_clock::now()));
    scheduler.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    scheduler.stop();

    EXPECT_EQ(executions.load(), 1);
}

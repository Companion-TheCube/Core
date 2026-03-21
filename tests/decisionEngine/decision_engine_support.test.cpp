#include <gtest/gtest.h>

#include "../src/decisionEngine/functionRegistry.h"
#include "../src/decisionEngine/intentRegistry.h"
#include "../src/decisionEngine/scheduler.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

using namespace DecisionEngine;

namespace {

class FakeConversationClient : public TheCubeServer::IRemoteConversationClient {
public:
    std::optional<std::string> createConversationSession() override
    {
        return std::string("fake-session");
    }

    std::future<std::string> getChatResponseAsync(
        const std::string& sessionId,
        const std::string& message,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) override
    {
        (void)sessionId;
        (void)message;
        (void)progressCB;
        return std::async(std::launch::deferred, []() { return std::string(); });
    }

    std::future<std::string> getGeneralAnswerAsync(
        const std::string& question,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) override
    {
        lastQuestion = question;
        if (progressCB) {
            progressCB("answer");
        }
        return std::async(std::launch::deferred, []() { return std::string("answer"); });
    }

    std::string lastQuestion;
};

} // namespace

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
    EXPECT_NE(registry.findCapability("core.get_date"), nullptr);
    EXPECT_NE(registry.findCapability("core.get_datetime"), nullptr);
    EXPECT_NE(registry.findCapability("core.ask_ai"), nullptr);
    EXPECT_NE(registry.findCapability("apps.list_installed"), nullptr);
    EXPECT_NE(registry.findCapability("audio.toggle_sound"), nullptr);
    EXPECT_NE(registry.findCapability("audio.set_sound"), nullptr);
    EXPECT_NE(registry.findCapability("core.play_sound"), nullptr);
    EXPECT_NE(registry.findCapability("core.speak_text"), nullptr);
}

TEST(FunctionRegistry, BuiltInAskAiCapabilityUsesRemoteConversationClient)
{
    auto registry = std::make_shared<FunctionRegistry>();
    auto fakeClient = std::make_shared<FakeConversationClient>();
    registry->setRemoteConversationClient(fakeClient);

    std::promise<nlohmann::json> promise;
    auto future = promise.get_future();
    registry->runCapabilityAsync("core.ask_ai", nlohmann::json({ { "transcript", "Why is the sky blue?" } }), [&promise](const nlohmann::json& result) mutable {
        promise.set_value(result);
    });

    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    const auto result = future.get();
    EXPECT_EQ(fakeClient->lastQuestion, "Why is the sky blue?");
    EXPECT_EQ(result["status"].get<std::string>(), "ok");
    EXPECT_EQ(result["answer"].get<std::string>(), "answer");
}

TEST(IntentRegistry, RegistersV1BuiltInIntents)
{
    IntentRegistry registry;
    const auto names = registry.getIntentNames();

    EXPECT_NE(std::find(names.begin(), names.end(), "core.ping"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.get_time"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.get_date"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.get_datetime"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.ask_ai"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "apps.list_installed"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.toggle_sound"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.set_sound_on"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "audio.set_sound_off"), names.end());

    const auto getTime = registry.getIntent("core.get_time");
    ASSERT_NE(getTime, nullptr);
    const auto params = getTime->getParameters();
    const auto it = params.find("speak_result");
    ASSERT_NE(it, params.end());
    EXPECT_EQ(it->second, "true");
}

TEST(GlobalSettings, GeneralAiResponseModeDefaultsToPopupOnly)
{
    GlobalSettings settings;
    EXPECT_EQ(
        settings.getSettingOfType<std::string>(GlobalSettings::SettingType::GENERAL_AI_RESPONSE_MODE),
        "popupOnly");
}

TEST(IntentRegistry, LoadsAppIntentManifestsWhenCapabilityExists)
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "cube_intent_manifest_test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    std::filesystem::create_directories(tempRoot / "sample-app" / "intents", ec);
    ASSERT_FALSE(ec);

    const auto manifestPath = tempRoot / "sample-app" / "intents" / "sample.intent.json";
    std::ofstream(manifestPath) << R"json(
{
  "intentName": "sample.say_hello",
  "capabilityName": "sample.hello",
  "briefDesc": "Say hello from a sample app intent.",
  "responseString": "Hello from the sample app.",
  "speakResult": true,
  "parameters": {
    "audience": "developer"
  }
}
)json";

    Config::set("APP_INSTALL_ROOTS", tempRoot.string());

    auto functionRegistry = std::make_shared<FunctionRegistry>();
    CapabilitySpec sampleCapability;
    sampleCapability.name = "sample.hello";
    sampleCapability.description = "Sample test capability";
    sampleCapability.action = [](const nlohmann::json& args) {
        return nlohmann::json({
            { "status", "ok" },
            { "audience", args.value("audience", "") }
        });
    };
    ASSERT_TRUE(functionRegistry->registerCapability(sampleCapability));

    IntentRegistry registry;
    registry.setFunctionRegistry(functionRegistry);

    const auto intent = registry.getIntent("sample.say_hello");
    ASSERT_NE(intent, nullptr);
    EXPECT_EQ(intent->getBriefDesc(), "Say hello from a sample app intent.");
    EXPECT_EQ(intent->getResponseString(), "Hello from the sample app.");
    const auto params = intent->getParameters();
    EXPECT_EQ(params.at("capability_name"), "sample.hello");
    EXPECT_EQ(params.at("audience"), "developer");
    EXPECT_EQ(params.at("speak_result"), "true");

    Config::erase("APP_INSTALL_ROOTS");
    std::filesystem::remove_all(tempRoot, ec);
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

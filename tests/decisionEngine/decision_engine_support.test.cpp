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

    std::future<TheCubeServer::ResolvedIntentCall> getResolvedIntentCallAsync(
        const std::string& utterance,
        const nlohmann::json& functions,
        const nlohmann::json& context = nlohmann::json::object()) override
    {
        lastResolvedUtterance = utterance;
        lastResolvedFunctions = functions;
        lastResolvedContext = context;
        return std::async(std::launch::deferred, []() { return TheCubeServer::ResolvedIntentCall {}; });
    }

    std::string lastQuestion;
    std::string lastResolvedUtterance;
    nlohmann::json lastResolvedFunctions = nlohmann::json::array();
    nlohmann::json lastResolvedContext = nlohmann::json::object();
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
    EXPECT_NE(registry.findCapability("core.create_reminder"), nullptr);
    EXPECT_NE(registry.findCapability("core.create_alarm"), nullptr);
    EXPECT_NE(registry.findCapability("core.list_reminders"), nullptr);
    EXPECT_NE(registry.findCapability("core.list_alarms"), nullptr);
    EXPECT_NE(registry.findCapability("core.cancel_notification"), nullptr);
    EXPECT_NE(registry.findCapability("core.snooze_alarm"), nullptr);
    EXPECT_NE(registry.findCapability("apps.list_installed"), nullptr);
    EXPECT_NE(registry.findCapability("audio.toggle_sound"), nullptr);
    EXPECT_NE(registry.findCapability("audio.set_sound"), nullptr);
    EXPECT_NE(registry.findCapability("core.play_sound"), nullptr);
    EXPECT_NE(registry.findCapability("core.speak_text"), nullptr);
}

TEST(FunctionRegistry, ParameterizedVoiceCapabilitiesExposeSchemas)
{
    FunctionRegistry registry;

    const auto* reminder = registry.findCapability("core.create_reminder");
    ASSERT_NE(reminder, nullptr);
    EXPECT_TRUE(reminder->voiceEnabled);
    ASSERT_TRUE(reminder->voiceInputSchema.is_object());
    EXPECT_TRUE(reminder->voiceInputSchema["properties"].contains("scheduledForLocalIso"));
    EXPECT_TRUE(reminder->voiceInputSchema["properties"].contains("scheduledForEpochMs"));
    EXPECT_EQ(reminder->voiceInputSchema["required"], nlohmann::json::array({ "scheduledForLocalIso" }));

    const auto* alarm = registry.findCapability("core.create_alarm");
    ASSERT_NE(alarm, nullptr);
    EXPECT_TRUE(alarm->voiceEnabled);
    EXPECT_TRUE(alarm->voiceInputSchema["properties"].contains("scheduledForLocalIso"));
    EXPECT_TRUE(alarm->voiceInputSchema["properties"].contains("scheduledForEpochMs"));
    EXPECT_EQ(alarm->voiceInputSchema["required"], nlohmann::json::array({ "scheduledForLocalIso" }));

    const auto* setSound = registry.findCapability("audio.set_sound");
    ASSERT_NE(setSound, nullptr);
    EXPECT_TRUE(setSound->voiceEnabled);
    EXPECT_TRUE(setSound->voiceInputSchema["properties"].contains("soundOn"));
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
    EXPECT_NE(std::find(names.begin(), names.end(), "core.create_reminder"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.create_alarm"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.list_reminders"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.list_alarms"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.cancel_notification"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "core.snooze_alarm"), names.end());
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

TEST(IntentRegistry, ResponseStringIgnoresExtraParametersAndReplacesRepeatedPlaceholders)
{
    Parameters params = {
        { "summary", "Alarm set for 6:00 PM." },
        { "notificationId", "42" },
        { "scheduledForEpochMs", "1742594400000" }
    };
    Intent intent(
        "core.create_alarm",
        [](const Parameters&, Intent) {},
        params,
        "Create an alarm",
        "${summary} ${summary}");

    EXPECT_EQ(intent.getResponseString(), "Alarm set for 6:00 PM. Alarm set for 6:00 PM.");
}

TEST(GlobalSettings, GeneralAiResponseModeDefaultsToPopupOnly)
{
    GlobalSettings settings;
    EXPECT_EQ(
        settings.getSettingOfType<std::string>(GlobalSettings::SettingType::GENERAL_AI_RESPONSE_MODE),
        "popupOnly");
}

TEST(GlobalSettings, AlarmSnoozeMinutesDefaultsToTen)
{
    GlobalSettings settings;
    EXPECT_EQ(
        settings.getSettingOfType<int>(GlobalSettings::SettingType::ALARM_SNOOZE_MINUTES),
        10);
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

TEST(FunctionRegistry, LoadsVoiceSchemaFromCapabilityManifest)
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "cube_capability_manifest_voice_schema_test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    std::filesystem::create_directories(tempRoot / "sample-app" / "capabilities", ec);
    ASSERT_FALSE(ec);

    const auto manifestPath = tempRoot / "sample-app" / "capabilities" / "sample.capability.json";
    std::ofstream(manifestPath) << R"json(
{
  "name": "sample.schedule",
  "description": "Schedule a sample action.",
  "type": "rpc",
  "entry": "sample-app",
  "voiceEnabled": true,
  "voiceInputSchema": {
    "type": "object",
    "properties": {
      "scheduledForEpochMs": {
        "type": "integer"
      }
    },
    "required": ["scheduledForEpochMs"],
    "additionalProperties": false
  }
}
)json";

    Config::set("APP_INSTALL_ROOTS", tempRoot.string());
    FunctionRegistry registry;

    const auto* capability = registry.findCapability("sample.schedule");
    ASSERT_NE(capability, nullptr);
    EXPECT_TRUE(capability->voiceEnabled);
    EXPECT_TRUE(capability->voiceInputSchema.is_object());
    EXPECT_TRUE(capability->voiceInputSchema["properties"].contains("scheduledForEpochMs"));

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

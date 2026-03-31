#include <gtest/gtest.h>

#include "api/autoRegister.h"
#include "audio/audioManager.h"
#include "database/cubeDB.h"
#include "decisionEngine/functionRegistry.h"
#include "decisionEngine/intentRegistry.h"
#include "decisionEngine/notificationCenter.h"
#include "decisionEngine/personalityManager.h"
#include "decisionEngine/remoteServer.h"
#include "decisionEngine/scheduler.h"
#include "decisionEngine/transcriber.h"
#include "decisionEngine/transcriptionEvents.h"
#include "decisionEngine/triggers.h"
#include "threadsafeQueue.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define private public
#include "decisionEngine/decisions.h"
#undef private

#include "utils.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace {

class ScopedTranscriptWebSocketServer {
public:
    using Server = websocketpp::server<websocketpp::config::asio>;

    explicit ScopedTranscriptWebSocketServer(
        std::string transcriptPayload,
        std::string resolvedPayload = {},
        int requestedPort = 0,
        bool closeOnEnd = false)
        : transcriptPayload_(std::move(transcriptPayload))
        , resolvedPayload_(std::move(resolvedPayload))
        , closeOnEnd_(closeOnEnd)
    {
        server_.clear_access_channels(websocketpp::log::alevel::all);
        server_.clear_error_channels(websocketpp::log::elevel::all);
        server_.init_asio();
        server_.set_reuse_addr(true);
        server_.set_open_handler([this](websocketpp::connection_hdl hdl) {
            websocketpp::lib::error_code ec;
            server_.send(hdl, R"({"type":"voice_turn_started","sessionId":"voice-turn-session"})", websocketpp::frame::opcode::text, ec);
        });
        server_.set_message_handler([this](websocketpp::connection_hdl hdl, Server::message_ptr msg) {
            if (msg->get_opcode() != websocketpp::frame::opcode::text) {
                return;
            }

            if (msg->get_payload() == "__END__") {
                websocketpp::lib::error_code ec;
                if (closeOnEnd_) {
                    server_.close(hdl, websocketpp::close::status::normal, "closing", ec);
                    return;
                }
                server_.send(hdl, transcriptPayload_, websocketpp::frame::opcode::text, ec);
                if (!resolvedPayload_.empty()) {
                    server_.send(hdl, resolvedPayload_, websocketpp::frame::opcode::text, ec);
                }
            }
        });

        websocketpp::lib::error_code ec;
        server_.listen(
            websocketpp::lib::asio::ip::tcp::endpoint(
                websocketpp::lib::asio::ip::address_v4::loopback(),
                static_cast<uint16_t>(requestedPort)),
            ec);
        if (ec) {
            throw std::runtime_error("Failed to bind WebSocket test server: " + ec.message());
        }

        websocketpp::lib::asio::error_code endpointEc;
        port_ = static_cast<int>(server_.get_local_endpoint(endpointEc).port());
        if (endpointEc || port_ <= 0) {
            throw std::runtime_error("Failed to read WebSocket test server port");
        }

        server_.start_accept();
        thread_ = std::thread([this]() { server_.run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    ~ScopedTranscriptWebSocketServer()
    {
        websocketpp::lib::error_code ec;
        server_.stop_listening(ec);
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    int port() const { return port_; }

private:
    Server server_;
    std::thread thread_;
    std::string transcriptPayload_;
    std::string resolvedPayload_;
    bool closeOnEnd_ = false;
    int port_ = -1;
};

void resetVoiceConfig()
{
    Config::erase("REMOTE_SERVER_BASE_URL");
    Config::erase("REMOTE_SERVER_BEARER_TOKEN");
    Config::erase("REMOTE_SERVER_API_KEY");
    Config::erase("REMOTE_SERVER_SERIAL");
    Config::erase("REMOTE_TRANSCRIPTION_BASE_URL");
    Config::erase("REMOTE_TRANSCRIPTION_API_KEY");
    Config::erase("REMOTE_AUTH_KEY");
    Config::erase("REMOTE_API_KEY");
    Config::erase("DEVICE_SERIAL_NUMBER");
    Config::erase("DECISION_ENGINE_RESULT_HIDE_MS");
    Config::erase("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS");
    Config::erase("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS");
    Config::erase("REMOTE_TRANSCRIPTION_WAKEWORD_VAD_DELAY_MS");
    Config::erase("SILERO_VAD_ENABLED");
    Config::erase("SILERO_VAD_MODEL_PATH");
    Config::erase("SILERO_VAD_THRESHOLD");
    Config::erase("SILERO_VAD_RELEASE_DELTA");
    Config::erase("SILERO_VAD_WINDOW_MS");
    Config::erase("SILERO_VAD_MIN_SILENCE_MS");
}

bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

void pushSpeechChunk(const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>>& audioQueue)
{
    ASSERT_TRUE(audioQueue);
    audioQueue->push(std::vector<int16_t>(256, 2000));
}

class FakeRewriteServerAPI : public TheCubeServer::TheCubeServerAPI {
public:
    FakeRewriteServerAPI()
        : TheCubeServer::TheCubeServerAPI(std::make_shared<ThreadSafeQueue<std::vector<int16_t>>>())
    {
    }

    std::future<std::string> getEmotionalRewriteAsync(
        const std::string& responseText,
        const nlohmann::json& context = nlohmann::json::object(),
        const std::function<void(std::string)>& progressCB = [](std::string) {}) override
    {
        lastResponseText = responseText;
        lastContext = context;
        if (progressCB) {
            progressCB(rewriteText);
        }
        if (throwOnRewrite) {
            return std::async(std::launch::deferred, []() -> std::string {
                throw std::runtime_error("rewrite failed");
            });
        }
        return std::async(std::launch::deferred, [text = rewriteText]() { return text; });
    }

    std::string rewriteText;
    bool throwOnRewrite = false;
    std::string lastResponseText;
    nlohmann::json lastContext = nlohmann::json::object();
};

class DecisionEngineChatHistoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetVoiceConfig();
        Config::set("DECISION_ENGINE_RESULT_HIDE_MS", "5");

        originalCwd_ = std::filesystem::current_path();
        tempRoot_ = std::filesystem::temp_directory_path() / ("decision_engine_chat_history_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempRoot_);
        std::filesystem::current_path(tempRoot_);

        dbManager_ = std::make_shared<CubeDatabaseManager>();
        blobsManager_ = std::make_shared<BlobsManager>(dbManager_, "data/blobs.db");
        cubeDb_ = std::make_shared<CubeDB>(dbManager_, blobsManager_);
    }

    void TearDown() override
    {
        CubeDB::setCubeDBManager(nullptr);
        CubeDB::setBlobsManager(nullptr);
        cubeDb_.reset();
        blobsManager_.reset();
        dbManager_.reset();
        std::filesystem::current_path(originalCwd_);
        std::filesystem::remove_all(tempRoot_);
        resetVoiceConfig();
    }

    ChatHistoryEntry makeHistoryEntry(
        std::string historyKey,
        std::string requestText,
        std::string displayResponse,
        int64_t createdAtMs) const
    {
        const auto intentKey = historyKey;
        return ChatHistoryEntry {
            .createdAtMs = createdAtMs,
            .historyKey = std::move(historyKey),
            .intentName = intentKey,
            .capabilityName = intentKey,
            .requestText = std::move(requestText),
            .displayResponse = std::move(displayResponse)
        };
    }

    std::filesystem::path originalCwd_;
    std::filesystem::path tempRoot_;
    std::shared_ptr<CubeDatabaseManager> dbManager_;
    std::shared_ptr<BlobsManager> blobsManager_;
    std::shared_ptr<CubeDB> cubeDb_;
};

} // namespace

TEST(DecisionEngineMain, WakeWordStartupFailureSurfacesVoiceServiceUnavailable)
{
    resetVoiceConfig();

    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:1");
    Config::set("DECISION_ENGINE_RESULT_HIDE_MS", "25");

    DecisionEngine::DecisionEngineMain engine;
    engine.onWakeWordDetected();

    EXPECT_EQ(engine.currentTurnTiming.listeningUiShownEpochMs, 0);
    EXPECT_EQ(engine.lastDecisionResult.executionStatus, "voice_service_unavailable");
    EXPECT_EQ(engine.lastDecisionResult.error, "voice_service_unavailable");
    EXPECT_FALSE(engine.lastDecisionResult.speakResult);

    const auto status = engine.statusJson();
    EXPECT_EQ(status["remoteVoiceFailureCategory"].get<std::string>(), "voice_service_unavailable");
    EXPECT_FALSE(status["remoteVoiceFailureMessage"].get<std::string>().empty());
    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));
}

TEST(DecisionEngineMain, MidTurnVoiceServerFailureSurfacesVoiceServiceUnavailable)
{
    resetVoiceConfig();

    ScopedTranscriptWebSocketServer wsServer("", "", 0, true);
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(wsServer.port()));
    Config::set("DECISION_ENGINE_RESULT_HIDE_MS", "25");
    Config::set("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS", "40");
    Config::set("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS", "200");

    DecisionEngine::DecisionEngineMain engine;
    engine.onWakeWordDetected();
    pushSpeechChunk(engine.audioQueue);

    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.lastDecisionResult.error == "voice_service_unavailable"; },
        std::chrono::milliseconds(1000)));
    EXPECT_GT(engine.currentTurnTiming.listeningUiShownEpochMs, 0);
    EXPECT_FALSE(engine.lastDecisionResult.speakResult);
    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));
}

TEST(DecisionEngineMain, VoiceTurnRecoversWhenServerBecomesAvailableAgain)
{
    resetVoiceConfig();

    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:1");
    Config::set("DECISION_ENGINE_RESULT_HIDE_MS", "25");
    Config::set("REMOTE_TRANSCRIPTION_SILENCE_TIMEOUT_MS", "40");
    Config::set("REMOTE_TRANSCRIPTION_WAIT_FINAL_MS", "200");

    DecisionEngine::DecisionEngineMain engine;

    engine.onWakeWordDetected();
    EXPECT_EQ(engine.currentTurnTiming.listeningUiShownEpochMs, 0);
    EXPECT_EQ(engine.lastDecisionResult.error, "voice_service_unavailable");
    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));

    ScopedTranscriptWebSocketServer wsServer(
        R"({"type":"transcript_final","transcript":"what apps are installed"})",
        R"({"type":"intent_call_resolved","sessionId":"voice-turn-session","status":"matched","intentName":"apps.list_installed","arguments":{},"message":""})");
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(wsServer.port()));

    engine.onWakeWordDetected();
    pushSpeechChunk(engine.audioQueue);

    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.lastDecisionResult.executionStatus == "success"; },
        std::chrono::milliseconds(1000)));
    EXPECT_EQ(engine.lastDecisionResult.intentName, "apps.list_installed");
    EXPECT_TRUE(engine.lastDecisionResult.error.empty());
    EXPECT_GT(engine.currentTurnTiming.listeningUiShownEpochMs, 0);
}

TEST_F(DecisionEngineChatHistoryTest, CapabilityBackedTurnsUseCapabilityNameAsHistoryKey)
{
    DecisionEngine::DecisionEngineMain engine;

    auto capabilityIntent = engine.intentRegistry->getIntent("core.get_time");
    ASSERT_NE(capabilityIntent, nullptr);
    const auto capabilityResult = engine.executeIntent(capabilityIntent, "what time is it?");
    EXPECT_EQ(capabilityResult.capabilityName, "core.get_time");
    EXPECT_EQ(capabilityResult.historyKey, "core.get_time");

    auto plainIntent = std::make_shared<DecisionEngine::Intent>(
        "test.intent",
        [](const DecisionEngine::Parameters&, DecisionEngine::Intent) {},
        DecisionEngine::Parameters {},
        "Test intent",
        "Done.");
    const auto plainResult = engine.executeIntent(plainIntent, "run test");
    EXPECT_TRUE(plainResult.capabilityName.empty());
    EXPECT_EQ(plainResult.historyKey, "test.intent");
}

TEST_F(DecisionEngineChatHistoryTest, PresentTurnResultPassesPriorHistoryToRewriteAndStoresFinalDisplayedMessage)
{
    DecisionEngine::DecisionEngineMain engine;
    ASSERT_TRUE(engine.chatHistoryStore);

    auto fakeRemote = std::make_shared<FakeRewriteServerAPI>();
    fakeRemote->rewriteText = "The clock just hit 5:00 PM.";
    engine.remoteServerAPI = fakeRemote;

    ASSERT_TRUE(engine.chatHistoryStore->appendHistory(makeHistoryEntry(
        "core.get_time",
        "what time is it?",
        "The clock says 4:45 PM.",
        1000)));
    ASSERT_TRUE(engine.chatHistoryStore->appendHistory(makeHistoryEntry(
        "core.get_time",
        "what time is it now?",
        "It's 4:50 PM right now.",
        2000)));

    DecisionEngine::DecisionTurnResult result;
    result.transcript = "what time is it right now?";
    result.intentName = "core.get_time";
    result.capabilityName = "core.get_time";
    result.historyKey = "core.get_time";
    result.executionStatus = "success";
    result.responseText = "It's 5:00 PM right now.";
    result.timestampEpochMs = 3000;

    engine.presentTurnResult(result);

    ASSERT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));
    ASSERT_TRUE(fakeRemote->lastContext.contains("recentToolHistory"));
    ASSERT_TRUE(fakeRemote->lastContext["recentToolHistory"].is_array());
    ASSERT_EQ(fakeRemote->lastContext["recentToolHistory"].size(), 2u);
    EXPECT_EQ(fakeRemote->lastContext["recentToolHistory"][0]["requestText"], "what time is it?");
    EXPECT_EQ(fakeRemote->lastContext["recentToolHistory"][1]["requestText"], "what time is it now?");

    const auto storedHistory = engine.chatHistoryStore->getRecentHistory("core.get_time", 10);
    ASSERT_EQ(storedHistory.size(), 3u);
    EXPECT_EQ(storedHistory.back().requestText, "what time is it right now?");
    EXPECT_EQ(storedHistory.back().displayResponse, "The clock just hit 5:00 PM.");
}

TEST_F(DecisionEngineChatHistoryTest, PresentTurnResultStoresOriginalMessageWhenRewriteFails)
{
    DecisionEngine::DecisionEngineMain engine;
    ASSERT_TRUE(engine.chatHistoryStore);

    auto fakeRemote = std::make_shared<FakeRewriteServerAPI>();
    fakeRemote->throwOnRewrite = true;
    engine.remoteServerAPI = fakeRemote;

    DecisionEngine::DecisionTurnResult result;
    result.transcript = "what time is it right now?";
    result.intentName = "core.get_time";
    result.capabilityName = "core.get_time";
    result.historyKey = "core.get_time";
    result.executionStatus = "success";
    result.responseText = "It's 5:00 PM right now.";
    result.timestampEpochMs = 3000;

    engine.presentTurnResult(result);

    ASSERT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));
    const auto storedHistory = engine.chatHistoryStore->getRecentHistory("core.get_time", 10);
    ASSERT_EQ(storedHistory.size(), 1u);
    EXPECT_EQ(storedHistory.back().displayResponse, "It's 5:00 PM right now.");
}

TEST_F(DecisionEngineChatHistoryTest, PresentTurnResultOnlyPersistsSuccessfulDisplayedTurns)
{
    DecisionEngine::DecisionEngineMain engine;
    ASSERT_TRUE(engine.chatHistoryStore);

    DecisionEngine::DecisionTurnResult failureResult;
    failureResult.transcript = "what time is it?";
    failureResult.intentName = "core.get_time";
    failureResult.capabilityName = "core.get_time";
    failureResult.historyKey = "core.get_time";
    failureResult.executionStatus = "voice_service_unavailable";
    failureResult.responseText = "Voice service is unavailable right now. Please try again shortly.";
    failureResult.error = "voice_service_unavailable";
    failureResult.timestampEpochMs = 4000;

    engine.presentTurnResult(failureResult);

    ASSERT_TRUE(waitUntil(
        [&engine]() { return engine.turnState == DecisionEngine::DecisionEngineMain::TurnState::IDLE; },
        std::chrono::milliseconds(250)));
    EXPECT_TRUE(engine.chatHistoryStore->getRecentHistory("core.get_time", 10).empty());
}

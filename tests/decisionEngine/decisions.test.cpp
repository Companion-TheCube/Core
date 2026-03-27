#include <gtest/gtest.h>

#include "api/autoRegister.h"
#include "audio/audioManager.h"
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

#include "httplib.h"
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

int reserveUnusedLocalPort()
{
    httplib::Server probe;
    const int port = probe.bind_to_any_port("127.0.0.1");
    probe.stop();
    if (port <= 0) {
        throw std::runtime_error("Failed to reserve local test port");
    }
    return port;
}

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

} // namespace

TEST(DecisionEngineMain, WakeWordStartupFailureSurfacesVoiceServiceUnavailable)
{
    resetVoiceConfig();

    const int port = reserveUnusedLocalPort();
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(port));
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

    const int port = reserveUnusedLocalPort();
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(port));
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
        R"({"type":"intent_call_resolved","sessionId":"voice-turn-session","status":"matched","intentName":"apps.list_installed","arguments":{},"message":""})",
        port);

    engine.onWakeWordDetected();
    pushSpeechChunk(engine.audioQueue);

    EXPECT_TRUE(waitUntil(
        [&engine]() { return engine.lastDecisionResult.executionStatus == "success"; },
        std::chrono::milliseconds(1000)));
    EXPECT_EQ(engine.lastDecisionResult.intentName, "apps.list_installed");
    EXPECT_TRUE(engine.lastDecisionResult.error.empty());
    EXPECT_GT(engine.currentTurnTiming.listeningUiShownEpochMs, 0);
}

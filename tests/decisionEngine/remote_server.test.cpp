#include <gtest/gtest.h>

#include "decisionEngine/remoteServer.h"
#include "utils.h"
#include "httplib.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace {

class ScopedHttpServer {
public:
    ScopedHttpServer()
    {
        port_ = server_.bind_to_any_port("127.0.0.1");
        if (port_ <= 0) {
            throw std::runtime_error("Failed to bind HTTP test server");
        }
        thread_ = std::thread([this]() { server_.listen_after_bind(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    ~ScopedHttpServer()
    {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    int port() const { return port_; }
    httplib::Server& server() { return server_; }

private:
    httplib::Server server_;
    std::thread thread_;
    int port_ = -1;
};

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
        server_.set_open_handler([this](websocketpp::connection_hdl hdl) {
            websocketpp::lib::error_code ec;
            server_.send(hdl, R"({"type":"voice_turn_started","sessionId":"voice-turn-session"})", websocketpp::frame::opcode::text, ec);
        });
        server_.set_reuse_addr(true);
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
                return;
            }
            lastTextMessage_ = msg->get_payload();
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
    const std::string& lastTextMessage() const { return lastTextMessage_; }

private:
    Server server_;
    std::thread thread_;
    std::string transcriptPayload_;
    std::string resolvedPayload_;
    bool closeOnEnd_ = false;
    std::string lastTextMessage_;
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

void resetRemoteServerConfig()
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
}

} // namespace

TEST(TheCubeServerAPI, FallsBackToLegacyBaseUrlAndApiKey)
{
    resetRemoteServerConfig();

    std::string seenApiKey;
    std::string seenLegacyApiKey;
    ScopedHttpServer server;
    server.server().Post("/API/llm/session", [&](const httplib::Request& req, httplib::Response& res) {
        seenApiKey = req.get_header_value("X-API-Key");
        seenLegacyApiKey = req.get_header_value("ApiKey");
        res.set_content(R"({"sessionId":"legacy-session"})", "application/json");
    });

    Config::set("REMOTE_TRANSCRIPTION_BASE_URL", "http://127.0.0.1:" + std::to_string(server.port()));
    Config::set("REMOTE_TRANSCRIPTION_API_KEY", "legacy-key");
    Config::set("REMOTE_AUTH_KEY", "");
    Config::set("REMOTE_API_KEY", "");

    TheCubeServer::TheCubeServerAPI api;
    const auto sessionId = api.createConversationSession();

    ASSERT_TRUE(sessionId.has_value());
    EXPECT_EQ(*sessionId, "legacy-session");
    EXPECT_EQ(seenApiKey, "legacy-key");
    EXPECT_EQ(seenLegacyApiKey, "legacy-key");
}

TEST(TheCubeServerAPI, PrefersRemoteServerBaseUrlAndBearerToken)
{
    resetRemoteServerConfig();

    std::string seenAuthorization;
    ScopedHttpServer server;
    server.server().Post("/API/llm/session", [&](const httplib::Request& req, httplib::Response& res) {
        seenAuthorization = req.get_header_value("Authorization");
        res.set_content(R"({"sessionId":"preferred-session"})", "application/json");
    });

    Config::set("REMOTE_SERVER_BASE_URL", "http://127.0.0.1:" + std::to_string(server.port()));
    Config::set("REMOTE_SERVER_BEARER_TOKEN", "dev-user");
    Config::set("REMOTE_TRANSCRIPTION_BASE_URL", "http://127.0.0.1:1");
    Config::set("REMOTE_TRANSCRIPTION_API_KEY", "legacy-key");
    Config::set("REMOTE_SERVER_API_KEY", "");

    TheCubeServer::TheCubeServerAPI api;
    const auto sessionId = api.createConversationSession();

    ASSERT_TRUE(sessionId.has_value());
    EXPECT_EQ(*sessionId, "preferred-session");
    EXPECT_EQ(seenAuthorization, "Bearer dev-user");
}

TEST(TheCubeServerAPI, WaitForFinalTranscriptTreatsLegacyTranscriptFramesAsTerminal)
{
    resetRemoteServerConfig();

    ScopedTranscriptWebSocketServer wsServer(R"({"transcript":"play jazz"})");
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(wsServer.port()));

    TheCubeServer::TheCubeServerAPI api;
    api.prepareVoiceTurn(
        nlohmann::json::array({
            nlohmann::json({
                { "name", "core_x2e_get_time" },
                { "intentName", "core.get_time" },
                { "description", "Get time" },
                { "parameters", {
                      { "type", "object" },
                      { "properties", nlohmann::json::object() },
                      { "additionalProperties", false }
                  } }
            })
        }),
        nlohmann::json({
            { "deviceTimezone", "America/New_York" },
            { "deviceNowEpochMs", 1742510000000LL }
        }));
    ASSERT_TRUE(api.startStreamingTranscription());

    const std::vector<int16_t> audioChunk { 1, 2, 3, 4 };
    ASSERT_TRUE(api.sendAudioChunk(audioChunk));
    ASSERT_TRUE(api.finishStreamingTranscription());

    const auto transcript = api.waitForFinalTranscript(std::chrono::milliseconds(500));
    EXPECT_EQ(transcript, "play jazz");
    EXPECT_NE(wsServer.lastTextMessage().find("\"type\":\"voice_turn_init\""), std::string::npos);
    EXPECT_NE(wsServer.lastTextMessage().find("\"sessionId\":\"voice-turn-session\""), std::string::npos);
    api.cancelStreamingTranscription();
}

TEST(TheCubeServerAPI, WaitForResolvedIntentCallUsesAudioStreamSession)
{
    resetRemoteServerConfig();

    ScopedTranscriptWebSocketServer wsServer(
        R"({"type":"transcript_final","transcript":"set an alarm for 10 pm"})",
        R"({"type":"intent_call_resolved","sessionId":"voice-turn-session","status":"matched","intentName":"core.create_alarm","arguments":{"scheduledForLocalIso":"2026-03-23T22:00:00"},"message":""})");
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(wsServer.port()));

    TheCubeServer::TheCubeServerAPI api;
    api.prepareVoiceTurn(
        nlohmann::json::array({
            nlohmann::json({
                { "name", "core_x2e_create_alarm" },
                { "intentName", "core.create_alarm" },
                { "description", "Create alarm" },
                { "parameters", {
                      { "type", "object" },
                      { "properties", {
                            { "scheduledForLocalIso", { { "type", "string" } } }
                        } },
                      { "required", nlohmann::json::array({ "scheduledForLocalIso" }) },
                      { "additionalProperties", false }
                  } }
            })
        }),
        nlohmann::json({
            { "deviceTimezone", "America/New_York" },
            { "deviceNowEpochMs", 1774212000000LL }
        }));

    ASSERT_TRUE(api.startStreamingTranscription());
    ASSERT_TRUE(api.sendAudioChunk(std::vector<int16_t> { 1, 2, 3, 4 }));
    ASSERT_TRUE(api.finishStreamingTranscription());
    EXPECT_EQ(api.waitForFinalTranscript(std::chrono::milliseconds(500)), "set an alarm for 10 pm");

    const auto resolved = api.waitForResolvedIntentCall(std::chrono::milliseconds(500));
    EXPECT_EQ(resolved.status, "matched");
    EXPECT_EQ(resolved.intentName, "core.create_alarm");
    EXPECT_EQ(resolved.arguments["scheduledForLocalIso"].get<std::string>(), "2026-03-23T22:00:00");
    api.cancelStreamingTranscription();
}

TEST(TheCubeServerAPI, StartupFailureClassifiesVoiceServiceUnavailable)
{
    resetRemoteServerConfig();

    const int port = reserveUnusedLocalPort();
    Config::set("REMOTE_SERVER_BASE_URL", "ws://127.0.0.1:" + std::to_string(port));

    TheCubeServer::TheCubeServerAPI api;
    EXPECT_FALSE(api.startStreamingTranscription());
    EXPECT_EQ(
        api.getLastVoiceFailureCategory(),
        TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE);
    EXPECT_FALSE(api.getLastVoiceFailureMessage().empty());
}

TEST(TheCubeServerAPI, GeneralAnswerRequestsUseDedicatedChatMode)
{
    resetRemoteServerConfig();

    std::string seenMode;
    std::string seenMessage;
    ScopedHttpServer server;
    server.server().Post("/API/llm/session", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"sessionId":"general-answer-session"})", "application/json");
    });
    server.server().Post("/API/llm/chat", [&](const httplib::Request& req, httplib::Response& res) {
        const auto body = nlohmann::json::parse(req.body);
        seenMode = body.value("mode", "");
        seenMessage = body.value("message", "");
        res.set_content(R"({"message":"Because shorter wavelengths scatter more strongly."})", "application/json");
    });

    Config::set("REMOTE_SERVER_BASE_URL", "http://127.0.0.1:" + std::to_string(server.port()));
    Config::set("REMOTE_SERVER_BEARER_TOKEN", "dev-user");

    TheCubeServer::TheCubeServerAPI api;
    const auto answer = api.getGeneralAnswerAsync("Why is the sky blue?").get();

    EXPECT_EQ(seenMode, "general_answer");
    EXPECT_EQ(seenMessage, "Why is the sky blue?");
    EXPECT_EQ(answer, "Because shorter wavelengths scatter more strongly.");
}

TEST(TheCubeServerAPI, ConversationSessionFailureClassifiesVoiceServiceUnavailable)
{
    resetRemoteServerConfig();

    const int port = reserveUnusedLocalPort();
    Config::set("REMOTE_SERVER_BASE_URL", "http://127.0.0.1:" + std::to_string(port));
    Config::set("REMOTE_SERVER_BEARER_TOKEN", "dev-user");

    TheCubeServer::TheCubeServerAPI api;
    const auto sessionId = api.createConversationSession();

    EXPECT_FALSE(sessionId.has_value());
    EXPECT_EQ(
        api.getLastVoiceFailureCategory(),
        TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE);
    EXPECT_FALSE(api.getLastVoiceFailureMessage().empty());
}

TEST(TheCubeServerAPI, ResolvedIntentCallRequestsUseIntentCallModeAndParseArguments)
{
    resetRemoteServerConfig();

    std::string seenMode;
    std::string seenMessage;
    nlohmann::json seenFunctions;
    nlohmann::json seenContext;
    ScopedHttpServer server;
    server.server().Post("/API/llm/session", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"sessionId":"intent-call-session"})", "application/json");
    });
    server.server().Post("/API/llm/chat", [&](const httplib::Request& req, httplib::Response& res) {
        const auto body = nlohmann::json::parse(req.body);
        seenMode = body.value("mode", "");
        seenMessage = body.value("message", "");
        seenFunctions = body.value("functions", nlohmann::json::array());
        seenContext = body.value("context", nlohmann::json::object());
        res.set_content(
            R"({"status":"matched","intentName":"core.create_alarm","arguments":{"scheduledForEpochMs":1742517660000,"title":"Alarm"}})",
            "application/json");
    });

    Config::set("REMOTE_SERVER_BASE_URL", "http://127.0.0.1:" + std::to_string(server.port()));
    Config::set("REMOTE_SERVER_BEARER_TOKEN", "dev-user");

    TheCubeServer::TheCubeServerAPI api;
    const auto resolved = api.getResolvedIntentCallAsync(
        "set an alarm for 4:41 pm today",
        nlohmann::json::array({
            nlohmann::json({
                { "name", "core_x2e_create_alarm" },
                { "intentName", "core.create_alarm" },
                { "description", "Set an alarm" },
                { "parameters", {
                      { "type", "object" },
                      { "properties", {
                            { "scheduledForEpochMs", { { "type", "integer" } } }
                        } },
                      { "required", nlohmann::json::array({ "scheduledForEpochMs" }) }
                  } }
            })
        }),
        nlohmann::json({
            { "deviceTimezone", "America/New_York" },
            { "deviceNowEpochMs", 1742510000000LL }
        }))
                              .get();

    EXPECT_EQ(seenMode, "intent_call");
    EXPECT_EQ(seenMessage, "set an alarm for 4:41 pm today");
    ASSERT_TRUE(seenFunctions.is_array());
    ASSERT_FALSE(seenFunctions.empty());
    EXPECT_EQ(seenFunctions[0]["name"].get<std::string>(), "core_x2e_create_alarm");
    EXPECT_EQ(seenFunctions[0]["intentName"].get<std::string>(), "core.create_alarm");
    EXPECT_EQ(seenContext["deviceTimezone"].get<std::string>(), "America/New_York");

    EXPECT_EQ(resolved.status, "matched");
    EXPECT_EQ(resolved.intentName, "core.create_alarm");
    EXPECT_EQ(resolved.arguments["scheduledForEpochMs"].get<long long>(), 1742517660000LL);
    EXPECT_EQ(resolved.arguments["title"].get<std::string>(), "Alarm");
}

TEST(TheCubeServerAPI, ResolvedIntentCallHttpFailureClassifiesVoiceServiceUnavailable)
{
    resetRemoteServerConfig();

    ScopedHttpServer server;
    server.server().Post("/API/llm/session", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"sessionId":"intent-call-session"})", "application/json");
    });
    server.server().Post("/API/llm/chat", [&](const httplib::Request&, httplib::Response& res) {
        res.status = 503;
        res.set_content(R"({"error":"voice service offline"})", "application/json");
    });

    Config::set("REMOTE_SERVER_BASE_URL", "http://127.0.0.1:" + std::to_string(server.port()));
    Config::set("REMOTE_SERVER_BEARER_TOKEN", "dev-user");

    TheCubeServer::TheCubeServerAPI api;
    const auto resolved = api.getResolvedIntentCallAsync(
        "what apps are installed",
        nlohmann::json::array(),
        nlohmann::json::object())
                              .get();

    EXPECT_EQ(resolved.status, "error");
    EXPECT_EQ(
        api.getLastVoiceFailureCategory(),
        TheCubeServer::TheCubeServerAPI::VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE);
    EXPECT_FALSE(api.getLastVoiceFailureMessage().empty());
}

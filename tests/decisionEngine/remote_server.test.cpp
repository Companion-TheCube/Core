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

    explicit ScopedTranscriptWebSocketServer(std::string payload)
        : payload_(std::move(payload))
    {
        server_.clear_access_channels(websocketpp::log::alevel::all);
        server_.clear_error_channels(websocketpp::log::elevel::all);
        server_.init_asio();
        server_.set_reuse_addr(true);
        server_.set_message_handler([this](websocketpp::connection_hdl hdl, Server::message_ptr msg) {
            if (msg->get_opcode() == websocketpp::frame::opcode::text && msg->get_payload() == "__END__") {
                websocketpp::lib::error_code ec;
                server_.send(hdl, payload_, websocketpp::frame::opcode::text, ec);
            }
        });

        websocketpp::lib::error_code ec;
        server_.listen(
            websocketpp::lib::asio::ip::tcp::endpoint(
                websocketpp::lib::asio::ip::address_v4::loopback(),
                0),
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
    std::string payload_;
    int port_ = -1;
};

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
    ASSERT_TRUE(api.startStreamingTranscription());

    const std::vector<int16_t> audioChunk { 1, 2, 3, 4 };
    ASSERT_TRUE(api.sendAudioChunk(audioChunk));
    ASSERT_TRUE(api.finishStreamingTranscription());

    const auto transcript = api.waitForFinalTranscript(std::chrono::milliseconds(500));
    EXPECT_EQ(transcript, "play jazz");
}

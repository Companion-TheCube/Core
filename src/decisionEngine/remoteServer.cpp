/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "remoteServer.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include "httplib.h"
#include "utils.h"

#include <chrono>
#include <condition_variable>
#include <random>
#include <sstream>
#include <thread>
#include <utility>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

using namespace TheCubeServer;

namespace {

std::string trimTrailingSlash(std::string url)
{
    while (!url.empty() && url.size() > 1 && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

bool hasScheme(const std::string& value)
{
    return value.rfind("http://", 0) == 0
        || value.rfind("https://", 0) == 0
        || value.rfind("ws://", 0) == 0
        || value.rfind("wss://", 0) == 0;
}

std::string normalizeHttpBaseUrl(std::string value)
{
    if (value.empty()) {
        return value;
    }
    if (value.rfind("ws://", 0) == 0) {
        value.replace(0, 5, "http://");
    } else if (value.rfind("wss://", 0) == 0) {
        value.replace(0, 6, "https://");
    } else if (!hasScheme(value)) {
        value = "http://" + value;
    }
    return trimTrailingSlash(std::move(value));
}

std::string normalizeWebSocketBaseUrl(std::string value)
{
    if (value.empty()) {
        return value;
    }
    if (value.rfind("http://", 0) == 0) {
        value.replace(0, 7, "ws://");
    } else if (value.rfind("https://", 0) == 0) {
        value.replace(0, 8, "wss://");
    } else if (!hasScheme(value)) {
        value = "ws://" + value;
    }
    return trimTrailingSlash(std::move(value));
}

std::string randomSessionId()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << dist(gen) << dist(gen);
    return oss.str();
}

std::string jsonString(const nlohmann::json& j, const char* key)
{
    if (!j.contains(key)) return {};
    if (j[key].is_string()) return j[key].get<std::string>();
    if (j[key].is_number_integer()) return std::to_string(j[key].get<long long>());
    if (j[key].is_number_float()) return std::to_string(j[key].get<double>());
    if (j[key].is_boolean()) return j[key].get<bool>() ? "true" : "false";
    return j[key].dump();
}

std::string parseErrorMessage(const nlohmann::json& j)
{
    if (j.contains("error")) {
        if (j["error"].is_string()) {
            return j["error"].get<std::string>();
        }
        if (j["error"].is_object()) {
            auto message = jsonString(j["error"], "message");
            if (!message.empty()) return message;
            auto code = jsonString(j["error"], "code");
            if (!code.empty()) return code;
        }
    }
    auto message = jsonString(j, "message");
    if (!message.empty()) return message;
    return j.dump();
}

std::vector<std::pair<std::string, std::string>> buildAuthHeaders(
    const std::string& bearerToken,
    const std::string& apiKey,
    const std::string& serialNumber)
{
    std::vector<std::pair<std::string, std::string>> headers;
    if (!bearerToken.empty()) {
        headers.emplace_back("Authorization", "Bearer " + bearerToken);
        return headers;
    }
    if (!apiKey.empty()) {
        headers.emplace_back("X-API-Key", apiKey);
        headers.emplace_back("ApiKey", apiKey);
        return headers;
    }
    if (!serialNumber.empty()) {
        const auto basic = base64_encode_cube(serialNumber + ":" + TheCubeServer::serialNumberToPassword(serialNumber));
        headers.emplace_back("Authorization", "Basic " + basic);
    }
    return headers;
}

void configureHttpAuth(
    httplib::Client& client,
    const std::string& bearerToken,
    const std::string& apiKey,
    const std::string& serialNumber)
{
    httplib::Headers headers = {
        { "Content-Type", "application/json" },
        { "Accept", "application/json" }
    };
    if (!apiKey.empty()) {
        headers.insert({ "X-API-Key", apiKey });
        headers.insert({ "ApiKey", apiKey });
    }
    client.set_default_headers(headers);
    if (!bearerToken.empty()) {
        client.set_bearer_token_auth(bearerToken.c_str());
    } else if (!serialNumber.empty()) {
        client.set_basic_auth(serialNumber.c_str(), TheCubeServer::serialNumberToPassword(serialNumber).c_str());
    }
}

} // namespace

class TheCubeServerAPI::HttpClientHolder {
public:
    explicit HttpClientHolder(const std::string& baseUrl)
        : client(baseUrl)
    {
        client.set_read_timeout(30, 0);
        client.set_write_timeout(30, 0);
        client.set_connection_timeout(5, 0);
    }

    httplib::Client client;
};

struct TheCubeServerAPI::WsBridge {
    using PlainClient = websocketpp::client<websocketpp::config::asio_client>;
    using TlsClient = websocketpp::client<websocketpp::config::asio_tls_client>;

    enum class Mode {
        NONE,
        PLAIN,
        TLS
    };

    Mode mode = Mode::NONE;
    std::unique_ptr<PlainClient> plainClient;
    std::unique_ptr<TlsClient> tlsClient;
    websocketpp::connection_hdl connection;
    std::thread ioThread;

    std::mutex mutex;
    std::condition_variable cv;
    bool open = false;
    bool failed = false;
    bool closed = false;
    bool finalReady = false;
    std::string errorMessage;
    std::string latestTranscript;
    std::string finalTranscript;

    ~WsBridge() { close(); }

    void resetStateLocked()
    {
        open = false;
        failed = false;
        closed = false;
        finalReady = false;
        errorMessage.clear();
        latestTranscript.clear();
        finalTranscript.clear();
    }

    void onMessage(const std::string& payload)
    {
        std::lock_guard<std::mutex> lock(mutex);
        try {
            const auto j = nlohmann::json::parse(payload);
            if (j.contains("transcript") && j["transcript"].is_string()) {
                latestTranscript = j["transcript"].get<std::string>();
                cv.notify_all();
                return;
            }
            if (j.contains("error")) {
                failed = true;
                errorMessage = parseErrorMessage(j);
                cv.notify_all();
            }
        } catch (...) {
            // Ignore non-JSON or control frames.
        }
    }

    template <typename ClientT>
    void wireHandlers(ClientT& client)
    {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
        client.start_perpetual();

        client.set_open_handler([this](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(mutex);
            connection = hdl;
            open = true;
            cv.notify_all();
        });

        client.set_fail_handler([this, &client](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(mutex);
            failed = true;
            auto con = client.get_con_from_hdl(hdl);
            errorMessage = con ? con->get_ec().message() : "WebSocket connection failed";
            cv.notify_all();
        });

        client.set_close_handler([this](websocketpp::connection_hdl) {
            std::lock_guard<std::mutex> lock(mutex);
            closed = true;
            open = false;
            if (!failed && !latestTranscript.empty()) {
                finalTranscript = latestTranscript;
                finalReady = true;
            } else if (!failed && !finalReady && errorMessage.empty()) {
                errorMessage = "WebSocket closed before transcript was produced";
            }
            cv.notify_all();
        });

        client.set_message_handler([this](websocketpp::connection_hdl, typename ClientT::message_ptr msg) {
            onMessage(msg->get_payload());
        });
    }

    bool connect(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers,
        std::chrono::milliseconds timeout)
    {
        close();

        {
            std::lock_guard<std::mutex> lock(mutex);
            resetStateLocked();
        }

        websocketpp::lib::error_code ec;
        if (url.rfind("wss://", 0) == 0) {
            mode = Mode::TLS;
            tlsClient = std::make_unique<TlsClient>();
            wireHandlers(*tlsClient);
            tlsClient->set_tls_init_handler([](websocketpp::connection_hdl) {
                auto ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
                    websocketpp::lib::asio::ssl::context::tlsv12_client);
                try {
                    ctx->set_default_verify_paths();
                    ctx->set_verify_mode(websocketpp::lib::asio::ssl::verify_none);
                } catch (...) {
                }
                return ctx;
            });

            auto con = tlsClient->get_connection(url, ec);
            if (ec || !con) {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                errorMessage = "WS connect init failed: " + ec.message();
                mode = Mode::NONE;
                return false;
            }
            for (const auto& [key, value] : headers) {
                con->append_header(key, value);
            }
            tlsClient->connect(con);
            ioThread = std::thread([this]() { tlsClient->run(); });
        } else {
            mode = Mode::PLAIN;
            plainClient = std::make_unique<PlainClient>();
            wireHandlers(*plainClient);

            auto con = plainClient->get_connection(url, ec);
            if (ec || !con) {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                errorMessage = "WS connect init failed: " + ec.message();
                mode = Mode::NONE;
                return false;
            }
            for (const auto& [key, value] : headers) {
                con->append_header(key, value);
            }
            plainClient->connect(con);
            ioThread = std::thread([this]() { plainClient->run(); });
        }

        std::unique_lock<std::mutex> lock(mutex);
        const bool signaled = cv.wait_for(lock, timeout, [this]() { return open || failed; });
        if (!signaled) {
            failed = true;
            errorMessage = "Timed out waiting for WebSocket connection";
            lock.unlock();
            close();
            return false;
        }
        if (failed) {
            lock.unlock();
            close();
            return false;
        }
        return true;
    }

    bool sendBinary(const void* data, size_t numBytes)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!open || failed || mode == Mode::NONE || !data || numBytes == 0) {
            return false;
        }

        websocketpp::lib::error_code ec;
        if (mode == Mode::TLS && tlsClient) {
            tlsClient->send(connection, data, numBytes, websocketpp::frame::opcode::binary, ec);
        } else if (mode == Mode::PLAIN && plainClient) {
            plainClient->send(connection, data, numBytes, websocketpp::frame::opcode::binary, ec);
        }
        if (ec) {
            failed = true;
            errorMessage = "WebSocket send failed: " + ec.message();
            cv.notify_all();
            return false;
        }
        return true;
    }

    bool sendText(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!open || failed || mode == Mode::NONE) {
            return false;
        }

        websocketpp::lib::error_code ec;
        if (mode == Mode::TLS && tlsClient) {
            tlsClient->send(connection, text, websocketpp::frame::opcode::text, ec);
        } else if (mode == Mode::PLAIN && plainClient) {
            plainClient->send(connection, text, websocketpp::frame::opcode::text, ec);
        }
        if (ec) {
            failed = true;
            errorMessage = "WebSocket send failed: " + ec.message();
            cv.notify_all();
            return false;
        }
        return true;
    }

    bool waitForFinal(std::chrono::milliseconds timeout, std::string& transcriptOut, std::string& errorOut)
    {
        std::unique_lock<std::mutex> lock(mutex);
        const bool signaled = cv.wait_for(lock, timeout, [this]() { return finalReady || failed || closed; });
        if (!signaled) {
            errorOut = "Timed out waiting for final transcript event";
            return false;
        }
        if (failed) {
            errorOut = errorMessage.empty() ? "WebSocket transcription stream failed" : errorMessage;
            return false;
        }
        if (finalReady) {
            transcriptOut = finalTranscript;
            return true;
        }
        if (!latestTranscript.empty()) {
            transcriptOut = latestTranscript;
            return true;
        }
        errorOut = errorMessage.empty() ? "WebSocket closed before transcript was produced" : errorMessage;
        return false;
    }

    void close()
    {
        Mode activeMode = Mode::NONE;
        {
            std::lock_guard<std::mutex> lock(mutex);
            activeMode = mode;
            open = false;
            closed = true;
        }

        websocketpp::lib::error_code ec;
        if (activeMode == Mode::TLS && tlsClient) {
            tlsClient->close(connection, websocketpp::close::status::normal, "closing", ec);
            tlsClient->stop_perpetual();
        } else if (activeMode == Mode::PLAIN && plainClient) {
            plainClient->close(connection, websocketpp::close::status::normal, "closing", ec);
            plainClient->stop_perpetual();
        }

        if (ioThread.joinable()) {
            ioThread.join();
        }

        std::lock_guard<std::mutex> lock(mutex);
        mode = Mode::NONE;
        plainClient.reset();
        tlsClient.reset();
    }
};

TheCubeServerAPI::TheCubeServerAPI(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer)
    : audioBuffer(std::move(audioBuffer))
{
    const std::string configuredBase = Config::get(
        "REMOTE_SERVER_BASE_URL",
        Config::get("REMOTE_TRANSCRIPTION_BASE_URL", "https://api.4thecube.com"));
    baseUrl = normalizeHttpBaseUrl(configuredBase);
    websocketUrl = normalizeWebSocketBaseUrl(configuredBase) + "/API/audio/stream";

    bearerToken = Config::get("REMOTE_SERVER_BEARER_TOKEN", Config::get("REMOTE_AUTH_KEY", ""));
    apiKey = Config::get(
        "REMOTE_SERVER_API_KEY",
        Config::get("REMOTE_TRANSCRIPTION_API_KEY", Config::get("REMOTE_API_KEY", "")));
    serialNumber = Config::get("REMOTE_SERVER_SERIAL", Config::get("DEVICE_SERIAL_NUMBER", ""));

    httpClient = std::make_unique<HttpClientHolder>(baseUrl);
    configureHttpAuth(httpClient->client, bearerToken, apiKey, serialNumber);
    wsBridge = std::make_unique<WsBridge>();

    services.set(3);
    initServerConnection();
}

TheCubeServerAPI::~TheCubeServerAPI()
{
    cancelStreamingTranscription();
}

bool TheCubeServerAPI::openAudioStreamLocked()
{
    if (!wsBridge) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    }

    const auto headers = buildAuthHeaders(bearerToken, apiKey, serialNumber);
    if (!wsBridge->connect(websocketUrl, headers, std::chrono::milliseconds(5000))) {
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        return false;
    }

    state = ServerState::SERVER_STATE_STREAMING;
    return true;
}

void TheCubeServerAPI::resetActiveSessionLocked()
{
    activeSession.reset();
    state = ServerState::SERVER_STATE_IDLE;
    status = ServerStatus::SERVER_STATUS_READY;
}

bool TheCubeServerAPI::startStreamingTranscription()
{
    return createTranscriptionSession();
}

bool TheCubeServerAPI::createTranscriptionSession()
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    if (status == ServerStatus::SERVER_STATUS_INITIALIZING) {
        initServerConnection();
    }
    if (status == ServerStatus::SERVER_STATUS_ERROR) {
        return false;
    }

    if (activeSession.has_value()) {
        if (wsBridge) wsBridge->close();
        activeSession.reset();
    }

    activeSession = TranscriptionSessionMeta {
        .sessionId = randomSessionId(),
        .wsUrl = websocketUrl,
        .expiresAt = "",
        .audioEncoding = "pcm16le",
        .sampleRateHz = 16000,
        .channels = 1
    };
    status = ServerStatus::SERVER_STATUS_BUSY;
    state = ServerState::SERVER_STATE_TRANSCRIBING;
    error = ServerError::SERVER_ERROR_NONE;

    if (!openAudioStreamLocked()) {
        activeSession.reset();
        return false;
    }
    return true;
}

bool TheCubeServerAPI::sendAudioChunk(std::span<const int16_t> audio)
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    if (!activeSession.has_value() || !wsBridge || audio.empty()) {
        return false;
    }
    return wsBridge->sendBinary(audio.data(), audio.size_bytes());
}

bool TheCubeServerAPI::sendAudioChunk(const int16_t* audio, size_t samples)
{
    if (!audio || samples == 0) return false;
    return sendAudioChunk(std::span<const int16_t>(audio, samples));
}

bool TheCubeServerAPI::sendAudioChunk(const std::vector<int16_t>& audio)
{
    if (audio.empty()) return false;
    return sendAudioChunk(std::span<const int16_t>(audio.data(), audio.size()));
}

bool TheCubeServerAPI::finishStreamingTranscription()
{
    return finishTranscriptionSession();
}

bool TheCubeServerAPI::finishTranscriptionSession()
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    if (!activeSession.has_value() || !wsBridge) {
        return false;
    }
    state = ServerState::SERVER_STATE_TRANSCRIBING;
    return wsBridge->sendText("__END__");
}

std::string TheCubeServerAPI::waitForFinalTranscript(std::chrono::milliseconds timeout)
{
    std::string transcript;
    std::string waitError;

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (!activeSession.has_value() || !wsBridge) {
            return {};
        }
    }

    if (!wsBridge->waitForFinal(timeout, transcript, waitError)) {
        CubeLog::error("TheCubeServerAPI: transcription wait failed: " + waitError);
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        if (wsBridge) wsBridge->close();
        activeSession.reset();
        state = ServerState::SERVER_STATE_IDLE;
        return {};
    }

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        error = transcript.empty() ? ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR : ServerError::SERVER_ERROR_NONE;
        status = transcript.empty() ? ServerStatus::SERVER_STATUS_ERROR : ServerStatus::SERVER_STATUS_READY;
        state = ServerState::SERVER_STATE_IDLE;
        if (wsBridge) wsBridge->close();
        activeSession.reset();
    }
    return transcript;
}

bool TheCubeServerAPI::cancelStreamingTranscription()
{
    return cancelTranscriptionSession();
}

bool TheCubeServerAPI::cancelTranscriptionSession()
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    if (wsBridge) {
        wsBridge->close();
    }
    activeSession.reset();
    state = ServerState::SERVER_STATE_IDLE;
    status = ServerStatus::SERVER_STATUS_READY;
    error = ServerError::SERVER_ERROR_NONE;
    return true;
}

bool TheCubeServerAPI::hasActiveTranscriptionSession() const
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    return activeSession.has_value();
}

std::optional<TheCubeServerAPI::TranscriptionSessionMeta> TheCubeServerAPI::getActiveTranscriptionSession() const
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    return activeSession;
}

std::optional<std::string> TheCubeServerAPI::createConversationSession()
{
    if (!httpClient) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        return std::nullopt;
    }

    auto res = httpClient->client.Post("/API/llm/session", "{}", "application/json");
    if (!res) {
        error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        return std::nullopt;
    }
    if (res->status != 200 && res->status != 201) {
        error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        return std::nullopt;
    }

    try {
        auto j = nlohmann::json::parse(res->body);
        auto sessionId = jsonString(j, "sessionId");
        if (sessionId.empty()) {
            sessionId = jsonString(j, "session_id");
        }
        if (sessionId.empty()) {
            return std::nullopt;
        }
        status = ServerStatus::SERVER_STATUS_READY;
        error = ServerError::SERVER_ERROR_NONE;
        return sessionId;
    } catch (...) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        return std::nullopt;
    }
}

std::future<std::string> TheCubeServerAPI::getChatResponseAsync(
    const std::string& sessionId,
    const std::string& message,
    const std::function<void(std::string)>& progressCB)
{
    return std::async(std::launch::async, [this, sessionId, message, progressCB]() {
        if (!httpClient || sessionId.empty()) {
            return std::string();
        }

        nlohmann::json payload = {
            { "sessionId", sessionId },
            { "message", message }
        };

        auto res = httpClient->client.Post("/API/llm/chat", payload.dump(), "application/json");
        if (!res) {
            error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            return std::string();
        }
        if (res->status != 200) {
            error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            return std::string();
        }

        try {
            auto j = nlohmann::json::parse(res->body);
            auto reply = jsonString(j, "message");
            if (!reply.empty() && progressCB) {
                progressCB(reply);
            }
            error = ServerError::SERVER_ERROR_NONE;
            status = ServerStatus::SERVER_STATUS_READY;
            return reply;
        } catch (...) {
            error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            return std::string();
        }
    });
}

std::future<std::string> TheCubeServerAPI::getChatResponseAsync(
    const std::string& message,
    const std::function<void(std::string)>& progressCB)
{
    return std::async(std::launch::async, [this, message, progressCB]() {
        const auto sessionId = createConversationSession();
        if (!sessionId.has_value()) {
            return std::string();
        }
        return getChatResponseAsync(*sessionId, message, progressCB).get();
    });
}

bool TheCubeServerAPI::initTranscribing()
{
    return createTranscriptionSession();
}

bool TheCubeServerAPI::streamAudio()
{
    if (!audioBuffer || audioBuffer->size() == 0) {
        return true;
    }
    auto audioOpt = audioBuffer->pop();
    if (!audioOpt || audioOpt->empty()) {
        return true;
    }
    return sendAudioChunk(*audioOpt);
}

bool TheCubeServerAPI::stopTranscribing()
{
    if (!finishTranscriptionSession()) {
        return false;
    }
    return !waitForFinalTranscript(std::chrono::milliseconds(15000)).empty();
}

bool TheCubeServerAPI::initServerConnection()
{
    if (baseUrl.empty()) {
        status = ServerStatus::SERVER_STATUS_ERROR;
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    }

    if (bearerToken.empty() && apiKey.empty() && serialNumber.empty()) {
        CubeLog::warning("TheCubeServerAPI: no remote auth configured; proceeding without auth headers");
    }

    status = ServerStatus::SERVER_STATUS_READY;
    error = ServerError::SERVER_ERROR_NONE;
    state = ServerState::SERVER_STATE_IDLE;
    return true;
}

bool TheCubeServerAPI::resetServerConnection()
{
    cancelStreamingTranscription();
    status = ServerStatus::SERVER_STATUS_INITIALIZING;
    error = ServerError::SERVER_ERROR_NONE;
    state = ServerState::SERVER_STATE_IDLE;
    return initServerConnection();
}

TheCubeServerAPI::ServerStatus TheCubeServerAPI::getServerStatus()
{
    return status;
}

TheCubeServerAPI::ServerError TheCubeServerAPI::getServerError()
{
    return error;
}

TheCubeServerAPI::ServerState TheCubeServerAPI::getServerState()
{
    return state;
}

TheCubeServerAPI::FourBit TheCubeServerAPI::getAvailableServices()
{
    return services;
}

std::string TheCubeServer::serialNumberToPassword(const std::string& serialNumber)
{
    std::string output = base64_encode_cube(serialNumber);
    return crc32(sha256(output));
}

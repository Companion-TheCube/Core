/*
██████╗ ███████╗███╗   ███╗ ██████╗ ████████╗███████╗███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗     ██████╗██████╗ ██████╗
██╔══██╗██╔════╝████╗ ████║██╔═══██╗╚══██╔══╝██╔════╝██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██████╔╝█████╗  ██╔████╔██║██║   ██║   ██║   █████╗  ███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██╗██╔══╝  ██║╚██╔╝██║██║   ██║   ██║   ██╔══╝  ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝   ██║   ███████╗███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝    ╚═╝   ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/

#include "remoteServer.h"
#include "transcriptionEvents.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include "httplib.h"
#include "utils.h"

#include <chrono>
#include <condition_variable>
#include <iomanip>
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

std::string formatDurationMs(const std::chrono::steady_clock::time_point& start, const std::chrono::steady_clock::time_point& end = std::chrono::steady_clock::now())
{
    const auto durationMs = std::chrono::duration<double, std::milli>(end - start).count();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << durationMs;
    return oss.str();
}

std::string describeHttpFailure(const httplib::Result& res, const std::string& context)
{
    if (!res) {
        return context + " request failed";
    }

    std::string description = context + " returned HTTP " + std::to_string(res->status);
    if (!res->body.empty()) {
        try {
            const auto parsed = nlohmann::json::parse(res->body);
            const auto message = parseErrorMessage(parsed);
            if (!message.empty()) {
                description += ": " + message;
            }
        } catch (...) {
            description += ": " + res->body;
        }
    }
    return description;
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
    bool sessionStarted = false;
    bool intentReady = false;
    std::string errorMessage;
    std::string latestTranscript;
    std::string finalTranscript;
    std::string serverSessionId;
    ResolvedIntentCall resolvedIntentCall;
    size_t binaryMessagesSent = 0;
    size_t binaryBytesSent = 0;
    size_t textMessagesSent = 0;

    ~WsBridge() { close(); }

    void resetStateLocked()
    {
        open = false;
        failed = false;
        closed = false;
        finalReady = false;
        sessionStarted = false;
        intentReady = false;
        errorMessage.clear();
        latestTranscript.clear();
        finalTranscript.clear();
        serverSessionId.clear();
        resolvedIntentCall = ResolvedIntentCall();
        binaryMessagesSent = 0;
        binaryBytesSent = 0;
        textMessagesSent = 0;
    }

    std::string debugSummaryLocked() const
    {
        std::ostringstream oss;
        oss << "open=" << (open ? "true" : "false")
            << ", failed=" << (failed ? "true" : "false")
            << ", closed=" << (closed ? "true" : "false")
            << ", finalReady=" << (finalReady ? "true" : "false")
            << ", sessionStarted=" << (sessionStarted ? "true" : "false")
            << ", intentReady=" << (intentReady ? "true" : "false")
            << ", binaryMessagesSent=" << binaryMessagesSent
            << ", binaryBytesSent=" << binaryBytesSent
            << ", textMessagesSent=" << textMessagesSent;
        if (!errorMessage.empty()) {
            oss << ", errorMessage=" << errorMessage;
        }
        if (!latestTranscript.empty()) {
            oss << ", latestTranscriptLength=" << latestTranscript.size();
        }
        if (!finalTranscript.empty()) {
            oss << ", finalTranscriptLength=" << finalTranscript.size();
        }
        return oss.str();
    }

    std::string debugSummary()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return debugSummaryLocked();
    }

    void onMessage(const std::string& payload)
    {
        std::lock_guard<std::mutex> lock(mutex);
        try {
            const auto j = nlohmann::json::parse(payload);
            const auto type = jsonString(j, "type");
            if (type == "voice_turn_started") {
                serverSessionId = jsonString(j, "sessionId");
                if (!serverSessionId.empty()) {
                    sessionStarted = true;
                    cv.notify_all();
                }
                return;
            }
            if (type == "transcript_partial" || type == "transcript_final") {
                const auto transcript = jsonString(j, "transcript");
                if (!transcript.empty()) {
                    latestTranscript = transcript;
                }
                std::string appendTranscript;
                if (j.contains("append") && j["append"].is_string()) {
                    appendTranscript = j["append"].get<std::string>();
                }
                const bool isFinal = (type == "transcript_final");
                if (!isFinal && !latestTranscript.empty()) {
                    CubeLog::info("TheCubeServerAPI: partial transcript received: " + latestTranscript);
                    DecisionEngine::TranscriptionEvents::publish({
                        .fullText = latestTranscript,
                        .appendText = appendTranscript,
                        .isFinal = false
                    });
                }
                if (isFinal && !latestTranscript.empty()) {
                    finalTranscript = latestTranscript;
                    finalReady = true;
                }
                cv.notify_all();
                return;
            }
            if (type == "intent_call_resolved") {
                intentReady = true;
                resolvedIntentCall.status = jsonString(j, "status");
                if (resolvedIntentCall.status.empty()) {
                    resolvedIntentCall.status = "no_match";
                }
                resolvedIntentCall.intentName = jsonString(j, "intentName");
                if (j.contains("arguments") && j["arguments"].is_object()) {
                    resolvedIntentCall.arguments = j["arguments"];
                } else {
                    resolvedIntentCall.arguments = nlohmann::json::object();
                }
                resolvedIntentCall.message = jsonString(j, "message");
                cv.notify_all();
                return;
            }
            if (type == "intent_call_error") {
                intentReady = true;
                resolvedIntentCall.status = "error";
                resolvedIntentCall.intentName.clear();
                resolvedIntentCall.arguments = nlohmann::json::object();
                resolvedIntentCall.message = jsonString(j, "message");
                cv.notify_all();
                return;
            }
            if (j.contains("transcript") && j["transcript"].is_string()) {
                latestTranscript = j["transcript"].get<std::string>();
                std::string appendTranscript;
                if (j.contains("append") && j["append"].is_string()) {
                    appendTranscript = j["append"].get<std::string>();
                }
                const bool isFinal = !j.contains("final")
                    || !j["final"].is_boolean()
                    || j["final"].get<bool>();
                if (!isFinal && !latestTranscript.empty()) {
                    CubeLog::info("TheCubeServerAPI: partial transcript received: " + latestTranscript);
                    DecisionEngine::TranscriptionEvents::publish({
                        .fullText = latestTranscript,
                        .appendText = appendTranscript,
                        .isFinal = false
                    });
                }
                if (isFinal && !latestTranscript.empty()) {
                    finalTranscript = latestTranscript;
                    finalReady = true;
                }
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
        ++binaryMessagesSent;
        binaryBytesSent += numBytes;
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
        ++textMessagesSent;
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

    bool waitForSessionStart(std::chrono::milliseconds timeout, std::string& sessionIdOut, std::string& errorOut)
    {
        std::unique_lock<std::mutex> lock(mutex);
        const bool signaled = cv.wait_for(lock, timeout, [this]() { return sessionStarted || failed || closed; });
        if (!signaled) {
            errorOut = "Timed out waiting for voice turn session bootstrap";
            return false;
        }
        if (failed) {
            errorOut = errorMessage.empty() ? "WebSocket voice turn bootstrap failed" : errorMessage;
            return false;
        }
        if (!sessionStarted || serverSessionId.empty()) {
            errorOut = errorMessage.empty() ? "WebSocket closed before voice turn session bootstrap" : errorMessage;
            return false;
        }
        sessionIdOut = serverSessionId;
        return true;
    }

    bool waitForIntent(std::chrono::milliseconds timeout, ResolvedIntentCall& intentOut, std::string& errorOut)
    {
        std::unique_lock<std::mutex> lock(mutex);
        const bool signaled = cv.wait_for(lock, timeout, [this]() { return intentReady || failed || closed; });
        if (!signaled) {
            errorOut = "Timed out waiting for resolved intent call";
            return false;
        }
        if (failed) {
            errorOut = errorMessage.empty() ? "WebSocket voice turn failed" : errorMessage;
            return false;
        }
        if (!intentReady) {
            errorOut = errorMessage.empty() ? "WebSocket closed before resolved intent call was produced" : errorMessage;
            return false;
        }
        intentOut = resolvedIntentCall;
        return true;
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
    reloadRuntimeConfig();
    wsBridge = std::make_unique<WsBridge>();

    services.set(3);
    initServerConnection();
}

TheCubeServerAPI::~TheCubeServerAPI()
{
    cancelStreamingTranscription();
}

void TheCubeServerAPI::setVoiceFailure(VoiceFailureCategory category, std::string message)
{
    std::lock_guard<std::mutex> lock(voiceFailureMutex);
    lastVoiceFailureCategory = category;
    lastVoiceFailureMessage = std::move(message);
}

void TheCubeServerAPI::clearVoiceFailure()
{
    std::lock_guard<std::mutex> lock(voiceFailureMutex);
    lastVoiceFailureCategory = VoiceFailureCategory::NONE;
    lastVoiceFailureMessage.clear();
}

bool TheCubeServerAPI::openAudioStreamLocked()
{
    const auto startupStart = std::chrono::steady_clock::now();
    if (!wsBridge) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        setVoiceFailure(VoiceFailureCategory::OTHER_VOICE_FAILURE, "Voice stream bridge is unavailable");
        return false;
    }

    const auto headers = buildAuthHeaders(bearerToken, apiKey, serialNumber);
    const auto connectStart = std::chrono::steady_clock::now();
    if (!wsBridge->connect(websocketUrl, headers, std::chrono::milliseconds(5000))) {
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            "Unable to open remote voice stream: " + wsBridge->debugSummary());
        CubeLog::error(
            "TheCubeServerAPI: voice stream websocket connect failed"
            " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
            + ", connectMs=" + formatDurationMs(connectStart)
            + ", bridge={" + wsBridge->debugSummary() + "}");
        return false;
    }
    const auto connectedAt = std::chrono::steady_clock::now();
    CubeLog::info(
        "TheCubeServerAPI: voice stream websocket connected"
        " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
        + ", connectMs=" + formatDurationMs(connectStart, connectedAt));

    std::string sessionId;
    std::string waitError;
    const auto bootstrapWaitStart = std::chrono::steady_clock::now();
    if (wsBridge->waitForSessionStart(std::chrono::milliseconds(1000), sessionId, waitError)) {
        const auto bootstrapReadyAt = std::chrono::steady_clock::now();
        if (activeSession.has_value()) {
            activeSession->sessionId = sessionId;
        }
        CubeLog::info(
            "TheCubeServerAPI: voice turn bootstrap received"
            " session=" + sessionId
            + ", bootstrapWaitMs=" + formatDurationMs(bootstrapWaitStart, bootstrapReadyAt)
            + ", sinceStartupMs=" + formatDurationMs(startupStart, bootstrapReadyAt));
    } else {
        CubeLog::warning(
            "TheCubeServerAPI: voice turn bootstrap not received"
            " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
            + ", bootstrapWaitMs=" + formatDurationMs(bootstrapWaitStart)
            + ", error=" + waitError);
    }

    const auto voiceFunctions = pendingVoiceFunctions.is_array() ? pendingVoiceFunctions : nlohmann::json::array();
    const auto voiceContext = pendingVoiceContext.is_object() ? pendingVoiceContext : nlohmann::json::object();
    nlohmann::json initPayload = {
        { "type", "voice_turn_init" },
        { "sessionId", activeSession.has_value() ? activeSession->sessionId : std::string() },
        { "functions", voiceFunctions },
        { "context", voiceContext }
    };
    const auto initSendStart = std::chrono::steady_clock::now();
    if (!wsBridge->sendText(initPayload.dump())) {
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            "Unable to initialize remote voice turn: " + wsBridge->debugSummary());
        return false;
    }
    const auto readyAt = std::chrono::steady_clock::now();
    CubeLog::info(
        "TheCubeServerAPI: voice turn stream ready"
        " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
        + ", toolCount=" + std::to_string(voiceFunctions.is_array() ? voiceFunctions.size() : 0)
        + ", hasContext=" + std::string(voiceContext.is_object() && !voiceContext.empty() ? "true" : "false"));
    CubeLog::info(
        "TheCubeServerAPI: voice turn startup timings"
        " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
        + ", connectMs=" + formatDurationMs(connectStart, connectedAt)
        + ", initSendMs=" + formatDurationMs(initSendStart, readyAt)
        + ", totalStartupMs=" + formatDurationMs(startupStart, readyAt));
    pendingVoiceFunctions = nlohmann::json::array();
    pendingVoiceContext = nlohmann::json::object();

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
    const auto sessionCreateStart = std::chrono::steady_clock::now();
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
    clearVoiceFailure();

    if (!openAudioStreamLocked()) {
        activeSession.reset();
        return false;
    }
    CubeLog::info(
        "TheCubeServerAPI: transcription session created"
        " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
        + ", createMs=" + formatDurationMs(sessionCreateStart));
    return true;
}

bool TheCubeServerAPI::sendAudioChunk(std::span<const int16_t> audio)
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    if (!activeSession.has_value() || !wsBridge || audio.empty()) {
        return false;
    }
    const bool sent = wsBridge->sendBinary(audio.data(), audio.size_bytes());
    if (!sent) {
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            "Failed while streaming audio chunk: " + wsBridge->debugSummary());
        CubeLog::error(
            "TheCubeServerAPI: sendAudioChunk failed for session="
            + activeSession->sessionId
            + ", samples=" + std::to_string(audio.size())
            + ", bytes=" + std::to_string(audio.size_bytes())
            + ", bridge={" + wsBridge->debugSummary() + "}");
    }
    return sent;
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
    const bool sent = wsBridge->sendText("__END__");
    if (!sent) {
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            "Failed while finalizing remote voice stream: " + wsBridge->debugSummary());
        CubeLog::error(
            "TheCubeServerAPI: finishTranscriptionSession failed for session="
            + activeSession->sessionId
            + ", bridge={" + wsBridge->debugSummary() + "}");
    }
    return sent;
}

void TheCubeServerAPI::prepareVoiceTurn(const nlohmann::json& functions, const nlohmann::json& context)
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);
    pendingVoiceFunctions = functions.is_array() ? functions : nlohmann::json::array();
    pendingVoiceContext = context.is_object() ? context : nlohmann::json::object();
    CubeLog::info(
        "TheCubeServerAPI: prepared voice turn"
        " toolCount=" + std::to_string(pendingVoiceFunctions.is_array() ? pendingVoiceFunctions.size() : 0)
        + ", hasContext=" + std::string(pendingVoiceContext.is_object() && !pendingVoiceContext.empty() ? "true" : "false"));
}

std::string TheCubeServerAPI::waitForFinalTranscript(std::chrono::milliseconds timeout)
{
    std::string transcript;
    std::string waitError;
    const auto waitStart = std::chrono::steady_clock::now();
    std::string sessionIdForLog = "<none>";

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (!activeSession.has_value() || !wsBridge) {
            return {};
        }
        sessionIdForLog = activeSession->sessionId;
        CubeLog::info(
            "TheCubeServerAPI: waiting for final transcript for session="
            + activeSession->sessionId
            + ", timeoutMs=" + std::to_string(timeout.count())
            + ", bridge={" + wsBridge->debugSummary() + "}");
    }

    if (!wsBridge->waitForFinal(timeout, transcript, waitError)) {
        CubeLog::error(
            "TheCubeServerAPI: transcription wait failed"
            " session=" + sessionIdForLog
            + ", waitMs=" + formatDurationMs(waitStart)
            + ", error=" + waitError);
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            waitError.empty() ? "Remote voice service did not produce a transcript" : waitError);
        if (wsBridge) wsBridge->close();
        resetActiveSessionLocked();
        return {};
    }

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (activeSession.has_value() && wsBridge) {
            CubeLog::info(
                "TheCubeServerAPI: final transcript ready for session="
                + activeSession->sessionId
                + ", transcriptLength=" + std::to_string(transcript.size())
                + ", waitMs=" + formatDurationMs(waitStart)
                + ", bridge={" + wsBridge->debugSummary() + "}");
        }
        error = transcript.empty() ? ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR : ServerError::SERVER_ERROR_NONE;
        status = transcript.empty() ? ServerStatus::SERVER_STATUS_ERROR : ServerStatus::SERVER_STATUS_BUSY;
        state = ServerState::SERVER_STATE_TRANSCRIBING;
        if (!transcript.empty()) {
            clearVoiceFailure();
        }
    }
    return transcript;
}

ResolvedIntentCall TheCubeServerAPI::waitForResolvedIntentCall(std::chrono::milliseconds timeout)
{
    ResolvedIntentCall resolved;
    std::string waitError;
    const auto waitStart = std::chrono::steady_clock::now();
    std::string sessionIdForLog = "<none>";

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (!activeSession.has_value() || !wsBridge) {
            resolved.status = "error";
            resolved.message = "no_active_voice_turn";
            return resolved;
        }
        sessionIdForLog = activeSession->sessionId;
        CubeLog::info(
            "TheCubeServerAPI: waiting for resolved intent call for session="
            + activeSession->sessionId
            + ", timeoutMs=" + std::to_string(timeout.count())
            + ", bridge={" + wsBridge->debugSummary() + "}");
    }

    if (!wsBridge->waitForIntent(timeout, resolved, waitError)) {
        CubeLog::error(
            "TheCubeServerAPI: intent wait failed"
            " session=" + sessionIdForLog
            + ", waitMs=" + formatDurationMs(waitStart)
            + ", error=" + waitError);
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            waitError.empty() ? "Remote voice service did not resolve the intent call" : waitError);
        if (wsBridge) wsBridge->close();
        resetActiveSessionLocked();
        resolved.status = "error";
        resolved.message = waitError.empty() ? "intent_resolution_failed" : waitError;
        return resolved;
    }

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        error = ServerError::SERVER_ERROR_NONE;
        status = ServerStatus::SERVER_STATUS_READY;
        state = ServerState::SERVER_STATE_IDLE;
        clearVoiceFailure();
        CubeLog::info(
            "TheCubeServerAPI: resolved intent call ready"
            " session=" + (activeSession.has_value() ? activeSession->sessionId : std::string("<none>"))
            + ", waitMs=" + formatDurationMs(waitStart)
            + ", status=" + resolved.status
            + ", intentName=" + (resolved.intentName.empty() ? std::string("<none>") : resolved.intentName));
    }
    return resolved;
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
    error = ServerError::SERVER_ERROR_NONE;
    resetActiveSessionLocked();
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
    const auto requestStart = std::chrono::steady_clock::now();
    if (!httpClient) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(VoiceFailureCategory::OTHER_VOICE_FAILURE, "Remote HTTP client is unavailable");
        return std::nullopt;
    }

    clearVoiceFailure();
    auto res = httpClient->client.Post("/API/llm/session", "{}", "application/json");
    if (!res) {
        error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            "Unable to create remote conversation session: " + httplib::to_string(res.error()));
        CubeLog::error(
            "TheCubeServerAPI: conversation session request failed"
            " durationMs=" + formatDurationMs(requestStart)
            + ", error=" + httplib::to_string(res.error()));
        return std::nullopt;
    }
    if (res->status != 200 && res->status != 201) {
        error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(
            VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
            describeHttpFailure(res, "Remote conversation session"));
        CubeLog::error(
            "TheCubeServerAPI: conversation session request returned unexpected status"
            " durationMs=" + formatDurationMs(requestStart)
            + ", status=" + std::to_string(res->status));
        return std::nullopt;
    }

    try {
        auto j = nlohmann::json::parse(res->body);
        auto sessionId = jsonString(j, "sessionId");
        if (sessionId.empty()) {
            sessionId = jsonString(j, "session_id");
        }
        if (sessionId.empty()) {
            setVoiceFailure(
                VoiceFailureCategory::OTHER_VOICE_FAILURE,
                "Remote conversation session response did not include a session identifier");
            return std::nullopt;
        }
        status = ServerStatus::SERVER_STATUS_READY;
        error = ServerError::SERVER_ERROR_NONE;
        clearVoiceFailure();
        CubeLog::info(
            "TheCubeServerAPI: conversation session created"
            " durationMs=" + formatDurationMs(requestStart)
            + ", sessionId=" + sessionId);
        return sessionId;
    } catch (...) {
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        setVoiceFailure(
            VoiceFailureCategory::OTHER_VOICE_FAILURE,
            "Remote conversation session response could not be parsed");
        CubeLog::error(
            "TheCubeServerAPI: conversation session response parse failed"
            " durationMs=" + formatDurationMs(requestStart));
        return std::nullopt;
    }
}

std::future<std::string> TheCubeServerAPI::getChatResponseAsync(
    const std::string& sessionId,
    const std::string& message,
    const std::function<void(std::string)>& progressCB)
{
    return getChatResponseWithModeAsync(sessionId, message, "", progressCB);
}

std::future<std::string> TheCubeServerAPI::getChatResponseWithModeAsync(
    const std::string& sessionId,
    const std::string& message,
    const std::string& mode,
    const std::function<void(std::string)>& progressCB)
{
    return std::async(std::launch::async, [this, sessionId, message, mode, progressCB]() {
        nlohmann::json payload = {
            { "sessionId", sessionId },
            { "message", message }
        };
        if (!mode.empty()) {
            payload["mode"] = mode;
        }

        const auto j = postChatRequestAsync(payload).get();
        if (!j.is_object()) {
            return std::string();
        }
        auto reply = jsonString(j, "message");
        if (!reply.empty() && progressCB) {
            progressCB(reply);
        }
        return reply;
    });
}

std::future<nlohmann::json> TheCubeServerAPI::postChatRequestAsync(const nlohmann::json& payload)
{
    return std::async(std::launch::async, [this, payload]() {
        if (!httpClient || !payload.contains("sessionId") || jsonString(payload, "sessionId").empty()) {
            if (!httpClient) {
                error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
                status = ServerStatus::SERVER_STATUS_ERROR;
                setVoiceFailure(VoiceFailureCategory::OTHER_VOICE_FAILURE, "Remote HTTP client is unavailable");
            }
            return nlohmann::json();
        }

        clearVoiceFailure();
        const auto requestStart = std::chrono::steady_clock::now();
        const auto mode = jsonString(payload, "mode");
        const auto sessionId = jsonString(payload, "sessionId");
        auto res = httpClient->client.Post("/API/llm/chat", payload.dump(), "application/json");
        if (!res) {
            error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            setVoiceFailure(
                VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
                "Unable to reach remote voice service: " + httplib::to_string(res.error()));
            CubeLog::error(
                "TheCubeServerAPI: chat request failed"
                " sessionId=" + (sessionId.empty() ? std::string("<none>") : sessionId)
                + ", mode=" + (mode.empty() ? std::string("<default>") : mode)
                + ", durationMs=" + formatDurationMs(requestStart)
                + ", error=" + httplib::to_string(res.error()));
            return nlohmann::json();
        }
        if (res->status != 200) {
            error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            setVoiceFailure(
                VoiceFailureCategory::VOICE_SERVICE_UNAVAILABLE,
                describeHttpFailure(res, "Remote voice service"));
            CubeLog::error(
                "TheCubeServerAPI: chat request returned unexpected status"
                " sessionId=" + (sessionId.empty() ? std::string("<none>") : sessionId)
                + ", mode=" + (mode.empty() ? std::string("<default>") : mode)
                + ", durationMs=" + formatDurationMs(requestStart)
                + ", status=" + std::to_string(res->status));
            return nlohmann::json();
        }

        try {
            auto j = nlohmann::json::parse(res->body);
            error = ServerError::SERVER_ERROR_NONE;
            status = ServerStatus::SERVER_STATUS_READY;
            clearVoiceFailure();
            CubeLog::info(
                "TheCubeServerAPI: chat request completed"
                " sessionId=" + (sessionId.empty() ? std::string("<none>") : sessionId)
                + ", mode=" + (mode.empty() ? std::string("<default>") : mode)
                + ", durationMs=" + formatDurationMs(requestStart));
            return j;
        } catch (...) {
            error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
            status = ServerStatus::SERVER_STATUS_ERROR;
            setVoiceFailure(
                VoiceFailureCategory::OTHER_VOICE_FAILURE,
                "Remote voice service returned invalid JSON");
            CubeLog::error(
                "TheCubeServerAPI: chat response parse failed"
                " sessionId=" + (sessionId.empty() ? std::string("<none>") : sessionId)
                + ", mode=" + (mode.empty() ? std::string("<default>") : mode)
                + ", durationMs=" + formatDurationMs(requestStart));
            return nlohmann::json();
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

std::future<std::string> TheCubeServerAPI::getGeneralAnswerAsync(
    const std::string& question,
    const std::function<void(std::string)>& progressCB)
{
    return std::async(std::launch::async, [this, question, progressCB]() {
        const auto sessionId = createConversationSession();
        if (!sessionId.has_value()) {
            return std::string();
        }
        return getChatResponseWithModeAsync(*sessionId, question, "general_answer", progressCB).get();
    });
}

std::future<std::string> TheCubeServerAPI::getEmotionalRewriteAsync(
    const std::string& responseText,
    const nlohmann::json& context,
    const std::function<void(std::string)>& progressCB)
{
    return std::async(std::launch::async, [this, responseText, context, progressCB]() {
        const auto sessionId = createConversationSession();
        if (!sessionId.has_value()) {
            return responseText;
        }

        nlohmann::json payload = {
            { "sessionId", *sessionId },
            { "message", responseText },
            { "mode", "emotional_rewrite" }
        };
        if (context.is_object() && !context.empty()) {
            payload["context"] = context;
        }

        const auto j = postChatRequestAsync(payload).get();
        if (!j.is_object()) {
            return responseText;
        }
        auto reply = jsonString(j, "message");
        if (reply.empty()) {
            reply = jsonString(j, "output");
        }
        if (reply.empty()) {
            return responseText;
        }
        if (progressCB) {
            progressCB(reply);
        }
        return reply;
    });
}

std::future<ResolvedIntentCall> TheCubeServerAPI::getResolvedIntentCallAsync(
    const std::string& utterance,
    const nlohmann::json& functions,
    const nlohmann::json& context)
{
    return std::async(std::launch::async, [this, utterance, functions, context]() {
        ResolvedIntentCall resolved;
        const auto sessionId = createConversationSession();
        if (!sessionId.has_value()) {
            resolved.status = "error";
            resolved.message = "session_create_failed";
            return resolved;
        }

        nlohmann::json payload = {
            { "sessionId", *sessionId },
            { "message", utterance },
            { "mode", "intent_call" },
            { "functions", functions.is_array() ? functions : nlohmann::json::array() }
        };
        if (context.is_object() && !context.empty()) {
            payload["context"] = context;
        }

        const auto j = postChatRequestAsync(payload).get();
        if (!j.is_object()) {
            resolved.status = "error";
            resolved.message = "invalid_response";
            return resolved;
        }

        resolved.status = jsonString(j, "status");
        if (resolved.status.empty()) {
            resolved.status = "no_match";
        }
        resolved.intentName = jsonString(j, "intentName");
        if (j.contains("arguments") && j["arguments"].is_object()) {
            resolved.arguments = j["arguments"];
        }
        resolved.message = jsonString(j, "message");
        return resolved;
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

void TheCubeServerAPI::reloadRuntimeConfig()
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
}

bool TheCubeServerAPI::initServerConnection()
{
    reloadRuntimeConfig();
    if (baseUrl.empty()) {
        status = ServerStatus::SERVER_STATUS_ERROR;
        error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        setVoiceFailure(VoiceFailureCategory::OTHER_VOICE_FAILURE, "Remote server base URL is empty");
        return false;
    }

    if (bearerToken.empty() && apiKey.empty() && serialNumber.empty()) {
        CubeLog::warning("TheCubeServerAPI: no remote auth configured; proceeding without auth headers");
    }

    status = ServerStatus::SERVER_STATUS_READY;
    error = ServerError::SERVER_ERROR_NONE;
    state = ServerState::SERVER_STATE_IDLE;
    clearVoiceFailure();
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

TheCubeServerAPI::VoiceFailureCategory TheCubeServerAPI::getLastVoiceFailureCategory() const
{
    std::lock_guard<std::mutex> lock(voiceFailureMutex);
    return lastVoiceFailureCategory;
}

std::string TheCubeServerAPI::getLastVoiceFailureMessage() const
{
    std::lock_guard<std::mutex> lock(voiceFailureMutex);
    return lastVoiceFailureMessage;
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

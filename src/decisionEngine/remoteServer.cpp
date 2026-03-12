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

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// TheCubeServerAPI implementation: remote transcription v1 (HTTP control +
// WebSocket PCM stream) and legacy chat endpoint support.
#include "remoteServer.h"

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <asio/ssl/context.hpp>
#include <chrono>
#include <condition_variable>
#include <random>
#include <sstream>
#include <utility>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_tls_client.hpp>

using namespace TheCubeServer;

namespace {

std::string trimTrailingSlash(std::string url)
{
    while (!url.empty() && url.size() > 1 && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

std::string jsonString(const nlohmann::json& j, const char* key)
{
    if (!j.contains(key)) return std::string();
    if (j[key].is_string()) return j[key].get<std::string>();
    if (j[key].is_number_integer()) return std::to_string(j[key].get<long long>());
    if (j[key].is_number_float()) return std::to_string(j[key].get<double>());
    if (j[key].is_boolean()) return j[key].get<bool>() ? "true" : "false";
    return j[key].dump();
}

std::string websocketUrlFromBase(const std::string& baseUrl, const std::string& sessionId)
{
    std::string out = baseUrl;
    if (out.rfind("https://", 0) == 0) {
        out.replace(0, 8, "wss://");
    } else if (out.rfind("http://", 0) == 0) {
        out.replace(0, 7, "ws://");
    }
    if (!out.empty() && out.back() == '/') out.pop_back();
    out += "/v1/transcription/sessions/" + sessionId + "/stream";
    return out;
}

} // namespace

struct TheCubeServerAPI::WsBridge {
    using PlainClient = websocketpp::client<websocketpp::config::asio_client>;
    using TlsClient = websocketpp::client<websocketpp::config::asio_tls_client>;

    enum class Mode {
        NONE,
        PLAIN,
        TLS
    };

    Mode mode = Mode::NONE;
    PlainClient plainClient;
    TlsClient tlsClient;
    websocketpp::connection_hdl connection;
    std::thread ioThread;

    std::mutex mutex;
    std::condition_variable cv;
    bool open = false;
    bool failed = false;
    bool closed = false;
    bool finalReady = false;
    std::string errorMessage;
    std::string finalTranscript;

    ~WsBridge() { close(); }

    void resetStateLocked()
    {
        open = false;
        failed = false;
        closed = false;
        finalReady = false;
        errorMessage.clear();
        finalTranscript.clear();
    }

    void onMessage(const std::string& payload)
    {
        std::lock_guard<std::mutex> lock(mutex);
        try {
            auto j = nlohmann::json::parse(payload);
            std::string type = jsonString(j, "type");
            if (type.empty()) type = jsonString(j, "event");
            if (type.empty()) type = jsonString(j, "status");

            if (type == "session_ready") {
                return;
            }

            if (type == "final" || type == "complete") {
                std::string transcript = jsonString(j, "transcript");
                if (transcript.empty()) transcript = jsonString(j, "text");
                if (transcript.empty()) transcript = jsonString(j, "message");
                if (transcript.empty() && j.contains("result") && j["result"].is_object()) {
                    transcript = jsonString(j["result"], "transcript");
                    if (transcript.empty()) transcript = jsonString(j["result"], "text");
                }
                finalTranscript = transcript;
                finalReady = true;
                cv.notify_all();
                return;
            }

            if (type == "error") {
                failed = true;
                errorMessage = jsonString(j, "message");
                if (errorMessage.empty()) errorMessage = jsonString(j, "error");
                if (errorMessage.empty()) errorMessage = payload;
                cv.notify_all();
                return;
            }

            if (type == "waiting") {
                return;
            }

            if (j.contains("status") && jsonString(j, "status") == "error") {
                failed = true;
                errorMessage = jsonString(j, "message");
                if (errorMessage.empty()) errorMessage = jsonString(j, "error");
                if (errorMessage.empty()) errorMessage = payload;
                cv.notify_all();
            }
        } catch (...) {
            // Ignore non-JSON keepalive/control frames.
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
            if (!finalReady && !failed) {
                errorMessage = "WebSocket closed before final transcript";
            }
            cv.notify_all();
        });

        client.set_message_handler([this](websocketpp::connection_hdl, typename ClientT::message_ptr msg) {
            onMessage(msg->get_payload());
        });
    }

    bool connect(const std::string& url, const std::string& token, std::chrono::milliseconds timeout)
    {
        close();

        {
            std::lock_guard<std::mutex> lock(mutex);
            resetStateLocked();
        }

        websocketpp::lib::error_code ec;
        if (url.rfind("wss://", 0) == 0) {
            mode = Mode::TLS;
            wireHandlers(tlsClient);
            tlsClient.set_tls_init_handler([](websocketpp::connection_hdl) {
                auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
                try {
                    ctx->set_default_verify_paths();
                    // Allow private/dev certs by default. Tighten in production deployment.
                    ctx->set_verify_mode(asio::ssl::verify_none);
                } catch (...) {
                    // keep default context even if verification config fails
                }
                return ctx;
            });

            auto con = tlsClient.get_connection(url, ec);
            if (ec || !con) {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                errorMessage = "WS connect init failed: " + ec.message();
                mode = Mode::NONE;
                return false;
            }
            if (!token.empty()) {
                con->append_header("Authorization", "Bearer " + token);
                con->append_header("X-Session-Token", token);
            }
            tlsClient.connect(con);
            ioThread = std::thread([this]() { tlsClient.run(); });
        } else {
            mode = Mode::PLAIN;
            wireHandlers(plainClient);

            auto con = plainClient.get_connection(url, ec);
            if (ec || !con) {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                errorMessage = "WS connect init failed: " + ec.message();
                mode = Mode::NONE;
                return false;
            }
            if (!token.empty()) {
                con->append_header("Authorization", "Bearer " + token);
                con->append_header("X-Session-Token", token);
            }
            plainClient.connect(con);
            ioThread = std::thread([this]() { plainClient.run(); });
        }

        std::unique_lock<std::mutex> lock(mutex);
        bool signaled = cv.wait_for(lock, timeout, [this]() { return open || failed; });
        if (!signaled) {
            failed = true;
            errorMessage = "Timed out waiting for WebSocket connection";
            lock.unlock();
            close();
            return false;
        }

        if (failed) {
            std::string localErr = errorMessage;
            lock.unlock();
            close();
            std::lock_guard<std::mutex> lockErr(mutex);
            failed = true;
            errorMessage = localErr;
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
        if (mode == Mode::TLS) {
            tlsClient.send(connection, data, numBytes, websocketpp::frame::opcode::binary, ec);
        } else {
            plainClient.send(connection, data, numBytes, websocketpp::frame::opcode::binary, ec);
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
        bool signaled = cv.wait_for(lock, timeout, [this]() { return finalReady || failed; });
        if (!signaled) {
            errorOut = "Timed out waiting for final transcript event";
            return false;
        }
        if (failed) {
            errorOut = errorMessage.empty() ? "WebSocket transcription stream failed" : errorMessage;
            return false;
        }
        transcriptOut = finalTranscript;
        return true;
    }

    void close()
    {
        Mode activeMode;
        {
            std::lock_guard<std::mutex> lock(mutex);
            activeMode = mode;
            open = false;
            closed = true;
        }

        websocketpp::lib::error_code ec;
        if (activeMode == Mode::TLS) {
            tlsClient.close(connection, websocketpp::close::status::normal, "closing", ec);
            tlsClient.stop_perpetual();
        } else if (activeMode == Mode::PLAIN) {
            plainClient.close(connection, websocketpp::close::status::normal, "closing", ec);
            plainClient.stop_perpetual();
        }

        if (ioThread.joinable()) ioThread.join();

        std::lock_guard<std::mutex> lock(mutex);
        mode = Mode::NONE;
    }
};

TheCubeServerAPI::TheCubeServerAPI(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer)
{
    this->audioBuffer = std::move(audioBuffer);

    this->baseUrl = trimTrailingSlash(Config::get("REMOTE_TRANSCRIPTION_BASE_URL", Config::get("REMOTE_SERVER_BASE_URL", SERVER_API_URL)));
    this->serialNumber = Config::get("DEVICE_SERIAL_NUMBER", "TESTING-SERIAL-NUMBER");
    this->apiKey = Config::get("REMOTE_TRANSCRIPTION_API_KEY", Config::get("REMOTE_API_KEY", ""));
    this->authKey = Config::get("REMOTE_AUTH_KEY", "");

    this->cli = new httplib::Client(this->baseUrl);
    this->cli->set_read_timeout(30, 0);
    this->cli->set_write_timeout(30, 0);
    this->cli->set_connection_timeout(5, 0);

    httplib::Headers headers = {
        { "Content-Type", "application/json" },
        { "Accept", "application/json" }
    };
    if (!this->apiKey.empty()) {
        headers.insert({ "X-API-Key", this->apiKey });
        headers.insert({ "ApiKey", this->apiKey }); // Legacy compatibility
    }
    this->cli->set_default_headers(headers);

    if (!this->authKey.empty()) {
        this->cli->set_bearer_token_auth(this->authKey.c_str());
    }
    if (!this->serialNumber.empty()) {
        this->cli->set_basic_auth(this->serialNumber.c_str(), serialNumberToPassword(this->serialNumber).c_str());
    }

    this->wsBridge = std::make_unique<WsBridge>();

    if (!this->initServerConnection()) {
        CubeLog::warning("TheCubeServerAPI: server initialization failed; remote transcription unavailable until config/connectivity is fixed");
    }

    CubeLog::info("TheCubeServerAPI initialized with base URL: " + this->baseUrl);
}

TheCubeServerAPI::~TheCubeServerAPI()
{
    CubeLog::info("TheCubeServerAPI closing");
    try {
        cancelTranscriptionSession();
    } catch (...) {
        CubeLog::error("TheCubeServerAPI: exception while cancelling active transcription session during shutdown");
    }
    if (this->cli) {
        delete this->cli;
        this->cli = nullptr;
    }
    CubeLog::info("TheCubeServerAPI closed");
}

bool TheCubeServerAPI::openSessionWebSocketLocked()
{
    if (!activeSession || !wsBridge) {
        CubeLog::error("openSessionWebSocketLocked: no active session");
        return false;
    }
    if (activeSession->wsUrl.empty()) {
        CubeLog::error("openSessionWebSocketLocked: ws_url is empty");
        return false;
    }

    if (!wsBridge->connect(activeSession->wsUrl, activeSession->wsToken, std::chrono::milliseconds(5000))) {
        CubeLog::error("TheCubeServerAPI: failed to connect transcription WebSocket for session " + activeSession->sessionId);
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        return false;
    }

    CubeLog::info("TheCubeServerAPI: transcription WebSocket connected for session " + activeSession->sessionId);
    state = ServerState::SERVER_STATE_STREAMING;
    return true;
}

bool TheCubeServerAPI::createTranscriptionSession()
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);

    if (status != ServerStatus::SERVER_STATUS_READY) {
        CubeLog::error("createTranscriptionSession: server connection is not ready");
        return false;
    }

    if (activeSession.has_value()) {
        CubeLog::warning("createTranscriptionSession: active session exists; replacing previous session");
        if (wsBridge) wsBridge->close();
        activeSession.reset();
    }

    nlohmann::json req = {
        { "audio_format", {
            { "encoding", "pcm16le" },
            { "sample_rate_hz", 16000 },
            { "channels", 1 }
        } }
    };

    auto res = cli->Post("/v1/transcription/sessions", req.dump(), "application/json");
    if (!res) {
        CubeLog::error("createTranscriptionSession: HTTP request failed");
        error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        return false;
    }

    if (res->status != 200 && res->status != 201) {
        CubeLog::error("createTranscriptionSession: server rejected request. status=" + std::to_string(res->status));
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        return false;
    }

    TranscriptionSessionMeta session;
    try {
        auto j = nlohmann::json::parse(res->body);
        session.sessionId = jsonString(j, "session_id");
        session.wsUrl = jsonString(j, "ws_url");
        session.wsToken = jsonString(j, "ws_token");
        session.expiresAt = jsonString(j, "expires_at");

        if (j.contains("audio_format") && j["audio_format"].is_object()) {
            session.audioEncoding = jsonString(j["audio_format"], "encoding");
            if (j["audio_format"].contains("sample_rate_hz") && j["audio_format"]["sample_rate_hz"].is_number_integer()) {
                session.sampleRateHz = j["audio_format"]["sample_rate_hz"].get<int>();
            }
            if (j["audio_format"].contains("channels") && j["audio_format"]["channels"].is_number_integer()) {
                session.channels = j["audio_format"]["channels"].get<int>();
            }
        }
    } catch (const std::exception& e) {
        CubeLog::error("createTranscriptionSession: failed to parse response: " + std::string(e.what()));
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        return false;
    }

    if (session.sessionId.empty()) {
        CubeLog::error("createTranscriptionSession: missing session_id");
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        return false;
    }
    if (session.wsUrl.empty()) {
        session.wsUrl = websocketUrlFromBase(baseUrl, session.sessionId);
    }

    activeSession = session;
    sentChunkCount = 0;
    sentByteCount = 0;
    state = ServerState::SERVER_STATE_TRANSCRIBING;

    CubeLog::info("TheCubeServerAPI: transcription session created id=" + session.sessionId);
    if (!openSessionWebSocketLocked()) {
        activeSession.reset();
        return false;
    }

    error = ServerError::SERVER_ERROR_NONE;
    status = ServerStatus::SERVER_STATUS_BUSY;
    return true;
}

bool TheCubeServerAPI::sendAudioChunk(const int16_t* audio, size_t samples)
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);

    if (!activeSession.has_value()) {
        CubeLog::error("sendAudioChunk: no active transcription session");
        return false;
    }
    if (!audio || samples == 0) {
        CubeLog::warning("sendAudioChunk: empty audio payload");
        return false;
    }
    if (!wsBridge) {
        CubeLog::error("sendAudioChunk: websocket bridge unavailable");
        return false;
    }

    size_t bytes = samples * sizeof(int16_t);
    if (!wsBridge->sendBinary(audio, bytes)) {
        CubeLog::error("sendAudioChunk: failed to send audio chunk over websocket");
        error = ServerError::SERVER_ERROR_STREAMING_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        return false;
    }

    ++sentChunkCount;
    sentByteCount += bytes;
    return true;
}

bool TheCubeServerAPI::sendAudioChunk(std::span<const int16_t> audio)
{
    if (audio.empty()) return false;
    return sendAudioChunk(audio.data(), audio.size());
}

bool TheCubeServerAPI::sendAudioChunk(const std::vector<int16_t>& audio)
{
    if (audio.empty()) return false;
    return sendAudioChunk(audio.data(), audio.size());
}

bool TheCubeServerAPI::finishTranscriptionSession()
{
    std::lock_guard<std::mutex> lock(transcriptionMutex);

    if (!activeSession.has_value()) {
        CubeLog::error("finishTranscriptionSession: no active session");
        return false;
    }

    nlohmann::json req = {
        { "chunks_sent", sentChunkCount },
        { "bytes_sent", sentByteCount }
    };

    httplib::Headers headers;
    headers.emplace("Idempotency-Key", activeSession->sessionId + "-finish");

    std::string path = "/v1/transcription/sessions/" + activeSession->sessionId + "/finish";
    auto res = cli->Post(path, headers, req.dump(), "application/json");

    if (!res) {
        CubeLog::error("finishTranscriptionSession: HTTP request failed for session " + activeSession->sessionId);
        error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        return false;
    }

    if (res->status != 200 && res->status != 202 && res->status != 204 && res->status != 409) {
        CubeLog::error("finishTranscriptionSession: server rejected request. status=" + std::to_string(res->status));
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
        return false;
    }

    CubeLog::info("TheCubeServerAPI: finish sent for session " + activeSession->sessionId +
                  " chunks=" + std::to_string(sentChunkCount) +
                  " bytes=" + std::to_string(sentByteCount));

    state = ServerState::SERVER_STATE_TRANSCRIBING;
    return true;
}

std::optional<std::string> TheCubeServerAPI::parseFinalFromStatusResponse(const std::string& body) const
{
    try {
        auto j = nlohmann::json::parse(body);

        if (j.contains("type") && jsonString(j, "type") == "final") {
            std::string text = jsonString(j, "transcript");
            if (text.empty()) text = jsonString(j, "text");
            if (text.empty()) text = jsonString(j, "message");
            if (!text.empty()) return text;
        }

        if (j.contains("status") && jsonString(j, "status") == "complete") {
            std::string text = jsonString(j, "transcript");
            if (text.empty()) text = jsonString(j, "text");
            if (text.empty()) text = jsonString(j, "message");
            if (text.empty() && j.contains("result") && j["result"].is_object()) {
                text = jsonString(j["result"], "transcript");
                if (text.empty()) text = jsonString(j["result"], "text");
            }
            if (!text.empty()) return text;
        }

        if (j.contains("final") && j["final"].is_object()) {
            std::string text = jsonString(j["final"], "transcript");
            if (text.empty()) text = jsonString(j["final"], "text");
            if (!text.empty()) return text;
        }
    } catch (...) {
        return std::nullopt;
    }

    return std::nullopt;
}

std::string TheCubeServerAPI::waitForFinalTranscript(std::chrono::milliseconds timeout)
{
    std::string sessionId;
    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (!activeSession.has_value()) {
            CubeLog::error("waitForFinalTranscript: no active session");
            return std::string();
        }
        sessionId = activeSession->sessionId;
    }

    std::string finalText;
    std::string wsError;
    if (wsBridge && wsBridge->waitForFinal(timeout, finalText, wsError)) {
        if (!finalText.empty()) {
            CubeLog::info("TheCubeServerAPI: final transcript received for session " + sessionId);
        } else {
            CubeLog::warning("TheCubeServerAPI: final transcript event received but payload was empty for session " + sessionId);
        }

        {
            std::lock_guard<std::mutex> lock(transcriptionMutex);
            status = ServerStatus::SERVER_STATUS_READY;
            state = ServerState::SERVER_STATE_IDLE;
            error = finalText.empty() ? ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR : ServerError::SERVER_ERROR_NONE;
            activeSession.reset();
            sentChunkCount = 0;
            sentByteCount = 0;
        }

        if (wsBridge) wsBridge->close();
        return finalText;
    }

    if (!wsError.empty()) {
        CubeLog::warning("TheCubeServerAPI: websocket final wait failed for session " + sessionId + ": " + wsError + "; trying HTTP status fallback");
    }

    auto res = cli->Get(("/v1/transcription/sessions/" + sessionId).c_str());
    if (res && res->status == 200) {
        auto maybeText = parseFinalFromStatusResponse(res->body);
        if (maybeText.has_value()) {
            std::lock_guard<std::mutex> lock(transcriptionMutex);
            status = ServerStatus::SERVER_STATUS_READY;
            state = ServerState::SERVER_STATE_IDLE;
            error = ServerError::SERVER_ERROR_NONE;
            activeSession.reset();
            sentChunkCount = 0;
            sentByteCount = 0;
            if (wsBridge) wsBridge->close();
            return maybeText.value();
        }
    }

    CubeLog::error("TheCubeServerAPI: no final transcript available for session " + sessionId);
    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        status = ServerStatus::SERVER_STATUS_ERROR;
        state = ServerState::SERVER_STATE_IDLE;
        error = ServerError::SERVER_ERROR_TRANSCRIPTION_ERROR;
    }
    if (wsBridge) wsBridge->close();
    return std::string();
}

bool TheCubeServerAPI::cancelTranscriptionSession()
{
    std::string sessionId;
    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        if (!activeSession.has_value()) {
            if (wsBridge) wsBridge->close();
            state = ServerState::SERVER_STATE_IDLE;
            return true;
        }
        sessionId = activeSession->sessionId;
    }

    bool ok = true;
    auto res = cli->Delete(("/v1/transcription/sessions/" + sessionId).c_str());
    if (res) {
        if (res->status != 200 && res->status != 202 && res->status != 204 && res->status != 404) {
            ok = false;
            CubeLog::warning("cancelTranscriptionSession: unexpected status=" + std::to_string(res->status));
        }
    } else {
        ok = false;
        CubeLog::warning("cancelTranscriptionSession: HTTP request failed for session " + sessionId);
    }

    if (wsBridge) wsBridge->close();

    {
        std::lock_guard<std::mutex> lock(transcriptionMutex);
        activeSession.reset();
        sentChunkCount = 0;
        sentByteCount = 0;
        state = ServerState::SERVER_STATE_IDLE;
        status = ServerStatus::SERVER_STATUS_READY;
        if (!ok) error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
    }

    return ok;
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

bool TheCubeServerAPI::initTranscribing()
{
    return createTranscriptionSession();
}

bool TheCubeServerAPI::streamAudio()
{
    if (!audioBuffer) {
        CubeLog::error("streamAudio: audio buffer is null");
        return false;
    }

    if (audioBuffer->size() == 0) {
        return true;
    }

    auto audioOpt = audioBuffer->pop();
    if (!audioOpt || audioOpt->empty()) return true;
    return sendAudioChunk(*audioOpt);
}

bool TheCubeServerAPI::stopTranscribing()
{
    if (!finishTranscriptionSession()) return false;
    auto result = waitForFinalTranscript(std::chrono::milliseconds(15000));
    return !result.empty();
}

bool TheCubeServerAPI::initServerConnection()
{
    if (this->status == ServerStatus::SERVER_STATUS_READY) {
        return true;
    }

    if (this->baseUrl.empty()) {
        CubeLog::error("TheCubeServerAPI: base URL is empty");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_INTERNAL_ERROR;
        return false;
    }

    if (this->apiKey.empty()) {
        CubeLog::error("API key is empty");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_AUTHENTICATION_ERROR;
        return false;
    }

    if (!ableToCommunicateWithRemoteServer()) {
        CubeLog::error("Unable to communicate with remote server");
        this->status = ServerStatus::SERVER_STATUS_ERROR;
        this->error = ServerError::SERVER_ERROR_CONNECTION_ERROR;
        return false;
    }

    this->status = ServerStatus::SERVER_STATUS_READY;
    this->error = ServerError::SERVER_ERROR_NONE;
    this->state = ServerState::SERVER_STATE_IDLE;
    return true;
}

bool TheCubeServerAPI::resetServerConnection()
{
    CubeLog::info("Resetting server connection");
    this->error = ServerError::SERVER_ERROR_NONE;
    this->status = ServerStatus::SERVER_STATUS_INITIALIZING;
    this->state = ServerState::SERVER_STATE_IDLE;
    return this->initServerConnection();
}

bool TheCubeServerAPI::ableToCommunicateWithRemoteServer()
{
    // Connectivity check to transcription service root endpoint.
    auto res = this->cli->Get("/v1/transcription/health");
    if (res) return true;

    // If health endpoint does not exist, fall back to root probe.
    res = this->cli->Get("/");
    return static_cast<bool>(res);
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

/**
 * @brief Get the Available Services as a bitfield.
 * @return TheCubeServerAPI::FourBit A bitfield of the available services.
 * bit 0 - OpenAI
 * bit 1 - Google
 * bit 2 - Amazon
 * bit 3 - TheCubeServer
 */
TheCubeServerAPI::FourBit TheCubeServerAPI::getAvailableServices()
{
    return services;
}

/**
 * @brief Get a chat response from the server.
 *
 * @param message The message to send to the LLM. Must contain all relevant information for the LLM to generate a response.
 * @param progressCB A callback function that will be called with the progress of the chat session. (optional)
 * @return std::future<std::string> A future that will contain the chat response when it is ready. Use .get() to retrieve the response and .wait_for() to check if the response is ready.
 */
// Post a message to the chat endpoint and poll for completion, invoking
// progressCB as the server streams interim messages. Returns a future that
// resolves to the final message string (or empty on error).
std::future<std::string> TheCubeServerAPI::getChatResponseAsync(const std::string& message, const std::function<void(std::string)>& progressCB)
{
    CubeLog::info("Getting chat response async");
    // generate a random number between 1 and 1000000
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    auto r = std::to_string(dis(gen));
    // This random number gets appended to the chat endpoint to create a unique chat session.
    // This allows for multiple chat sessions to be active at the same time for the same client.
    return std::async(std::launch::async, [&, r]() {
        auto res = this->cli->Post("/chat/" + r, message, "application/json");
        if (!res) {
            CubeLog::error("Failed to get chat response");
            return std::string();
        }

        switch (res->status) {
        case 200:
            if (res->body == "waiting") {
                CubeLog::info("Chat response initiated");
            }
            break;
        case 400:
        case 401:
        case 403:
            CubeLog::error("Unauthorized");
            return std::string();
        case 418:
            CubeLog::error("Server indicates that it is a teapot. This is unusual since we are not making tea.");
            return std::string();
        case 500:
            CubeLog::error("Internal server error");
            return std::string();
        default:
            CubeLog::error("Unknown error");
            return std::string();
        }

        auto resCheck = [&, r]() {
            auto check = this->cli->Get("/chat/" + r);
            if (!check) {
                CubeLog::error("Failed to get chat response");
                return std::pair<bool, std::string>(false, "");
            }
            switch (check->status) {
            case 200: {
                CubeLog::info("Chat response retrieved");
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(check->body);
                } catch (std::exception&) {
                    if (check->body == "waiting")
                        return std::pair<bool, std::string>(true, "");
                    return std::pair<bool, std::string>(false, "");
                }
                if (j.contains("status") && j["status"] == "waiting" && j.contains("message")) {
                    progressCB(j["message"]);
                }
                if (j.contains("status") && j["status"] == "complete" && j.contains("message")) {
                    return std::pair<bool, std::string>(true, j["message"]);
                }
                return std::pair<bool, std::string>(true, "");
            }
            case 400:
            case 401:
            case 403:
                CubeLog::error("Unauthorized");
                return std::pair<bool, std::string>(false, "");
            case 418:
                CubeLog::error("Server indicates that it is a teapot. This is unusual since we are not making tea.");
                return std::pair<bool, std::string>(false, "");
            case 500:
                CubeLog::error("Internal server error");
                return std::pair<bool, std::string>(false, "");
            default:
                CubeLog::error("Unknown error");
                return std::pair<bool, std::string>(false, "");
            }
        };

        std::pair<bool, std::string> checkResult = resCheck();
        while (checkResult.first && checkResult.second.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            checkResult = resCheck();
        }

        if (!checkResult.first) {
            CubeLog::error("Failed to get chat response");
            return std::string();
        }

        return checkResult.second;
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Derive a password-like token from a device serial number by base64-encoding,
// hashing (SHA-256), and CRC32 post-processing to a shorter string.
std::string TheCubeServer::serialNumberToPassword(const std::string& serialNumber)
{
    std::string output;
    // base64 encode the serial number
    output = base64_encode_cube(serialNumber);
    // hash the base64 encoded serial number
    output = sha256(output);
    // CRC32 the hashed serial number to get something shorter and return it
    return crc32(output);
}

/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#pragma once

#include "../threadsafeQueue.h"
#include "nlohmann/json.hpp"
#include <bitset>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace TheCubeServer {

class IRemoteAudioClient {
public:
    virtual ~IRemoteAudioClient() = default;
    virtual bool startStreamingTranscription() = 0;
    virtual bool sendAudioChunk(std::span<const int16_t> audio) = 0;
    virtual bool finishStreamingTranscription() = 0;
    virtual std::string waitForFinalTranscript(std::chrono::milliseconds timeout) = 0;
    virtual bool cancelStreamingTranscription() = 0;
};

class IRemoteConversationClient {
public:
    virtual ~IRemoteConversationClient() = default;
    virtual std::optional<std::string> createConversationSession() = 0;
    virtual std::future<std::string> getChatResponseAsync(
        const std::string& sessionId,
        const std::string& message,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) = 0;
    virtual std::future<std::string> getGeneralAnswerAsync(
        const std::string& question,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) = 0;
};

class TheCubeServerAPI : public IRemoteAudioClient, public IRemoteConversationClient {
public:
    using FourBit = std::bitset<4>;

    enum class ServerStatus {
        SERVER_STATUS_INITIALIZING,
        SERVER_STATUS_READY,
        SERVER_STATUS_BUSY,
        SERVER_STATUS_ERROR
    } status = ServerStatus::SERVER_STATUS_INITIALIZING;

    enum class ServerError {
        SERVER_ERROR_NONE,
        SERVER_ERROR_CONNECTION_ERROR,
        SERVER_ERROR_AUTHENTICATION_ERROR,
        SERVER_ERROR_INTERNAL_ERROR,
        SERVER_ERROR_TRANSCRIPTION_ERROR,
        SERVER_ERROR_STREAMING_ERROR,
        SERVER_ERROR_UNKNOWN
    } error = ServerError::SERVER_ERROR_NONE;

    enum class ServerState {
        SERVER_STATE_IDLE,
        SERVER_STATE_TRANSCRIBING,
        SERVER_STATE_STREAMING
    } state = ServerState::SERVER_STATE_IDLE;

    enum class AvailableServices {
        AVAILABLE_SERVICE_NONE = 0,
        AVAILABLE_SERVICE_OPENAI = 1,
        AVAILABLE_SERVICE_GOOGLE = 2,
        AVAILABLE_SERVICE_AMAZON = 4,
        AVAILABLE_SERVICE_THECUBESERVER = 8
    };

    struct TranscriptionSessionMeta {
        std::string sessionId;
        std::string wsUrl;
        std::string expiresAt;
        std::string audioEncoding = "pcm16le";
        int sampleRateHz = 16000;
        int channels = 1;
    };

    explicit TheCubeServerAPI(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer = nullptr);
    ~TheCubeServerAPI();

    bool startStreamingTranscription() override;
    bool sendAudioChunk(std::span<const int16_t> audio) override;
    bool finishStreamingTranscription() override;
    std::string waitForFinalTranscript(std::chrono::milliseconds timeout = std::chrono::milliseconds(15000)) override;
    bool cancelStreamingTranscription() override;

    std::optional<std::string> createConversationSession() override;
    std::future<std::string> getChatResponseAsync(
        const std::string& sessionId,
        const std::string& message,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) override;
    std::future<std::string> getChatResponseAsync(
        const std::string& message,
        const std::function<void(std::string)>& progressCB = [](std::string) {});
    std::future<std::string> getGeneralAnswerAsync(
        const std::string& question,
        const std::function<void(std::string)>& progressCB = [](std::string) {}) override;

    // Compatibility wrappers while the rest of the codebase finishes moving to
    // the narrower interfaces.
    bool createTranscriptionSession();
    bool sendAudioChunk(const int16_t* audio, size_t samples);
    bool sendAudioChunk(const std::vector<int16_t>& audio);
    bool finishTranscriptionSession();
    bool cancelTranscriptionSession();
    bool hasActiveTranscriptionSession() const;
    std::optional<TranscriptionSessionMeta> getActiveTranscriptionSession() const;

    bool initTranscribing();
    bool streamAudio();
    bool stopTranscribing();

    bool initServerConnection();
    bool resetServerConnection();
    ServerStatus getServerStatus();
    ServerError getServerError();
    ServerState getServerState();
    FourBit getAvailableServices();

    FourBit services = 0;

private:
    struct WsBridge;

    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer;
    mutable std::mutex transcriptionMutex;
    std::optional<TranscriptionSessionMeta> activeSession;
    std::unique_ptr<WsBridge> wsBridge;

    std::string baseUrl;
    std::string websocketUrl;
    std::string apiKey;
    std::string bearerToken;
    std::string serialNumber;

    class HttpClientHolder;
    std::unique_ptr<HttpClientHolder> httpClient;

    bool openAudioStreamLocked();
    void resetActiveSessionLocked();
    std::future<std::string> getChatResponseWithModeAsync(
        const std::string& sessionId,
        const std::string& message,
        const std::string& mode,
        const std::function<void(std::string)>& progressCB);
};

std::string serialNumberToPassword(const std::string& serialNumber);

} // namespace TheCubeServer

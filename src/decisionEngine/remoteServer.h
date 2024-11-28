#pragma once
#ifndef REMOTESERVER_H
#define REMOTESERVER_H

#ifndef LOGGER_H
#include <logger.h>
#endif
#include "../database/cubeDB.h"
#include "../threadsafeQueue.h"
#include "globalSettings.h"
#include "nlohmann/json.hpp"
#include "utils.h"
#include <bitset>
#include <functional>
#include <future>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define SERVER_API_URL "https://api.4thecube.com"

namespace TheCubeServer {

class TheCubeServerAPI {
public:
    using FourBit = std::bitset<4>;
    enum class ServerStatus {
        SERVER_STATUS_INITIALIZING,
        SERVER_STATUS_READY,
        SERVER_STATUS_BUSY,
        SERVER_STATUS_ERROR
    } status
        = ServerStatus::SERVER_STATUS_INITIALIZING;
    enum class ServerError {
        SERVER_ERROR_NONE,
        SERVER_ERROR_CONNECTION_ERROR,
        SERVER_ERROR_AUTHENTICATION_ERROR,
        SERVER_ERROR_INTERNAL_ERROR,
        SERVER_ERROR_TRANSCRIPTION_ERROR,
        SERVER_ERROR_STREAMING_ERROR,
        SERVER_ERROR_UNKNOWN
    } error
        = ServerError::SERVER_ERROR_NONE;
    enum class ServerState {
        SERVER_STATE_IDLE,
        SERVER_STATE_TRANSCRIBING,
        SERVER_STATE_STREAMING
    } state
        = ServerState::SERVER_STATE_IDLE;
    enum class AvailableServices {
        AVAILABLE_SERVICE_NONE = 0,
        AVAILABLE_SERVICE_OPENAI = 1,
        AVAILABLE_SERVICE_GOOGLE = 2,
        AVAILABLE_SERVICE_AMAZON = 4,
        AVAILABLE_SERVICE_THECUBESERVER = 8
    };
    FourBit services = 0;
    TheCubeServerAPI(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer);
    ~TheCubeServerAPI();
    bool initTranscribing();
    bool streamAudio();
    bool stopTranscribing();
    bool initServerConnection();
    bool resetServerConnection();
    std::future<std::string> getChatResponseAsync(const std::string& message, const std::function<void(std::string)>& progressCB);
    ServerStatus getServerStatus();
    ServerError getServerError();
    ServerState getServerState();

private:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioBuffer;
    httplib::Client* cli;
    std::string apiKey;
    std::string authKey;
    std::string serialNumber;
    bool ableToCommunicateWithRemoteServer();
    FourBit getAvailableServices();
};

std::string serialNumberToPassword(const std::string& serialNumber);

}; // namespace TheCubeServer

#endif // REMOTESERVER_H

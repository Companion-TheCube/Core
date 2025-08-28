/*
██████╗ ███████╗███╗   ███╗ ██████╗ ████████╗███████╗███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗    ██╗  ██╗
██╔══██╗██╔════╝████╗ ████║██╔═══██╗╚══██╔══╝██╔════╝██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗   ██║  ██║
██████╔╝█████╗  ██╔████╔██║██║   ██║   ██║   █████╗  ███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝   ███████║
██╔══██╗██╔══╝  ██║╚██╔╝██║██║   ██║   ██║   ██╔══╝  ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝   ██║   ███████╗███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝    ╚═╝   ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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

// TheCubeServerAPI: thin client for interacting with TheCube cloud services.
//
// Capabilities
// - Establish authenticated connection (API key, bearer token, basic auth using device serial)
// - Query available services and current server status/state
// - Stream audio and request asynchronous chat responses (LLM output)
//
// Usage notes
// - getChatResponseAsync() returns a future; use wait_for()/get() to consume
// - Methods assume API keys/secrets are loaded from CubeDB and never logged
// - Thread-safety: each instance owns its own HTTP client; coordinate externally if sharing
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

// TODO: This URL should be configurable via settings or environment variable so that we
// can point to different server instances for deployment and for user implementations.
#define SERVER_API_URL "https://api.4thecube.com"

namespace TheCubeServer {

class TheCubeServerAPI {
public:
    using FourBit = std::bitset<4>;
    // High-level lifecycle status for the server connection
    enum class ServerStatus {
        SERVER_STATUS_INITIALIZING,
        SERVER_STATUS_READY,
        SERVER_STATUS_BUSY,
        SERVER_STATUS_ERROR
    } status
        = ServerStatus::SERVER_STATUS_INITIALIZING;
    // Error categories surfaced by the API client
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
    // Streaming state machine (idle/transcribing/streaming)
    enum class ServerState {
        SERVER_STATE_IDLE,
        SERVER_STATE_TRANSCRIBING,
        SERVER_STATE_STREAMING
    } state
        = ServerState::SERVER_STATE_IDLE;
    // Bitmask of services this device/account may use
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
    std::future<std::string> getChatResponseAsync(const std::string& message, const std::function<void(std::string)>& progressCB = [](std::string){});
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

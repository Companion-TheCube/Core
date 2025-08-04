#pragma once
#ifndef INTERFACE_DEFS_HPP
#define INTERFACE_DEFS_HPP


#include "../database/cubeDB.h"
#include "utils.h"
#ifndef LOGGER_H
#include <logger.h>
#endif
#ifndef API_I_H
#include "../api/api.h"

#endif
#include "cubeWhisper.h"
#include "nlohmann/json.hpp"
#include "remoteServer.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../api/autoRegister.h"
#include "../audio/audioManager.h"
#include "../threadsafeQueue.h"
#include "globalSettings.h"
#include "httplib.h"


namespace DecisionEngine {

class I_RemoteApi {
public:
    using Server = TheCubeServer::TheCubeServerAPI;
    virtual ~I_RemoteApi() = default;
    bool resetServerConnection();
    Server::ServerStatus getServerStatus();
    Server::ServerError getServerError();
    Server::ServerState getServerState();
    Server::FourBit getAvailableServices();
    void setRemoteServerAPIObject(std::shared_ptr<Server> remoteServerAPI);

protected:
    std::shared_ptr<Server> remoteServerAPI;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_AudioQueue {
public:
    virtual ~I_AudioQueue() = default;
    void setThreadSafeQueue(std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue)
    {
        this->audioQueue = audioQueue;
    }
    const std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> getThreadSafeQueue()
    {
        return audioQueue;
    }

protected:
    std::shared_ptr<ThreadSafeQueue<std::vector<int16_t>>> audioQueue;
};

/////////////////////////////////////////////////////////////////////////////////////

class I_Transcriber: public I_AudioQueue {
public:
    virtual ~I_Transcriber() = default;
    virtual std::string transcribeBuffer(const uint16_t* audio, size_t length) = 0;
    virtual std::string transcribeStream(const uint16_t* audio, size_t bufSize) = 0;
};

}

#endif // INTERFACE_DEFS_HPP
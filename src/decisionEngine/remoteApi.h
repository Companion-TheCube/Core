#pragma once
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
#include "personalityManager.h"
#include "intentRegistry.h"
#include "functionRegistry.h"

namespace DecisionEngine {

class I_RemoteApi {
public:
    using Server = TheCubeServer::TheCubeServerAPI;
    virtual ~I_RemoteApi() = default;
    bool resetServerConnection(){return remoteServerAPI->resetServerConnection();}
    Server::ServerStatus getServerStatus(){return remoteServerAPI->getServerStatus();}
    Server::ServerError getServerError(){return remoteServerAPI->getServerError();}
    Server::ServerState getServerState(){return remoteServerAPI->getServerState();}
    Server::FourBit getAvailableServices(){return remoteServerAPI->services;}
    void setRemoteServerAPIObject(std::shared_ptr<Server> remoteServerAPI){}

protected:
    std::shared_ptr<Server> remoteServerAPI;
};

}
/*
██████╗ ███████╗███╗   ███╗ ██████╗ ████████╗███████╗ █████╗ ██████╗ ██╗   ██╗  ██╗
██╔══██╗██╔════╝████╗ ████║██╔═══██╗╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██║   ██║  ██║
██████╔╝█████╗  ██╔████╔██║██║   ██║   ██║   █████╗  ███████║██████╔╝██║   ███████║
██╔══██╗██╔══╝  ██║╚██╔╝██║██║   ██║   ██║   ██╔══╝  ██╔══██║██╔═══╝ ██║   ██╔══██║
██║  ██║███████╗██║ ╚═╝ ██║╚██████╔╝   ██║   ███████╗██║  ██║██║     ██║██╗██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝    ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝
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
    void setRemoteServerAPIObject(std::shared_ptr<Server> remoteServerAPI){
        this->remoteServerAPI = std::move(remoteServerAPI);
    }

protected:
    std::shared_ptr<Server> remoteServerAPI;
};

}

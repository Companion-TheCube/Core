#ifndef REMOTESERVER_H
#define REMOTESERVER_H

#include <logger.h>
#include "utils.h"
#include <httplib.h>
#include <string>
#include <iostream>
#include <thread>
#include <functional>
#include "nlohmann/json.hpp"

class TheCubeServerAPI{
public:
    TheCubeServerAPI(uint16_t* audioBuf);
    ~TheCubeServerAPI();
    bool initTranscribing();
    bool streamAudio();
    bool stopTranscribing();
    
private:
    httplib::Client* cli;
    std::string apiKey;
};

#endif// REMOTESERVER_H

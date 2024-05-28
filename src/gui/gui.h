#pragma once
#define WIN32_LEAN_AND_MEAN
#include "renderer.h"
#include <thread>
#include "eventHandler/eventHandler.h"
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "menu/menu.h"
#include <latch>
#include "../api/api.h"

class GUI : public I_API_Interface{
public:
    GUI(CubeLog *logger);
    ~GUI();
    void eventLoop();
    void stop();
    EndPointData_t getEndpointData();
    std::vector<std::string> getEndpointNames();
    std::string getIntefaceName() const;
private:
    CubeLog *logger;
    Renderer *renderer;
    std::jthread eventLoopThread;
    EventManager *eventManager;
};

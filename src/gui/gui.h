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
#include "messageBox/messageBox.h"
#include <latch>
#include "../api/api.h"
#include <utils.h>



class GUI : public I_API_Interface{
public:
    GUI();
    ~GUI();
    void eventLoop();
    void stop();
    static void showMessageBox(std::string title, std::string message);
    // API Interface
    HttpEndPointData_t getHttpEndpointData();
    std::vector<std::pair<std::string,std::vector<std::string>>> getHttpEndpointNamesAndParams();
    std::string getIntefaceName() const;
private:
    Renderer *renderer;
    std::jthread eventLoopThread;
    EventManager *eventManager;
    static CubeMessageBox* messageBox;
};

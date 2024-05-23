#pragma once
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
#include <barrier>



class GUI{
public:
    GUI(CubeLog *logger);
    ~GUI();
    void eventLoop();
    void stop();
private:
    CubeLog *logger;
    Renderer *renderer;
    std::jthread eventLoopThread;
    EventManager *eventManager;
};

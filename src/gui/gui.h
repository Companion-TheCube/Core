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

class GUI{
public:
    GUI(CubeLog *logger);
    ~GUI();
    void eventLoop();
    void stop();
private:
    CubeLog *logger;
    Renderer *renderer;
    std::thread eventLoopThread;
    EventManager *eventManager;
};

class Clickable{
public:
    virtual void onClick(void*) = 0;
    virtual void onRightClick(void*) = 0;
};


#pragma once
#include "renderer.h"
#include <thread>
#include "eventHandler/eventHandler.h"

class GUI{
public:
    GUI(CubeLog *logger);
    ~GUI();
    void eventLoop();
private:
    CubeLog *logger;
    Renderer *renderer;
    std::thread eventLoopThread;
    EventManager *eventManager;
};
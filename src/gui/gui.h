#pragma once
#include "renderer.h"
#include <thread>
#include "eventHandler/eventHandler.h"

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

class Menu: public Clickable{
private:
    CubeLog *logger;
    bool visible;
public:
    Menu(CubeLog *logger);
    ~Menu();
    void onClick(void*);
    void onRightClick(void*);
};
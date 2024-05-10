#pragma once
#include "renderer.h"

class GUI{
public:
    GUI(CubeLog *logger);
    ~GUI();
private:
    CubeLog *logger;
    Renderer *renderer;
};
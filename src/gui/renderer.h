#pragma once
#ifndef __GLEW_H__
#include "GL/glew.h"
#endif
#include <thread>
#include "SFML/Graphics.hpp"
#include <iostream>
#include "../logger/logger.h"
#include <cmath>
#include <vector>
#include "objects.h"
#include <mutex>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include "shader.h"
#include "characterManager.h"


class Renderer{
    private:
        int thread();
        sf::Font font;
        std::thread t;
        CubeLog *logger;
        bool running = true;
        sf::RenderWindow window;
        std::vector<sf::Event> events;
    public:
        Renderer(CubeLog *logger);
        ~Renderer();
        void stop();
        std::vector<sf::Event> getEvents();
        bool getIsRunning();
        void addObject(Object *object);
};

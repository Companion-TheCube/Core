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
#include <atomic>
#include <queue>
#include <functional>
#include <condition_variable>
#include <latch>
#include <utils.h>

class Renderer{
    private:
        int thread();
        sf::Font font;
        std::thread t;
        bool running = true;
        sf::RenderWindow window;
        std::vector<sf::Event> events;
        std::vector<Object*> objects;
        Shader* shader;
        Shader* textShader;
        TaskQueue setupQueue;
        TaskQueue loopQueue;
        std::atomic<bool> ready = false;
        std::latch* latch;
    public:
        Renderer(std::latch& latch);
        ~Renderer();
        void stop();
        std::vector<sf::Event> getEvents();
        bool getIsRunning();
        void addObject(Object *object);
        Shader* getShader();
        Shader* getTextShader();
        void addSetupTask(std::function<void()> task);
        void addLoopTask(std::function<void()> task);
        void setupTasksRun();
        void loopTasksRun();
        bool isReady();
};
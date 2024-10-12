#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#ifndef __GLEW_H__
#include "GL/glew.h"
#endif
#include <thread>
#include "SFML/Graphics.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#ifndef OBJECTS_H
#include "objects.h"
#endif
#include <mutex>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#ifndef SHADER_H
#include "shader.h"
#endif
#ifndef CHARACTERMANAGER_H
#include "characterManager.h"
#endif
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

#endif// RENDERER_H

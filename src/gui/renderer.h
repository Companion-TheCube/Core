/*
██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗ ███████╗██████╗    ██╗  ██╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗██╔════╝██╔══██╗   ██║  ██║
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝█████╗  ██████╔╝   ███████║
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗██╔══╝  ██╔══██╗   ██╔══██║
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║███████╗██║  ██║██╗██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
#include <mutex>
#include <latch>
#include <utils.h>

class Renderer{
    private:
        int thread();
        std::mutex mutex;
        sf::Font font;
        std::thread t;
        bool running = true;
        bool stillRunning = true;
        sf::RenderWindow window;
        std::vector<sf::Event> events;
        std::vector<Object*> objects;
        Shader* meshShader;
        Shader* textShader;
        Shader* stencilShader;
        TaskQueue<std::function<void()>> setupQueue;
        TaskQueue<std::function<void()>> loopQueue;
        std::atomic<bool> ready = false;
        std::latch* latch;
    public:
        Renderer(){}; // Default constructor
        Renderer(std::latch& latch);
        ~Renderer();
        void stop();
        std::vector<sf::Event> getEvents();
        bool getIsRunning();
        void addObject(Object *object);
        Shader* getMeshShader();
        Shader* getTextShader();
        Shader* getStencilShader();
        void addSetupTask(std::function<void()> task);
        void addLoopTask(std::function<void()> task);
        void setupTasksRun();
        void loopTasksRun();
        bool isReady();
};

#endif// RENDERER_H

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

Copyright (c) 2026 A-McD Technology LLC

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
#ifndef __GLEW_H__
#include "GL/glew.h"
#endif
#include <thread>
#include <vector>
#ifndef OBJECTS_H
#include "objects.h"
#endif
#ifndef SHADER_H
#include "shader.h"
#endif
#ifndef CHARACTERMANAGER_H
#include "characterManager.h"
#endif
#include <atomic>
#include <functional>
#include <latch>
#include <optional>
#include <utils.h>

#include "../threadsafeQueue.h"
#include "eventHandler/cubeEvent.h"

struct GLFWwindow;

class Renderer {
private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void windowCloseCallback(GLFWwindow* window);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void pushEvent(const CubeEvent& event);
    int thread();
    std::mutex mutex;
    std::thread t;
    std::atomic<bool> running = true;
    std::atomic<bool> stillRunning = true;
    GLFWwindow* window = nullptr;
    ThreadSafeQueue<CubeEvent> eventQueue;
    std::vector<Object*> objects;
    Shader* meshShader = nullptr;
    Shader* textShader = nullptr;
    Shader* stencilShader = nullptr;
    TaskQueue<std::function<void()>> setupQueue;
    TaskQueue<std::function<void()>> loopQueue;
    std::atomic<bool> ready = false;
    std::latch* latch = nullptr;
    int framebufferWidth = 720;
    int framebufferHeight = 720;

public:
    Renderer() { };
    Renderer(std::latch& latch);
    ~Renderer();
    void stop();
    void closeEventQueue();
    std::optional<CubeEvent> popEvent();
    bool getIsRunning();
    void addObject(Object* object);
    Shader* getMeshShader();
    Shader* getTextShader();
    Shader* getStencilShader();
    void addSetupTask(std::function<void()> task);
    void addLoopTask(std::function<void()> task);
    void setupTasksRun();
    void loopTasksRun();
    bool isReady();
};

#endif // RENDERER_H

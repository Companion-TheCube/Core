/*
██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗ ███████╗██████╗     ██████╗██████╗ ██████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝█████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║███████╗██║  ██║██╗╚██████╗██║     ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "renderer.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

constexpr auto kTargetFrameDuration = std::chrono::duration<double>(1.0 / 30.0);

void glfwErrorCallback(int error, const char* description)
{
    CubeLog::error("GLFW error " + std::to_string(error) + ": " + (description != nullptr ? description : "unknown"));
}

#ifdef __linux__
void configureLinuxGlfwInitHints()
{
    // These init hints were added in newer GLFW releases. Guard them so
    // distro-provided older GLFW builds on the Pi still compile.
#if defined(GLFW_WAYLAND_LIBDECOR) && defined(GLFW_WAYLAND_DISABLE_LIBDECOR)
    glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);
#endif

#if defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_X11)
    if (std::getenv("DISPLAY") != nullptr) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }
#endif
}
#endif

}

Renderer::Renderer(std::latch& latch)
{
    this->latch = &latch;
    this->t = std::thread(&Renderer::thread, this);
}

Renderer::~Renderer()
{
    this->ready = false;
    stop();
    closeEventQueue();
    if (this->t.joinable()) {
        this->t.join();
    }
    CubeLog::info("Renderer destroyed");
}

void Renderer::stop()
{
    this->running = false;
}

void Renderer::closeEventQueue()
{
    this->eventQueue.close();
}

std::optional<CubeEvent> Renderer::popEvent()
{
    return this->eventQueue.pop();
}

void Renderer::pushEvent(const CubeEvent& event)
{
    this->eventQueue.push(event);
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }

    renderer->framebufferWidth = width;
    renderer->framebufferHeight = height;
    glViewport(0, 0, width, height);
}

void Renderer::windowCloseCallback(GLFWwindow* window)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }

    CubeEvent event;
    event.type = CubeEventType::WindowClosed;
    renderer->pushEvent(event);
}

void Renderer::keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }
    if (action != GLFW_PRESS && action != GLFW_RELEASE && action != GLFW_REPEAT) {
        return;
    }

    CubeEvent event;
    event.type = action == GLFW_RELEASE ? CubeEventType::KeyReleased : CubeEventType::KeyPressed;
    event.key = cubeKeyFromGlfwKey(key);
    event.isRepeat = action == GLFW_REPEAT;
    renderer->pushEvent(event);
}

void Renderer::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }

    CubeEvent event;
    event.type = CubeEventType::MouseMoved;
    event.x = static_cast<int>(std::lround(xpos));
    event.y = static_cast<int>(std::lround(ypos));
    renderer->pushEvent(event);
}

void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);

    CubeEvent event;
    event.type = action == GLFW_PRESS ? CubeEventType::MouseButtonPressed : CubeEventType::MouseButtonReleased;
    event.mouseButton = cubeMouseButtonFromGlfwButton(button);
    event.x = static_cast<int>(std::lround(xpos));
    event.y = static_cast<int>(std::lround(ypos));
    renderer->pushEvent(event);
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer == nullptr) {
        return;
    }

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);

    CubeEvent event;
    event.type = CubeEventType::MouseWheelScrolled;
    event.x = static_cast<int>(std::lround(xpos));
    event.y = static_cast<int>(std::lround(ypos));
    event.scrollX = xoffset;
    event.scrollY = yoffset;
    renderer->pushEvent(event);
}

int Renderer::thread()
{
    glfwSetErrorCallback(glfwErrorCallback);
#ifdef __linux__
    configureLinuxGlfwInitHints();
#endif
    if (!glfwInit()) {
        CubeLog::error("Failed to initialize GLFW");
        this->stillRunning = false;
        this->eventQueue.close();
        if (this->latch != nullptr) {
            this->latch->count_down();
        }
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    this->window = glfwCreateWindow(720, 720, "TheCube", nullptr, nullptr);
    if (this->window == nullptr) {
        CubeLog::error("Failed to create GLFW window");
        glfwTerminate();
        this->stillRunning = false;
        this->eventQueue.close();
        if (this->latch != nullptr) {
            this->latch->count_down();
        }
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(this->window, this);
    glfwSetFramebufferSizeCallback(this->window, &Renderer::framebufferSizeCallback);
    glfwSetWindowCloseCallback(this->window, &Renderer::windowCloseCallback);
    glfwSetKeyCallback(this->window, &Renderer::keyCallback);
    glfwSetCursorPosCallback(this->window, &Renderer::cursorPositionCallback);
    glfwSetMouseButtonCallback(this->window, &Renderer::mouseButtonCallback);
    glfwSetScrollCallback(this->window, &Renderer::scrollCallback);

    bool mpVisible = Config::get("SHOW_MOUSE_POINTER", "false") == "true";
    glfwSetInputMode(this->window, GLFW_CURSOR, mpVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);

    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) {
        CubeLog::error("Failed to initialize GLEW");
        glfwDestroyWindow(this->window);
        this->window = nullptr;
        glfwTerminate();
        this->stillRunning = false;
        this->eventQueue.close();
        if (this->latch != nullptr) {
            this->latch->count_down();
        }
        return EXIT_FAILURE;
    }

    glfwGetFramebufferSize(this->window, &this->framebufferWidth, &this->framebufferHeight);
    glViewport(0, 0, this->framebufferWidth, this->framebufferHeight);

    CubeLog::info("OpenGL Version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glDepthRange(0.f, 1.f);
    glClearDepth(1.f);
    glMatrixMode(GL_PROJECTION);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader edgesShader("./shaders/edges.vs", "./shaders/edges.fs");
    Shader textureShader("./shaders/text.vs", "./shaders/text.fs");
    Shader stencilShader("./shaders/menuStencil.vs", "./shaders/menuStencil.fs");
    this->meshShader = &edgesShader;
    this->textShader = &textureShader;
    this->stencilShader = &stencilShader;

    auto characterManager = std::make_shared<CharacterManager>(&edgesShader);
    characterManager->registerInterface();
    Character_generic* character = characterManager->getCharacterByName("TheCube");
    characterManager->setCharacter(character);

    this->setupTasksRun();
    CubeLog::info("Renderer initialized. Starting Loop...");
    this->ready = true;
    if (this->latch != nullptr) {
        this->latch->count_down();
    }

    auto screenMessage = new M_Text(this->textShader, "", 12, { 0, 1, 0 }, { 2, 2 });

    while (this->running.load()) {
        const auto frameStart = std::chrono::steady_clock::now();
        glfwPollEvents();

        if (!this->running.load() || glfwWindowShouldClose(this->window)) {
            break;
        }

        this->setupTasksRun();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (characterManager->getCharacter() != nullptr) {
            std::unique_lock<std::mutex> animationLock(characterManager->animationMutex);
            std::unique_lock<std::mutex> expressionLock(characterManager->expressionMutex);
            animationLock.unlock();
            expressionLock.unlock();
            characterManager->getCharacter()->draw();
            characterManager->triggerAnimationAndExpressionThreads();
        }

        if (this->running.load()) {
            this->loopTasksRun();
        }

        {
            std::lock_guard<std::mutex> lock(this->mutex);
            for (auto object : this->objects) {
                object->draw();
            }
        }

        if (CubeLog::getScreenMessage() != "") {
            screenMessage->setText(CubeLog::getScreenMessage());
            screenMessage->draw();
        }

        glfwSwapBuffers(this->window);
        std::this_thread::sleep_until(frameStart + kTargetFrameDuration);
    }

    delete screenMessage;

    if (this->window != nullptr) {
        glfwMakeContextCurrent(this->window);
        glfwDestroyWindow(this->window);
        this->window = nullptr;
    }
    glfwTerminate();

    this->stillRunning = false;
    this->eventQueue.close();
    return EXIT_SUCCESS;
}

bool Renderer::getIsRunning()
{
    return this->running.load() && this->stillRunning.load();
}

void Renderer::addObject(Object* object)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->objects.push_back(object);
}

Shader* Renderer::getMeshShader()
{
    return this->meshShader;
}

Shader* Renderer::getStencilShader()
{
    return this->stencilShader;
}

Shader* Renderer::getTextShader()
{
    return this->textShader;
}

void Renderer::addLoopTask(std::function<void()> task)
{
    this->loopQueue.push(task);
}

void Renderer::addSetupTask(std::function<void()> task)
{
    this->setupQueue.push(task);
}

void Renderer::setupTasksRun()
{
    while (this->setupQueue.size() > 0) {
        this->setupQueue.shift()();
    }
}

void Renderer::loopTasksRun()
{
    if (this->loopQueue.size() == 0) {
        return;
    }
    for (size_t i = 0; i < this->loopQueue.size(); i++) {
        this->loopQueue.peek(i)();
    }
}

bool Renderer::isReady()
{
    return this->ready.load();
}

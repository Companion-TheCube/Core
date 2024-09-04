// TODO: migrate from SFML to GLFW since we aren't using any SFML specific features
//  Path: src/gui/renderer.h
#include "renderer.h"

Renderer::Renderer(std::latch& latch)
{
    this->t = std::thread(&Renderer::thread, this);
    this->latch = &latch;
}

Renderer::~Renderer()
{
    this->ready = false;
    this->stop();
    this->t.join();
    CubeLog::info("Renderer destroyed");
}

void Renderer::stop()
{
    this->running = false;
}

int Renderer::thread()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 4.0;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None, settings);
    this->window.setVerticalSyncEnabled(true);
    this->window.setFramerateLimit(30);
    glewExperimental = GL_TRUE; // Enable full GLEW functionality
    if (GLEW_OK != glewInit()) {
        CubeLog::error("Failed to initialize GLEW");
        exit(EXIT_FAILURE);
    }
    CubeLog::info("OpenGL Version: " + std::string((char*)glGetString(GL_VERSION)));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glDepthRange(0.f, 1.f);
    glClearDepth(1.f);
    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, window.getSize().x, window.getSize().y);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _WIN32
    this->window.setMouseCursorVisible(true);
#endif
#ifdef __linux__
    this->window.setMouseCursorVisible(false);
#endif

    Shader edgesShader("./shaders/edges.vs", "./shaders/edges.fs");
    this->shader = &edgesShader;

    Shader textShader("shaders/text.vs", "shaders/text.fs");
    this->textShader = &textShader;

    auto characterManager = new CharacterManager(&edgesShader);
    Character_generic* character = characterManager->getCharacterByName("TheCube"); // TODO: this call should return a nullptr if the character is not found. Then we should throw an error.
    characterManager->setCharacter(character);
    this->setupTasksRun();
    CubeLog::info("Renderer initialized. Starting Loop...");
    this->ready = true;
    this->latch->count_down(); // Send a signal to the GUI that the renderer is ready
    // this->window.setActive();
    while (running) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);) {
            this->events.push_back(event);
        }
        this->setupTasksRun();
        this->window.setActive();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set clear color to black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear the color buffer, the depth buffer and the stencil buffer
        if (this->running)
            this->loopTasksRun();
        if (characterManager->getCharacter() != nullptr) {
            // hold here until the animation and expression threads are ready
            std::unique_lock<std::mutex> lock2(characterManager->animationMutex);
            std::unique_lock<std::mutex> lock3(characterManager->expressionMutex);
            lock2.unlock();
            lock3.unlock();

            characterManager->getCharacter()->draw();

            characterManager->triggerAnimationAndExpressionThreads();
        }
        for (auto object : this->objects) {
            object->draw();
        }
        this->window.display();
    }
    this->window.setActive(false);
    this->window.close();
    return 0;
}

std::vector<sf::Event> Renderer::getEvents()
{
    std::vector<sf::Event> events;
    for (auto event : this->events) {
        events.push_back(event);
    }
    this->events.clear();
    return events;
}

bool Renderer::getIsRunning()
{
    return this->running;
}

void Renderer::addObject(Object* object)
{
    this->objects.push_back(object);
}

Shader* Renderer::getShader()
{
    return this->shader;
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

// TODO: add one-shot tasks that run once inside the loop similar to the setup tasks and loop tasks. Will also need to remove
//  the setupTasks from the loop.

void Renderer::setupTasksRun()
{
    while (this->setupQueue.size() > 0) {
        auto task = this->setupQueue.pop();
        task();
    }
}

void Renderer::loopTasksRun()
{
    if (this->loopQueue.size() == 0)
        return;
    TaskQueue queue;
    while (this->loopQueue.size() > 0) {
        auto task = this->loopQueue.pop();
        queue.push(task);
    }
    while (queue.size() > 0) {
        auto task = queue.pop();
        task();
        this->addLoopTask(task);
    }
}

bool Renderer::isReady()
{
    return this->ready;
}
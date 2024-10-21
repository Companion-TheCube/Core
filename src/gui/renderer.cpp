// TODO: migrate from SFML to GLFW since we aren't using any SFML specific features
//  Path: src/gui/renderer.h
#include "renderer.h"

/**
 * @brief Construct a new Renderer object
 *
 * @param latch - a latch to synchronize the setup of the renderer
 */
Renderer::Renderer(std::latch& latch)
{
    this->t = std::thread(&Renderer::thread, this);
    this->latch = &latch;
}

/**
 * @brief Destroy the Renderer object. Stops the renderer thread and waits for it to join.
 *
 */
Renderer::~Renderer()
{
    this->ready = false;
    this->stop();
    this->t.join();
    CubeLog::info("Renderer destroyed");
}

/**
 * @brief Stop the renderer thread
 *
 */
void Renderer::stop()
{
    this->running = false;
}

/**
 * @brief The main renderer thread. the thread will call all the setup tasks and loop tasks in the queue, along with drawing all objects.
 *
 * @return int
 */
int Renderer::thread()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 4.0;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None, settings);
    this->window.setVerticalSyncEnabled(true);
    this->window.setFramerateLimit(30);
    // TODO: maybe one day, support a second window that renders on the second screen.
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
#ifdef PRODUCTION_BUILD
    this->window.setMouseCursorVisible(false);
#else
    this->window.setMouseCursorVisible(true);
#endif
#endif
    Shader edgesShader("./shaders/edges.vs", "./shaders/edges.fs");
    Shader textureShader("./shaders/text.vs", "./shaders/text.fs");
    Shader stencilShader("./shaders/menuStencil.vs", "./shaders/menuStencil.fs");
    this->meshShader = &edgesShader;
    this->textShader = &textureShader;
    this->stencilShader = &stencilShader;
    auto characterManager = new CharacterManager(&edgesShader);
    Character_generic* character = characterManager->getCharacterByName("TheCube"); // TODO: this call should return a nullptr if the character is not found. Then we should throw an error.
    characterManager->setCharacter(character);
    this->setupTasksRun();
    CubeLog::info("Renderer initialized. Starting Loop...");
    this->ready = true;
    this->latch->count_down(); // Send a signal to the GUI that the renderer is ready
    auto screenMessage = new M_Text(this->textShader, "", 12, { 0, 1, 0 }, { 2, 2 }); // Logger output for CubeLog::screen()
    while (running) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);)
            this->events.push_back(event);
        if (this->running)
            this->setupTasksRun();
        this->window.setActive();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set clear color to black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear the color buffer, the depth buffer and the stencil buffer
        if (this->running)
            this->loopTasksRun();
        if (characterManager->getCharacter() != nullptr) {
            // hold here until the animation and expression threads are ready
            // TODO: replace this with a CountingLatch. This will be cleaner.
            std::unique_lock<std::mutex> lock2(characterManager->animationMutex);
            std::unique_lock<std::mutex> lock3(characterManager->expressionMutex);
            lock2.unlock();
            lock3.unlock();
            characterManager->getCharacter()->draw();
            characterManager->triggerAnimationAndExpressionThreads();
        }
        for (auto object : this->objects)
            object->draw();
        if (CubeLog::getScreenMessage() != "") {
            screenMessage->setText(CubeLog::getScreenMessage());
            screenMessage->draw();
        }
        this->window.display();
    }
    this->window.setActive(false);
    this->window.close();
    return 0;
}

/**
 * @brief Get the events from the event queue
 *
 * @return std::vector<sf::Event>
 */
std::vector<sf::Event> Renderer::getEvents()
{
    std::vector<sf::Event> events;
    for (auto event : this->events) {
        events.push_back(event);
    }
    this->events.clear();
    return events;
}

/**
 * @brief Get the running status of the renderer
 *
 * @return bool
 */
bool Renderer::getIsRunning()
{
    return this->running;
}

/**
 * @brief Add an object to the renderer
 *
 * @param object - the object to add
 */
void Renderer::addObject(Object* object)
{
    this->objects.push_back(object);
}

/**
 * @brief Get the general shader object
 *
 * @return Shader*
 */
Shader* Renderer::getMeshShader()
{
    return this->meshShader;
}

/**
 * @brief Get the stencil shader object
 *
 * @return Shader*
 */
Shader* Renderer::getStencilShader()
{
    return this->stencilShader;
}

/**
 * @brief Get the text shader object
 *
 * @return Shader*
 */
Shader* Renderer::getTextShader()
{
    return this->textShader;
}

/**
 * @brief Add a task to the loop queue
 *
 * @param task - the task to add
 */
void Renderer::addLoopTask(std::function<void()> task)
{
    this->loopQueue.push(task);
}

/**
 * @brief Add a task to the setup queue
 *
 * @param task - the task to add
 */
void Renderer::addSetupTask(std::function<void()> task)
{
    this->setupQueue.push(task);
}

/**
 * @brief Run all the setup tasks in the queue
 *
 */
void Renderer::setupTasksRun()
{
    while (this->setupQueue.size() > 0)
        this->setupQueue.shift()();
}

/**
 * @brief Run all the loop tasks in the queue
 *
 */
void Renderer::loopTasksRun()
{
    if (this->loopQueue.size() == 0)
        return;
    for (size_t i = 0; i < this->loopQueue.size(); i++)
        this->loopQueue.peek(i)(); // Run the task
}

/**
 * @brief Check if the renderer is ready
 *
 * @return bool - true if the renderer is ready
 */
bool Renderer::isReady()
{
    return this->ready;
}
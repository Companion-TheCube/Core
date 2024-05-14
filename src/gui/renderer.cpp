// Path: src/gui/renderer.h
#include "renderer.h"

Renderer::Renderer(CubeLog* logger)
{
    this->t = std::thread(&Renderer::thread, this);
    this->logger = logger;
}

Renderer::~Renderer()
{
    this->stop();
    this->t.join();
    this->logger->log("Renderer destroyed", true);
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
    this->window.create(sf::VideoMode(720, 720), "TheCube", sf::Style::None, settings);
    window.setVerticalSyncEnabled(true);
    this->window.setFramerateLimit(30);
    glewExperimental = GL_TRUE; // Enable full GLEW functionality
    if (GLEW_OK != glewInit()) {
        this->logger->error("Failed to initialize GLEW");
        exit(EXIT_FAILURE);
    }
    this->logger->log("OpenGL Version: " + std::string((char*)glGetString(GL_VERSION)), true);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glDepthRange(0.f, 1.f);
    glClearDepth(1.f);
    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, window.getSize().x, window.getSize().y);
    glDepthFunc(GL_LESS);


    // this->window.setMouseCursorVisible(false);

    Shader edges("./shaders/edges.vs", "./shaders/edges.fs", logger);
    this->shader = &edges;

    // int textSize = 24;
    // if (!this->font.loadFromFile("/home/cube/.local/share/fonts/Hack-Regular.ttf")) {
    //     this->logger->error("Could not load font");
    // }

    auto characterManager = new CharacterManager(&edges, logger);
    Character* character = characterManager->getCharacterByName("TheCube");
    characterManager->setCharacter(character);
    this->logger->log("Renderer initialized. Starting Loop...", true);
    while (running) {
        for (auto event = sf::Event {}; this->window.pollEvent(event);) {
            this->events.push_back(event);
        }
        this->window.setActive();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set clear color to black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if(characterManager->getCharacter() != nullptr){
            characterManager->getCharacter()->draw();
            characterManager->getCharacter()->animateRandomFunny();
        }
        for(auto object : this->objects){
            object->draw();
        }
        this->window.display();
    }
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
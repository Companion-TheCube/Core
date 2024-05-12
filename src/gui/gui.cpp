#include "./gui.h"

GUI::GUI(CubeLog* logger)
{
    this->logger = logger;
    this->renderer = new Renderer(this->logger);
    this->eventManager = new EventManager(this->logger);
    this->eventLoopThread = std::thread(&GUI::eventLoop, this);
    // need to implement a method of waiting for hte rendering thread to be ready
    this->logger->log("GUI initialized", true);
}

GUI::~GUI()
{
    delete this->renderer;
    this->logger->log("GUI destroyed", true);
    this->eventLoopThread.join();
}

void GUI::eventLoop()
{
    // create an event handler for KeyPressed events
    int keyPressIndex = this->eventManager->createEvent("KeyPressed");
    EventHandler* keyPressHandler = this->eventManager->getEvent(keyPressIndex);
    keyPressHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        this->logger->log("Key pressed: " + std::to_string(event->key.code), true);
    });
    keyPressHandler->setName("KeyPressed");
    keyPressHandler->setEventType(sf::Event::KeyPressed);
    this->logger->log("Starting event handler loop...", true);
    while (this->renderer->getIsRunning()) {
        std::vector<sf::Event> events = this->renderer->getEvents();
        for(int i = 0; i < events.size(); i++){
            this->eventManager->triggerEvent(events[i].type);
        }
#ifdef __linux__
        usleep(1000);
#endif
#ifdef _WIN32
        Sleep(1);
#endif
    }
}
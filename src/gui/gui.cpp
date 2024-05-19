#include "./gui.h"

/**
 * @brief Construct a new GUI::GUI object
 * 
 * @param logger a CubeLog object
 */
GUI::GUI(CubeLog* logger)
{
    this->logger = logger;
    this->renderer = new Renderer(this->logger);
    while(!this->renderer->isReady()){
        #ifdef __linux__
        usleep(1000);
        #endif
        #ifdef _WIN32
        Sleep(1);
        #endif
    }
    this->eventManager = new EventManager(this->logger);
    this->eventLoopThread = std::thread(&GUI::eventLoop, this);
    // need to implement a method of waiting for hte rendering thread to be ready
    this->logger->log("GUI initialized", true);
}

/**
 * @brief Destroy the GUI::GUI object
 * 
 */
GUI::~GUI()
{
    delete this->renderer;
    this->logger->log("GUI destroyed", true);
    this->eventLoopThread.join();
}

/**
 * @brief Start the event loop. This method will run until the renderer is no longer running or stop() is called
 * 
 */
void GUI::eventLoop()
{
    // create an event handler for KeyPressed events
    int keyPressIndex = this->eventManager->createEvent("KeyPressed");
    EventHandler* keyPressHandler = this->eventManager->getEvent(keyPressIndex);
    keyPressHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if(event!=nullptr) this->logger->log("Key pressed: " + std::to_string(event->key.code), true);
        else this->logger->log("Key pressed: nullptr", true);
    });
    keyPressHandler->setName("KeyPressed");
    keyPressHandler->setEventType(sf::Event::KeyPressed);

    int keyAPressedIndex = this->eventManager->createEvent("KeyAPressed");
    EventHandler* keyAPressedHandler = this->eventManager->getEvent(keyAPressedIndex);
    keyAPressedHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if(event!=nullptr) this->logger->log("Key A pressed", true);
        else this->logger->log("Key A pressed: nullptr", true);
    });
    keyAPressedHandler->setName("KeyAPressed");
    keyAPressedHandler->setEventType(sf::Event::KeyPressed);
    keyAPressedHandler->setSpecificEventType(SpecificEventTypes::KEYPRESS_A);

    auto menu = new Menu(this->logger, "menu.txt", this->renderer->getShader());
    // menu->setVisible(false);
    this->renderer->addSetupTask([&](){
        menu->setup();
    });

    int mouseClickIndex = this->eventManager->createEvent("MouseClick");
    EventHandler* mouseClickHandler = this->eventManager->getEvent(mouseClickIndex);
    mouseClickHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if(event!=nullptr) this->logger->log("Mouse clicked at location: " + std::to_string(event->mouseButton.x) + ", " + std::to_string(event->mouseButton.y), true);
        else this->logger->log("Mouse clicked: nullptr", true);
    });
    mouseClickHandler->setName("MouseClick");
    mouseClickHandler->setEventType(sf::Event::MouseButtonPressed);

    
    
    while(!menu->isReady()){
        #ifdef __linux__
        usleep(1000);
        #endif
        #ifdef _WIN32
        Sleep(1);
        #endif
    }
    this->renderer->addLoopTask([&](){
        menu->draw();
    });
    for(auto area: menu->getClickableAreas()){
        this->eventManager->addClickableArea(area);
    }

    this->logger->log("Starting event handler loop...", true);
    while (this->renderer->getIsRunning()) {
        std::vector<EventHandler*> managerEvents = this->eventManager->getEvents();
        std::vector<sf::Event> events = this->renderer->getEvents();
        for(int i = 0; i < events.size(); i++){
            this->eventManager->triggerEvent(events[i].type, &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), events[i].type, &events[i]);
        }
#ifdef __linux__
        usleep(1000);
#endif
#ifdef _WIN32
        Sleep(1);
#endif
    }
    this->logger->log("Event handler loop stopped", true);
    delete menu;
}

/**
 * @brief Stop the event loop
 * 
 */
void GUI::stop(){
    this->renderer->stop();
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
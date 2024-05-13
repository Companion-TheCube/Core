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

    this->logger->log("Starting event handler loop...", true);
    while (this->renderer->getIsRunning()) {
        std::vector<EventHandler*> managerEvents = this->eventManager->getEvents();
        std::vector<sf::Event> events = this->renderer->getEvents();
        for(int i = 0; i < events.size(); i++){
            for(auto event : managerEvents){
                if(event->getEventType() == events[i].type){
                    this->eventManager->triggerEvent(event->getEventType(), &events[i]);
                }
            }
            for(auto event : managerEvents){
                if(event->getSpecificEventType() == events[i].key.code && events[i].type == sf::Event::KeyPressed){
                    this->eventManager->triggerEvent(event->getSpecificEventType(), &events[i]);
                }
            }
        }
#ifdef __linux__
        usleep(1000);
#endif
#ifdef _WIN32
        Sleep(1);
#endif
    }
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
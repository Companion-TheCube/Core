// TODO: Need to add a sort of status bar to the top of the screen. It should show the time and whether or not a person is detected. probably more.

#include "./gui.h"

/**
 * @brief Construct a new GUI::GUI object
 * 
 * @param logger a CubeLog object
 */
GUI::GUI()
{
    std::latch latch(1);
    this->renderer = new Renderer(latch);
    latch.wait();
    this->eventManager = new EventManager();
    this->eventLoopThread = std::jthread(&GUI::eventLoop, this);
    CubeLog::log("GUI initialized", true);
}

/**
 * @brief Destroy the GUI::GUI object
 * 
 */
GUI::~GUI()
{
    delete this->renderer;
    CubeLog::log("GUI destroyed", true);
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
        if(event!=nullptr) CubeLog::log("Key pressed: " + std::to_string(event->key.code), true);
        else CubeLog::log("Key pressed: nullptr", true);
    });
    keyPressHandler->setName("KeyPressed");
    keyPressHandler->setEventType(sf::Event::KeyPressed);

    int keyAPressedIndex = this->eventManager->createEvent("KeyAPressed");
    EventHandler* keyAPressedHandler = this->eventManager->getEvent(keyAPressedIndex);
    keyAPressedHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if(event!=nullptr) CubeLog::log("Key A pressed", true);
        else CubeLog::log("Key A pressed: nullptr", true);
    });
    keyAPressedHandler->setName("KeyAPressed");
    keyAPressedHandler->setEventType(sf::Event::KeyPressed);
    keyAPressedHandler->setSpecificEventType(SpecificEventTypes::KEYPRESS_A);

    std::latch latch(2); // make sure this accounts for all of the setup tasks
    auto menu = new Menu(this->renderer->getShader(), latch);
    messageBox = new CubeMessageBox(this->renderer->getShader(), this->renderer->getTextShader(), this->renderer, latch);
    // menu->setVisible(false);
    this->renderer->addSetupTask([&](){
        menu->setup();
        messageBox->setup();
    });
    

    // TODO: make this not visible by default. Add a method for showing message box messages that checks if the menu is visible so
    // that we don't draw on top of the menu. Then, in the loop, if the messagebox is pending, and the menu gets closed, show the messagebox.
    messageBox->setVisible(true); 

    latch.wait();
    this->renderer->addLoopTask([&](){
        menu->draw();
        messageBox->draw();
    });
    
    for(auto area: menu->getClickableAreas()){
        this->eventManager->addClickableArea(area);
    }

    CubeLog::log("Starting event handler loop...", true);
    while (this->renderer->getIsRunning()) {
        // std::vector<EventHandler*> managerEvents = this->eventManager->getEvents();
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
    CubeLog::log("Event handler loop stopped", true);
    delete menu;
    delete messageBox;
}

/**
 * @brief Stop the event loop
 * 
 */
void GUI::stop(){
    this->renderer->stop();
}

EndPointData_t GUI::getEndpointData()
{
    EndPointData_t actions;
    actions.push_back({true, [&](std::string response, EndPointParams_t params){
        // this->stop();
        std::string p = "no param";
        for(auto param : params){
            if(param.first == "text"){
                this->messageBox->setText(param.second);
                p = param.second;
            }
        }
        CubeLog::log("Endpoint stop called and message set to: " + p, true);
        return "Stop called";
    }});
    actions.push_back({true, [&](std::string response, EndPointParams_t params){
        CubeLog::log("Endpoint action 2: " + response, true);
        for(auto param : params){
            CubeLog::log(param.first + ": " + param.second, true);
        }
        return "\"Endpoint action 2\" logged";
    }});
    return actions;
}

std::vector<std::string> GUI::getEndpointNames()
{
    std::vector<std::string> names;
    names.push_back("stop");
    names.push_back("action2");
    return names;
}

std::string GUI::getIntefaceName() const
{
    return "GUI";
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
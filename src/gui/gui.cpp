// TODO: Need to add a sort of status bar to the top of the screen. It should show the time and whether or not a person is detected. probably more.
// TODO: we should monitor the CubeLog for errors and display them in the status bar. This will require a way to get the last error message from the CubeLog. <- this is done in CubeLog

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
    CubeLog::info("GUI initialized");
}

/**
 * @brief Destroy the GUI::GUI object. Deletes the renderer and joins the event loop thread
 *
 */
GUI::~GUI()
{
    delete this->renderer;
    this->eventLoopThread.join();
    CubeLog::info("GUI destroyed");
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
        if (event != nullptr)
            CubeLog::info("Key pressed: " + std::to_string(event->key.code));
        else
            CubeLog::info("Key pressed: nullptr");
    });
    keyPressHandler->setName("KeyPressed");
    keyPressHandler->setEventType(sf::Event::KeyPressed);

    int keyAPressedIndex = this->eventManager->createEvent("KeyAPressed");
    EventHandler* keyAPressedHandler = this->eventManager->getEvent(keyAPressedIndex);
    keyAPressedHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if (event != nullptr)
            CubeLog::info("Key A pressed");
        else
            CubeLog::info("Key A pressed: nullptr");
    });
    keyAPressedHandler->setName("KeyAPressed");
    keyAPressedHandler->setEventType(sf::Event::KeyPressed);
    keyAPressedHandler->setSpecificEventType(SpecificEventTypes::KEYPRESS_A);

    std::latch latch(2); // make sure this accounts for all of the setup tasks
    auto menu = new Menu(this->renderer->getShader(), latch);

    int drag_y_HandlerIndex = this->eventManager->createEvent("DRAG_Y");
    EventHandler* drag_y_Handler = this->eventManager->getEvent(drag_y_HandlerIndex);
    drag_y_Handler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if (event != nullptr)
            menu->scrollVert(event->mouseWheelScroll.delta);
        else
            CubeLog::info("Drag Y: nullptr");
    });
    drag_y_Handler->setName("DRAG_Y");
    drag_y_Handler->setEventType(sf::Event::Count);
    drag_y_Handler->setSpecificEventType(SpecificEventTypes::DRAG_Y);

    messageBox = new CubeMessageBox(this->renderer->getShader(), this->renderer->getTextShader(), this->renderer, latch);
    // menu->setVisible(false);
    this->renderer->addSetupTask([&]() {
        menu->setup();
        messageBox->setup();
    });

    // TODO: make this not visible by default. Add a method for showing message box messages that checks if the menu is visible so
    // that we don't draw on top of the menu. Then, in the loop, if the messagebox is pending, and the menu gets closed, show the messagebox.
    messageBox->setVisible(false);

    latch.wait();
    if (this->renderer->isReady() && this->renderer->getIsRunning()) {
        this->renderer->addLoopTask([&]() {
            menu->draw();
            messageBox->draw();
        });
    } else {
        CubeLog::error("Renderer is not ready or is not running");
    }

    for (auto area : menu->getClickableAreas()) {
        this->eventManager->addClickableArea(area);
    }

    CubeLog::info("Starting event handler loop...");
    while (this->renderer->getIsRunning()) {
        // std::vector<EventHandler*> managerEvents = this->eventManager->getEvents();
        std::vector<sf::Event> events = this->renderer->getEvents();
        for (int i = 0; i < events.size(); i++) {
            this->eventManager->triggerEvent(events[i].type, &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), events[i].type, &events[i]);
        }
        genericSleep(5);
    }
    CubeLog::info("Event handler loop stopped");
    delete menu;
    delete messageBox;
}

/**
 * @brief Stop the event loop
 *
 */
void GUI::stop()
{
    this->renderer->stop();
}

/**
 * @brief Get the Http Endpoint Data object
 *
 * @return HttpEndPointData_t a vector of HttpEndPointData_t objects
 */
HttpEndPointData_t GUI::getHttpEndpointData()
{
    HttpEndPointData_t actions;
    actions.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            // this->stop();
            std::string p = "no param";
            for (auto param : req.params) {
                if (param.first == "text") {
                    this->messageBox->setText(param.second);
                    p = param.second;
                }
            }
            CubeLog::info("Endpoint stop called and message set to: " + p);
            return "Stop called";
        } });
    actions.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            std::string paramsString;
            for (auto param : req.params) {
                paramsString += param.first + ": " + param.second + "\n";
            }
            CubeLog::info("Endpoint action 2: \n");
            CubeLog::info(paramsString);
            return "\"Endpoint action 2\" logged";
        } });
    return actions;
}

/**
 * @brief Get the Http Endpoint Names And Params object
 *
 * @return std::vector<std::pair<std::string, std::vector<std::string>>> a vector of pairs of strings and vectors of strings
 */
std::vector<std::pair<std::string, std::vector<std::string>>> GUI::getHttpEndpointNamesAndParams()
{
    std::vector<std::pair<std::string, std::vector<std::string>>> names;
    std::vector<std::string> stopParams;
    stopParams.push_back("text");
    names.push_back({ "stop", stopParams });
    names.push_back({ "action2", {} });
    return names;
}

/**
 * @brief Get the Inteface Name
 *
 * @return std::string the name of the interface
 */
std::string GUI::getIntefaceName() const
{
    return "GUI";
}
// TODO: Need to add a sort of status bar to the top of the screen. It should show the time and whether or not a person is detected. probably more.
// TODO: we should monitor the CubeLog for errors and display them in the status bar. This will require a way to get the last error message from the CubeLog. <- this is done in CubeLog

#include "./gui.h"

CubeMessageBox* GUI::messageBox = nullptr;

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
    // keyPressHandler->setName("KeyPressed");
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
    // keyAPressedHandler->setName("KeyAPressed");
    keyAPressedHandler->setEventType(sf::Event::KeyPressed);
    keyAPressedHandler->setSpecificEventType(SpecificEventTypes::KEYPRESS_A);

    ////////////////////////////////////////
    /// TESTING FUNCTION
    ////////////////////////////////////////
    auto repeatChar = [](std::string& inOut, int n, std::string repeatChars) {
        for (int i = 0; i < n; i++) {
            inOut += repeatChars;
        }
        return inOut;
    };

    ////////////////////////////////////////
    /// Here we build the menus
    ////////////////////////////////////////
    // std::vector<Menu*> menus;
    // std::latch latch(3); // make sure this accounts for all of the setup tasks
    CountingLatch countingLatch(1);

    ///////// Submenu /////////
    countingLatch.count_up();
    auto testSubMenu = new Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    testSubMenu->setMenuName("Test Sub Menu");
    testSubMenu->setVisible(false);
    menus.push_back(testSubMenu);
    drag_y_actions.push_back({ [&]() { return testSubMenu->getVisible(); }, [&](int y) { testSubMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        testSubMenu->addMenuEntry("< Back", [&](void* data) {
            CubeLog::info("Back clicked");
            testSubMenu->setVisible(false);
            if (testSubMenu->getParentMenu() != nullptr) {
                testSubMenu->getParentMenu()->setVisible(true);
            }
        });
        testSubMenu->addHorizontalRule();
        testSubMenu->addMenuEntry("Test Sub Menu Entry 1", [&](void*) { CubeLog::info("Test Sub Menu Entry 1 clicked"); });
        testSubMenu->addMenuEntry("Test Sub Menu Entry 2", [&](void*) { CubeLog::info("Test Sub Menu Entry 2 clicked"); });
        testSubMenu->setup();
        testSubMenu->setVisible(false);
    });
    this->renderer->addLoopTask([&]() {
        testSubMenu->draw();
    });

    // addMenu("Test Menu 2", "Main Menu", {"Test Menu 2 Entry 1", "Test Menu 2 Entry 2", "Test Menu 2 Entry 3"}, {"Test Menu 2 Entry 1", "Test Menu 2 Entry 2", "Test Menu 2 Entry 3"});
    // addMenu("Test Menu 3", "Test Menu 1", {"Test Menu 3 Entry 1", "Test Menu 3 Entry 2", "Test Menu 3 Entry 3"}, {"Test Menu 3 Entry 1", "Test Menu 3 Entry 2", "Test Menu 3 Entry 3"});

    ///////// Main Menu /////////
    auto mainMenu = new Menu(this->renderer, countingLatch);
    testSubMenu->setParentMenu(mainMenu);
    mainMenu->setMenuName("Main Menu");

    menus.push_back(mainMenu);
    drag_y_actions.push_back({ [&]() { return mainMenu->getVisible(); }, [&](int y) { mainMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        mainMenu->addMenuEntry("< Settings", [&](void* data) {
            CubeLog::info("Settings clicked");
            mainMenu->setVisible(false);
            mainMenu->setIsClickable(true);
        });
        mainMenu->addHorizontalRule();
        mainMenu->addMenuEntry("Test Sub Menu", [&](void* data) {
            CubeLog::info("Test Sub Menu clicked");
            testSubMenu->setVisible(true);
            mainMenu->setVisible(false);
            mainMenu->setIsClickable(false);
        });
        mainMenu->addMenuEntry("Test Menu Entry 1", [&](void*) { CubeLog::info("Test Menu Entry 1 clicked"); });
        for (int i = 0; i < 25; i++) {
            std::string s = " - repeats: ";
            repeatChar(s, i, "a ");
            mainMenu->addMenuEntry("Test Menu Entry " + std::to_string(i) + s, [&, i](void*) { CubeLog::info("Test Menu Entry " + std::to_string(i) + " clicked"); });
        }
        mainMenu->setup();
        mainMenu->setVisible(false);
        mainMenu->setAsMainMenu(); // No other menus should call this method
        mainMenu->setIsClickable(true);
    });
    this->renderer->addLoopTask([&]() {
        mainMenu->draw();
    });

    addMenu("Test Menu 1", "Main Menu", { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, countingLatch);

    ////////////////////////////////////////
    /// Set up the event handlers
    ////////////////////////////////////////

    int drag_y_HandlerIndex = this->eventManager->createEvent("DRAG_Y");
    EventHandler* drag_y_Handler = this->eventManager->getEvent(drag_y_HandlerIndex);

    int lastMouseY = INT_MIN;
    drag_y_Handler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if (event == nullptr) {
            CubeLog::error("Drag Y: nullptr");
            return;
        }
        
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) || event->type == sf::Event::EventType::TouchMoved && lastMouseY > INT_MIN) {
            for (auto action : drag_y_actions) {
                if (action.first()) {
                    action.second(-(event->mouseMove.y - lastMouseY));
                }
            }
        }
        lastMouseY = event->mouseMove.y;
    });
    drag_y_Handler->setEventType(sf::Event::MouseMoved);

    messageBox = new CubeMessageBox(this->renderer->getShader(), this->renderer->getTextShader(), this->renderer, countingLatch);
    countingLatch.count_up();
    // menu->setVisible(false);
    this->renderer->addSetupTask([&]() {
        // TODO: remove the testing menu entries from the menu class and build the menu here.
        messageBox->setup();
    });
    this->renderer->addLoopTask([&]() {
        messageBox->draw();
    });

    // TODO: make this not visible by default. Add a method for showing message box messages that checks if the menu is visible so
    // that we don't draw on top of the menu. Then, in the loop, if the messagebox is pending, and the menu gets closed, show the messagebox.
    messageBox->setVisible(false);

    countingLatch.wait();

    if (!this->renderer->isReady() || !this->renderer->getIsRunning())
        CubeLog::error("Renderer is not ready or is not running");

    for (auto menu : menus) {
        for (auto area : menu->getClickableAreas()) {
            this->eventManager->addClickableArea(area);
        }
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
    delete mainMenu;
    delete messageBox;
}

// TODO: This is not working. Need to add a menuentry to the parentmenu with action to show this new menu.
void GUI::addMenu(std::string menuName, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, CountingLatch& latch)
{
    latch.count_up();
    auto menu = new Menu(this->renderer, latch);
    menu->setMenuName(menuName);
    menu->setVisible(false);
    menus.push_back(menu);
    drag_y_actions.push_back({ [&, menu]() { return menu->getVisible(); }, [&, menu](int y) { menu->scrollVert(y); } });
    this->renderer->addSetupTask([&, menu, entryTexts, parentName]() {
        menu->addMenuEntry("< Back", [&](void* data) {
            CubeLog::info("Back clicked");
            menu->setVisible(false);
            if (menu->getParentMenu() != nullptr) {
                menu->getParentMenu()->setVisible(true);
            }
        });
        menu->addHorizontalRule();
        for (int i = 0; i < entryTexts.size(); i++) {
            std::string temp = entryTexts[i];
            menu->addMenuEntry(entryTexts[i], [&, temp](void*) { CubeLog::info("Test Sub Menu Entry " + std::to_string(i) + " clicked. " + temp); });
        }
        menu->setup();
        menu->setVisible(false);
        for (auto menu : menus) {
            CubeLog::info("Comparing: " + menu->getMenuName() + " to " + parentName);
            if (menu->getMenuName() == parentName) {
                menu->setParentMenu(menu);
            }
        }
    });
    this->renderer->addLoopTask([&, menu]() {
        menu->draw();
    });
};

/**
 * @brief Stop the event loop
 *
 */
void GUI::stop()
{
    this->renderer->stop();
}

/**
 * @brief Show a message box with a title and message
 *
 * @param title
 * @param message
 */
void GUI::showMessageBox(std::string title, std::string message)
{
    // check that messageBox is not null pointer
    if (messageBox == nullptr) {
        CubeLog::error("Message box is null. Cannot show message.");
        return;
    }
    CubeLog::info("Showing message box with title: " + title + " and message: " + message);
    messageBox->setText(message);
    messageBox->setVisible(true);
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
                    p.length() > 0 ? this->messageBox->setVisible(true) : this->messageBox->setVisible(false);
                }
            }
            CubeLog::info("Endpoint stop called and message set to: " + p);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Message set to: " + p);
        } });
    actions.push_back({ PUBLIC_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            std::string paramsString;
            for (auto param : req.params) {
                paramsString += param.first + ": " + param.second + "\n";
            }
            CubeLog::info("Endpoint action 2: \n");
            CubeLog::info(paramsString);
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Endpoint action 2 called");
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
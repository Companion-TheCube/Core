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
    /////////////////////////////////
    /// Test events TODO: remove this
    /////////////////////////////////
    int keyPressIndex = this->eventManager->createEvent("KeyPressed");
    EventHandler* keyPressHandler = this->eventManager->getEvent(keyPressIndex);
    keyPressHandler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if (event != nullptr)
            CubeLog::info("Key pressed: " + std::to_string(event->key.code));
        else
            CubeLog::info("Key pressed: nullptr");
    });
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
    /// TESTING FUNCTION TODO: remove this
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
    CountingLatch countingLatch(0);

    ///////// Submenu ///////// TODO: fill in all the submenus
    countingLatch.count_up();
    auto testSubMenu = new Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    testSubMenu->setMenuName("Test Sub Menu");
    testSubMenu->setUniqueMenuIdentifier("Test Sub Menu");
    testSubMenu->setVisible(false);
    menus.push_back(testSubMenu);
    drag_y_actions.push_back({ [&]() { return testSubMenu->getVisible(); }, [&](int y) { testSubMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        testSubMenu->addMenuEntry(
            "< Back",
            "Test Sub Menu_back",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [&](void* data) {
                CubeLog::info("Back clicked");
                testSubMenu->setVisible(false);
                if (testSubMenu->getParentMenu() != nullptr) {
                    testSubMenu->getParentMenu()->setVisible(true);
                }
                return 0;
            },
            [](void* data) { return 0; },
            nullptr);
        testSubMenu->addHorizontalRule();
        testSubMenu->addMenuEntry(
            "Test Sub Menu Entry 1",
            "Test Sub Menu Entry 1",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [](void*) {
                CubeLog::info("Test Sub Menu Entry 1 clicked");
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        testSubMenu->addMenuEntry(
            "Test Sub Menu Entry 2",
            "Test Sub Menu Entry 2",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [](void*) {
                CubeLog::info("Test Sub Menu Entry 2 clicked");
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        testSubMenu->setup();
        testSubMenu->setVisible(false);
    });
    this->renderer->addLoopTask([&]() {
        testSubMenu->draw();
    });

    // addMenu("Test Menu 2", "Main Menu", {"Test Menu 2 Entry 1", "Test Menu 2 Entry 2", "Test Menu 2 Entry 3"}, {"Test Menu 2 Entry 1", "Test Menu 2 Entry 2", "Test Menu 2 Entry 3"});
    // addMenu("Test Menu 3", "Test Menu 1", {"Test Menu 3 Entry 1", "Test Menu 3 Entry 2", "Test Menu 3 Entry 3"}, {"Test Menu 3 Entry 1", "Test Menu 3 Entry 2", "Test Menu 3 Entry 3"});

    ///////// Main Menu /////////
    countingLatch.count_up();
    auto mainMenu = new Menu(this->renderer, countingLatch);
    testSubMenu->setParentMenu(mainMenu);
    mainMenu->setMenuName("Main Menu");
    mainMenu->setUniqueMenuIdentifier("Main Menu");
    menus.push_back(mainMenu);
    drag_y_actions.push_back({ [&]() { return mainMenu->getVisible(); }, [&](int y) { mainMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        mainMenu->addMenuEntry(
            "< Settings",
            "Settings_back",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [&](void* data) {
                CubeLog::info("Settings clicked");
                mainMenu->setVisible(false);
                mainMenu->setIsClickable(true);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        mainMenu->addHorizontalRule();
        mainMenu->addMenuEntry(
            "Test Sub Menu",
            "Test Sub Menu2",
            MenuEntry::EntryType::MENUENTRY_TYPE_SUBMENU,
            [&](void* data) {
                CubeLog::info("Test Sub Menu clicked");
                testSubMenu->setVisible(true);
                mainMenu->setVisible(false);
                mainMenu->setIsClickable(false);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        mainMenu->addMenuEntry(
            "Test Menu Entry but with radio button",
            "Test Menu Entry but with radio button",
            MenuEntry::EntryType::MENUENTRY_TYPE_RADIOBUTTON,
            [&](void* data) {
                CubeLog::info("Test Menu Entry 1 clicked");
                return 0;
            },
            [](void*) {
                // generate a random number between 0 and 1
                return rand() % 2;
            },
            nullptr);
        for (int i = 0; i < 25; i++) {
            std::string s = " - repeats: ";
            repeatChar(s, i, "a ");
            mainMenu->addMenuEntry(
                "Test Menu Entry " + std::to_string(i) + s,
                "Test Menu Entry " + std::to_string(i),
                MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
                [&, i](void*) { CubeLog::info("Test Menu Entry " + std::to_string(i) + " clicked"); return 0; },
                [](void*) { return 0; },
                nullptr);
        }
        mainMenu->setup();
        mainMenu->setVisible(false);
        mainMenu->setAsMainMenu(); // No other menus should call this method
        mainMenu->setIsClickable(true);
    });
    this->renderer->addLoopTask([&]() {
        mainMenu->draw();
    });

    addMenu("Test Menu 1", "testMenu1", "Main Menu", { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, { "sub1_1", "sub1_2", "sub1_3" }, countingLatch);

    ////////////////////////////////////////
    /// Set up the event handlers
    ////////////////////////////////////////

    int lastMouseY = INT_MIN;
    bool last_isButtonPressed = false;
    int drag_y_HandlerIndex = this->eventManager->createEvent("DRAG_Y");
    EventHandler* drag_y_Handler = this->eventManager->getEvent(drag_y_HandlerIndex);
    drag_y_Handler->setAction([&](void* data) {
        sf::Event* event = (sf::Event*)data;
        if (event == nullptr) {
            CubeLog::error("Drag Y: nullptr");
            return;
        }
        bool touchChange = false;
        // touch event edge detection
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !last_isButtonPressed) {
            last_isButtonPressed = true;
            touchChange = true;
        } else if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && last_isButtonPressed) {
            last_isButtonPressed = false;
            touchChange = true;
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && lastMouseY > INT_MIN) {
            for (auto action : drag_y_actions) {
                if (action.first() && !touchChange) {
                    action.second(-(event->mouseMove.y - lastMouseY));
                }
            }
        }
        lastMouseY = event->mouseMove.y;
    });
    drag_y_Handler->setEventType(sf::Event::MouseMoved);

    ////////////////////////////////////////
    /// Set up the popup message box
    ////////////////////////////////////////
    messageBox = new CubeMessageBox(this->renderer->getShader(), this->renderer->getTextShader(), this->renderer, countingLatch);
    countingLatch.count_up();
    this->renderer->addSetupTask([&]() {
        messageBox->setup();
    });
    this->renderer->addLoopTask([&]() {
        messageBox->draw();
    });

    // TODO: make this not visible by default. Add a method for showing message box messages that checks if the menu is visible so
    // that we don't draw on top of the menu. Then, in the loop, if the messagebox is pending, and the menu gets closed, show the messagebox.
    messageBox->setVisible(false);

    ////////////////////////////////////////
    /// Wait for the rendered elements to be ready
    ////////////////////////////////////////
    countingLatch.wait();

    if (!this->renderer->isReady() || !this->renderer->getIsRunning())
        CubeLog::error("Renderer is not ready or is not running");

    // Add all the clickable areas to the event manager
    for (auto menu : menus) {
        for (auto area : menu->getClickableAreas()) {
            this->eventManager->addClickableArea(area);
        }
    }

    CubeLog::info("Starting event handler loop...");
    while (this->renderer->getIsRunning()) {
        std::vector<sf::Event> events = this->renderer->getEvents();
        for (int i = 0; i < events.size(); i++) {
            this->eventManager->triggerEvent(events[i].type, &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), &events[i]);
            this->eventManager->triggerEvent(static_cast<SpecificEventTypes>(events[i].key.code), events[i].type, &events[i]);
        }
        genericSleep(5);
    }
    CubeLog::info("Event handler loop stopped");
    for (auto menu : menus) {
        delete menu;
    }
    delete messageBox;
}

// TODO: This is working, but we need to execute the endpoint in the entry action
// TODO: remove this overload of the addMenu method since any time we add a menu that needs the latch, it should be done above in menu setup.
// TODO: The endpoints param needs a parsing function that will take the string and return a function that will execute the endpoint.
/**
 * @brief Add a menu to the GUI
 *
 * @param menuName the name of the menu
 * @param parentName the name of the parent menu
 * @param entryTexts the text for the menu entries
 * @param endpoints the endpoints for the menu entries. This can be simple HTTP GET endpoints or JSON objects that describe the endpoint.
 * @param latch a CountingLatch object that will be used to wait for the menu to be ready
 */
GUI_Error GUI::addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, std::vector<std::string> uniqueIDs, CountingLatch& latch)
{
    latch.count_up();
    auto aNewMenu = new Menu(this->renderer, latch);
    aNewMenu->setMenuName(menuName);
    aNewMenu->setUniqueMenuIdentifier(thisUniqueID);
    aNewMenu->setVisible(false);
    Menu* parentMenuPtr = nullptr;
    bool unique = true;
    for (auto m : menus) {
        if (m->getUniqueMenuIdentifier() == parentID) {
            parentMenuPtr = m;
        }
        for (auto t : uniqueIDs) {
            if (m->getUniqueMenuIdentifier() == t) {
                unique = false;
            }
        }
        if(thisUniqueID == m->getUniqueMenuIdentifier()) {
            unique = false;
        }
    }
    if (!unique) {
        CubeLog::error("Unique menu identifier is not unique for menu: " + menuName);
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_UNIQUE_EXISTS, "Unique menu identifier is not unique for menu: " + menuName);
    }
    if (parentMenuPtr == nullptr) {
        CubeLog::error("Parent menu not found for menu: " + menuName);
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_PARENT_NOT_FOUND, "Parent menu not found for menu: " + menuName);
    }
    menus.push_back(aNewMenu);
    drag_y_actions.push_back({ [aNewMenu]() { return aNewMenu->getVisible(); }, [aNewMenu](int y) { aNewMenu->scrollVert(y); } });
    this->renderer->addSetupTask([parentMenuPtr, menuName, aNewMenu, entryTexts, uniqueIDs]() {
        aNewMenu->addMenuEntry(
            "< Back",
            menuName + "_back",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [aNewMenu](void* data) {
                CubeLog::info("Back clicked");
                aNewMenu->setVisible(false);
                if (aNewMenu->getParentMenu() != nullptr) {
                    aNewMenu->getParentMenu()->setVisible(true);
                }
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        aNewMenu->addHorizontalRule();
        for (int i = 0; i < entryTexts.size(); i++) {
            std::string temp = entryTexts[i];
            // generate a random number
            int random = rand() % 100;
            aNewMenu->addMenuEntry(
                entryTexts[i],
                uniqueIDs[i],
                MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
                [i, temp, random](void*) {
                    CubeLog::info("Test Sub Menu Entry " + std::to_string(i) + " clicked. (" + std::to_string(random) + ")" + temp);
                    return 0;
                },
                [i, temp](void*) { return 0; },
                [random](void*) { return random; },
                nullptr);
        }
        aNewMenu->setup();
        aNewMenu->setVisible(false);
        aNewMenu->setParentMenu(parentMenuPtr);
        aNewMenu->getParentMenu()->addMenuEntry(
            menuName,
            menuName + "_entry",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [aNewMenu](void*) {
                CubeLog::info("Test Sub Menu clicked");
                aNewMenu->setVisible(true);
                aNewMenu->getParentMenu()->setVisible(false);
                aNewMenu->getParentMenu()->setIsClickable(false);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
    });
    CubeLog::moreInfo("Added menu: " + menuName + " with parent: " + parentID);
    CubeLog::moreInfo("Adding draw function for menu: " + menuName);
    this->renderer->addLoopTask([aNewMenu]() {
        aNewMenu->draw();
    });
    CubeLog::moreInfo("Menu setup complete: " + menuName);
    return GUI_Error(GUI_Error::ERROR_TYPES::GUI_NO_ERROR, "Menu setup complete: " + menuName);
};

/**
 * @brief Add a menu to the GUI
 *
 * @param menuName the name of the menu
 * @param parentName the name of the parent menu
 * @param entryTexts the text for the menu entries
 * @param endpoints the endpoints for the menu entries. This can be simple HTTP GET endpoints or JSON objects that describe the endpoint.
 */
GUI_Error GUI::addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, std::vector<std::string> uniqueIDs)
{
    auto aNewMenu = new Menu(this->renderer);
    aNewMenu->setMenuName(menuName);
    aNewMenu->setUniqueMenuIdentifier(thisUniqueID);
    aNewMenu->setVisible(false);
    Menu* parentMenuPtr = nullptr;
    bool unique = true;
    for (auto m : menus) {
        if (m->getUniqueMenuIdentifier() == parentID) {
            parentMenuPtr = m;
        }
        for (auto t : uniqueIDs) {
            if (m->getUniqueMenuIdentifier() == t) {
                unique = false;
            }
        }
        if(thisUniqueID == m->getUniqueMenuIdentifier()) {
            unique = false;
        }
    }
    if (!unique) {
        CubeLog::error("Unique menu identifiers for menu and children are not unique for menu: " + menuName);
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_UNIQUE_EXISTS, "Unique menu identifiers for menu and children are not unique for menu: " + menuName);
    }
    if (parentMenuPtr == nullptr) {
        CubeLog::error("Parent menu not found for menu: " + menuName);
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_PARENT_NOT_FOUND, "Parent menu not found for menu: " + menuName);
    }
    menus.push_back(aNewMenu);
    EventManager* eventManagerPtr = this->eventManager;
    drag_y_actions.push_back({ [aNewMenu]() { return aNewMenu->getVisible(); }, [aNewMenu](int y) { aNewMenu->scrollVert(y); } });
    this->renderer->addSetupTask([parentMenuPtr, menuName, aNewMenu, entryTexts, eventManagerPtr, uniqueIDs]() {
        aNewMenu->addMenuEntry(
            "< Back",
            menuName + "_back",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [aNewMenu](void* data) {
                CubeLog::info("Back clicked");
                aNewMenu->setVisible(false);
                if (aNewMenu->getParentMenu() != nullptr) {
                    aNewMenu->getParentMenu()->setVisible(true);
                }
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        aNewMenu->addHorizontalRule();
        for (int i = 0; i < entryTexts.size(); i++) {
            std::string temp = entryTexts[i];
            aNewMenu->addMenuEntry(
                entryTexts[i],
                uniqueIDs[i],
                MenuEntry::EntryType::MENUENTRY_TYPE_RADIOBUTTON,
                [i, temp](void*) {
                    CubeLog::info("Test Sub Menu Entry " + std::to_string(i) + " clicked. " + temp);
                    return 0;
                },
                [i, temp](void*) {
                    // get random 0 or 1
                    int random = (rand() % 2) + 1;
                    CubeLog::info("Test Sub Menu Entry " + std::to_string(i) + " right clicked. " + temp + " (" + std::to_string(random) + ")");
                    return random;
                },
                nullptr);
        }
        aNewMenu->setup();
        aNewMenu->setVisible(false);
        aNewMenu->setParentMenu(parentMenuPtr);
        aNewMenu->getParentMenu()->addMenuEntry(
            menuName,
            menuName + "_entry",
            MenuEntry::EntryType::MENUENTRY_TYPE_ACTION,
            [aNewMenu](void*) {
                CubeLog::info("Test Sub Menu clicked");
                aNewMenu->setVisible(true);
                aNewMenu->getParentMenu()->setVisible(false);
                aNewMenu->getParentMenu()->setIsClickable(false);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        eventManagerPtr->addClickableArea(aNewMenu->getParentMenu()->getClickableAreas().at(aNewMenu->getParentMenu()->getClickableAreas().size() - 1));
        for (auto area : aNewMenu->getClickableAreas()) {
            eventManagerPtr->addClickableArea(area);
        }
    });
    CubeLog::moreInfo("Added menu: " + menuName + " with parent: " + parentID);
    CubeLog::moreInfo("Adding draw function for menu: " + menuName);
    this->renderer->addLoopTask([aNewMenu]() {
        aNewMenu->draw();
    });
    CubeLog::moreInfo("Menu setup complete: " + menuName);
    return GUI_Error(GUI_Error::ERROR_TYPES::GUI_NO_ERROR, "Menu setup complete: " + menuName);
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
    /*
    TODO: Add the following endpoints:
    - addMenu
    - showMessageBoxMessage
    - addCharacter
    - removeCharacter
    - animateCharacter
    */
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
            res.set_content("Endpoint action 2 called", "text/plain");
            GUI_Error temp = addMenu(
                "Test Menu 34", 
                "TestMenu34dfgd", 
                "testMenu1", 
                { "Test Menu 34 Entry 1 kjhsdfjkhsdfkjhsdfkjhsdf", "Test Menu 34 Entry 2", "Test Menu 34 Entry 3" }, 
                { "Test Menu 34 Entry 1", "Test Menu 34 Entry 2", "Test Menu 34 Entry 3" }, 
                { "test34_df1", "test34_2ff", "test34_3ddd" }
                );
            if(temp.errorType != GUI_Error::ERROR_TYPES::GUI_NO_ERROR) {
                CubeLog::error("Failed to add menu. Error: " + temp.errorString);
                res.set_content("Failed to add menu. Error: " + temp.errorString, "text/plain");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to add menu. Error: " + temp.errorString);
            }
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
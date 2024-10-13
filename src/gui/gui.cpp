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
    auto testSubMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    testSubMenu->setMenuName("Test Sub Menu");
    testSubMenu->setUniqueMenuIdentifier("Test Sub Menu");
    testSubMenu->setVisible(false);
    menus.push_back(testSubMenu);
    drag_y_actions.push_back({ [&]() { return testSubMenu->getVisible(); }, [&](int y) { testSubMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        testSubMenu->addMenuEntry(
            "< Back",
            "Test Sub Menu_back",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [](void*) {
                CubeLog::info("Test Sub Menu Entry 1 clicked");
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        testSubMenu->addMenuEntry(
            "Test Sub Menu Entry 2",
            "Test Sub Menu Entry 2",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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

    ///////// Main Menu /////////
    countingLatch.count_up();
    auto mainMenu = new MENUS::Menu(this->renderer, countingLatch);
    testSubMenu->setParentMenu(mainMenu);
    mainMenu->setMenuName("Main Menu");
    mainMenu->setUniqueMenuIdentifier("Main Menu");
    menus.push_back(mainMenu);
    drag_y_actions.push_back({ [&]() { return mainMenu->getVisible(); }, [&](int y) { mainMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        mainMenu->addMenuEntry(
            "< Settings",
            "Settings_back",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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
            MENUS::EntryType::MENUENTRY_TYPE_SUBMENU,
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
            MENUS::EntryType::MENUENTRY_TYPE_RADIOBUTTON_GROUP,
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
                MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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

    // TODO: remove this
    // addMenu("Test Menu 1", "testMenu1", "Main Menu", { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, { "Test Menu 1 Entry 1", "Test Menu 1 Entry 2", "Test Menu 1 Entry 3" }, { "sub1_1", "sub1_2", "sub1_3" }, countingLatch);

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
// TODO: Part of parsing the endpoint will involve determining if the endpoint is for a boolean value (radiobutton), another menu, etc.
/**
 * @brief Add a menu to the GUI
 *
 * @param menuName the name of the menu
 * @param parentName the name of the parent menu
 * @param data a vector of tuples containing the text, json data, and unique identifier for the menu entries
 */
GUI_Error GUI::addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, AddMenu_Data_t data)
{
    std::vector<std::string> entryTexts;
    std::vector<std::string> endpoints;
    std::vector<std::string> uniqueIDs;
    std::unique_lock<std::mutex> lock(this->addMenuMutex);
    auto aNewMenu = new MENUS::Menu(this->renderer);
    aNewMenu->setMenuName(menuName);
    aNewMenu->setUniqueMenuIdentifier(thisUniqueID);
    aNewMenu->setVisible(false);
    MENUS::Menu* parentMenuPtr = nullptr;
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
        if (thisUniqueID == m->getUniqueMenuIdentifier()) {
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
    this->renderer->addSetupTask([parentMenuPtr, menuName, aNewMenu, data, eventManagerPtr, thisUniqueID]() {
        aNewMenu->addMenuEntry(
            "< Back",
            menuName + "_back",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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
        bool success = true;
        for (int i = 0; i < data.size(); i++) {
            if(!parseJsonAndAddEntriesToMenu(std::get<1>(data[i]), aNewMenu)) {
                CubeLog::error("Error parsing json and adding entries to menu: " + menuName);
                success = false;
            }
        }
        if(!success) {
            CubeLog::error("Error parsing json and adding entries to menu: " + menuName);
            return;
        }
        aNewMenu->setup();
        aNewMenu->setVisible(false);
        aNewMenu->setParentMenu(parentMenuPtr);
        aNewMenu->getParentMenu()->addMenuEntry(
            menuName,
            thisUniqueID,
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
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
    actions.push_back(
        { PRIVATE_ENDPOINT | POST_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                // ge the json data from the request
                nlohmann::json j;
                nlohmann::json response;
                try {
                    j = nlohmann::json::parse(req.body);
                } catch (nlohmann::json::exception& e) {
                    CubeLog::error("Error parsing json: " + std::string(e.what()));
                    response["success"] = false;
                    response["message"] = "Error parsing json: " + std::string(e.what());
                    res.set_content(response.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Error parsing json: " + std::string(e.what()));
                }
                AddMenu_Data_t data;
                std::string menuName, thisUniqueID, parentID;
                if (!breakJsonApart(j, data, &menuName, &thisUniqueID, &parentID)) {
                    CubeLog::error("Error breaking json apart");
                    response["success"] = false;
                    response["message"] = "Error breaking json apart";
                    res.set_content(response.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Error breaking json apart");
                }
                GUI_Error temp = addMenu(menuName, thisUniqueID, parentID, data);
                if (temp.errorType != GUI_Error::ERROR_TYPES::GUI_NO_ERROR) {
                    CubeLog::error("Failed to add menu. Error: " + temp.errorString);
                    response["success"] = false;
                    response["message"] = "Failed to add menu. Error: " + temp.errorString;
                    res.set_content(response.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INTERNAL_ERROR, "Failed to add menu. Error: " + temp.errorString);
                }
                response["success"] = true;
                response["message"] = "Menu added";
                res.set_content(response.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Menu added");
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
    names.push_back({ "addMenu", {} });
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

bool parseJsonAndAddEntriesToMenu(nlohmann::json j, MENUS::Menu* menuEntry)
{
    std::string entryText, uniqueID, entryType;
    try {
        entryText = j["entryText"];
        uniqueID = j["uniqueID"];
        entryType = j["entryData"]["entryType"];
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Error parsing json: " + std::string(e.what()));
        return false;
    }

    auto entry_it = MENUS::entryTypeMap.find(entryType);
    if (entry_it == MENUS::entryTypeMap.end()) {
        CubeLog::error("Entry type not found: " + entryType);
        return false;
    }

    MENUS::EntryType type = entry_it->second;

    std::vector<std::string> enabledVals;
    try {
        for (auto it : j["entryData"].items()) {
            if (it.key() == "enabledValues") {
                for (auto v : it.value()) {
                    if (v.is_string()) {
                        enabledVals.push_back(v);
                    } else if (v.is_number()) {
                        enabledVals.push_back(std::to_string((int)v));
                    } else if (v.is_boolean()) {
                        enabledVals.push_back(std::to_string((bool)v));
                    }
                }
            }
        }
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Error parsing json: " + std::string(e.what()));
        return false;
    }

    std::string actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token;
    std::string statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token;
    bool hasActionData = false, hasStatusData = false;
    try {
        actionEP_AddrPort = j["entryData"]["actionEndpoint"]["addr_port"];
        actionEP_Path = j["entryData"]["actionEndpoint"]["path"];
        actionEP_Method = j["entryData"]["actionEndpoint"]["method"];
        actionEP_User = j["entryData"]["actionEndpoint"]["user"];
        actionEP_Pass = j["entryData"]["actionEndpoint"]["pass"];
        actionEP_Token = j["entryData"]["actionEndpoint"]["token"];
        hasActionData = true;
    } catch (nlohmann::json::exception& e) {
        CubeLog::moreInfo("No action endpoint provided: " + entryText);
    }
    try {
        statusEP_AddrPort = j["entryData"]["statusEndpoint"]["addr_port"];
        statusEP_Path = j["entryData"]["statusEndpoint"]["path"];
        statusEP_Method = j["entryData"]["statusEndpoint"]["method"];
        statusEP_User = j["entryData"]["statusEndpoint"]["user"];
        statusEP_Pass = j["entryData"]["statusEndpoint"]["pass"];
        statusEP_Token = j["entryData"]["statusEndpoint"]["token"];
        hasStatusData = true;
    } catch (nlohmann::json::exception& e) {
        CubeLog::moreInfo("No status endpoint provided: " + entryText);
    }
    if (type != MENUS::EntryType::MENUENTRY_TYPE_SUBMENU && !hasActionData && !hasStatusData) {
        CubeLog::error("No action or status endpoint provided: " + entryText);
        return false;
    }
    if (type == MENUS::EntryType::MENUENTRY_TYPE_ACTION && !hasActionData) {
        CubeLog::error("No action endpoint provided: " + entryText);
        return false;
    }
    if (!hasActionData || !hasStatusData) {
        CubeLog::error("No status endpoint provided or no action endpoint provided: " + entryText);
        return false;
    }

    switch (type) {
    case MENUS::EntryType::MENUENTRY_TYPE_ACTION: {
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token](void*) {
                if (actionEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + actionEP_AddrPort + actionEP_Path);
                    return 0;
                }
                if (actionEP_Method != "GET" && actionEP_Method != "POST" && actionEP_Method != "PUT" && actionEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + actionEP_Method + "From: " + actionEP_AddrPort + actionEP_Path);
                    return 1;
                }
                httplib::Client client(actionEP_AddrPort);
                if ((actionEP_User.length() > 0 || actionEP_Pass.length() > 0) && actionEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth " + actionEP_AddrPort + actionEP_Path);
                    return 1;
                }
                if (actionEP_User.length() > 0 && actionEP_Pass.length() > 0) {
                    client.set_basic_auth(actionEP_User.c_str(), actionEP_Pass.c_str());
                }
                if (actionEP_Token.length() > 0) {
                    client.set_bearer_token_auth(actionEP_Token.c_str());
                }
                auto res = client.Get(actionEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response: " + actionEP_AddrPort + actionEP_Path);
                    return 3;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status));
                    return 2;
                }
                CubeLog::info("Response: " + res->body + "From: " + actionEP_AddrPort + actionEP_Path);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_CHECKBOX: {
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token](void*) {
                if (actionEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_Method != "GET" && actionEP_Method != "POST" && actionEP_Method != "PUT" && actionEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + actionEP_Method + "From: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(actionEP_AddrPort);
                if ((actionEP_User.length() > 0 || actionEP_Pass.length() > 0) && actionEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_User.length() > 0 && actionEP_Pass.length() > 0) {
                    client.set_basic_auth(actionEP_User.c_str(), actionEP_Pass.c_str());
                }
                if (actionEP_Token.length() > 0) {
                    client.set_bearer_token_auth(actionEP_Token.c_str());
                }
                httplib::Result res;
                if (actionEP_Method == "GET")
                    res = client.Get(actionEP_Path.c_str());
                if (actionEP_Method == "POST")
                    res = client.Post(actionEP_Path.c_str());
                if (actionEP_Method == "PUT")
                    res = client.Put(actionEP_Path.c_str());
                if (actionEP_Method == "DELETE")
                    res = client.Delete(actionEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status));
                    return (unsigned int)1;
                }
                CubeLog::info("Response: " + res->body + "From: " + actionEP_AddrPort + actionEP_Path);
                return (unsigned int)0;
            },
            [statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals](void*) {
                if (statusEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (statusEP_Method != "GET" && statusEP_Method != "POST" && statusEP_Method != "PUT" && statusEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + statusEP_Method + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(statusEP_AddrPort);
                if ((statusEP_User.length() > 0 || statusEP_Pass.length() > 0) && statusEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth");
                    return (unsigned int)2;
                }
                if (statusEP_User.length() > 0 && statusEP_Pass.length() > 0) {
                    client.set_basic_auth(statusEP_User.c_str(), statusEP_Pass.c_str());
                }
                if (statusEP_Token.length() > 0) {
                    client.set_bearer_token_auth(statusEP_Token.c_str());
                }
                httplib::Result res;
                if (statusEP_Method == "GET")
                    res = client.Get(statusEP_Path.c_str());
                if (statusEP_Method == "POST")
                    res = client.Post(statusEP_Path.c_str());
                if (statusEP_Method == "PUT")
                    res = client.Put(statusEP_Path.c_str());
                if (statusEP_Method == "DELETE")
                    res = client.Delete(statusEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response from status endpoint: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status) + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                std::string response = res->body;
                CubeLog::moreInfo("Response: " + response + "From: " + statusEP_AddrPort + statusEP_Path);
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(response);
                } catch (nlohmann::json::exception& e) {
                    CubeLog::error("Error parsing json: " + std::string(e.what()));
                    return (unsigned int)2;
                }
                std::string enabled = j["enabled"];
                for (auto v : enabledVals) {
                    if (v == enabled) {
                        return (unsigned int)1;
                    }
                }
                return (unsigned int)0;
            },
            nullptr);
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_RADIOBUTTON_GROUP: {
        int groupID = 0;
        try {
            groupID = j["entryData"]["radioGroup"];
        } catch (nlohmann::json::exception& e) {
            CubeLog::error("Error parsing json: " + std::string(e.what()));
            return false;
        }
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token](void*) {
                if (actionEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_Method != "GET" && actionEP_Method != "POST" && actionEP_Method != "PUT" && actionEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + actionEP_Method + "From: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(actionEP_AddrPort);
                if ((actionEP_User.length() > 0 || actionEP_Pass.length() > 0) && actionEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_User.length() > 0 && actionEP_Pass.length() > 0) {
                    client.set_basic_auth(actionEP_User.c_str(), actionEP_Pass.c_str());
                }
                if (actionEP_Token.length() > 0) {
                    client.set_bearer_token_auth(actionEP_Token.c_str());
                }
                httplib::Result res;
                if (actionEP_Method == "GET")
                    res = client.Get(actionEP_Path.c_str());
                if (actionEP_Method == "POST")
                    res = client.Post(actionEP_Path.c_str());
                if (actionEP_Method == "PUT")
                    res = client.Put(actionEP_Path.c_str());
                if (actionEP_Method == "DELETE")
                    res = client.Delete(actionEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status));
                    return (unsigned int)2;
                }
                CubeLog::info("Response: " + res->body + "From: " + actionEP_AddrPort + actionEP_Path);
                return (unsigned int)0;
            },
            [&menuEntry, groupID, statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals](void*) {
                if (statusEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (statusEP_Method != "GET" && statusEP_Method != "POST" && statusEP_Method != "PUT" && statusEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + statusEP_Method + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(statusEP_AddrPort);
                if ((statusEP_User.length() > 0 || statusEP_Pass.length() > 0) && statusEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth");
                    return (unsigned int)2;
                }
                if (statusEP_User.length() > 0 && statusEP_Pass.length() > 0) {
                    client.set_basic_auth(statusEP_User.c_str(), statusEP_Pass.c_str());
                }
                if (statusEP_Token.length() > 0) {
                    client.set_bearer_token_auth(statusEP_Token.c_str());
                }
                httplib::Result res;
                if (statusEP_Method == "GET")
                    res = client.Get(statusEP_Path.c_str());
                if (statusEP_Method == "POST")
                    res = client.Post(statusEP_Path.c_str());
                if (statusEP_Method == "PUT")
                    res = client.Put(statusEP_Path.c_str());
                if (statusEP_Method == "DELETE")
                    res = client.Delete(statusEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response from status endpoint: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status) + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                std::string response = res->body;
                CubeLog::moreInfo("Response: " + response + "From: " + statusEP_AddrPort + statusEP_Path);
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(response);
                } catch (nlohmann::json::exception& e) {
                    CubeLog::error("Error parsing json: " + std::string(e.what()));
                    return (unsigned int)2;
                }
                std::string enabled = j["enabled"];
                unsigned int retVal = 0;
                for (auto v : enabledVals) {
                    if (v == enabled) {
                        retVal = 1;
                    }
                }
                for (auto area : menuEntry->getParentMenu()->getClickableAreas()) {
                    MENUS::MenuEntry* entry = reinterpret_cast<MENUS::MenuEntry*>(area);
                    if (entry->getGroupID() == groupID) {
                        entry->setStatusReturnData(0);
                    }
                }
                return retVal;
            },
            nullptr);
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_SUBMENU: {
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [entryText](void*) {
                CubeLog::moreInfo("MenuEntry clicked: " + entryText);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_TOGGLE: {
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token](void*) {
                if (actionEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_Method != "GET" && actionEP_Method != "POST" && actionEP_Method != "PUT" && actionEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + actionEP_Method + "From: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(actionEP_AddrPort);
                if ((actionEP_User.length() > 0 || actionEP_Pass.length() > 0) && actionEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_User.length() > 0 && actionEP_Pass.length() > 0) {
                    client.set_basic_auth(actionEP_User.c_str(), actionEP_Pass.c_str());
                }
                if (actionEP_Token.length() > 0) {
                    client.set_bearer_token_auth(actionEP_Token.c_str());
                }
                httplib::Result res;
                if (actionEP_Method == "GET")
                    res = client.Get(actionEP_Path.c_str());
                if (actionEP_Method == "POST")
                    res = client.Post(actionEP_Path.c_str());
                if (actionEP_Method == "PUT")
                    res = client.Put(actionEP_Path.c_str());
                if (actionEP_Method == "DELETE")
                    res = client.Delete(actionEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status));
                    return (unsigned int)2;
                }
                CubeLog::info("Response: " + res->body + "From: " + actionEP_AddrPort + actionEP_Path);
                return (unsigned int)0;
            },
            [statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals](void*) {
                if (statusEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (statusEP_Method != "GET" && statusEP_Method != "POST" && statusEP_Method != "PUT" && statusEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + statusEP_Method + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(statusEP_AddrPort);
                if ((statusEP_User.length() > 0 || statusEP_Pass.length() > 0) && statusEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth");
                    return (unsigned int)2;
                }
                if (statusEP_User.length() > 0 && statusEP_Pass.length() > 0) {
                    client.set_basic_auth(statusEP_User.c_str(), statusEP_Pass.c_str());
                }
                if (statusEP_Token.length() > 0) {
                    client.set_bearer_token_auth(statusEP_Token.c_str());
                }
                httplib::Result res;
                if (statusEP_Method == "GET")
                    res = client.Get(statusEP_Path.c_str());
                if (statusEP_Method == "POST")
                    res = client.Post(statusEP_Path.c_str());
                if (statusEP_Method == "PUT")
                    res = client.Put(statusEP_Path.c_str());
                if (statusEP_Method == "DELETE")
                    res = client.Delete(statusEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response from status endpoint: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status) + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                std::string response = res->body;
                CubeLog::moreInfo("Response: " + response + "From: " + statusEP_AddrPort + statusEP_Path);
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(response);
                } catch (nlohmann::json::exception& e) {
                    CubeLog::error("Error parsing json: " + std::string(e.what()));
                    return (unsigned int)2;
                }
                std::string enabled = j["enabled"];
                unsigned int retVal = 0;
                for (auto v : enabledVals) {
                    if (v == enabled) {
                        retVal = 1;
                    }
                }
                return retVal;
            },
            nullptr);
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_TEXT_INPUT: {
        menuEntry->addMenuEntry(
            entryText,
            uniqueID,
            type,
            [actionEP_AddrPort, actionEP_Path, actionEP_Method, actionEP_User, actionEP_Pass, actionEP_Token](void*) {
                if (actionEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_Method != "GET" && actionEP_Method != "POST" && actionEP_Method != "PUT" && actionEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + actionEP_Method + "From: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(actionEP_AddrPort);
                if ((actionEP_User.length() > 0 || actionEP_Pass.length() > 0) && actionEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (actionEP_User.length() > 0 && actionEP_Pass.length() > 0) {
                    client.set_basic_auth(actionEP_User.c_str(), actionEP_Pass.c_str());
                }
                if (actionEP_Token.length() > 0) {
                    client.set_bearer_token_auth(actionEP_Token.c_str());
                }
                httplib::Result res;
                if (actionEP_Method == "GET")
                    res = client.Get(actionEP_Path.c_str());
                if (actionEP_Method == "POST")
                    res = client.Post(actionEP_Path.c_str());
                if (actionEP_Method == "PUT")
                    res = client.Put(actionEP_Path.c_str());
                if (actionEP_Method == "DELETE")
                    res = client.Delete(actionEP_Path.c_str());
                if (!res) {
                    CubeLog::error("Error getting response: " + actionEP_AddrPort + actionEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status));
                    return (unsigned int)2;
                }
                CubeLog::info("Response: " + res->body + "From: " + actionEP_AddrPort + actionEP_Path);
                return (unsigned int)0;
            },
            [statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals](void* inText) {
                std::string text = *(std::string*)inText;
                if (statusEP_AddrPort.length() == 0) {
                    CubeLog::info("No action endpoint provided: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (statusEP_Method != "GET" && statusEP_Method != "POST" && statusEP_Method != "PUT" && statusEP_Method != "DELETE") {
                    CubeLog::error("Invalid method: " + statusEP_Method + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                httplib::Client client(statusEP_AddrPort);
                if ((statusEP_User.length() > 0 || statusEP_Pass.length() > 0) && statusEP_Token.length() != 0) {
                    CubeLog::error("Cannot have both basic auth and bearer token auth");
                    return (unsigned int)2;
                }
                if (statusEP_User.length() > 0 && statusEP_Pass.length() > 0) {
                    client.set_basic_auth(statusEP_User.c_str(), statusEP_Pass.c_str());
                }
                if (statusEP_Token.length() > 0) {
                    client.set_bearer_token_auth(statusEP_Token.c_str());
                }
                httplib::Result res;
                if (statusEP_Method == "POST")
                    res = client.Post(statusEP_Path.c_str(), text, "application/json");
                if (statusEP_Method == "PUT")
                    res = client.Put(statusEP_Path.c_str(), text, "application/json");
                if (statusEP_Method == "GET")
                    res = client.Get((statusEP_Path + "?text=" + text).c_str());
                if (!res) {
                    CubeLog::error("Error getting response from status endpoint: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                if (res->status != 200) {
                    CubeLog::error("Response code: " + std::to_string(res->status) + "From: " + statusEP_AddrPort + statusEP_Path);
                    return (unsigned int)2;
                }
                std::string response = res->body;
                CubeLog::moreInfo("Response: " + response + "From: " + statusEP_AddrPort + statusEP_Path);
                nlohmann::json j;
                try {
                    j = nlohmann::json::parse(response);
                } catch (nlohmann::json::exception& e) {
                    CubeLog::error("Error parsing json: " + std::string(e.what()));
                    return (unsigned int)2;
                }
                std::string enabled = j["enabled"];
                unsigned int retVal = 0;
                for (auto v : enabledVals) {
                    if (v == enabled) {
                        retVal = 1;
                    }
                }
                return retVal;
            },
            nullptr
        );
        break;
    }
    default:
        CubeLog::error("Entry type not found: " + entryType);
        return false;
    };

    return true;
}

bool breakJsonApart(nlohmann::json j, AddMenu_Data_t& data, std::string* menuName, std::string* thisUniqueID, std::string* parentID)
{
    try {
        *menuName = j["menuName"];
        *thisUniqueID = j["uniqueID"];
        *parentID = j["parentID"];
        for (auto it = j["entries"].begin(); it != j["entries"].end(); ++it) {
            std::string entryText = it.value()["entryText"];
            nlohmann::json entryData = it.value();
            std::string uniqueID = it.value()["uniqueID"];

            data.push_back({ entryText, entryData, uniqueID });
        }
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Error parsing json: " + std::string(e.what()));
        return false;
    }
    return true;
}
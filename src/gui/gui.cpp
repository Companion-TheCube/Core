// TODO: Need to add a sort of status bar to the top of the screen. It should show the time and whether or not a person is detected. probably more.
// TODO: we should monitor the CubeLog for errors and display them in the status bar. This will require a way to get the last error message from the CubeLog. <- this is done in CubeLog

#include "./gui.h"

CubeMessageBox* GUI::messageBox = nullptr;
CubeTextBox* GUI::fullScreenTextBox = nullptr;

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
    CountingLatch countingLatch(20); // this value must be equal to count of "new MENUS::Menu()" calls in this method + 2 (for the message box and text box)

    // Helper function to add a back button to a menu with a horizontal rule
    auto addBackButton = [](auto* menu) {
        menu->addMenuEntry(
            "< Back - " + menu->getMenuName(),
            menu->getMenuName() + "_back",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [menu](void* data) {
                CubeLog::info("Back clicked");
                menu->setVisible(false);
                menu->setIsClickable(true);
                if (menu->getParentMenu() != nullptr) {
                    menu->getParentMenu()->setVisible(true);
                }
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        menu->addHorizontalRule();
    };

    // Helper function to add a menu to its parent menu
    auto addToParent = [](auto* menu) {
        menu->getParentMenu()->addMenuEntry(
            menu->getMenuName(),
            menu->getParentMenu()->getMenuName() + "_" + menu->getMenuName(),
            MENUS::EntryType::MENUENTRY_TYPE_SUBMENU,
            [menu](void* data) {
                CubeLog::info(menu->getMenuName() + " clicked");
                menu->setVisible(true);
                menu->getParentMenu()->setVisible(false);
                menu->getParentMenu()->setIsClickable(false);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
    };

    ///////// Main Menu /////////
    auto mainMenu = new MENUS::Menu(this->renderer, countingLatch);
    mainMenu->setAsMainMenu();
    mainMenu->setMenuName("Main Menu");
    mainMenu->setUniqueMenuIdentifier("Main Menu");
    menus.push_back(mainMenu);
    drag_y_actions.push_back({ [mainMenu]() { return mainMenu->getVisible(); }, [mainMenu](int y) { mainMenu->scrollVert(y); } });
    this->renderer->addSetupTask([mainMenu, addBackButton]() {
        addBackButton(mainMenu);
        mainMenu->setup();
        mainMenu->setVisible(false);
        mainMenu->setIsClickable(true);
    });
    this->renderer->addLoopTask([mainMenu]() {
        mainMenu->draw();
    });

    /////////
    /*
    1. Connections
    2. Personality
    3. Sensors
    4. Sound
    5. Notifications
    6. Display
    7. Privacy
    8. Accounts
    9. Apps
    10. General Settings
    11. Accessibility
    12.Updates
    13. About
    */

    /////////
    ///////// Connections Menu /////////
    auto connectionsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    connectionsMenu->setMenuName("Connections");
    connectionsMenu->setUniqueMenuIdentifier("Connections");
    connectionsMenu->setVisible(false);
    connectionsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [connectionsMenu]() { return connectionsMenu->getVisible(); }, [connectionsMenu](int y) { connectionsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([connectionsMenu, addBackButton, addToParent]() {
        addBackButton(connectionsMenu);
        connectionsMenu->setup();
        connectionsMenu->setVisible(false);
        addToParent(connectionsMenu);
    });
    this->renderer->addLoopTask([connectionsMenu]() {
        connectionsMenu->draw();
    });
    menus.push_back(connectionsMenu);

    ///////// Personality Menu /////////
    auto personalityMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    personalityMenu->setMenuName("Personality");
    personalityMenu->setUniqueMenuIdentifier("Personality");
    personalityMenu->setVisible(false);
    personalityMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [personalityMenu]() { return personalityMenu->getVisible(); }, [personalityMenu](int y) { personalityMenu->scrollVert(y); } });
    this->renderer->addSetupTask([personalityMenu, addBackButton, addToParent]() {
        addBackButton(personalityMenu);
        personalityMenu->setup();
        personalityMenu->setVisible(false);
        addToParent(personalityMenu);
    });
    this->renderer->addLoopTask([personalityMenu]() {
        personalityMenu->draw();
    });
    menus.push_back(personalityMenu);

    ///////// Sensors Menu /////////
    auto sensorsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    sensorsMenu->setMenuName("Sensors");
    sensorsMenu->setUniqueMenuIdentifier("Sensors");
    sensorsMenu->setVisible(false);
    sensorsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [sensorsMenu]() { return sensorsMenu->getVisible(); }, [sensorsMenu](int y) { sensorsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([sensorsMenu, addBackButton, addToParent]() {
        addBackButton(sensorsMenu);
        sensorsMenu->setup();
        sensorsMenu->setVisible(false);
        addToParent(sensorsMenu);
    });
    this->renderer->addLoopTask([sensorsMenu]() {
        sensorsMenu->draw();
    });
    menus.push_back(sensorsMenu);

    ///////// Sound Menu /////////
    auto soundMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    soundMenu->setMenuName("Sound");
    soundMenu->setUniqueMenuIdentifier("Sound");
    soundMenu->setVisible(false);
    soundMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [soundMenu]() { return soundMenu->getVisible(); }, [soundMenu](int y) { soundMenu->scrollVert(y); } });
    this->renderer->addSetupTask([soundMenu, addBackButton, addToParent]() {
        addBackButton(soundMenu);
        soundMenu->setup();
        soundMenu->setVisible(false);
        addToParent(soundMenu);
    });
    this->renderer->addLoopTask([soundMenu]() {
        soundMenu->draw();
    });
    menus.push_back(soundMenu);

    ///////// Notifications Menu /////////
    auto notificationsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    notificationsMenu->setMenuName("Notifications");
    notificationsMenu->setUniqueMenuIdentifier("Notifications");
    notificationsMenu->setVisible(false);
    notificationsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [&]() { return notificationsMenu->getVisible(); }, [notificationsMenu](int y) { notificationsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([notificationsMenu, addBackButton, addToParent]() {
        addBackButton(notificationsMenu);
        notificationsMenu->setup();
        notificationsMenu->setVisible(false);
        addToParent(notificationsMenu);
    });
    this->renderer->addLoopTask([notificationsMenu]() {
        notificationsMenu->draw();
    });
    menus.push_back(notificationsMenu);

    ///////// Display Menu /////////
    auto displayMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    displayMenu->setMenuName("Display");
    displayMenu->setUniqueMenuIdentifier("Display");
    displayMenu->setVisible(false);
    displayMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [displayMenu]() { return displayMenu->getVisible(); }, [displayMenu](int y) { displayMenu->scrollVert(y); } });
    this->renderer->addSetupTask([displayMenu, addBackButton, addToParent]() {
        addBackButton(displayMenu);
        displayMenu->setup();
        displayMenu->setVisible(false);
        addToParent(displayMenu);
    });
    this->renderer->addLoopTask([displayMenu]() {
        displayMenu->draw();
    });
    menus.push_back(displayMenu);

    ///////// Privacy Menu /////////
    auto privacyMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    privacyMenu->setMenuName("Privacy");
    privacyMenu->setUniqueMenuIdentifier("Privacy");
    privacyMenu->setVisible(false);
    privacyMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [privacyMenu]() { return privacyMenu->getVisible(); }, [privacyMenu](int y) { privacyMenu->scrollVert(y); } });
    this->renderer->addSetupTask([privacyMenu, addBackButton, addToParent]() {
        addBackButton(privacyMenu);
        privacyMenu->setup();
        privacyMenu->setVisible(false);
        addToParent(privacyMenu);
    });
    this->renderer->addLoopTask([privacyMenu]() {
        privacyMenu->draw();
    });
    menus.push_back(privacyMenu);

    ///////// Privacy Menu - Privacy Settings /////////
    // TODO: figure out what to put here: list apps that use microphone, presence detection, etc. and allow the user to disable them
    // also, telemetry stuff

    ///////// Accounts Menu /////////
    auto accountsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    accountsMenu->setMenuName("Accounts");
    accountsMenu->setUniqueMenuIdentifier("Accounts");
    accountsMenu->setVisible(false);
    accountsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [accountsMenu]() { return accountsMenu->getVisible(); }, [accountsMenu](int y) { accountsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([accountsMenu, addBackButton, addToParent]() {
        addBackButton(accountsMenu);
        accountsMenu->setup();
        accountsMenu->setVisible(false);
        addToParent(accountsMenu);
    });
    this->renderer->addLoopTask([accountsMenu]() {
        accountsMenu->draw();
    });
    menus.push_back(accountsMenu);

    ///////// Accounts Menu - Account List /////////
    // TODO: list all the accounts that have been added to the system. These are stored in the database.

    ///////// Accounts Menu - Account List - Account Settings /////////
    // TODO:

    ///////// Accounts Menu - Account List - Remove Account /////////
    // TODO:

    ///////// Accounts Menu - Add Account /////////
    // TODO: add a button to add an account

    ///////// Apps Menu /////////
    auto appsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    appsMenu->setMenuName("Apps");
    appsMenu->setUniqueMenuIdentifier("Apps");
    appsMenu->setVisible(false);
    appsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [appsMenu]() { return appsMenu->getVisible(); }, [appsMenu](int y) { appsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([appsMenu, addBackButton, addToParent]() {
        addBackButton(appsMenu);
        appsMenu->setup();
        appsMenu->setVisible(false);
        addToParent(appsMenu);
    });
    this->renderer->addLoopTask([appsMenu]() {
        appsMenu->draw();
    });
    menus.push_back(appsMenu);

    ///////// Apps Menu - Installed Apps /////////
    // TODO:

    ///////// Apps Menu - Installed Apps - App List /////////
    // TODO: for each installed app do the following
    ///////// Apps Menu - Installed Apps - App List - Force Stop /////////
    ///////// Apps Menu - Installed Apps - App List - Uninstall /////////
    ///////// Apps Menu - Installed Apps - App List - Reload /////////
    ///////// Apps Menu - Installed Apps - App List - App Settings /////////
    // TODO: each app will fill in the details here

    ///////// Apps Menu - Available Apps /////////
    // TODO:

    ///////// Apps Menu - Available Apps - App List /////////
    // TODO: List all the available apps here. each app gets an install button and a details button

    ///////// General Settings Menu /////////
    auto generalSettingsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    generalSettingsMenu->setMenuName("General Settings");
    generalSettingsMenu->setUniqueMenuIdentifier("General Settings");
    generalSettingsMenu->setVisible(false);
    generalSettingsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [generalSettingsMenu]() { return generalSettingsMenu->getVisible(); }, [generalSettingsMenu](int y) { generalSettingsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([generalSettingsMenu, addBackButton, addToParent]() {
        addBackButton(generalSettingsMenu);
        generalSettingsMenu->setup();
        generalSettingsMenu->setVisible(false);
        addToParent(generalSettingsMenu);
    });
    this->renderer->addLoopTask([generalSettingsMenu]() {
        generalSettingsMenu->draw();
    });
    menus.push_back(generalSettingsMenu);

    ///////// General Settings Menu - Date and Time /////////
    auto generalSettingsDateTimeMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    generalSettingsDateTimeMenu->setMenuName("Date and Time");
    generalSettingsDateTimeMenu->setUniqueMenuIdentifier("Date and Time");
    generalSettingsDateTimeMenu->setVisible(false);
    generalSettingsDateTimeMenu->setParentMenu(generalSettingsMenu);
    drag_y_actions.push_back({ [generalSettingsDateTimeMenu]() { return generalSettingsDateTimeMenu->getVisible(); }, [generalSettingsDateTimeMenu](int y) { generalSettingsDateTimeMenu->scrollVert(y); } });
    this->renderer->addSetupTask([generalSettingsDateTimeMenu, addBackButton, addToParent]() {
        addBackButton(generalSettingsDateTimeMenu);
        generalSettingsDateTimeMenu->setup();
        generalSettingsDateTimeMenu->setVisible(false);
        addToParent(generalSettingsDateTimeMenu);
    });
    this->renderer->addLoopTask([generalSettingsDateTimeMenu]() {
        generalSettingsDateTimeMenu->draw();
    });
    menus.push_back(generalSettingsDateTimeMenu);

    ///////// General Settings Menu - Date and Time - Set Date and Time /////////
    auto generalSettingsDateTimeSetDateTimeMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    generalSettingsDateTimeSetDateTimeMenu->setMenuName("Set Date and Time");
    generalSettingsDateTimeSetDateTimeMenu->setUniqueMenuIdentifier("Set Date and Time");
    generalSettingsDateTimeSetDateTimeMenu->setVisible(false);
    generalSettingsDateTimeSetDateTimeMenu->setParentMenu(generalSettingsDateTimeMenu);
    drag_y_actions.push_back({ [generalSettingsDateTimeSetDateTimeMenu]() { return generalSettingsDateTimeSetDateTimeMenu->getVisible(); }, [generalSettingsDateTimeSetDateTimeMenu](int y) { generalSettingsDateTimeSetDateTimeMenu->scrollVert(y); } });
    this->renderer->addSetupTask([generalSettingsDateTimeSetDateTimeMenu, addBackButton, addToParent]() {
        generalSettingsDateTimeSetDateTimeMenu->setup();
        generalSettingsDateTimeSetDateTimeMenu->setVisible(false);
        std::vector<std::string>* fields = new std::vector<std::string>({ "Year", "Month", "Day", "Hour", "Minute" });
        generalSettingsDateTimeSetDateTimeMenu->getParentMenu()->addMenuEntry(
            "Set Date and Time",
            "Date_Time_Set_Date_Time",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INPUT,
            [generalSettingsDateTimeSetDateTimeMenu](void* data) {
                CubeLog::info("Set Date and Time clicked");
                std::vector<std::string> fields = { "Year", "Month", "Day", "Hour", "Minute" };
                // GUI::showTextInput("Set Date and Time", field, { 720, 720 }, { 0, 0 }, [generalSettingsDateTimeSetDateTimeMenu](std::vector<std::string> textVector) {
                //     CubeLog::info("Set Date and Time: " + text);
                //     // do something with the text in the textVector
                //     generalSettingsDateTimeSetDateTimeMenu->getParentMenu()->setVisible(true);
                //     generalSettingsDateTimeSetDateTimeMenu->getParentMenu()->setIsClickable(false);
                // });
                // generalSettingsDateTimeSetDateTimeMenu->getParentMenu()->setVisible(false);
                // generalSettingsDateTimeSetDateTimeMenu->getParentMenu()->setIsClickable(true); 
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
    });
    this->renderer->addLoopTask([generalSettingsDateTimeSetDateTimeMenu]() {
        generalSettingsDateTimeSetDateTimeMenu->draw();
    });
    menus.push_back(generalSettingsDateTimeSetDateTimeMenu);
    ///////// General Settings Menu - Date and Time - Set Time Zone /////////
    ///////// General Settings Menu - Date and Time - Set Time Format /////////
    ///////// General Settings Menu - Date and Time - Set Date Format /////////
    ///////// General Settings Menu - Date and Time - Enable Automatic Date and Time /////////

    ///////// General Settings Menu - Language /////////
    // TODO:

    ///////// Accessibility Menu /////////
    auto accessibilityMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    accessibilityMenu->setMenuName("Accessibility");
    accessibilityMenu->setUniqueMenuIdentifier("Accessibility");
    accessibilityMenu->setVisible(false);
    accessibilityMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [accessibilityMenu]() { return accessibilityMenu->getVisible(); }, [accessibilityMenu](int y) { accessibilityMenu->scrollVert(y); } });
    this->renderer->addSetupTask([accessibilityMenu, addBackButton, addToParent]() {
        addBackButton(accessibilityMenu);
        accessibilityMenu->setup();
        accessibilityMenu->setVisible(false);
        addToParent(accessibilityMenu);
    });
    this->renderer->addLoopTask([accessibilityMenu]() {
        accessibilityMenu->draw();
    });
    menus.push_back(accessibilityMenu);

    ///////// Accessibility Menu - ??? /////////
    // TODO: figure out what should go here

    ///////// Updates Menu /////////
    auto updatesMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    updatesMenu->setMenuName("Updates");
    updatesMenu->setUniqueMenuIdentifier("Updates");
    updatesMenu->setVisible(false);
    updatesMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [updatesMenu]() { return updatesMenu->getVisible(); }, [updatesMenu](int y) { updatesMenu->scrollVert(y); } });
    this->renderer->addSetupTask([updatesMenu, addBackButton, addToParent]() {
        addBackButton(updatesMenu);
        updatesMenu->setup();
        updatesMenu->setVisible(false);
        addToParent(updatesMenu);
    });
    this->renderer->addLoopTask([updatesMenu]() {
        updatesMenu->draw();
    });
    menus.push_back(updatesMenu);

    ///////// Updates Menu - Check for Updates /////////
    // TODO:

    ///////// Updates Menu - Last Update Info /////////
    // TODO:

    ///////// Updates Menu - Auto Update Enable /////////
    // TODO:

    ///////// About Menu /////////
    auto aboutMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    aboutMenu->setMenuName("About");
    aboutMenu->setUniqueMenuIdentifier("About");
    aboutMenu->setVisible(false);
    aboutMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [aboutMenu]() { return aboutMenu->getVisible(); }, [aboutMenu](int y) { aboutMenu->scrollVert(y); } });
    this->renderer->addSetupTask([aboutMenu, addBackButton, addToParent]() {
        addBackButton(aboutMenu);
        aboutMenu->addMenuEntry(
            "Serial Number",
            "About_Serial_Number",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
            [aboutMenu](void* data) {
                CubeLog::info("Serial Number clicked");
                // TODO: get the serial number from the hardware class
                GUI::showTextBox("Serial Number", "Serial Number: 1234567890h", { 720, 720 }, { 0, 0 }, [aboutMenu]() {
                    aboutMenu->setVisible(true);
                    aboutMenu->setIsClickable(false);
                });
                aboutMenu->setVisible(false);
                aboutMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
        aboutMenu->setup();
        aboutMenu->setVisible(false);
        addToParent(aboutMenu);
    });
    this->renderer->addLoopTask([aboutMenu]() {
        aboutMenu->draw();
    });
    menus.push_back(aboutMenu);

    ///////// About Menu - Serial Number /////////
    // auto aboutSerialNumberMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    // aboutSerialNumberMenu->setMenuName("Serial Number");
    // aboutSerialNumberMenu->setUniqueMenuIdentifier("Serial Number");
    // aboutSerialNumberMenu->setVisible(false);
    // aboutSerialNumberMenu->setParentMenu(aboutMenu);
    // drag_y_actions.push_back({ [aboutSerialNumberMenu]() { return aboutSerialNumberMenu->getVisible(); }, [aboutSerialNumberMenu](int y) { aboutSerialNumberMenu->scrollVert(y); } });
    // this->renderer->addSetupTask([&]() {
    //     aboutSerialNumberMenu->setup();
    //     aboutSerialNumberMenu->setVisible(false);
    // aboutSerialNumberMenu->getParentMenu()->addMenuEntry(
    //     "Serial Number",
    //     "About_Serial_Number",
    //     MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
    //     [aboutSerialNumberMenu](void* data) {
    //         CubeLog::info("Serial Number clicked");
    //         // TODO: get the serial number from the hardware class
    //         GUI::showTextBox("Serial Number", "Serial Number: 1234567890h", { 720, 720 }, { 0, 0 }, [aboutSerialNumberMenu]() {
    //             aboutSerialNumberMenu->getParentMenu()->setVisible(true);
    //             aboutSerialNumberMenu->getParentMenu()->setIsClickable(false);
    //         });
    //         aboutSerialNumberMenu->getParentMenu()->setVisible(false);
    //         aboutSerialNumberMenu->getParentMenu()->setIsClickable(true);
    //         return 0;
    //     },
    //     [](void*) {
    //         return 0;
    //     },
    //     nullptr);
    // });
    // this->renderer->addLoopTask([aboutSerialNumberMenu]() {
    //     aboutSerialNumberMenu->draw();
    // });
    // menus.push_back(aboutSerialNumberMenu);

    ///////// About Menu - Hardware Version /////////
    auto aboutHardwareVersionMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    aboutHardwareVersionMenu->setMenuName("Hardware Version");
    aboutHardwareVersionMenu->setUniqueMenuIdentifier("Hardware Version");
    aboutHardwareVersionMenu->setVisible(false);
    aboutHardwareVersionMenu->setParentMenu(aboutMenu);
    drag_y_actions.push_back({ [aboutHardwareVersionMenu]() { return aboutHardwareVersionMenu->getVisible(); }, [aboutHardwareVersionMenu](int y) { aboutHardwareVersionMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&]() {
        aboutHardwareVersionMenu->setup();
        aboutHardwareVersionMenu->setVisible(false);
        aboutHardwareVersionMenu->getParentMenu()->addMenuEntry(
            "Hardware Version",
            "About_Hardware_Version",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
            [aboutHardwareVersionMenu](void* data) {
                CubeLog::info("Hardware Version clicked");
                // TODO: get the hardware version from the hardware class
                GUI::showTextBox("Hardware Version", "Hardware Version: 1.0.0", { 720, 720 }, { 0, 0 }, [aboutHardwareVersionMenu]() {
                    aboutHardwareVersionMenu->getParentMenu()->setVisible(true);
                    aboutHardwareVersionMenu->getParentMenu()->setIsClickable(false);
                });
                aboutHardwareVersionMenu->getParentMenu()->setVisible(false);
                aboutHardwareVersionMenu->getParentMenu()->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
    });
    this->renderer->addLoopTask([aboutHardwareVersionMenu]() {
        aboutHardwareVersionMenu->draw();
    });
    menus.push_back(aboutHardwareVersionMenu);

    ///////// About Menu - Software Information /////////
    auto aboutSoftwareInformationMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    aboutSoftwareInformationMenu->setMenuName("Software Information");
    aboutSoftwareInformationMenu->setUniqueMenuIdentifier("Software Information");
    aboutSoftwareInformationMenu->setVisible(false);
    aboutSoftwareInformationMenu->setParentMenu(aboutMenu);
    drag_y_actions.push_back({ [aboutSoftwareInformationMenu]() { return aboutSoftwareInformationMenu->getVisible(); }, [aboutSoftwareInformationMenu](int y) { aboutSoftwareInformationMenu->scrollVert(y); } });
    this->renderer->addSetupTask([aboutSoftwareInformationMenu, addBackButton, addToParent]() {
        addBackButton(aboutSoftwareInformationMenu);
        aboutSoftwareInformationMenu->setup();
        aboutSoftwareInformationMenu->setVisible(false);
        aboutSoftwareInformationMenu->getParentMenu()->addMenuEntry(
            "Software Information",
            "About_Software_Information",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
            [aboutSoftwareInformationMenu](void* data) {
                CubeLog::info("Software Information clicked");
                aboutSoftwareInformationMenu->setVisible(true);
                aboutSoftwareInformationMenu->getParentMenu()->setVisible(false);
                aboutSoftwareInformationMenu->getParentMenu()->setIsClickable(false);
                // TODO: replace this with the actual software information similar to the hardware information
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
    });
    this->renderer->addLoopTask([aboutSoftwareInformationMenu]() {
        aboutSoftwareInformationMenu->draw();
    });
    menus.push_back(aboutSoftwareInformationMenu);

    ///////// About Menu - Software Information - TheCube-CORE Version /////////
    // TODO:

    ///////// About Menu - Software Information - Build Number /////////
    // TODO:

    ///////// About Menu - Status /////////
    // TODO:

    ///////// About Menu - Status - IP Address /////////
    // TODO:

    ///////// About Menu - Status - WIFi MAC Address /////////
    // TODO:

    ///////// About Menu - Status - BT Mac Address /////////
    // TODO:

    ///////// About Menu - Status - Up Time /////////
    // TODO:

    ///////// About Menu - Status - Memory Usage /////////
    // TODO:

    ///////// About Menu - Status - Disk Usage /////////
    // TODO:

    ///////// About Menu - Status - CPU Usage /////////
    // TODO:

    ///////// About Menu - Status - FCC ID /////////
    // TODO:

    ///////// About Menu - Legal Information /////////
    // TODO:

    ///////// About Menu - Legal Information - Open Source Licenses /////////
    // TODO:

    ///////// About Menu - Legal Information - Legal Notices /////////
    // TODO:

    ///////// About Menu - Legal Information - Trademarks /////////
    // TODO:

    ///////// About Menu - Legal Information - EULA /////////
    // TODO:

    ///////// About Menu - Legal Information - Privacy Policy /////////
    // TODO:

    ///////// Developer Settings Menu /////////
    auto developerSettingsMenu = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
    developerSettingsMenu->setMenuName("Developer Settings");
    developerSettingsMenu->setUniqueMenuIdentifier("Developer Settings");
    developerSettingsMenu->setVisible(false);
    developerSettingsMenu->setParentMenu(mainMenu);
    drag_y_actions.push_back({ [developerSettingsMenu]() { return developerSettingsMenu->getVisible(); }, [developerSettingsMenu](int y) { developerSettingsMenu->scrollVert(y); } });
    this->renderer->addSetupTask([developerSettingsMenu, addBackButton, addToParent]() {
        addBackButton(developerSettingsMenu);
        developerSettingsMenu->setup();
        developerSettingsMenu->setVisible(false);
        addToParent(developerSettingsMenu);
    });
    this->renderer->addLoopTask([developerSettingsMenu]() {
        developerSettingsMenu->draw();
    });
    menus.push_back(developerSettingsMenu);

    ///////// Developer Settings Menu - Developer Mode Enable /////////
    // TODO:

    ///////// Developer Settings Menu - SSH /////////
    // TODO:

    ///////// Developer Settings Menu - SSH - Enable SSH /////////
    // TODO:

    ///////// Developer Settings Menu - SSH - Change Password /////////
    // TODO:

    ///////// Developer Settings Menu - CPU and Memory Display /////////
    // TODO:

    ///////// Developer Settings Menu - Send Bug Report /////////
    // TODO:

    ///////// Developer Settings Menu - Verify SSD Integrity /////////
    // TODO:

    ///////// Developer Settings Menu - Factory Reset /////////
    // TODO:

    ///////// Developer Settings Menu - Reboot /////////
    // TODO:

    ///////// Developer Settings Menu - Shutdown /////////
    // TODO:

    ///////// Developer Settings Menu - Logging /////////
    // TODO:

    ///////// Developer Settings Menu - Logging - File Log Level /////////
    // TODO:

    ///////// Developer Settings Menu - Logging - Telemetry Log Level /////////
    // TODO:

    ///////// Developer Settings Menu - Logging - Log Verbosity /////////
    // TODO:

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
    messageBox = new CubeMessageBox(this->renderer->getMeshShader(), this->renderer->getTextShader(), this->renderer, countingLatch);
    // countingLatch.count_up();
    this->renderer->addSetupTask([&]() {
        messageBox->setup();
    });
    this->renderer->addLoopTask([&]() {
        messageBox->draw();
    });
    messageBox->setVisible(false);
    MakeCubeBoxClickable<CubeMessageBox> clickable_messageBox(messageBox);
    this->eventManager->addClickableArea(clickable_messageBox.getClickableArea());

    ////////////////////////////////////////
    /// Set up the full screen text box
    ////////////////////////////////////////
    fullScreenTextBox = new CubeTextBox(this->renderer->getMeshShader(), this->renderer->getTextShader(), this->renderer, countingLatch);
    // countingLatch.count_up();
    this->renderer->addSetupTask([&]() {
        fullScreenTextBox->setup();
    });
    this->renderer->addLoopTask([&]() {
        fullScreenTextBox->draw();
    });
    fullScreenTextBox->setVisible(false);
    MakeCubeBoxClickable<CubeTextBox> clickable_fullScreenTextBox(fullScreenTextBox);
    this->eventManager->addClickableArea(clickable_fullScreenTextBox.getClickableArea());

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

/**
 * @brief Add a menu to the GUI
 *
 * @param menuName the name of the menu
 * @param parentName the name of the parent menu
 * @param data a vector of tuples containing the text, json data, and unique identifier for the menu entries
 */
GUI_Error GUI::addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, AddMenu_Data_t data)
{
    CubeLog::debugSilly("Adding menu: " + menuName + " with parent: " + parentID);
    std::vector<std::string> uniqueIDs;
    for (int i = 0; i < data.size(); i++) {
        uniqueIDs.push_back(std::get<2>(data.at(i)));
    }
    CubeLog::debugSilly("Locking addMenuMutex");
    std::unique_lock<std::mutex> lock(this->addMenuMutex);
    auto aNewMenu = new MENUS::Menu(this->renderer);
    aNewMenu->setMenuName(menuName);
    aNewMenu->setUniqueMenuIdentifier(thisUniqueID);
    aNewMenu->setVisible(false);
    CubeLog::debugSilly("Checking for unique identifiers for menu: " + menuName);
    MENUS::Menu* parentMenuPtr = nullptr;
    bool unique = true;
    for (auto m : menus) {
        if (m->getUniqueMenuIdentifier() == parentID) {
            parentMenuPtr = m;
            CubeLog::moreInfo("Parent menu found for menu: " + menuName);
        }
        for (auto t : uniqueIDs) {
            if (m->getUniqueMenuIdentifier() == t) {
                unique = false;
                CubeLog::moreInfo("Unique identifier: " + t + " already exists");
            }
        }
        if (thisUniqueID == m->getUniqueMenuIdentifier()) {
            unique = false;
            CubeLog::moreInfo("Unique identifier: " + thisUniqueID + " already exists");
        }
    }
    if (!unique) {
        CubeLog::error("Unique menu identifiers for menu and children are not unique for menu: " + menuName);
        delete aNewMenu;
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_UNIQUE_EXISTS, "Unique menu identifiers for menu and children are not unique for menu: " + menuName);
    }
    if (parentMenuPtr == nullptr) {
        CubeLog::error("Parent menu not found for menu: " + menuName);
        delete aNewMenu;
        return GUI_Error(GUI_Error::ERROR_TYPES::GUI_PARENT_NOT_FOUND, "Parent menu not found for menu: " + menuName);
    }
    menus.push_back(aNewMenu);
    EventManager* eventManagerPtr = this->eventManager;
    drag_y_actions.push_back({ [aNewMenu]() { return aNewMenu->getVisible(); }, [aNewMenu](int y) { aNewMenu->scrollVert(y); } });
    int menuAddParsed = 0;
    std::mutex menuAddParsedMutex;
    this->renderer->addSetupTask([parentMenuPtr, menuName, aNewMenu, data, eventManagerPtr, thisUniqueID, &menuAddParsed, &menuAddParsedMutex]() {
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
            if (!parseJsonAndAddEntriesToMenu(std::get<1>(data[i]), aNewMenu)) {
                CubeLog::error("Error parsing json and adding entries to menu: " + menuName);
                success = false;
            }
        }
        if (!success) {
            CubeLog::error("Error parsing json and adding entries to menu: " + menuName);
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
        std::unique_lock<std::mutex> lock(menuAddParsedMutex);
        if (!success) {
            menuAddParsed = 1;
        } else {
            menuAddParsed = 2;
        }
    });
    CubeLog::moreInfo("Added menu: " + menuName + " with parent: " + parentID);
    CubeLog::moreInfo("Adding draw function for menu: " + menuName);
    this->renderer->addLoopTask([aNewMenu]() {
        aNewMenu->draw();
    });
    CubeLog::moreInfo("Menu setup complete: " + menuName);
    bool wait = true;
    while (wait) {
        {
            std::unique_lock<std::mutex> lock(menuAddParsedMutex);
            if (menuAddParsed == 1) {
                return GUI_Error(GUI_Error::ERROR_TYPES::GUI_INTERNAL_ERROR, "Error parsing json and adding entries to menu: " + menuName);
            }
            if (menuAddParsed == 2) {
                wait = false;
            }
        }
        genericSleep(15);
    }
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
    messageBox->setText(message, title);
    messageBox->setVisible(true);
}

void GUI::showMessageBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position)
{
    // check that messageBox is not null pointer
    if (messageBox == nullptr) {
        CubeLog::error("Message box is null. Cannot show message.");
        return;
    }
    messageBox->setSize(size);
    messageBox->setPosition(position);
    showMessageBox(title, message);
}

void GUI::showMessageBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position, std::function<void()> callback)
{
    // check that messageBox is not null pointer
    if (messageBox == nullptr) {
        CubeLog::error("Message box is null. Cannot show message.");
        return;
    }
    messageBox->setCallback(callback);
    showMessageBox(title, message, size, position);
}

void GUI::showTextBox(std::string title, std::string message)
{
    // check that fullScreenTextBox is not null pointer
    if (fullScreenTextBox == nullptr) {
        CubeLog::error("Full screen text box is null. Cannot show text.");
        return;
    }
    fullScreenTextBox->setText(message, title);
    fullScreenTextBox->setVisible(true);
}

void GUI::showTextBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position)
{
    // check that fullScreenTextBox is not null pointer
    if (fullScreenTextBox == nullptr) {
        CubeLog::error("Full screen text box is null. Cannot show text.");
        return;
    }
    fullScreenTextBox->setSize(size);
    fullScreenTextBox->setPosition(position);
    showTextBox(title, message);
}

void GUI::showTextBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position, std::function<void()> callback)
{
    // check that fullScreenTextBox is not null pointer
    if (fullScreenTextBox == nullptr) {
        CubeLog::error("Full screen text box is null. Cannot show text.");
        return;
    }
    fullScreenTextBox->setCallback(callback);
    showTextBox(title, message, size, position);
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
    - addCharacter
    - removeCharacter
    - animateCharacter
    */
    HttpEndPointData_t actions;
    actions.push_back(
        { PUBLIC_ENDPOINT | GET_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                // this->stop();
                std::string mes = "";
                std::string title = "";
                for (auto param : req.params) {
                    if (param.first == "text") {
                        mes = param.second;
                    } else if (param.first == "title") {
                        title = param.second;
                    }
                }
                if (title == "" || mes == "")
                    messageBox->setVisible(false);
                else
                    showMessageBox(title, mes);
                CubeLog::info("Endpoint stop called and message set to: " + mes + " with title: " + title);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Message set to: " + mes);
            },
            "messageBox",
            { "text", "title" },
            "Show a message box with a title and message" });
    actions.push_back(
        { PUBLIC_ENDPOINT | GET_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                std::string mes = "";
                std::string title = "";
                if (req.has_param("text"))
                    mes = req.get_param_value("text");
                if (req.has_param("title"))
                    title = req.get_param_value("title");
                std::string size_x, size_y, position_x, position_y;
                if (req.has_param("size-x"))
                    size_x = req.get_param_value("size-x");
                else
                    size_x = "720";
                if (req.has_param("size-y"))
                    size_y = req.get_param_value("size-y");
                else
                    size_y = "720";
                if (req.has_param("position-x"))
                    position_x = req.get_param_value("position-x");
                else
                    position_x = "0";
                if (req.has_param("position-y"))
                    position_y = req.get_param_value("position-y");
                else
                    position_y = "0";
                if (title == "" || mes == "")
                    fullScreenTextBox->setVisible(false);
                else
                    showTextBox(title, mes, { std::stoi(size_x), std::stoi(size_y) }, { std::stoi(position_x), std::stoi(position_y) });
                CubeLog::info("Endpoint stop called and message set to: " + mes + " with title: " + title);
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Message set to: " + mes);
            },
            "textBox",
            { "text", "title", "size-x", "size-y", "position-x", "position-y" },
            "Show a text box with a title and message of a specified size and position. Default values for size and position are 720x720 at 0,0." });
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
            },
            "addMenu",
            {},
            "Add a menu to the GUI" });
    return actions;
}

/**
 * @brief Get the Http Endpoint Names And Params object
 *
 * @return std::vector<std::pair<std::string, std::vector<std::string>>> a vector of pairs of strings and vectors of strings
 */
// std::vector<std::pair<std::string, std::vector<std::string>>> GUI::getHttpEndpointNamesAndParams()
// {
//     std::vector<std::pair<std::string, std::vector<std::string>>> names;
//     names.push_back({ "messageBox", { "text", "title" } });
//     names.push_back({ "textBox", { "text", "title", "size-x", "size-y", "position-x", "position-y" } });
//     names.push_back({ "addMenu", {} });
//     return names;
// }

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
        if (!j["entryData"]["actionEndpoint"].is_null()) {
            // for some reason we can't index more than two levels deep with [] operator. So we'll just make a new json object and index that.
            nlohmann::json j2 = j["entryData"]["actionEndpoint"];
            actionEP_AddrPort = j2["addr_port"];
            actionEP_Path = j2["path"];
            actionEP_Method = j2["method"];
            actionEP_User = j2["user"];
            actionEP_Pass = j2["password"];
            actionEP_Token = j2["token"];
            hasActionData = true;
        }
    } catch (nlohmann::json::exception& e) {
        CubeLog::moreInfo("No action endpoint provided: " + entryText);
        CubeLog::debugSilly("Error provided by json: " + std::string(e.what()));
    }
    try {
        if (!j["entryData"]["statusEndpoint"].is_null()) {
            nlohmann::json j2 = j["entryData"]["statusEndpoint"];
            statusEP_AddrPort = j2["addr_port"];
            statusEP_Path = j2["path"];
            statusEP_Method = j2["method"];
            statusEP_User = j2["user"];
            statusEP_Pass = j2["password"];
            statusEP_Token = j2["token"];
            hasStatusData = true;
        }
    } catch (nlohmann::json::exception& e) {
        CubeLog::moreInfo("No status endpoint provided: " + entryText);
        CubeLog::debugSilly("Error provided by json: " + std::string(e.what()));
    }
    if (type != MENUS::EntryType::MENUENTRY_TYPE_SUBMENU && !hasActionData && !hasStatusData) {
        CubeLog::error("No action or status endpoint provided: " + entryText);
        return false;
    }
    if (type == MENUS::EntryType::MENUENTRY_TYPE_ACTION && !hasActionData) {
        CubeLog::error("No action endpoint provided: " + entryText);
        return false;
    }
    if ((!hasActionData || !hasStatusData) && (type != MENUS::EntryType::MENUENTRY_TYPE_ACTION && type != MENUS::EntryType::MENUENTRY_TYPE_SUBMENU)) {
        CubeLog::error("No status endpoint provided or no action endpoint provided: " + entryText);
        return false;
    }
    int sliderMinValue = 0, sliderMaxValue = 0, sliderStep = 0;
    if (type == MENUS::EntryType::MENUENTRY_TYPE_SLIDER) {
        try {
            sliderMinValue = j["entryData"]["minValue"];
            sliderMaxValue = j["entryData"]["maxValue"];
            sliderStep = j["entryData"]["step"];
        } catch (nlohmann::json::exception& e) {
            CubeLog::error("Error parsing json: " + std::string(e.what()));
            return false;
        }
        if (sliderMinValue >= sliderMaxValue || sliderStep <= 0) {
            CubeLog::error("Invalid slider values: " + entryText);
            return false;
        }
    }
    unsigned int menuEntryID = 0;
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
                    CubeLog::error("Invalid method: " + statusEP_Method + " From: " + statusEP_AddrPort + statusEP_Path);
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
            (void*)new std::string("default text"));
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO: {
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
            [statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals](void* sharedPtr_textInfo) {
                std::shared_ptr<std::string>* stringPtr = static_cast<std::shared_ptr<std::string>*>(sharedPtr_textInfo);
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
                    std::string* t = new std::string(response);
                    stringPtr->reset(t);
                    return (unsigned int)0;
                }
                std::string* t = new std::string(j["text"]);
                stringPtr->reset(t);
                return (unsigned int)0;
            },
            (void*)new std::shared_ptr<std::string>(new std::string("default text")));
        break;
    }
    case MENUS::EntryType::MENUENTRY_TYPE_SLIDER: {
        menuEntryID = menuEntry->addMenuEntry(
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
            [statusEP_AddrPort, statusEP_Path, statusEP_Method, statusEP_User, statusEP_Pass, statusEP_Token, enabledVals, sliderMinValue, sliderMaxValue, sliderStep](void* int_value) {
                int value = *(int*)int_value;
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
                    res = client.Get(statusEP_Path + "?value=" + std::to_string(value));
                if (statusEP_Method == "POST") {
                    nlohmann::json j;
                    j["value"] = value;
                    res = client.Post(statusEP_Path.c_str(), j.dump(), "application/json");
                }
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

                std::string enabled = j["success"];
                unsigned int retVal = 0;
                if (enabled == "true")
                    retVal = 1;
                return retVal;
            },
            (void*)new int(0));
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
// TODO: Need to add a sort of status bar to the top of the screen. It should show the time and whether or not a person is detected. probably more.
// TODO: we should monitor the CubeLog for errors and display them in the status bar. This will require a way to get the last error message from the CubeLog. <- this is done in CubeLog
// TODO: setup notifications that pop up with a CubeMessageBox. this will need to have notifications.cpp fleshed out.

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
    /// TESTING FUNCTION TODO: remove this or move to utils
    ////////////////////////////////////////
    auto repeatChar = [](std::string& inOut, int n, const std::string& repeatChars) {
        for (size_t i = 0; i < n; i++) {
            inOut += repeatChars;
        }
        return inOut;
    };

    ////////////////////////////////////////
    /// Here we build the menus
    ////////////////////////////////////////
    CountingLatch countingLatch(22); // this value must be equal to count of "new MENUS::Menu()" calls in this method + 2 (for the message box and text box)

    // Helper function to add a back button to a menu with a horizontal rule
    auto addBackButton = [](auto* menu) {
        menu->addMenuEntry(
            "< Back - " + menu->getMenuName(),
            menu->getMenuName() + "_back",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [menu](void* data) {
                CubeLog::info(menu->getMenuName() + " - Back clicked");
                menu->setVisible(false);
                menu->setChildrenClickables_isClickable(false);
                menu->setIsClickable(true);
                if (menu->getParentMenu() != nullptr) {
                    menu->getParentMenu()->setVisible(true);
                    menu->getParentMenu()->setIsClickable(false);
                    menu->getParentMenu()->setChildrenClickables_isClickable(true);
                }
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        menu->addHorizontalRule();
        menu->setChildrenClickables_isClickable(false);
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
                menu->setIsClickable(false);
                menu->setChildrenClickables_isClickable(true);
                menu->getParentMenu()->setVisible(false);
                menu->getParentMenu()->setIsClickable(false);
                menu->getParentMenu()->setChildrenClickables_isClickable(false);
                return 0;
            },
            [](void*) { return 0; },
            nullptr);
        menu->getParentMenu()->setChildrenClickables_isClickable(false);
    };

    // Helper function to create a new submenu
    auto createANewSubMenu = [&](const std::string& name, const std::string& u_id, MENUS::Menu* parent) -> MENUS::Menu* {
        auto m = new MENUS::Menu(this->renderer, countingLatch, 0, 0, 0, 0);
        m->setMenuName(name);
        m->setUniqueMenuIdentifier(u_id);
        m->setVisible(false);
        m->setIsClickable(false);
        m->setChildrenClickables_isClickable(false);
        m->setParentMenu(parent);
        drag_y_actions.push_back({ [m]() { return m->getVisible(); }, [m](int y) { m->scrollVert(y); } });
        this->renderer->addLoopTask([m]() {
            m->draw();
        });
        menus.push_back(m);
        return m;
    };

    ///////// Main Menu /////////
    auto mainMenu = new MENUS::Menu(this->renderer, countingLatch);
    mainMenu->setAsMainMenu();
    mainMenu->setMenuName(_("Main Menu"));
    mainMenu->setUniqueMenuIdentifier("Main Menu");
    menus.push_back(mainMenu);
    drag_y_actions.push_back({ [&mainMenu]() { return mainMenu->getVisible(); }, [mainMenu](int y) { mainMenu->scrollVert(y); } });
    this->renderer->addSetupTask([&mainMenu, addBackButton]() {
        addBackButton(mainMenu);
        mainMenu->setup();
    });
    mainMenu->setOnClick([&](void* data) {
        mainMenu->setVisible(!mainMenu->getVisible());
        mainMenu->setIsClickable(!mainMenu->getIsClickable());
        if (mainMenu->getVisible()) {
            mainMenu->setChildrenClickables_isClickable(true);
        }
        return 0;
    });
    mainMenu->setIsClickable(true);
    mainMenu->setVisible(false);
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
    auto connectionsMenu = createANewSubMenu(_("Connections"), "Connections", mainMenu);
    this->renderer->addSetupTask([&connectionsMenu, addBackButton, addToParent]() {
        addBackButton(connectionsMenu);
        connectionsMenu->setup();
        addToParent(connectionsMenu);
        connectionsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Connections Menu - WiFi /////////
    auto wifiMenu = createANewSubMenu(_("WiFi"), "WiFi", connectionsMenu);
    this->renderer->addSetupTask([&wifiMenu, addBackButton, addToParent]() {
        addBackButton(wifiMenu);
        ///////// Connections Menu - WiFi - Enable/Disable WiFi /////////
        wifiMenu->addMenuEntry(
            _("Enable/Disable WiFi"),
            "WiFi_EnableDisable",
            MENUS::EntryType::MENUENTRY_TYPE_TOGGLE,
            [](void* data) {
                CubeLog::info("WiFi - Enable/Disable WiFi clicked");
                // TODO: make this actually enable/disable wifi. the settings should have a callback registered with the GlobalSettings class that will enable/disable wifi when the setting is changed.
                // GlobalSettings::setSetting(GlobalSettings::SettingType::WIFI_ENABLED, !GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::WIFI_ENABLED));
                return 0;
            },
            [](void*) {
                // return GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::WIFI_ENABLED);
                int random0or1 = rand() % 2;
                return random0or1;
            },
            nullptr);
        ///////// Connections Menu - WiFi - Current Network /////////
        wifiMenu->setup();
        addToParent(wifiMenu);
        wifiMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Connections Menu - WiFi - WiFi Networks /////////
    ///////// Connections Menu - WiFi - WiFi Networks - Specify SSID /////////
    ///////// Connections Menu - WiFi - WiFi Networks - Scan /////////
    ///////// Connections Menu - WiFi - WiFi Networks - List available networks /////////
    ///////// Connections Menu - WiFi - About WiFi /////////
    ///////// Connections Menu - WiFi - About WiFi - MAC Address /////////
    ///////// Connections Menu - WiFi - About WiFi - IP Address /////////
    ///////// Connections Menu - WiFi - About WiFi - Signal Strength /////////
    ///////// Connections Menu - WiFi - About WiFi - Network Name /////////
    ///////// Connections Menu - WiFi - About WiFi - Network Type /////////
    ///////// Connections Menu - WiFi - About WiFi - Security Type /////////
    ///////// Connections Menu - WiFi - About WiFi - Frequency /////////
    ///////// Connections Menu - WiFi - About WiFi - Channel /////////
    ///////// Connections Menu - WiFi - About WiFi - BSSID /////////
    ///////// Connections Menu - WiFi - About WiFi - Subnet Mask /////////
    ///////// Connections Menu - WiFi - About WiFi - Gateway /////////
    ///////// Connections Menu - WiFi - About WiFi - DNS Servers /////////
    ///////// Connections Menu - WiFi - About WiFi - DHCP Server /////////
    ///////// Connections Menu - WiFi - About WiFi - Lease Time /////////
    ///////// Connections Menu - WiFi - About WiFi - Connection Time /////////
    ///////// Connections Menu - WiFi - About WiFi - Data Rate /////////
    ///////// Connections Menu - WiFi - About WiFi - Link Quality /////////
    ///////// Connections Menu - WiFi - About WiFi - TX Power /////////
    ///////// Connections Menu - WiFi - About WiFi - RX Power /////////
    ///////// Connections Menu - WiFi - About WiFi - TX Bytes /////////
    ///////// Connections Menu - WiFi - About WiFi - RX Bytes /////////

    ///////// Connections Menu - Bluetooth /////////
    auto bluetoothMenu = createANewSubMenu(_("Bluetooth"), "Bluetooth", connectionsMenu);
    this->renderer->addSetupTask([&bluetoothMenu, addBackButton, addToParent]() {
        addBackButton(bluetoothMenu);
        ///////// Connections Menu - Bluetooth - Enable/Disable Bluetooth /////////
        // Options below should be greyed out and not clickable if bluetooth is disabled
        ///////// Connections Menu - Bluetooth - Pairing Mode /////////
        // TODO: when pairing mode is clicked, bluetooth should go into pairing mode and show a list of devices that can be paired with.
        ///////// Connections Menu - Bluetooth - Known devices /////////
        // TODO: list all the bluetooth devices that have been paired with the cube
        ///////// Connections Menu - Bluetooth - About Bluetooth /////////
        bluetoothMenu->setup();
        addToParent(bluetoothMenu);
        bluetoothMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Connections Menu - NFC /////////
    auto nfcMenu = createANewSubMenu(_("NFC"), "NFC", connectionsMenu);
    this->renderer->addSetupTask([&nfcMenu, addBackButton, addToParent]() {
        addBackButton(nfcMenu);
        ///////// Connections Menu - NFC - Enable/Disable NFC /////////
        ///////// Connections Menu - NFC - About NFC /////////
        nfcMenu->setup();
        addToParent(nfcMenu);
        nfcMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Personality Menu /////////
    auto personalityMenu = createANewSubMenu(_("Personality"), "Personality", mainMenu);
    this->renderer->addSetupTask([&personalityMenu, addBackButton, addToParent]() {
        addBackButton(personalityMenu);
        ///////// Personality Menu - Enable/Disable Personality /////////
        ///////// Personality Menu - Personality reset /////////
        personalityMenu->setup();
        addToParent(personalityMenu);
        personalityMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Personality Menu - Personality Settings /////////
    // TODO: list each attribute of the personality and provide a slider to adjust.

    ///////// Sensors Menu /////////
    auto sensorsMenu = createANewSubMenu(_("Sensors"), "Sensors", mainMenu);
    this->renderer->addSetupTask([&sensorsMenu, addBackButton, addToParent]() {
        addBackButton(sensorsMenu);
        ///////// Sensors Menu - Microphone enable/disable /////////
        ///////// Sensors Menu - Presence Detection enable/disable /////////
        sensorsMenu->setup();
        addToParent(sensorsMenu);
        sensorsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Sound Menu /////////
    auto soundMenu = createANewSubMenu(_("Sound"), "Sound", mainMenu);
    this->renderer->addSetupTask([&soundMenu, addBackButton, addToParent]() {
        addBackButton(soundMenu);
        ///////// Sound Menu - Volume /////////
        // TODO: add a slider to control the volume
        soundMenu->setup();
        addToParent(soundMenu);
        soundMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Sound Menu - Notification Sound /////////
    ///////// Sound Menu - Alarm Sound /////////
    ///////// Sound Menu - Voice Command Sound /////////

    ///////// Notifications Menu /////////
    auto notificationsMenu = createANewSubMenu(_("Notifications"), "Notifications", mainMenu);
    this->renderer->addSetupTask([&notificationsMenu, addBackButton, addToParent]() {
        addBackButton(notificationsMenu);
        notificationsMenu->setup();
        addToParent(notificationsMenu);
        notificationsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Notifications Menu - Allow Notifications from Network Sources (Other cubes) /////////
    ///////// Notifications Menu - Recent Notifications /////////

    ///////// Display Menu /////////
    auto displayMenu = createANewSubMenu(_("Display"), "Display", mainMenu);
    this->renderer->addSetupTask([&displayMenu, addBackButton, addToParent]() {
        addBackButton(displayMenu);
        displayMenu->setup();
        addToParent(displayMenu);
        displayMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Display Menu - Animations /////////
    ///////// Display Menu - Animations - Select Idle Animation /////////
    ///////// Display Menu - Animations - Enable remote animations /////////
    ///////// Display Menu - Brightness /////////
    ///////// Display Menu - Auto Off ///////// time?
    ///////// Display Menu - Font /////////

    ///////// Privacy Menu /////////
    auto privacyMenu = createANewSubMenu(_("Privacy"), "Privacy", mainMenu);
    this->renderer->addSetupTask([&privacyMenu, addBackButton, addToParent]() {
        addBackButton(privacyMenu);
        privacyMenu->setup();
        addToParent(privacyMenu);
        privacyMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Privacy Menu - Privacy Settings /////////
    // TODO: figure out what to put here: list apps that use microphone, presence detection, etc. and allow the user to disable them
    // also, telemetry stuff

    ///////// Accounts Menu /////////
    auto accountsMenu = createANewSubMenu(_("Accounts"), "Accounts", mainMenu);
    this->renderer->addSetupTask([&accountsMenu, addBackButton, addToParent]() {
        addBackButton(accountsMenu);
        accountsMenu->setup();
        addToParent(accountsMenu);
        accountsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Accounts Menu - Account List /////////
    // TODO: list all the accounts that have been added to the system. These are stored in the database.

    ///////// Accounts Menu - Account List - Account Settings /////////
    // TODO:

    ///////// Accounts Menu - Account List - Remove Account /////////
    // TODO:

    ///////// Accounts Menu - Add Account /////////
    // TODO: add a button to add an account

    ///////// Apps Menu /////////
    auto appsMenu = createANewSubMenu(_("Apps"), "Apps", mainMenu);
    this->renderer->addSetupTask([&appsMenu, addBackButton, addToParent]() {
        addBackButton(appsMenu);
        appsMenu->setup();
        addToParent(appsMenu);
        appsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Apps Menu - Core Apps /////////
    // list all the core apps here and allow the user to enable/disable them

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
    auto generalSettingsMenu = createANewSubMenu(_("General Settings"), "General Settings", mainMenu);
    this->renderer->addSetupTask([&generalSettingsMenu, addBackButton, addToParent]() {
        addBackButton(generalSettingsMenu);
        generalSettingsMenu->setup();
        addToParent(generalSettingsMenu);
        generalSettingsMenu->setChildrenClickables_isClickable(false);
    });

    ///////// General Settings Menu - Date and Time /////////
    auto generalSettingsDateTimeMenu = createANewSubMenu(_("Date and Time"), "Date and Time", generalSettingsMenu);
    this->renderer->addSetupTask([&generalSettingsDateTimeMenu, addBackButton, addToParent]() {
        addBackButton(generalSettingsDateTimeMenu);
        ///////// General Settings Menu - Date and Time - Set Date and Time /////////
        // generalSettingsDateTimeMenu->addMenuEntry(
        //     _("Set Date and Time"),
        //     "Date_Time_Set_Date_Time",
        //     MENUS::EntryType::MENUENTRY_TYPE_TEXT_INPUT,
        //     [generalSettingsDateTimeMenu](void* data) {
        //         CubeLog::info("Set Date and Time clicked");
        //         std::vector<std::string> fields = { "Year", "Month", "Day", "Hour", "Minute" };
        // GUI::showTextInput(_("Set Date and Time"), fields, [generalSettingsDateTimeMenu](std::vector<std::string> textVector) {
        //     CubeLog::info("Set Date and Time: " + text);
        //     // do something with the text in the textVector
        //     generalSettingsDateTimeMenu->setVisible(true);
        //     generalSettingsDateTimeMenu->setIsClickable(false);
        // });
        // generalSettingsDateTimeMenu->setVisible(false);
        // generalSettingsDateTimeMenu->setIsClickable(true);
        //     return 0;
        // },
        // [](void*) { return 0; },
        // nullptr);
        ///////// General Settings Menu - Date and Time - Set Time Zone ///////// TODO:
        ///////// General Settings Menu - Date and Time - Set Time Format ///////// TODO:
        ///////// General Settings Menu - Date and Time - Set Date Format ///////// TODO:
        ///////// General Settings Menu - Date and Time - Enable Automatic Date and Time ///////// TODO:
        generalSettingsDateTimeMenu->setup();
        addToParent(generalSettingsDateTimeMenu);
        generalSettingsDateTimeMenu->setChildrenClickables_isClickable(false);
    });

    ///////// General Settings Menu - Language /////////
    // TODO:
    // This menu should allow the user to change the language of the system. This will require a restart of the system.
    // TODO: All the strings in the system should be in a language file that can be changed at runtime.

    ///////// Accessibility Menu /////////
    auto accessibilityMenu = createANewSubMenu(_("Accessibility"), "Accessibility", mainMenu);
    this->renderer->addSetupTask([&accessibilityMenu, addBackButton, addToParent]() {
        addBackButton(accessibilityMenu);
        accessibilityMenu->setup();
        addToParent(accessibilityMenu);
        accessibilityMenu->setChildrenClickables_isClickable(false);
    });

    ///////// Accessibility Menu - ??? /////////
    // TODO: figure out what should go here

    ///////// Updates Menu /////////
    auto updatesMenu = createANewSubMenu(_("Updates"), "Updates", mainMenu);
    this->renderer->addSetupTask([&updatesMenu, addBackButton, addToParent]() {
        addBackButton(updatesMenu);
        ///////// Updates Menu - Check for Updates /////////
        // TODO:
        ///////// Updates Menu - Last Update Info /////////
        // TODO:
        // This menu should show the last time the system was updated and what was updated.
        // For example: "Last updated:\nJanuary 1, 1979 at 12:00pm\n \nUpdated:\nTheCube-CORE to version 1.0.0\nJSON library to version 1.0.0\netc."
        ///////// Updates Menu - Auto Update Enable /////////
        // TODO:
        updatesMenu->setup();
        addToParent(updatesMenu);
        updatesMenu->setChildrenClickables_isClickable(false);
    });

    ///////// About Menu /////////
    auto aboutMenu = createANewSubMenu(_("About"), "About", mainMenu);
    this->renderer->addSetupTask([&aboutMenu, addBackButton, addToParent]() {
        addBackButton(aboutMenu);
        ///////// About Menu - Serial Number /////////
        aboutMenu->addMenuEntry(
            _("Serial Number"),
            "About_Serial_Number",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
            [aboutMenu](void* data) {
                CubeLog::info("Serial Number clicked");
                // TODO: get the serial number from the hardware class
                GUI::showTextBox(_("Serial Number"), _("Serial Number: 1234567890h"), { 720, 720 }, { 0, 0 }, [aboutMenu]() {
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
        ///////// About Menu - Hardware Version /////////
        aboutMenu->addMenuEntry(
            _("Hardware Version"),
            "About_Hardware_Version",
            MENUS::EntryType::MENUENTRY_TYPE_TEXT_INFO,
            [aboutMenu](void* data) {
                CubeLog::info("Hardware Version clicked");
                GUI::showTextBox(_("Hardware Version"), _("Hardware Version: 1.0.0"), { 720, 720 }, { 0, 0 }, [aboutMenu]() {
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
        addToParent(aboutMenu);
        aboutMenu->setChildrenClickables_isClickable(false);
    });

    ///////// About Menu - Software Information /////////
    auto aboutSoftwareInformationMenu = createANewSubMenu(_("Software Information"), "Software Information", aboutMenu);
    this->renderer->addSetupTask([&aboutSoftwareInformationMenu, addBackButton, addToParent]() {
        addBackButton(aboutSoftwareInformationMenu);
        ///////// About Menu - Software Information - TheCube-CORE /////////
        // TODO: Show the version, build number, and build date of TheCube-CORE
        ///////// About Menu - Software Information - System /////////
        // TODO: Show the Raspbian version, kernel version, etc
        ///////// About Menu - Software Information - Libraries /////////
        // TODO: Show the versions of all the libraries used in the system
        aboutSoftwareInformationMenu->setup();
        addToParent(aboutSoftwareInformationMenu);
        aboutSoftwareInformationMenu->setChildrenClickables_isClickable(false);
    });

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
    // TODO: list all the open source licenses used in the system

    ///////// About Menu - Legal Information - Legal Notices /////////
    // TODO: list all the legal notices

    ///////// About Menu - Legal Information - Trademarks /////////
    // TODO: list all the trademarks used in the system

    ///////// About Menu - Legal Information - EULA /////////
    // TODO: Read in the EULA from a file and display it

    ///////// About Menu - Legal Information - Privacy Policy /////////
    // TODO: Read in the privacy policy from a file and display it

    ///////// Developer Settings Menu /////////
    auto developerSettingsMenu = createANewSubMenu(_("Developer Settings"), "Developer Settings", mainMenu);
    this->renderer->addSetupTask([&developerSettingsMenu, addBackButton, addToParent]() {
        addBackButton(developerSettingsMenu);
        ///////// Developer Settings Menu - Developer Mode Enable /////////
        developerSettingsMenu->addMenuEntry(
            _("Developer Mode Enable"),
            "Developer_Settings_Developer_Mode_Enable",
            MENUS::EntryType::MENUENTRY_TYPE_TOGGLE,
            [developerSettingsMenu](void* data) {
                CubeLog::info("Developer Mode Enable clicked");
                GlobalSettings::setSetting(
                    GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED,
                    !GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED));
                std::string message = _("Developer mode ");
                message += _((GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED)) ? "enabled." : "disabled.");
                message += _("\nPlease reboot the system for changes to take effect.");
                GUI::showMessageBox(_("Developer Mode"), message, { 720, 720 }, { 0, 0 }, [developerSettingsMenu]() {
                    developerSettingsMenu->setVisible(true);
                    developerSettingsMenu->setIsClickable(false);
                });
                developerSettingsMenu->setVisible(false);
                developerSettingsMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED);
            },
            nullptr);
        ///////// Developer Settings Menu - Verify SSD Integrity /////////
        developerSettingsMenu->addMenuEntry(
            _("Verify SSD Integrity"),
            "Developer_Settings_Verify_SSD_Integrity",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [developerSettingsMenu](void* data) {
                CubeLog::info("Verify SSD Integrity clicked");
                // TODO: call some method that checks the SSD integrity
                GUI::showMessageBox(_("Verifying SSD Integrity"), _("SSD Integrity verification in process.\nYou will be notified when it is complete."), { 720, 720 }, { 0, 0 }, [developerSettingsMenu]() {
                    developerSettingsMenu->setVisible(true);
                    developerSettingsMenu->setIsClickable(false);
                });
                developerSettingsMenu->setVisible(false);
                developerSettingsMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
        ///////// Developer Settings Menu - Factory Reset /////////
        developerSettingsMenu->addMenuEntry(
            _("Factory Reset"),
            "Developer_Settings_Factory_Reset",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [developerSettingsMenu](void* data) {
                CubeLog::info("Factory Reset clicked");
                // TODO: call some method that resets the system to factory settings
                GUI::showMessageBox(_("Factory Reset"), _("Factory Reset in process.\nSystem will reboot when it is complete."), { 720, 720 }, { 0, 0 }, [developerSettingsMenu]() {});
                developerSettingsMenu->setVisible(false);
                developerSettingsMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
        ///////// Developer Settings Menu - Reboot /////////
        developerSettingsMenu->addMenuEntry(
            _("Reboot"),
            "Developer_Settings_Reboot",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [developerSettingsMenu](void* data) {
                CubeLog::info("Reboot clicked");
                // TODO: trigger a reboot
                GUI::showMessageBox(_("Reboot"), _("Rebooting the system."), { 720, 720 }, { 0, 0 }, [developerSettingsMenu]() {});
                developerSettingsMenu->setVisible(false);
                developerSettingsMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
        ///////// Developer Settings Menu - Shutdown /////////
        developerSettingsMenu->addMenuEntry(
            _("Shutdown"),
            "Developer_Settings_Shutdown",
            MENUS::EntryType::MENUENTRY_TYPE_ACTION,
            [developerSettingsMenu](void* data) {
                CubeLog::info("Shutdown clicked");
                // TODO: trigger a shutdown
                GUI::showMessageBox(_("Shutdown"), _("Shutting down the system."), { 720, 720 }, { 0, 0 }, [developerSettingsMenu]() {});
                developerSettingsMenu->setVisible(false);
                developerSettingsMenu->setIsClickable(true);
                return 0;
            },
            [](void*) {
                return 0;
            },
            nullptr);
        if (GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED)) {
            ///////// Developer Settings Menu - CPU and Memory Display /////////
            // TODO:

            ///////// Developer Settings Menu - Send Bug Report /////////
            // TODO:
        }
        developerSettingsMenu->setup();
        addToParent(developerSettingsMenu);
        developerSettingsMenu->setChildrenClickables_isClickable(false);
    });

    if (GlobalSettings::getSettingOfType<bool>(GlobalSettings::SettingType::DEVELOPER_MODE_ENABLED)) {
        ///////// Developer Settings Menu - SSH /////////
        // TODO:

        ///////// Developer Settings Menu - SSH - Enable SSH /////////
        // TODO:

        ///////// Developer Settings Menu - SSH - Change Password /////////
        // TODO:

        ///////// Developer Settings Menu - Logging /////////
        // TODO:

        ///////// Developer Settings Menu - Logging - File Log Level /////////
        // TODO:

        ///////// Developer Settings Menu - Logging - Telemetry Log Level /////////
        // TODO:

        ///////// Developer Settings Menu - Logging - Log Verbosity /////////
        // TODO:
    }

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
    /// Set up the notifications text box
    ////////////////////////////////////////

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
        for (size_t i = 0; i < events.size(); i++) {
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
GUI_Error GUI::addMenu(const std::string& menuName, const std::string& thisUniqueID, const std::string& parentID, AddMenu_Data_t data)
{
    CubeLog::debugSilly("Adding menu: " + menuName + " with parent: " + parentID);
    std::vector<std::string> uniqueIDs;
    for (size_t i = 0; i < data.size(); i++) {
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
    drag_y_actions.push_back({ [&aNewMenu]() { return aNewMenu->getVisible(); }, [aNewMenu](int y) { aNewMenu->scrollVert(y); } });
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
        for (size_t i = 0; i < data.size(); i++) {
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
void GUI::showMessageBox(const std::string& title, const std::string& message)
{
    // check that messageBox is not null pointer
    if (messageBox == nullptr) {
        CubeLog::error("Message box is null. Cannot show message.");
        return;
    }
    messageBox->setText(message, title);
    messageBox->setVisible(true);
}

void GUI::showMessageBox(const std::string& title, const std::string& message, glm::vec2 size, glm::vec2 position)
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

void GUI::showMessageBox(const std::string& title, const std::string& message, glm::vec2 size, glm::vec2 position, std::function<void()> callback)
{
    // check that messageBox is not null pointer
    if (messageBox == nullptr) {
        CubeLog::error("Message box is null. Cannot show message.");
        return;
    }
    messageBox->setCallback(callback);
    showMessageBox(title, message, size, position);
}

void GUI::showTextBox(const std::string& title, const std::string& message)
{
    // check that fullScreenTextBox is not null pointer
    if (fullScreenTextBox == nullptr) {
        CubeLog::error("Full screen text box is null. Cannot show text.");
        return;
    }
    fullScreenTextBox->setText(message, title);
    fullScreenTextBox->setVisible(true);
}

void GUI::showTextBox(const std::string& title, const std::string& message, glm::vec2 size, glm::vec2 position)
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

void GUI::showTextBox(const std::string& title, const std::string& message, glm::vec2 size, glm::vec2 position, std::function<void()> callback)
{
    // check that fullScreenTextBox is not null pointer
    if (fullScreenTextBox == nullptr) {
        CubeLog::error("Full screen text box is null. Cannot show text.");
        return;
    }
    fullScreenTextBox->setCallback(callback);
    showTextBox(title, message, size, position);
}

void GUI::showNotification(const std::string& title, const std::string& message, NotificationsManager::NotificationType type)
{
    // TODO: show a notification
}

void GUI::showNotificationWithCallback(const std::string& title, const std::string& message, NotificationsManager::NotificationType type, std::function<void()> callback)
{
    // TODO: show a notification with a callback
}

void GUI::showNotificationWithCallback(const std::string& title, const std::string& message, NotificationsManager::NotificationType type, std::function<void()> callbackYes, std::function<void()> cancelNo)
{
    // TODO: show a notification with a callback for yes and no
}

void GUI::showTextInputBox(const std::string& title, std::vector<std::string> fields, std::function<void(std::vector<std::string>&)> callback)
{
    // TODO: show a text input box
    std::vector<std::string> textVector;
    for (size_t i = 0; i < fields.size(); i++) {
        textVector.push_back(fields[i]);
    }
    callback(textVector);
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
                // TODO: anything that gets displayed needs to be logged in the DB->notifications
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
            _("Show a message box with a title and message") });
    actions.push_back(
        { PUBLIC_ENDPOINT | GET_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                // TODO: anything that gets displayed needs to be logged in the DB->notifications
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
            _("Show a text box with a title and message of a specified size and position. Default values for size and position are 720x720 at 0,0.") });
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
            _("Add a menu to the GUI") });
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
std::string GUI::getInterfaceName() const
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

/////////////////////////////////////////////////////////////////////////////////

NotificationsManager::NotificationsManager()
{
    CubeLog::info("Notifications initialized");
}

NotificationsManager::~NotificationsManager()
{
    CubeLog::info("Notifications destroyed");
}

void NotificationsManager::showNotification(const std::string& title, const std::string& message, NotificationType type)
{
    CubeLog::info("Notification shown: " + title + " - " + message);
}

void NotificationsManager::showNotificationWithCallback(const std::string& title, const std::string& message, NotificationType type, std::function<void()> callback)
{
    CubeLog::info("Notification shown with callback: " + title + " - " + message);
}

void NotificationsManager::showNotificationWithCallback(const std::string& title, const std::string& message, NotificationType type, std::function<void()> callbackYes, std::function<void()> callbackNo)
{
    CubeLog::info("Notification shown with callback: " + title + " - " + message);
}

std::string NotificationsManager::getInterfaceName() const
{
    return "Notifications";
}

HttpEndPointData_t NotificationsManager::getHttpEndpointData()
{
    HttpEndPointData_t actions;
    actions.push_back(
        { PRIVATE_ENDPOINT | POST_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Notification shown with callback");
            },
            "showNotificationOkayWarningError",
            {},
            _("Show a notification with an optional callback") });
    actions.push_back(
        { PRIVATE_ENDPOINT | POST_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Notification shown with callback");
            },
            "showNotificationYesNo",
            {},
            _("Show a yes/no notification with two callbacks") });
    return actions;
}
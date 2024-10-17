// #pragma once
#ifndef GUI_H
#define GUI_H
#define WIN32_LEAN_AND_MEAN
#ifndef EVENTHANDLER_H
#include "eventHandler/eventHandler.h"
#endif
#ifndef MENU_H
#include "menu/menu.h"
#endif
#ifndef RENDERER_H
#include "renderer.h"
#endif
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#ifndef MESSAGEBOX_H
#include "messageBox/messageBox.h"
#endif
#ifndef API_H
#include "../api/api.h"
#endif
#ifndef API_I_H
#include "../api_i.h"
#endif
#ifndef UTILS_H
#include <utils.h>
#endif
#include "httplib.h"
#ifndef GLOBAL_SETTINGS_H
#include "globalSettings.h"
#endif

#ifdef __linux__
#include <libintl.h>
#include <locale.h>

#define _(String) gettext(String)
#define N_(String) String
// TODO: verify this works
#else
#define _(String) (String)
#endif

typedef std::vector<std::tuple<std::string, nlohmann::json, std::string>> AddMenu_Data_t;

bool parseJsonAndAddEntriesToMenu(nlohmann::json j, MENUS::Menu* menuEntry);
bool breakJsonApart(nlohmann::json j, AddMenu_Data_t& data, std::string* menuName, std::string* thisUniqueID, std::string* parentID);

struct GUI_Error {
    enum ERROR_TYPES {
        GUI_NO_ERROR,
        GUI_PARENT_NOT_FOUND,
        GUI_MENU_NOT_FOUND,
        GUI_UNIQUE_EXISTS,
        GUI_CHILD_UNIQUE_EXISTS,
        GUI_JSON_ERROR,
        GUI_INTERNAL_ERROR
    };
    GUI_Error(ERROR_TYPES errorType, std::string errorString)
        : errorType(errorType)
        , errorString(errorString)
    {
    }
    ERROR_TYPES errorType;
    std::string errorString;
};

class NotificationsManager : public I_API_Interface {
public:
    enum NotificationType {
        NOTIFICATION_OKAY,
        NOTIFICATION_WARNING,
        NOTIFICATION_ERROR,
        NOTIFICATION_YES_NO,
        NOTIFICATION_TYPE_COUNT
    };
    NotificationsManager();
    ~NotificationsManager();
    static void showNotification(std::string title, std::string message, NotificationType type);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callback);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callbackYes, std::function<void()> callbackNo);
    std::string getIntefaceName() const override;
    HttpEndPointData_t getHttpEndpointData() override;
};

class GUI : public I_API_Interface {
public:
    GUI();
    ~GUI();
    void eventLoop();
    void stop();
    static void showMessageBox(std::string title, std::string message);
    static void showMessageBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position);
    static void showMessageBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position, std::function<void()> callback);
    static void showTextBox(std::string title, std::string message);
    static void showTextBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position);
    static void showTextBox(std::string title, std::string message, glm::vec2 size, glm::vec2 position, std::function<void()> callback);
    static void showNotification(std::string title, std::string message, NotificationsManager::NotificationType type);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationsManager::NotificationType type, std::function<void()> callback);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationsManager::NotificationType type, std::function<void()> callbackYes, std::function<void()> callbackNo);
    static void showTextInputBox(std::string title, std::vector<std::string> fields, std::function<void(std::vector<std::string>&)> callback);
    static void showTextInputBox(std::string title, std::string field, std::function<void(std::string&)> callback);
    // API Interface
    HttpEndPointData_t getHttpEndpointData();
    std::string getIntefaceName() const;

private:
    // GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, std::vector<std::string> uniqueIDs, CountingLatch &latch);
    GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, AddMenu_Data_t data);
    Renderer* renderer;
    std::jthread eventLoopThread;
    EventManager* eventManager;
    static CubeMessageBox* messageBox;
    static CubeTextBox* fullScreenTextBox;
    // static NotificationBox* notificationBox;
    std::vector<MENUS::Menu*> menus;
    std::vector<std::pair<std::function<bool()>, std::function<void(int)>>> drag_y_actions; // bool is visibility. if the item is not visible, do not call the action.
    std::mutex addMenuMutex;
};

#endif
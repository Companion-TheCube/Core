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
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <latch>
#include <expected>
#include <regex>
#include <nlohmann/json.hpp>
#include <mutex>
#include <tuple>
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

typedef std::vector<std::tuple<std::string, nlohmann::json, std::string>> AddMenu_Data_t;

bool parseJsonAndAddEntriesToMenu(nlohmann::json j, MENUS::Menu* menuEntry);
bool breakJsonApart(nlohmann::json j, AddMenu_Data_t& data, std::string* menuName, std::string* thisUniqueID, std::string* parentID);

struct GUI_Error{
    enum ERROR_TYPES{
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
    // API Interface
    HttpEndPointData_t getHttpEndpointData();
    // std::vector<std::pair<std::string, std::vector<std::string>>> getHttpEndpointNamesAndParams();
    std::string getIntefaceName() const;

private:
    // GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, std::vector<std::string> uniqueIDs, CountingLatch &latch);
    GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, std::vector<std::tuple<std::string,nlohmann::json,std::string>> data);
    Renderer* renderer;
    std::jthread eventLoopThread;
    EventManager* eventManager;
    static CubeMessageBox* messageBox;
    static CubeTextBox* fullScreenTextBox;
    std::vector<MENUS::Menu*> menus;
    std::vector<std::pair<std::function<bool()>,std::function<void(int)>>> drag_y_actions; // bool is visibility. if the item is not visible, do not call the action.
    std::mutex addMenuMutex;
};

#endif
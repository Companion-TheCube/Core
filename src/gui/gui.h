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
#include <nlohmann/json.hpp>
#include <mutex>
#include <tuple>
#ifndef MESSAGEBOX_H
#include "messageBox/messageBox.h"
#endif
#ifndef API_H
#include "../api/api.h"
#endif
#ifndef UTILS_H
#include <utils.h>
#endif
#include "httplib.h"

typedef std::vector<std::tuple<std::string, nlohmann::json, std::string>> AddMenu_Data_t;

struct GUI_Error{
    enum ERROR_TYPES{
        GUI_NO_ERROR,
        GUI_PARENT_NOT_FOUND,
        GUI_MENU_NOT_FOUND,
        GUI_UNIQUE_EXISTS,
        GUI_CHILD_UNIQUE_EXISTS,
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
    // API Interface
    HttpEndPointData_t getHttpEndpointData();
    std::vector<std::pair<std::string, std::vector<std::string>>> getHttpEndpointNamesAndParams();
    std::string getIntefaceName() const;

private:
    // GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, std::vector<std::string> uniqueIDs, CountingLatch &latch);
    GUI_Error addMenu(std::string menuName, std::string thisUniqueID, std::string parentID, std::vector<std::tuple<std::string,nlohmann::json,std::string>> data);
    Renderer* renderer;
    std::jthread eventLoopThread;
    EventManager* eventManager;
    static CubeMessageBox* messageBox;
    std::vector<MENUS::Menu*> menus;
    std::vector<std::pair<std::function<bool()>,std::function<void(int)>>> drag_y_actions; // bool is visibility. if the item is not visible, do not call the action.
    std::mutex addMenuMutex;
};

bool parseJsonAndAddEntriesToMenu(nlohmann::json j, MENUS::Menu* menuEntry);
bool breakJsonApart(nlohmann::json j, std::vector<std::tuple<std::string,nlohmann::json,std::string>> &data);

#endif
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
#ifndef MESSAGEBOX_H
#include "messageBox/messageBox.h"
#endif
#ifndef API_H
#include "../api/api.h"
#endif
#ifndef UTILS_H
#include <utils.h>
#endif

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
    void addMenu(std::string menuName, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints, CountingLatch &latch);
    void addMenu(std::string menuName, std::string parentName, std::vector<std::string> entryTexts, std::vector<std::string> endpoints);
    Renderer* renderer;
    std::jthread eventLoopThread;
    EventManager* eventManager;
    static CubeMessageBox* messageBox;
    std::vector<Menu*> menus;
    std::vector<std::pair<std::function<bool()>,std::function<void(int)>>> drag_y_actions; // bool is visibility. if the item is not visible, do not call the action.
};

#endif
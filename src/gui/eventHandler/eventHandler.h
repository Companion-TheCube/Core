/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██╗  ██╗ █████╗ ███╗   ██╗██████╗ ██╗     ███████╗██████╗    ██╗  ██╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██║  ██║██╔══██╗████╗  ██║██╔══██╗██║     ██╔════╝██╔══██╗   ██║  ██║
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████║███████║██╔██╗ ██║██║  ██║██║     █████╗  ██████╔╝   ███████║
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██║██╔══██║██║╚██╗██║██║  ██║██║     ██╔══╝  ██╔══██╗   ██╔══██║
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██║  ██║██║  ██║██║ ╚████║██████╔╝███████╗███████╗██║  ██║██╗██║  ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2026 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "../objects.h"
#include "cubeEvent.h"
#include <functional>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <string>
#include <tuple>
#include <vector>

class EventHandler {
private:
    std::function<void(const CubeEvent&)> action;
    std::string name;
    CubeEventType eventType;
    SpecificEventTypes specificEventType;

public:
    EventHandler();
    ~EventHandler();
    bool triggerEvent(const CubeEvent& event);
    void setAction(std::function<void(const CubeEvent&)> action);
    std::string getName();
    void setName(const std::string& name);
    CubeEventType getEventType();
    void setEventType(CubeEventType eventType);
    void setSpecificEventType(SpecificEventTypes specificEventType);
    SpecificEventTypes getSpecificEventType();
};

class EventManager {
private:
    std::vector<EventHandler*> events;
    std::vector<ClickableArea*> clickableAreas;
    bool checkClickableAreas(const CubeEvent& event);
    std::tuple<int, int> mouseDownPosition;
    std::tuple<int, int> lastPointerPosition;
    bool primaryPointerDown = false;
    static constexpr int CLICK_THRESHOLD_PX = 5;

public:
    EventManager();
    ~EventManager();
    int createEvent(const std::string& eventName);
    bool removeEvent(EventHandler* event);
    bool removeEvent(int index);
    bool removeEvent(const std::string& eventName);
    EventHandler* getEvent(int index);
    EventHandler* getEvent(const std::string& eventName);
    bool triggerEvent(const CubeEvent& event);
    bool triggerEvent(int index, const CubeEvent& event);
    bool triggerEvent(const std::string& eventName, const CubeEvent& event);
    bool triggerEvent(CubeEventType eventType, const CubeEvent& event);
    bool triggerEvent(SpecificEventTypes specificEventType, const CubeEvent& event);
    bool triggerEvent(SpecificEventTypes specificEventType, CubeEventType eventType, const CubeEvent& event);
    std::vector<EventHandler*> getEvents();
    void addClickableArea(ClickableArea* clickableArea);
};

/*
███████╗██╗   ██╗███████╗███╗   ██╗████████╗██╗  ██╗ █████╗ ███╗   ██╗██████╗ ██╗     ███████╗██████╗     ██████╗██████╗ ██████╗
██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██║  ██║██╔══██╗████╗  ██║██╔══██╗██║     ██╔════╝██╔══██╗   ██╔════╝██╔══██╗██╔══██╗
█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████║███████║██╔██╗ ██║██║  ██║██║     █████╗  ██████╔╝   ██║     ██████╔╝██████╔╝
██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ██╔══██║██╔══██║██║╚██╗██║██║  ██║██║     ██╔══╝  ██╔══██╗   ██║     ██╔═══╝ ██╔═══╝
███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ██║  ██║██║  ██║██║ ╚████║██████╔╝███████╗███████╗██║  ██║██╗╚██████╗██║     ██║
╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
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

#include "eventHandler.h"

#include <cmath>

EventHandler::EventHandler()
{
    CubeLog::info("Event handler created with default values.");
    this->name = "";
    this->eventType = CubeEventType::None;
    this->specificEventType = SpecificEventTypes::NULL_EVENT;
    this->action = nullptr;
}

EventHandler::~EventHandler()
{
    CubeLog::info("Event handler destroyed");
}

bool EventHandler::triggerEvent(const CubeEvent& event)
{
    if (this->action == nullptr) {
        CubeLog::error("No action set for event: " + this->name);
        return false;
    }
    this->action(event);
    return true;
}

void EventHandler::setAction(std::function<void(const CubeEvent&)> action)
{
    CubeLog::info("Action set for event: " + this->name);
    this->action = action;
}

std::string EventHandler::getName()
{
    return this->name;
}

void EventHandler::setName(const std::string& name)
{
    this->name = name;
}

CubeEventType EventHandler::getEventType()
{
    return this->eventType;
}

void EventHandler::setEventType(CubeEventType eventType)
{
    this->eventType = eventType;
}

void EventHandler::setSpecificEventType(SpecificEventTypes specificEventType)
{
    this->specificEventType = specificEventType;
}

SpecificEventTypes EventHandler::getSpecificEventType()
{
    return this->specificEventType;
}

EventManager::EventManager()
    : mouseDownPosition { 0, 0 }
    , lastPointerPosition { 0, 0 }
{
    CubeLog::info("Event manager created with default values.");
}

EventManager::~EventManager()
{
    for (size_t i = 0; i < this->events.size(); i++) {
        delete this->events[i];
    }
    CubeLog::info("Event manager destroyed");
}

int EventManager::createEvent(const std::string& eventName)
{
    EventHandler* event = new EventHandler();
    event->setName(eventName);
    this->events.push_back(event);
    return static_cast<int>(this->events.size() - 1);
}

bool EventManager::removeEvent(EventHandler* event)
{
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i] == event) {
            delete this->events[i];
            this->events.erase(this->events.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

bool EventManager::removeEvent(int index)
{
    if (index < 0 || index >= static_cast<int>(this->events.size())) {
        CubeLog::error("Index out of range when removing event");
        return false;
    }
    delete this->events[index];
    this->events.erase(this->events.begin() + index);
    return true;
}

bool EventManager::removeEvent(const std::string& eventName)
{
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            delete this->events[i];
            this->events.erase(this->events.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

EventHandler* EventManager::getEvent(int index)
{
    return this->events[index];
}

std::vector<EventHandler*> EventManager::getEvents()
{
    return this->events;
}

EventHandler* EventManager::getEvent(const std::string& eventName)
{
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            return this->events[i];
        }
    }
    return nullptr;
}

bool EventManager::triggerEvent(const CubeEvent& event)
{
    bool anyTriggered = false;
    if (checkClickableAreas(event)) {
        return true;
    }

    anyTriggered = triggerEvent(event.type, event) || anyTriggered;

    const SpecificEventTypes keySpecificType = specificEventTypeFromCubeKey(event.key);
    if (keySpecificType != SpecificEventTypes::NULL_EVENT) {
        anyTriggered = triggerEvent(keySpecificType, event.type, event) || anyTriggered;
    }

    const SpecificEventTypes mouseSpecificType = specificEventTypeFromCubeMouseButton(event.mouseButton, event.type);
    if (mouseSpecificType != SpecificEventTypes::NULL_EVENT) {
        anyTriggered = triggerEvent(mouseSpecificType, event.type, event) || anyTriggered;
    }

    return anyTriggered;
}

bool EventManager::triggerEvent(int index, const CubeEvent& event)
{
    if (index < 0 || index >= static_cast<int>(this->events.size())) {
        CubeLog::error("Index out of range");
        return false;
    }
    return this->events[index]->triggerEvent(event);
}

bool EventManager::triggerEvent(const std::string& eventName, const CubeEvent& event)
{
    bool anyTriggered = false;
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            anyTriggered = this->events[i]->triggerEvent(event) || anyTriggered;
        }
    }
    return anyTriggered;
}

bool EventManager::triggerEvent(CubeEventType eventType, const CubeEvent& event)
{
    bool anyTriggered = false;
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getEventType() == eventType
            && this->events[i]->getSpecificEventType() == SpecificEventTypes::NULL_EVENT) {
            anyTriggered = this->events[i]->triggerEvent(event) || anyTriggered;
        }
    }
    return anyTriggered;
}

bool EventManager::triggerEvent(SpecificEventTypes specificEventType, const CubeEvent& event)
{
    if (specificEventType == SpecificEventTypes::NULL_EVENT) {
        return false;
    }

    bool anyTriggered = false;
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getSpecificEventType() == specificEventType
            && this->events[i]->getEventType() == CubeEventType::None) {
            anyTriggered = this->events[i]->triggerEvent(event) || anyTriggered;
        }
    }
    return anyTriggered;
}

bool EventManager::triggerEvent(SpecificEventTypes specificEventType, CubeEventType eventType, const CubeEvent& event)
{
    if (specificEventType == SpecificEventTypes::NULL_EVENT) {
        return false;
    }

    bool anyTriggered = false;
    for (size_t i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getEventType() == eventType
            && this->events[i]->getSpecificEventType() == specificEventType) {
            anyTriggered = this->events[i]->triggerEvent(event) || anyTriggered;
        }
    }
    return anyTriggered;
}

void EventManager::addClickableArea(ClickableArea* clickableArea)
{
    this->clickableAreas.push_back(clickableArea);
}

bool EventManager::checkClickableAreas(const CubeEvent& event)
{
    const bool isPointerEvent = event.type == CubeEventType::MouseButtonPressed
        || event.type == CubeEventType::MouseButtonReleased
        || event.type == CubeEventType::MouseMoved
        || event.type == CubeEventType::MouseWheelScrolled;
    const int lastPointerX = std::get<0>(this->lastPointerPosition);
    const int lastPointerY = std::get<1>(this->lastPointerPosition);

    if (event.type == CubeEventType::MouseButtonPressed) {
        this->mouseDownPosition = { event.x, event.y };
        this->lastPointerPosition = { event.x, event.y };
        if (event.mouseButton == CubeMouseButton::Left) {
            this->primaryPointerDown = true;
        }
    }

    const int mouseDownX = std::get<0>(this->mouseDownPosition);
    const int mouseDownY = std::get<1>(this->mouseDownPosition);
    const int xChange = event.x - mouseDownX;
    const int yChange = event.y - mouseDownY;
    const float distance = std::sqrt(static_cast<float>((xChange * xChange) + (yChange * yChange)));

    if ((event.type == CubeEventType::MouseButtonPressed || event.type == CubeEventType::MouseButtonReleased) && distance < CLICK_THRESHOLD_PX) {
        for (ClickableArea* area : this->clickableAreas) {
            if (area == nullptr || area->clickableObject == nullptr || !area->clickableObject->getIsClickable()) {
                continue;
            }

            if (event.x < area->xMax && event.x > area->xMin && event.y < area->yMax && event.y > area->yMin) {
                if (event.type == CubeEventType::MouseButtonReleased) {
                    area->clickableObject->onRelease(event);
                }
                if (event.mouseButton == CubeMouseButton::Left && event.type == CubeEventType::MouseButtonReleased) {
                    area->clickableObject->onClick(event);
                    this->primaryPointerDown = false;
                    return true;
                }
                if (event.mouseButton == CubeMouseButton::Right && event.type == CubeEventType::MouseButtonReleased) {
                    area->clickableObject->onRightClick(event);
                    return true;
                }
                if (event.type == CubeEventType::MouseButtonPressed) {
                    area->clickableObject->onMouseDown(event);
                    return true;
                }
            }
        }
    } else if (event.type == CubeEventType::MouseMoved && this->primaryPointerDown) {
        if (std::abs(yChange) > CLICK_THRESHOLD_PX || std::abs(xChange) > CLICK_THRESHOLD_PX) {
            CubeEvent dragEvent = event;
            dragEvent.deltaX = event.x - lastPointerX;
            dragEvent.deltaY = event.y - lastPointerY;
            if (std::abs(static_cast<float>(yChange)) * 0.5f > std::abs(static_cast<float>(xChange))) {
                triggerEvent(SpecificEventTypes::DRAG_Y, CubeEventType::MouseMoved, dragEvent);
            } else if (std::abs(static_cast<float>(xChange)) * 0.5f > std::abs(static_cast<float>(yChange))) {
                triggerEvent(SpecificEventTypes::DRAG_X, CubeEventType::MouseMoved, dragEvent);
            }
        }
    } else if (event.type == CubeEventType::MouseButtonReleased) {
        for (ClickableArea* area : this->clickableAreas) {
            if (area == nullptr || area->clickableObject == nullptr || !area->clickableObject->getIsClickable()) {
                continue;
            }
            area->clickableObject->onRelease(event);
        }
    }

    if (event.type == CubeEventType::MouseButtonReleased && event.mouseButton == CubeMouseButton::Left) {
        this->primaryPointerDown = false;
    }

    if (isPointerEvent) {
        this->lastPointerPosition = { event.x, event.y };
    }
    return false;
}

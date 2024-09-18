#include "eventHandler.h"

// TODO: add click-drag functionality

/**
 * @brief Construct a new Event Handler:: Event Handler object
 *
 * @param logger CubeLog object
 */
EventHandler::EventHandler()
{
    CubeLog::info("Event handler created with default values.");
    this->name = "";
    this->eventType = sf::Event::EventType::Count;
    this->specificEventType = SpecificEventTypes::NULL_EVENT;
    this->action = nullptr;
}

/**
 * @brief Destroy the Event Handler:: Event Handler object
 *
 */
EventHandler::~EventHandler()
{
    CubeLog::info("Event handler destroyed");
    // if(this->clickableArea != nullptr){
    //     delete this->clickableArea;
    // }
}

/**
 * @brief Trigger the event
 *
 */
bool EventHandler::triggerEvent(void* data)
{
    if (this->action == nullptr) {
        CubeLog::error("No action set for event: " + this->name);
        return false;
    }
    // CubeLog::debug("Event triggered: " + this->name);
    this->action(data);
    return true;
}

/**
 * @brief Set the action to be performed when the event is triggered
 *
 * @param action std::function<void(void*)> action
 */
void EventHandler::setAction(std::function<void(void*)> action)
{
    CubeLog::info("Action set for event: " + this->name);
    this->action = action;
}

/**
 * @brief Get the name of the event
 *
 * @return std::string
 */
std::string EventHandler::getName()
{
    return this->name;
}

/**
 * @brief Set the name of the event
 *
 * @param name std::string
 */
void EventHandler::setName(std::string name)
{
    this->name = name;
}

/**
 * @brief Get the event type
 *
 * @return sf::Event::EventType
 */
sf::Event::EventType EventHandler::getEventType()
{
    return this->eventType;
}

/**
 * @brief Set the event type
 *
 * @param eventType sf::Event::EventType
 */
void EventHandler::setEventType(sf::Event::EventType eventType)
{
    this->eventType = eventType;
}

/**
 * @brief Set the specific event type
 *
 * @param specificEventType SpecificEventTypes
 */
void EventHandler::setSpecificEventType(SpecificEventTypes specificEventType)
{
    this->specificEventType = specificEventType;
}

/**
 * @brief Get the specific event type
 *
 * @return SpecificEventTypes
 */
SpecificEventTypes EventHandler::getSpecificEventType()
{
    return this->specificEventType;
}

/////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new Event Manager:: Event Manager object
 *
 * @param logger CubeLog object
 */
EventManager::EventManager()
{
    CubeLog::info("Event manager created with default values.");
}

/**
 * @brief Destroy the Event Manager:: Event Manager object
 *
 */
EventManager::~EventManager()
{
    for (int i = 0; i < this->events.size(); i++) {
        delete this->events[i];
    }
    CubeLog::info("Event manager destroyed");
}

/**
 * @brief Create a new event
 *
 * @param eventName std::string
 * @return int index of the event
 */
int EventManager::createEvent(std::string eventName)
{
    EventHandler* event = new EventHandler();
    event->setName(eventName);
    this->events.push_back(event);
    return this->events.size() - 1;
}

/**
 * @brief Remove an event
 *
 * @param event EventHandler*
 */
bool EventManager::removeEvent(EventHandler* event)
{
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i] == event) {
            delete this->events[i];
            this->events.erase(this->events.begin() + i);
            return true;
        }
    }
    return false;
}

/**
 * @brief Remove an event
 *
 * @param index int, index of the event to remove, returned from createEvent
 */
bool EventManager::removeEvent(int index)
{
    delete this->events[index];
    this->events.erase(this->events.begin() + index);
    return true;
}

bool EventManager::removeEvent(std::string eventName)
{
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            delete this->events[i];
            this->events.erase(this->events.begin() + i);
            return true;
        }
    }
    return false;
}

/**
 * @brief get the event by index as returned from createEvent
 *
 * @param index
 * @return EventHandler*
 */
EventHandler* EventManager::getEvent(int index)
{
    return this->events[index];
}

/**
 * @brief Get all the events
 *
 * @return std::vector<EventHandler*>
 */
std::vector<EventHandler*> EventManager::getEvents()
{
    return this->events;
}

/**
 * @brief Get the event by name
 *
 * @param eventName
 * @return EventHandler*
 */
EventHandler* EventManager::getEvent(std::string eventName)
{
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            return this->events[i];
        }
    }
    return nullptr;
}

/**
 * @brief Trigger an event by sf::Event
 *
 * @param event
 * @return true if event was triggered
 * @return false if event was not triggered
 */
bool EventManager::triggerEvent(sf::Event event, void* data)
{
    if (checkClickableAreas(*((sf::Event*)data))) {
        return true;
    }
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getEventType() == event.type) {
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

/**
 * @brief Trigger an event by index
 *
 * @param index
 * @return true if event was triggered
 * @return false if event was not triggered
 */
bool EventManager::triggerEvent(int index, void* data)
{
    if (checkClickableAreas(*((sf::Event*)data))) {
        return true;
    }
    if(index >= this->events.size()){
        CubeLog::error("Index out of range");
        return false;
    }
    this->events[index]->triggerEvent(data);
    return true;
}

/**
 * @brief Trigger an event by name
 *
 * @param eventName
 * @return true if event was triggered
 * @return false if event was not triggered
 */
bool EventManager::triggerEvent(std::string eventName, void* data)
{
    if (checkClickableAreas(*((sf::Event*)data))) {
        return true;
    }
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getName() == eventName) {
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

/**
 * @brief Trigger an event by sf::Event::EventType
 *
 * @param eventType
 * @return true if event was triggered
 * @return false if event was not triggered
 */
bool EventManager::triggerEvent(sf::Event::EventType eventType, void* data)
{
    if (checkClickableAreas(*((sf::Event*)data))) {
        return true;
    }
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getEventType() == eventType && this->events[i]->getSpecificEventType() == SpecificEventTypes::NULL_EVENT) {
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

/**
 * @brief Trigger an event by SpecificEventTypes
 *
 * @param specificEventType
 * @return true if event was triggered
 * @return false if event was not triggered
 */
bool EventManager::triggerEvent(SpecificEventTypes specificEventType, void* data)
{
    // if(checkClickableAreas(*((sf::Event*)data))){
    //     return true;
    // }
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getSpecificEventType() == specificEventType && this->events[i]->getEventType() == sf::Event::Count) {
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

bool EventManager::triggerEvent(SpecificEventTypes specificEventType, sf::Event::EventType eventType, void* data)
{
    // if(checkClickableAreas(*((sf::Event*)data))){
    //     return true;
    // }
    for (int i = 0; i < this->events.size(); i++) {
        if (this->events[i]->getEventType() == eventType && this->events[i]->getSpecificEventType() == specificEventType) {
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

/**
 * @brief Add a clickable area to the event manager
 *
 * @param clickableArea
 */
void EventManager::addClickableArea(ClickableArea* clickableArea)
{
    this->clickableAreas.push_back(clickableArea);
}

/**
 * @brief Check the clickable areas for events
 *
 * @param event
 * @param data
 */
bool EventManager::checkClickableAreas(sf::Event event)
{
    if (sf::Event::MouseButtonPressed == event.type) {
        this->mouseDownPosition = { event.mouseButton.x, event.mouseButton.y };
    }
    if (event.type != sf::Event::MouseButtonReleased) {
        return false;
    }
    int xChange = event.mouseButton.x - std::get<0>(this->mouseDownPosition);
    int yChange = event.mouseButton.y - std::get<1>(this->mouseDownPosition);
    float distance = sqrt(pow(xChange, 2) + pow(yChange, 2));
    if (distance < 5) {
        CubeLog::info("Distance is less than 5");
        for (ClickableArea* area : this->clickableAreas) {
            // get the x and y from the event
            int x = event.mouseButton.x;
            int y = event.mouseButton.y;
            // check if the x and y are within the area
            CubeLog::debugSilly("Checking clickable area: x:" + std::to_string(x) + " y:" + std::to_string(y) + " area: xMin:" + std::to_string(area->xMin) + " xMax:" + std::to_string(area->xMax) + " yMin:" + std::to_string(area->yMin) + " yMax:" + std::to_string(area->yMax));
            if (x < area->xMax && x > area->xMin && y < area->yMax && y > area->yMin) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    area->clickableObject->onClick(&event);
                    return true;
                }
                if (event.mouseButton.button == sf::Mouse::Right) {
                    area->clickableObject->onRightClick(&event);
                    return true;
                }
            }
        }
    }else{
        if(std::abs(float(yChange)) * (0.5f) > std::abs(float(xChange))){
            CubeLog::info("Dragged Y");
            sf::Event ev;
            ev.type = sf::Event::MouseWheelScrolled;
            ev.mouseWheelScroll.delta = -yChange;
            triggerEvent(SpecificEventTypes::DRAG_Y, &ev);
        }else if(std::abs(float(xChange) * (0.5f)) > std::abs(float(yChange))){
            CubeLog::info("Dragged X");
            sf::Event ev;
            ev.type = sf::Event::MouseWheelScrolled;
            ev.mouseWheelScroll.delta = xChange;
            triggerEvent(SpecificEventTypes::DRAG_X, &ev);
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
// Path src/gui/eventHandler/eventHandler.cpp
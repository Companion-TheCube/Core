#include "eventHandler.h"

/**
 * @brief Construct a new Event Handler:: Event Handler object
 * 
 * @param logger CubeLog object
 */
EventHandler::EventHandler(CubeLog *logger){
    this->logger = logger;
}

/**
 * @brief Destroy the Event Handler:: Event Handler object
 * 
 */
EventHandler::~EventHandler(){
    this->logger->log("Event handler destroyed", true);
}

/**
 * @brief Trigger the event
 * 
 */
void EventHandler::triggerEvent(void* data){
    this->action(data);
}

/**
 * @brief Set the action to be performed when the event is triggered
 * 
 * @param action std::function<void(void*)> action
 */
void EventHandler::setAction(std::function<void(void*)> action){
    this->action = action;
}

/**
 * @brief Get the name of the event
 * 
 * @return std::string 
 */
std::string EventHandler::getName(){
    return this->name;
}

/**
 * @brief Set the name of the event
 * 
 * @param name std::string
 */
void EventHandler::setName(std::string name){
    this->name = name;
}

/**
 * @brief Get the event type
 * 
 * @return sf::Event::EventType 
 */
sf::Event::EventType EventHandler::getEventType(){
    return this->eventType;
}

/**
 * @brief Set the event type
 * 
 * @param eventType sf::Event::EventType
 */
void EventHandler::setEventType(sf::Event::EventType eventType){
    this->eventType = eventType;
}

/////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new Event Manager:: Event Manager object
 * 
 * @param logger CubeLog object
 */
EventManager::EventManager(CubeLog *logger){
    this->logger = logger;
}

/**
 * @brief Destroy the Event Manager:: Event Manager object
 * 
 */
EventManager::~EventManager(){
    for (int i = 0; i < this->events.size(); i++){
        delete this->events[i];
    }
    this->logger->log("Event manager destroyed", true);
}

/**
 * @brief Create a new event
 * 
 * @param eventName std::string
 * @return int index of the event
 */
int EventManager::createEvent(std::string eventName){
    EventHandler *event = new EventHandler(this->logger);
    this->events.push_back(event);
    return this->events.size() - 1;
}

/**
 * @brief Remove an event
 * 
 * @param event EventHandler*
 */
bool EventManager::removeEvent(EventHandler* event){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i] == event){
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
bool EventManager::removeEvent(int index){
    delete this->events[index];
    this->events.erase(this->events.begin() + index);
    return true;
}

bool EventManager::removeEvent(std::string eventName){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getName() == eventName){
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
EventHandler* EventManager::getEvent(int index){
    return this->events[index];
}

/**
 * @brief Get all the events
 * 
 * @return std::vector<EventHandler*> 
 */
std::vector<EventHandler*> EventManager::getEvents(){
    return this->events;
}

/**
 * @brief Get the event by name
 * 
 * @param eventName 
 * @return EventHandler* 
 */
EventHandler* EventManager::getEvent(std::string eventName){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getName() == eventName){
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
bool EventManager::triggerEvent(sf::Event event, void* data){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getEventType() == event.type){
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
bool EventManager::triggerEvent(int index, void* data){
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
bool EventManager::triggerEvent(std::string eventName, void* data){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getName() == eventName){
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
bool EventManager::triggerEvent(sf::Event::EventType eventType, void* data){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getEventType() == eventType){
            this->events[i]->triggerEvent(data);
            return true;
        }
    }
    return false;
}

// Path src/gui/eventHandler/eventHandler.cpp
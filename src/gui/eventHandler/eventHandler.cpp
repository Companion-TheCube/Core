#include "eventHandler.h"

EventHandler::EventHandler(CubeLog *logger){
    this->logger = logger;
}

EventHandler::~EventHandler(){
    this->logger->log("Event handler destroyed", true);
}

void EventHandler::triggerEvent(void* data){
    this->action(data);
}

void EventHandler::setAction(std::function<void(void*)> action){
    this->action = action;
}

std::string EventHandler::getName(){
    return this->name;
}

void EventHandler::setName(std::string name){
    this->name = name;
}

sf::Event::EventType EventHandler::getEventType(){
    return this->eventType;
}

void EventHandler::setEventType(sf::Event::EventType eventType){
    this->eventType = eventType;
}

/////////////////////////////////////////////////////////////////////////////////
EventManager::EventManager(CubeLog *logger){
    this->logger = logger;
}

EventManager::~EventManager(){
    for (int i = 0; i < this->events.size(); i++){
        delete this->events[i];
    }
    this->logger->log("Event manager destroyed", true);
}

int EventManager::createEvent(std::string eventName){
    EventHandler *event = new EventHandler(this->logger);
    this->events.push_back(event);
    return this->events.size() - 1;
}

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

EventHandler* EventManager::getEvent(int index){
    return this->events[index];
}

EventHandler* EventManager::getEvent(std::string eventName){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getName() == eventName){
            return this->events[i];
        }
    }
    return nullptr;
}

bool EventManager::triggerEvent(sf::Event event){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getEventType() == event.type){
            this->events[i]->triggerEvent(&event);
            return true;
        }
    }
    return false;
}

bool EventManager::triggerEvent(int index){
    this->events[index]->triggerEvent();
    return true;
}

bool EventManager::triggerEvent(std::string eventName){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getName() == eventName){
            this->events[i]->triggerEvent();
            return true;
        }
    }
    return false;
}

bool EventManager::triggerEvent(sf::Event::EventType eventType){
    for (int i = 0; i < this->events.size(); i++){
        if (this->events[i]->getEventType() == eventType){
            this->events[i]->triggerEvent();
            return true;
        }
    }
    return false;
}

// Path src/gui/eventHandler/eventHandler.cpp
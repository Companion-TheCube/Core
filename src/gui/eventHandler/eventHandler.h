#pragma once
#include <functional>
#include <vector>
#include <logger.h>
#include <tuple>
#include <SFML/Graphics.hpp>

class EventHandler{
private:
    std::function<void(void*)> action;
    CubeLog *logger;
    std::string name;
    sf::Event::EventType eventType;
public:
    EventHandler(CubeLog *logger);
    ~EventHandler();
    void triggerEvent(void* data = nullptr);
    void setAction(std::function<void(void*)> action);
    std::string getName();
    void setName(std::string name);
    sf::Event::EventType getEventType();
    void setEventType(sf::Event::EventType eventType);
};

class EventManager{
private:
    std::vector<EventHandler*> events;
    CubeLog *logger;
public:
    EventManager(CubeLog *logger);
    ~EventManager();
    int createEvent(std::string eventName); // returns index of the event
    bool removeEvent(EventHandler* event); // removes event from the list
    bool removeEvent(int index); // removes event from the list
    bool removeEvent(std::string eventName); // removes event from the list
    EventHandler* getEvent(int index); // returns event by index
    EventHandler* getEvent(std::string eventName); // returns event by name
    bool triggerEvent(sf::Event event); // triggers event by sf::Event
    bool triggerEvent(int index); // triggers event by index
    bool triggerEvent(std::string eventName); // triggers event by name
    bool triggerEvent(sf::Event::EventType eventType); // triggers event by sf::Event::EventType
    std::vector<EventHandler*> getEvents();

};


#pragma once
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include "../objects.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <logger.h>
#include <tuple>
#include <vector>

enum class SpecificEventTypes : unsigned int {
    KEYPRESS_A,
    KEYPRESS_B,
    KEYPRESS_C,
    KEYPRESS_D,
    KEYPRESS_E,
    KEYPRESS_F,
    KEYPRESS_G,
    KEYPRESS_H,
    KEYPRESS_I,
    KEYPRESS_J,
    KEYPRESS_K,
    KEYPRESS_L,
    KEYPRESS_M,
    KEYPRESS_N,
    KEYPRESS_O,
    KEYPRESS_P,
    KEYPRESS_Q,
    KEYPRESS_R,
    KEYPRESS_S,
    KEYPRESS_T,
    KEYPRESS_U,
    KEYPRESS_V,
    KEYPRESS_W,
    KEYPRESS_X,
    KEYPRESS_Y,
    KEYPRESS_Z,
    KEYPRESS_0,
    KEYPRESS_1,
    KEYPRESS_2,
    KEYPRESS_3,
    KEYPRESS_4,
    KEYPRESS_5,
    KEYPRESS_6,
    KEYPRESS_7,
    KEYPRESS_8,
    KEYPRESS_9,
    KEYPRESS_ESC,
    KEYPRESS_LCTRL,
    KEYPRESS_LSHIFT,
    KEYPRESS_LALT,
    KEYPRESS_LSYSTEM,
    KEYPRESS_RCTRL,
    KEYPRESS_RSHIFT,
    KEYPRESS_RALT,
    KEYPRESS_RSYSTEM,
    KEYPRESS_MENU,
    KEYPRESS_LBRACKET,
    KEYPRESS_RBRACKET,
    KEYPRESS_SEMICOLON,
    KEYPRESS_COMMA,
    KEYPRESS_PERIOD,
    KEYPRESS_QUOTE,
    KEYPRESS_SLASH,
    KEYPRESS_BACKSLASH,
    KEYPRESS_TILDE,
    KEYPRESS_EQUAL,
    KEYPRESS_DASH,
    KEYPRESS_SPACE,
    KEYPRESS_RETURN,
    KEYPRESS_BACKSPACE,
    KEYPRESS_TAB,
    KEYPRESS_PAGEUP,
    KEYPRESS_PAGEDOWN,
    KEYPRESS_END,
    KEYPRESS_HOME,
    KEYPRESS_INSERT,
    KEYPRESS_DELETE,
    KEYPRESS_ADD,
    KEYPRESS_SUBTRACT,
    KEYPRESS_MULTIPLY,
    KEYPRESS_DIVIDE,
    KEYPRESS_LEFT,
    KEYPRESS_RIGHT,
    KEYPRESS_UP,
    KEYPRESS_DOWN,
    KEYPRESS_NUMPAD0,
    KEYPRESS_NUMPAD1,
    KEYPRESS_NUMPAD2,
    KEYPRESS_NUMPAD3,
    KEYPRESS_NUMPAD4,
    KEYPRESS_NUMPAD5,
    KEYPRESS_NUMPAD6,
    KEYPRESS_NUMPAD7,
    KEYPRESS_NUMPAD8,
    KEYPRESS_NUMPAD9,
    KEYPRESS_F1,
    KEYPRESS_F2,
    KEYPRESS_F3,
    KEYPRESS_F4,
    KEYPRESS_F5,
    KEYPRESS_F6,
    KEYPRESS_F7,
    KEYPRESS_F8,
    KEYPRESS_F9,
    KEYPRESS_F10,
    KEYPRESS_F11,
    KEYPRESS_F12,
    KEYPRESS_F13,
    KEYPRESS_F14,
    KEYPRESS_F15,
    KEYPRESS_PAUSE,
    MOUSEPRESSED_LEFT,
    MOUSEPRESSED_RIGHT,
    MOUSEPRESSED_MIDDLE,
    MOUSEPRESSED_XBUTTON1,
    MOUSEPRESSED_XBUTTON2,
    MOUSERELEASED_LEFT,
    MOUSERELEASED_RIGHT,
    MOUSERELEASED_MIDDLE,
    MOUSERELEASED_XBUTTON1,
    MOUSERELEASED_XBUTTON2,
    MOUSEMOVED,
    MOUSEWHEEL,
    MOUSEENTERED,
    MOUSELEFT,
    JOYSTICKCONNECTED,
    JOYSTICKDISCONNECTED,
    JOYSTICKBUTTONPRESSED,
    JOYSTICKBUTTONRELEASED,
    JOYSTICKMOVED,
    DRAG_Y,
    DRAG_X,
    NULL_EVENT
};

class EventHandler {
private:
    std::function<void(void*)> action;
    std::string name;
    sf::Event::EventType eventType;
    SpecificEventTypes specificEventType;

public:
    EventHandler();
    ~EventHandler();
    bool triggerEvent(void* data = nullptr);
    void setAction(std::function<void(void*)> action);
    std::string getName();
    void setName(const std::string& name);
    sf::Event::EventType getEventType();
    void setEventType(sf::Event::EventType eventType);
    void setSpecificEventType(SpecificEventTypes specificEventType);
    SpecificEventTypes getSpecificEventType();
};

class EventManager {
private:
    std::vector<EventHandler*> events;
    std::vector<ClickableArea*> clickableAreas;
    bool checkClickableAreas(sf::Event event);
    std::tuple<int, int> mouseDownPosition;

public:
    EventManager();
    ~EventManager();
    int createEvent(const std::string& eventName); // returns index of the event
    bool removeEvent(EventHandler* event); // removes event from the list
    bool removeEvent(int index); // removes event from the list
    bool removeEvent(const std::string& eventName); // removes event from the list
    EventHandler* getEvent(int index); // returns event by index
    EventHandler* getEvent(const std::string& eventName); // returns event by name
    bool triggerEvent(sf::Event event, void* data); // triggers event by sf::Event
    bool triggerEvent(int index, void* data); // triggers event by index
    bool triggerEvent(const std::string& eventName, void* data); // triggers event by name
    bool triggerEvent(sf::Event::EventType eventType, void* data); // triggers event by sf::Event::EventType
    bool triggerEvent(SpecificEventTypes specificEventType, void* data); // triggers event by SpecificEventTypes
    bool triggerEvent(SpecificEventTypes specificEventType, sf::Event::EventType eventType, void* data); // triggers event by SpecificEventTypes
    std::vector<EventHandler*> getEvents();
    void addClickableArea(ClickableArea* clickableArea);
};

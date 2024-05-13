#pragma once
#include "./../gui.h"
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../renderables/objects.h"

class Menu: public Clickable{
private:
    CubeLog *logger;
    bool visible;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    std::string name;
    std::vector<Object> objects;
    bool loadObjects(std::string filename);
public:
    Menu(CubeLog *logger, std::string filename);
    ~Menu();
    void onClick(void*);
    void onRightClick(void*);
    void setVisible(bool visible);
    bool getVisible();
    void setOnClick(std::function<void(void*)> action);
    void setOnRightClick(std::function<void(void*)> action);
};

class MenuBox:public M_Box{
private:
    CubeLog *logger;
    sf::RectangleShape box;
    sf::Text text;
    sf::Font font;
    sf::Color color;
    std::string message;
    sf::Vector2f position;
    sf::Vector2f size;
    
public:
    ~MenuBox();
    MenuBox(CubeLog* logger, std::string message, sf::Vector2f position, sf::Vector2f size, sf::Font font, sf::Color color);
    void setPosition(sf::Vector2f position);
    void setColor(sf::Color color);
    void setFont(sf::Font font);
    void draw();
};
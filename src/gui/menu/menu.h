#pragma once
#include "./../gui.h"
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "./../renderables/meshObject.h"
#include "./../renderables/meshes/arc.h"
#include "./../objects.h"

class Menu: public Clickable{
private:
    CubeLog *logger;
    bool visible;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    std::string name;
    std::vector<Object*> objects;
    bool loadObjects(std::string filename);
public:
    Menu(CubeLog *logger, std::string filename, Shader* shader);
    ~Menu();
    void onClick(void*);
    void onRightClick(void*);
    void setVisible(bool visible);
    bool getVisible();
    void setOnClick(std::function<void(void*)> action);
    void setOnRightClick(std::function<void(void*)> action);
    std::vector<Object*> getObjects();
};

class MenuBox:public M_Box{
private:
    CubeLog *logger;
    glm::vec2 position;
    glm::vec2 size;
    std::vector<MeshObject*> objects;
public:
    MenuBox(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuBox();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
};
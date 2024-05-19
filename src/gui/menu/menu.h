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
#include "./../renderables/meshes/shapes.h"
#include "./../objects.h"
#include <typeinfo>

class MenuStencil;

class Menu: public Clickable{
private:
    CubeLog *logger;
    bool visible;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    std::string name;
    std::vector<Object*> objects;
    bool loadObjects(std::string filename);
    Shader* shader;
    std::string filename;
    bool ready = false;
    std::mutex mutex;
    std::vector<Clickable*> childrenClickables;
    ClickableArea clickArea;
    MenuStencil* stencil;
    Shader* textShader;
    float menuItemTextSize = 36.f;
public:
    Menu(CubeLog *logger, std::string filename, Shader* shader);
    ~Menu();
    void setup();
    void onClick(void*);
    void onRightClick(void*);
    bool setVisible(bool visible);
    bool getVisible();
    void setOnClick(std::function<void(void*)> action);
    void setOnRightClick(std::function<void(void*)> action);
    std::vector<MeshObject*> getObjects();
    bool isReady();
    void draw();
    std::vector<ClickableArea*> getClickableAreas();
    void addMenuEntry(std::string text, std::function<void(void*)> action);
    void addMenuEntry(std::string text, std::function<void(void*)> action, std::function<void(void*)> rightAction);
    void addHorizontalRule();
};

class MenuBox:public M_Box{
private:
    CubeLog *logger;
    glm::vec2 position;
    glm::vec2 size;
    std::vector<MeshObject*> objects;
    bool visible;
    static float index;
public:
    // TODO: add bool: border to constructor
    MenuBox(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuBox();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
};

class MenuHorizontalRule:public M_Box{  
private:
    CubeLog *logger;
    glm::vec2 position;
    glm::vec2 size;
    std::vector<MeshObject*> objects;
    bool visible;
public:
    MenuHorizontalRule(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuHorizontalRule();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
};

class MenuStencil: public Object{
private:
    CubeLog *logger;
    glm::vec2 position;
    glm::vec2 size;
    Shader* shader;
    GLuint VAO, VBO, EBO;
    std::vector<MeshObject*> objects;
    std::vector<Vertex> vertices;
    unsigned int indices[6] = {
        0, 1, 2,
        2, 3, 0
    };
public:
    MenuStencil(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuStencil();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
    void enable();
    void disable();
};

class MenuEntry:public Clickable{
private:
    CubeLog *logger;
    std::string text;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    bool visible;
    Shader* shader;
    std::vector<MeshObject*> objects;
    glm::vec2 position;
    ClickableArea clickArea;
public:
    MenuEntry(CubeLog* logger, std::string text, Shader* shader, glm::vec2 position, float size);
    ~MenuEntry();
    void onClick(void*);
    void onRightClick(void*);
    std::vector<MeshObject*> getObjects();
    bool setVisible(bool visible);
    bool getVisible();
    void setOnClick(std::function<void(void*)> action);
    void setOnRightClick(std::function<void(void*)> action);
    void draw();
    void setPosition(glm::vec2 position);
    void setVisibleWidth(float width);
};

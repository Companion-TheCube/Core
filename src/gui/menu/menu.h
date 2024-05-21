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

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
#define STENCIL_INSET_PX 6
#define MENU_ITEM_TEXT_SIZE 36.f
#define MENU_ITEM_PADDING_PX 15.f
#define MENU_TOP_PADDING_PX 35.f
#define MENU_HEIGHT_SCREEN_RELATIVE 2.0f
#define MENU_WIDTH_SCREEN_RELATIVE 1.2f
#define MENU_POSITION_SCREEN_RELATIVE_X_CENTER 0.4f
#define MENU_POSITION_SCREEN_RELATIVE_Y_CENTER 0.0f
#define MENU_POSITION_SCREEN_RELATIVE_X_LEFT (MENU_POSITION_SCREEN_RELATIVE_X_CENTER - (MENU_WIDTH_SCREEN_RELATIVE / 2))
#define MENU_POSITION_SCREEN_RELATIVE_X_RIGHT (MENU_POSITION_SCREEN_RELATIVE_X_CENTER + (MENU_WIDTH_SCREEN_RELATIVE / 2))
#define MENU_POSITION_SCREEN_RELATIVE_Y_TOP (MENU_POSITION_SCREEN_RELATIVE_Y_CENTER + (MENU_HEIGHT_SCREEN_RELATIVE / 2))
#define MENU_POSITION_SCREEN_RELATIVE_Y_BOTTOM (MENU_POSITION_SCREEN_RELATIVE_Y_CENTER - (MENU_HEIGHT_SCREEN_RELATIVE / 2))
#define SCREEN_RELATIVE_MAX_X 1.0f
#define SCREEN_RELATIVE_MAX_Y 1.0f
#define SCREEN_RELATIVE_MIN_X -1.0f
#define SCREEN_RELATIVE_MIN_Y -1.0f
#define SCREEN_RELATIVE_MAX_WIDTH 2.0f
#define SCREEN_RELATIVE_MAX_HEIGHT 2.0f
#define SCREEN_RELATIVE_MIN_WIDTH 0.0f
#define SCREEN_RELATIVE_MIN_HEIGHT 0.0f
#define SCREEN_PX_MAX_X 720.f
#define SCREEN_PX_MAX_Y 720.f
#define SCREEN_PX_MIN_X 0.f
#define SCREEN_PX_MIN_Y 0.f

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
    float menuItemTextSize = MENU_ITEM_TEXT_SIZE;
    long scrollPosition = 0;
    bool onClickEnabled = true;
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
    void scroll(int x);
    ClickableArea* getClickableArea();
    void setVisibleWidth(float width){}
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

class MenuHorizontalRule:public Clickable{
private:
    CubeLog *logger;
    glm::vec2 position;
    float size;
    std::vector<MeshObject*> objects;
    bool visible;
    Shader* shader;
    ClickableArea clickArea;
public:
    MenuHorizontalRule(CubeLog* logger, glm::vec2 position, float size, Shader* shader);
    ~MenuHorizontalRule();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setSize(float size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
    void onClick(void*);
    void onRightClick(void*);
    std::vector<MeshObject*> getObjects();
    void setOnClick(std::function<void(void*)> action);
    void setOnRightClick(std::function<void(void*)> action);
    ClickableArea* getClickableArea();
    void setVisibleWidth(float width){}
};

class MenuStencil: public Object{
private:
    CubeLog *logger;
    glm::vec2 position;
    glm::vec2 size;
    Shader* shader;
    GLuint VAO, VBO, EBO;
    std::vector<MeshObject*> objects;
    std::vector<glm::vec2> vertices;
    unsigned int indices[6] = {
        0, 3, 2,
        2, 1, 0
    };
    glm::mat4 projectionMatrix;
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
    glm::vec2 size;
    bool scrolling = false;
    float visibleWidth = 0;
    float scrollPosition = 0;
    float scrollWait = 0;
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
    ClickableArea* getClickableArea();
};

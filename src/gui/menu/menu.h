#ifndef MENU_H
#define MENU_H
// #ifndef GUI_H
// #include "./../gui.h"
// #endif
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <typeinfo>
#ifndef MESHOBJECT_H
#include "./../renderables/meshObject.h"
#endif
#ifndef SHAPES_H
#include "./../renderables/shapes.h"
#endif
#ifndef OBJECTS_H
#include "./../objects.h"
#endif
#include <typeinfo>
#ifndef RENDERER_H
#include "./../renderer.h"
#endif// MENU_H

#define MENU_ITEM_SCROLL_LEFT_SPEED 1.5f
#define MENU_ITEM_SCROLL_RIGHT_SPEED 4.2f

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
#define STENCIL_INSET_PX 6
#define MENU_ITEM_TEXT_SIZE 32.f
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

class Menu : public Clickable {
private:
    bool visible = false;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    std::string name;
    std::vector<Object*> objects;
    Renderer* renderer;
    bool ready = false;
    std::mutex mutex;
    std::vector<Clickable*> childrenClickables;
    ClickableArea clickArea;
    MenuStencil* stencil;
    float menuItemTextSize = MENU_ITEM_TEXT_SIZE;
    long scrollVertPosition = 0;
    bool onClickEnabled = true;
    CountingLatch* latch;
    int maxScrollY = 0;
    std::string menuName;
    Shader* textShader;
    std::mutex menuMutex;
    Menu* parentMenu = nullptr;
    bool isMainMenu = false;
    bool isClickable = false;
public:
    Menu(Renderer* renderer, CountingLatch& latch);
    Menu(Renderer* renderer, CountingLatch& latch, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax);
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
    void setParentMenu(Menu* parentMenu);
    Menu* getParentMenu();
    void setMenuName(std::string name);
    void setAsMainMenu();
    void setIsClickable(bool isClickable);
    std::string getMenuName();
    void addHorizontalRule();
    void scrollVert(int y);
    ClickableArea* getClickableArea();
    void setVisibleWidth(float width) { }
    void resetScroll() { }
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) { }
    void capturePosition() { }
    void restorePosition() { }
    bool getIsClickable();
};

class MenuBox : public M_Box {
private:
    glm::vec2 position;
    glm::vec2 size;
    std::vector<MeshObject*> objects;
    bool visible;
    static float index;

public:
    // TODO: add bool: border to constructor
    MenuBox(glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuBox();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
};

class MenuHorizontalRule : public Clickable {
private:
    glm::vec2 position;
    float size;
    std::vector<MeshObject*> objects;
    bool visible;
    Shader* shader;
    ClickableArea clickArea;

public:
    MenuHorizontalRule(glm::vec2 position, float size, Shader* shader);
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
    void setVisibleWidth(float width) { }
    void resetScroll() { }
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) { }
    void capturePosition();
    void restorePosition();
    bool getIsClickable(){return false;}
};

class MenuStencil : public Object {
private:
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
    MenuStencil(glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuStencil();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
    void enable();
    void disable();
};

class MenuEntry : public Clickable {
private:
enum ScrollingDirection{
    NOT_SCROLLING,
    SCROLL_LEFT,
    SCROLL_RIGHT
};
    std::string text;
    std::function<void(void*)> action;
    std::function<void(void*)> rightAction;
    bool visible;
    Shader* shader;
    std::vector<MeshObject*> objects;
    glm::vec2 position;
    ClickableArea clickArea;
    glm::vec2 size;
    ScrollingDirection scrolling = NOT_SCROLLING;
    float visibleWidth = 0;
    float scrollPositionLeft = 0;
    float scrollPositionRight = 0;
    float scrollWait = 0;
    glm::vec4 originalPosition;

public:
    MenuEntry(std::string text, Shader* shader, glm::vec2 position, float size);
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
    void resetScroll();
    ClickableArea* getClickableArea();
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax);
    void capturePosition();
    void restorePosition();
    bool getIsClickable();
};

#endif// MENU_H

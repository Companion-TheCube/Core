#ifndef MENU_H
#define MENU_H
// #ifndef GUI_H
// #include "./../gui.h"
// #endif
#include <bitset>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
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
#endif // MENU_H

#define MENU_ITEM_SCROLL_LEFT_SPEED 1.5f
#define MENU_ITEM_SCROLL_RIGHT_SPEED 4.2f
#define MENU_ENTRY_DIM_COLOR { 0.6f, 0.6f, 0.6f }

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
#define MENU_ITEM_TEXT_SIZE 32.f
#define MENU_ITEM_PADDING_PX 10.f
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

namespace MENUS {

class MenuStencil;

enum class EntryType {
    MENUENTRY_TYPE_SUBMENU,
    MENUENTRY_TYPE_ACTION,
    MENUENTRY_TYPE_RADIOBUTTON_GROUP,
    MENUENTRY_TYPE_CHECKBOX,
    MENUENTRY_TYPE_TOGGLE,
    MENUENTRY_TYPE_TEXT_INFO,
    MENUENTRY_TYPE_TEXT_INPUT,
    MENUENTRY_TYPE_INLINE_TEXT,
    MENUENTRY_TYPE_SLIDER,

    MENUENTRY_TYPE_COUNT
};

const std::unordered_map<std::string, EntryType> entryTypeMap = {
    { "MENUENTRY_TYPE_SUBMENU", EntryType::MENUENTRY_TYPE_SUBMENU },
    { "MENUENTRY_TYPE_ACTION", EntryType::MENUENTRY_TYPE_ACTION },
    { "MENUENTRY_TYPE_RADIOBUTTON_GROUP", EntryType::MENUENTRY_TYPE_RADIOBUTTON_GROUP },
    { "MENUENTRY_TYPE_CHECKBOX", EntryType::MENUENTRY_TYPE_CHECKBOX },
    { "MENUENTRY_TYPE_TOGGLE", EntryType::MENUENTRY_TYPE_TOGGLE },
    { "MENUENTRY_TYPE_TEXT_INFO", EntryType::MENUENTRY_TYPE_TEXT_INFO },
    { "MENUENTRY_TYPE_TEXT_INPUT", EntryType::MENUENTRY_TYPE_TEXT_INPUT },
    { "MENUENTRY_TYPE_SLIDER", EntryType::MENUENTRY_TYPE_SLIDER }
};

class MenuEntry : public Clickable {
private:
    enum class ScrollingDirection {
        NOT_SCROLLING,
        SCROLL_LEFT,
        SCROLL_RIGHT
    };
    std::string text;
    std::vector<std::function<unsigned int(void*)>> actions;
    std::function<unsigned int(void*)> rightAction;
    std::function<unsigned int(void*)> statusAction;
    void* statusActionArg;
    bool visible;
    Shader* textShader;
    Shader* meshShader;
    std::vector<MeshObject*> allObjects;
    std::vector<MeshObject*> scrollObjects;
    std::vector<MeshObject*> fixedObjects;
    std::vector<MeshObject*> xFixedObjects;
    std::vector<MeshObject*> yFixedObjects;
    glm::vec2 position;
    ClickableArea clickArea;
    glm::vec2 size;
    ScrollingDirection scrolling = ScrollingDirection::NOT_SCROLLING;
    float visibleWidth = 0;
    float scrollPositionLeft = 0;
    float scrollPositionRight = 0;
    float scrollWait = 0;
    glm::vec4 originalPosition;
    EntryType type = EntryType::MENUENTRY_TYPE_ACTION;
    void setVisibleWidth(float width);
    static unsigned int menuEntryCount;
    unsigned int menuEntryIndex;
    int groupID;
    M_Text* textObject;
    bool isClickable = false;

public:
    MenuEntry(Shader* t_shader, Shader* m_shader, const std::string& text, glm::vec2 position, float size, float visibleWidth, EntryType type, std::function<unsigned int(void*)> statusAction, void* statusActionArg);
    ~MenuEntry();
    void onClick(void*);
    void onRelease(void*);
    void onMouseDown(void*);
    void onRightClick(void*);
    std::vector<MeshObject*> getObjects();
    bool setVisible(bool visible);
    bool getVisible();
    bool setIsClickable(bool isClickable);
    bool getIsClickable();
    void setOnClick(std::function<unsigned int(void*)> action);
    void setOnRightClick(std::function<unsigned int(void*)> action);
    void setStatusAction(std::function<unsigned int(void*)> action);
    void draw();
    void resetScroll();
    ClickableArea* getClickableArea();
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax);
    void capturePosition();
    void restorePosition();
    void translate(glm::vec2 translation);
    unsigned int clickReturnData = 0;
    unsigned int statusReturnData = 0;
    unsigned int getMenuEntryIndex() { return this->menuEntryIndex; }
    void callStatusAction() { this->statusReturnData = this->statusAction(this->statusActionArg); }
    void setGroupID(int groupID) { this->groupID = groupID; }
    int getGroupID() { return this->groupID; }
    void setStatusReturnData(unsigned int statusReturnData) { this->statusReturnData = statusReturnData; }
    void setEntryTextTouched(bool touched);
    std::string getEntryText() { return this->text; }
    void setEntryText(const std::string& text);
};

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
    CountingLatch* latch = nullptr;
    bool hasLatch = true;
    int maxScrollY = 0;
    std::string menuName;
    std::string uniqueMenuIdentifier;
    Shader* textShader;
    std::mutex menuMutex;
    Menu* parentMenu = nullptr;
    bool isMainMenu = false;
    bool isClickable = false;
    static bool mainMenuSet;
    static std::vector<MenuEntry*> allMenuEntries_vec;

public:
    Menu(Renderer* renderer);
    Menu(Renderer* renderer, CountingLatch& latch);
    Menu(Renderer* renderer, CountingLatch& latch, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax);
    ~Menu();
    void setup();
    void onClick(void*);
    void onRelease(void*);
    void onMouseDown(void*);
    void onRightClick(void*);
    bool setVisible(bool visible);
    bool getVisible();
    void setOnClick(std::function<unsigned int(void*)> action);
    void setOnRightClick(std::function<unsigned int(void*)> action);
    std::vector<MeshObject*> getObjects();
    bool isReady();
    void draw();
    std::vector<ClickableArea*> getClickableAreas();
    unsigned int addMenuEntry(const std::string& text, const std::string& uniqueID, MENUS::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> statusAction, void* statusActionData);
    unsigned int addMenuEntry(const std::string& text, const std::string& uniqueID, MENUS::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> rightAction, std::function<unsigned int(void*)> statusAction, void* statusActionData);
    void setParentMenu(Menu* parentMenu);
    Menu* getParentMenu();
    void setMenuName(const std::string& name);
    void setAsMainMenu();
    bool setIsClickable(bool isClickable);
    std::string getMenuName();
    void setUniqueMenuIdentifier(const std::string& uniqueMenuIdentifier);
    std::string getUniqueMenuIdentifier();
    void addHorizontalRule();
    void scrollVert(int y);
    ClickableArea* getClickableArea();
    void setVisibleWidth(float width) { }
    void resetScroll() { }
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) { }
    void capturePosition() { }
    void restorePosition() { }
    bool getIsClickable();
    bool setChildrenClickables_isClickable(bool isClickable);
    static MenuEntry* getMenuEntryByIndex(unsigned int index);
    static std::vector<MenuEntry*> getMenuEntriesByGroupID(int groupID);
};

class MenuBox : public M_Box {
private:
    glm::vec2 position;
    glm::vec2 size;
    std::vector<MeshObject*> objects;
    bool visible;

public:
    static float index;
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
    // MenuStencil* stencil;

public:
    MenuHorizontalRule(glm::vec2 position, float size, Shader* shader);
    ~MenuHorizontalRule();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setSize(float size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
    bool setIsClickable(bool isClickable) { return false; }
    bool getIsClickable() { return false; }
    void onClick(void*);
    void onRelease(void*);
    void onMouseDown(void*);
    void onRightClick(void*);
    std::vector<MeshObject*> getObjects();
    void setOnClick(std::function<unsigned int(void*)> action);
    void setOnRightClick(std::function<unsigned int(void*)> action);
    ClickableArea* getClickableArea();
    void setVisibleWidth(float width) { }
    void resetScroll() { }
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) { }
    void capturePosition();
    void restorePosition();
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
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    int stencilIndex = 0;

public:
    MenuStencil(glm::vec2 position, glm::vec2 size, Shader* shader);
    ~MenuStencil();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void draw();
    bool setVisible(bool visible);
    bool getVisible();
    virtual void enable();
    virtual void disable();
    void translate(glm::vec2 translation);
};
} // namespace MENUS
#endif // MENU_H

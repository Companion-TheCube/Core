#include "menu.h"

namespace MENUS {
float screenRelativeToScreenPx(float screenRelative);
float screenPxToScreenRelative(float screenPx);
float screenPxToScreenRelativeWidth(float screenPx);
float screenRelativeToScreenPxWidth(float screenRelative);
bool Menu::mainMenuSet = false;
std::vector<MenuEntry*> Menu::allMenuEntries_vec = std::vector<MenuEntry*>();

/**
 * @brief Construct a new Menu object
 *
 * @param renderer - the renderer object
 *
 * @return Menu* - the new Menu object
 *
 * @note This constructor will create a Menu object without a CountingLatch. "this->latch" will be set to nullptr.
 */
Menu::Menu(Renderer* renderer)
    : Menu(renderer, *this->latch)
{
    this->hasLatch = false;
}

/**
 * @brief Construct a new Menu:: Menu object
 *
 * @param renderer
 * @param latch
 *
 * @note This constructor will create a Menu object with a CountingLatch. "this->latch" will be set to the latch parameter.
 */
Menu::Menu(Renderer* renderer, CountingLatch& latch)
    : Menu(renderer, latch, 0, 720, 0, 720)
{
    CubeLog::info("Creating Menu class object");
    this->latch = &latch;
}

/**
 * @brief Construct a new Menu:: Menu object
 *
 * @param renderer
 * @param latch
 * @param xMin
 * @param xMax
 * @param yMin
 * @param yMax
 */
Menu::Menu(Renderer* renderer, CountingLatch& latch, unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax)
{
    CubeLog::info("Creating Menu object with click area size and position of " + std::to_string(xMin) + "x" + std::to_string(yMin) + " to " + std::to_string(xMax) + "x" + std::to_string(yMax));
    this->latch = &latch;
    this->renderer = renderer;
    this->visible = false;
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = this;
    this->clickArea.xMin = xMin;
    this->clickArea.xMax = xMax;
    this->clickArea.yMin = yMin;
    this->clickArea.yMax = yMax;
    this->setOnClick([&](void* data) {
        this->setIsClickable(!this->getIsClickable());
        this->setVisible(!this->getVisible());
        return 0;
    });
    CubeLog::info("Menu created");
}

/**
 * @brief Destroy the Menu:: Menu object
 *
 */
Menu::~Menu()
{
    // CubeLog::info("Menu destroyed");
    for (auto object : this->objects) {
        delete object;
    }
}

/**
 * @brief Add a menu entry to the menu
 *
 * @param text The text to display
 * @param action The action to take when the menu entry is clicked
 */
unsigned int Menu::addMenuEntry(const std::string& text, const std::string& uniqueID, MENUS::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> statusAction, void* statusActionData)
{
    // TODO: add the ability to have an entry be fixed to the top of the menu. This will need a stencil so that other entries can be scrolled under it.
    // TODO: add icon support
    // TODO: add checkbox support
    // TODO: add slider support
    CubeLog::debug("Adding MenuEntry with text: " + text);
    float startY = (((menuItemTextSize * 1.2) + MENU_ITEM_PADDING_PX) * this->childrenClickables.size()) + MENU_TOP_PADDING_PX;
    float textX = mapRange(MENU_POSITION_SCREEN_RELATIVE_X_LEFT, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X) + (STENCIL_INSET_PX * 2);
    float textY = mapRange(MENU_POSITION_SCREEN_RELATIVE_Y_TOP, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y) - startY - (STENCIL_INSET_PX * 2) - this->menuItemTextSize;
    float menuWidthPx = mapRange(MENU_WIDTH_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
    float menuWidthAdjusted = menuWidthPx - (STENCIL_INSET_PX * 2) - (MENU_ITEM_PADDING_PX * 2);
    auto entry = new MenuEntry(this->renderer->getTextShader(), this->renderer->getMeshShader(), text, { textX, textY }, menuItemTextSize, menuWidthAdjusted, type, statusAction, statusActionData);
    entry->getClickableArea()->yMin -= (MENU_ITEM_PADDING_PX);
    entry->getClickableArea()->yMax += (MENU_ITEM_PADDING_PX);
    entry->setVisible(true);
    entry->setIsClickable(true);
    entry->setOnClick(action);
    for (auto object : entry->getObjects()) {
        object->capturePosition();
    }
    entry->setStatusAction(statusAction);
    this->childrenClickables.push_back(entry);
    if (this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax > this->maxScrollY)
        this->maxScrollY = this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax;

    CubeLog::debug("MenuEntry added with text: " + text + " and clickable area: " + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->xMin) + "x" + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->yMin) + " to " + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->xMax) + "x" + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->yMax));
    Menu::allMenuEntries_vec.push_back(entry);
    return entry->getMenuEntryIndex();
}

/**
 * @brief Add a menu entry to the menu with a right click action
 *
 * @param text The text to display
 * @param action The action to take when the menu entry is clicked
 * @param rightAction The action to take when the menu entry is right clicked
 */
unsigned int Menu::addMenuEntry(const std::string& text, const std::string& uniqueID, MENUS::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> rightAction, std::function<unsigned int(void*)> statusAction, void* statusActionData)
{
    unsigned int t = this->addMenuEntry(text, uniqueID, type, action, statusAction, statusActionData);
    this->childrenClickables.at(this->childrenClickables.size() - 1)->setOnRightClick(rightAction);
    return t;
}

/**
 * @brief Add a horizontal rule to the menu
 *
 */
void Menu::addHorizontalRule()
{
    CubeLog::info("Adding horizontal rule");
    float startY = (((menuItemTextSize * 1.2) + MENU_ITEM_PADDING_PX) * this->childrenClickables.size()) + MENU_TOP_PADDING_PX + ((menuItemTextSize + (MENU_ITEM_PADDING_PX * 2)) / 2);
    // get start x from screen relative position of menu
    float startX = mapRange(MENU_POSITION_SCREEN_RELATIVE_X_LEFT, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X) + (STENCIL_INSET_PX * 2) + 30;
    this->childrenClickables.push_back(new MenuHorizontalRule({ startX, startY }, 350, this->renderer->getMeshShader()));
    this->childrenClickables.at(this->childrenClickables.size() - 1)->setVisible(true);
    for (auto object : this->childrenClickables.at(this->childrenClickables.size() - 1)->getObjects()) {
        object->capturePosition();
    }
    if (this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax > this->maxScrollY)
        this->maxScrollY = this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax;
}

/**
 * @brief Scroll the menu vertically
 *
 * @param y the amount to scroll, positive moves the menu up, negative moves the menu down
 */
void Menu::scrollVert(int y)
{
    if (this->getVisible() == false)
        return;
    if (this->scrollVertPosition + y > this->maxScrollY - SCREEN_PX_MAX_Y) {
        int maxY = this->maxScrollY - SCREEN_PX_MAX_Y;
        y = maxY - this->scrollVertPosition;
    }

    this->scrollVertPosition += y;
    if (this->scrollVertPosition <= 0) {
        this->scrollVertPosition = 0;
        for (auto clickable : this->childrenClickables) {
            clickable->restorePosition();
            clickable->resetScroll();
        }
        return;
    }

    for (auto clickable : this->childrenClickables) {
        float new_y_min, new_y_max;
        new_y_min = clickable->getClickableArea()->yMin - y;
        new_y_max = clickable->getClickableArea()->yMax - y;
        clickable->setClickAreaSize(clickable->getClickableArea()->xMin, clickable->getClickableArea()->xMax, new_y_min, new_y_max);
        for (auto object : clickable->getObjects()) {
            if (object->type == "MenuHorizontalRule")
                object->translate({ 0, mapRange(float(y), SCREEN_PX_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_RELATIVE_MAX_HEIGHT, 0.f), 0 });
            else
                object->translate({ 0, y, 0 });
        }
    }
}

/**
 * @brief Set the reference to this menu's parent menu
 *
 * @param parentMenu the parent menu
 * @return void
 */
void Menu::setParentMenu(Menu* parentMenu)
{
    std::unique_lock<std::mutex> lock(this->menuMutex);
    this->parentMenu = parentMenu;
}

/**
 * @brief Get the parent menu
 */
Menu* Menu::getParentMenu()
{
    std::unique_lock<std::mutex> lock(this->menuMutex);
    return this->parentMenu;
}

/**
 * @brief Set the name of the menu. This is used to identify the menu in JSON menu configuration files and API calls
 */
void Menu::setMenuName(const std::string& name)
{
    this->menuName = name;
}

/**
 * @brief Get the name of the menu
 */
std::string Menu::getMenuName()
{
    return this->menuName;
}

/**
 * @brief Set the unique identifier for this menu. This is used to identify the menu in the menu structure.
 *
 * @param uniqueMenuIdentifier the unique identifier for this menu
 */
void Menu::setUniqueMenuIdentifier(const std::string& uniqueMenuIdentifier)
{
    this->uniqueMenuIdentifier = uniqueMenuIdentifier;
}

/**
 * @brief Get the unique identifier for this menu
 */
std::string Menu::getUniqueMenuIdentifier()
{
    return this->uniqueMenuIdentifier;
}

/**
 * @brief Set this menu as the main menu. Only one menu can be the main menu. Throws an error if a menu is already set as the main menu.
 */
void Menu::setAsMainMenu()
{
    if (Menu::mainMenuSet) {
        CubeLog::error("Main menu already set. Only one menu can be set as the main menu.");
        throw std::runtime_error("Main menu already set. Only one menu can be set as the main menu.");
        return;
    }
    this->isMainMenu = true;
    this->isClickable = true;
    Menu::mainMenuSet = true;
}

/**
 * @brief Setup the menu. This must be called by the renderer thread.
 *
 */
void Menu::setup()
{
    this->objects.push_back(new MenuBox({ MENU_POSITION_SCREEN_RELATIVE_X_CENTER, MENU_POSITION_SCREEN_RELATIVE_Y_CENTER }, { MENU_WIDTH_SCREEN_RELATIVE, MENU_HEIGHT_SCREEN_RELATIVE }, this->renderer->getMeshShader()));
    this->objects.at(0)->setVisible(true);
    // Shader* stencilShader = new Shader("shaders/menuStencil.vs", "shaders/menuStencil.fs");
    float stencilX_start_temp = MENU_POSITION_SCREEN_RELATIVE_X_CENTER - MENU_WIDTH_SCREEN_RELATIVE / 2;
    float stencilY_start_temp = MENU_POSITION_SCREEN_RELATIVE_Y_CENTER - MENU_HEIGHT_SCREEN_RELATIVE / 2;
    float stencilX_start = mapRange(stencilX_start_temp, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
    float stencilY_start = mapRange(stencilY_start_temp, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y);
    float stencilWidth = mapRange(MENU_WIDTH_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X) - (STENCIL_INSET_PX * 2);
    float stencilHeight = mapRange(MENU_HEIGHT_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_HEIGHT, SCREEN_RELATIVE_MAX_HEIGHT, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y) - (STENCIL_INSET_PX * 2);
    this->stencil = new MenuStencil({ stencilX_start + STENCIL_INSET_PX, stencilY_start + STENCIL_INSET_PX }, { stencilWidth, stencilHeight }, this->renderer->getStencilShader());
    std::lock_guard<std::mutex> lock(this->mutex);
    this->ready = true;
    if (this->hasLatch)
        this->latch->count_down();
    CubeLog::info("Menu setup done");
}

/**
 * @brief Handle the click event
 *
 * @param data the data to pass to the action
 */
void Menu::onClick(void* data)
{
    if (this->action != nullptr && this->onClickEnabled) {
        this->action(data);
    }
}

void Menu::onRelease(void* data)
{
    // CubeLog::info("Menu released");
    // noop
}

void Menu::onMouseDown(void* data)
{
    CubeLog::info("Menu mouse down");
}

/**
 * @brief Handle the right click event
 *
 * @param data the data to pass to the action
 */
void Menu::onRightClick(void* data)
{
    CubeLog::info("Menu right clicked");
    if (this->rightAction != nullptr && this->getVisible()) {
        this->rightAction(data);
    }
}

/**
 * @brief Set the visibility of the menu
 *
 * @param visible the visibility of the menu
 * @return bool the previous visibility
 */
bool Menu::setVisible(bool visible)
{
    std::unique_lock<std::mutex> lock(this->menuMutex);
    bool temp = this->visible;
    this->visible = visible;
    for (auto clickable : this->childrenClickables) {
        clickable->setVisible(visible);
    }
    if (this->visible) {
        // TODO: create a timeout that will hide the menu after a certain amount of time. will need to adjust clickable area so that it catches clicks
        // for the entire menu area to reset the timer.
    }
    return temp;
}

bool Menu::setIsClickable(bool isClickable)
{
    bool temp = this->isClickable;
    this->isClickable = isClickable;
    return temp;
}

bool Menu::setChildrenClickables_isClickable(bool isClickable)
{
    if (this->childrenClickables.size() == 0)
        return false;
    for (auto clickable : this->childrenClickables) {
        clickable->setIsClickable(isClickable);
    }
    return true;
}

/**
 * @brief Get the visibility of the menu
 *
 * @return bool the visibility of the menu
 */
bool Menu::getVisible()
{
    std::unique_lock<std::mutex> lock(this->menuMutex);
    return this->visible;
}

/**
 * @brief Set the action to take when the menu is clicked
 *
 * @param action the action to take
 */
void Menu::setOnClick(std::function<unsigned int(void*)> action)
{
    this->action = action;
}

/**
 * @brief Set the action to take when the menu is right clicked
 *
 * @param action the action to take
 */
void Menu::setOnRightClick(std::function<unsigned int(void*)> action)
{
    this->rightAction = action;
}

/**
 * @brief Get the objects in the menu
 *
 * @return std::vector<MeshObject*> the objects in the menu
 */
std::vector<MeshObject*> Menu::getObjects()
{
    // create a vector of objects
    std::vector<MeshObject*> objects;
    // add all the objects to the vector

    // return the vector
    return objects;
}

/**
 * @brief Check if the menu is ready
 *
 * @return true if the menu is ready
 * @return false if the menu is not ready
 */
bool Menu::isReady()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->ready;
}

/**
 * @brief Draw the menu. Should only be called from the renderer thread.
 *
 */
void Menu::draw()
{
    if (!this->visible)
        return;
    for (auto object : this->objects) {
        object->draw();
    }
    this->stencil->enable();
    for (auto clickable : this->childrenClickables) {
        clickable->draw();
    }
    this->stencil->disable();
}

/**
 * @brief Get the clickable areas in the menu
 *
 * @return std::vector<ClickableArea*> the clickable areas in the menu
 */
std::vector<ClickableArea*> Menu::getClickableAreas()
{
    std::vector<ClickableArea*> areas;
    areas.push_back(&this->clickArea);
    for (size_t i = 0; i < this->childrenClickables.size(); i++) {
        areas.push_back(this->childrenClickables.at(i)->getClickableArea());
    }
    return areas;
}

/**
 * @brief Get the clickable area of the menu
 *
 * @return ClickableArea*
 */
ClickableArea* Menu::getClickableArea()
{
    return &this->clickArea;
}

bool Menu::getIsClickable()
{
    if (this->isMainMenu)
        return this->isClickable;
    return this->isClickable && this->visible;
}

MenuEntry* Menu::getMenuEntryByIndex(unsigned int index)
{
    for (auto entry : Menu::allMenuEntries_vec) {
        if (entry->getMenuEntryIndex() == index)
            return entry;
    }
    return nullptr;
}

std::vector<MenuEntry*> Menu::getMenuEntriesByGroupID(int groupID)
{
    std::vector<MenuEntry*> entries;
    for (auto entry : Menu::allMenuEntries_vec) {
        if (entry->getGroupID() == groupID)
            entries.push_back(entry);
    }
    return entries;
}

//////////////////////////////////////////////////////////////////////////
float MenuBox::index = 0;

/**
 * @brief Construct a new Menu Box:: Menu Box object
 *
 * @param logger a CubeLog object
 * @param position the position of the center box
 * @param size the size of the box
 */
MenuBox::MenuBox(glm::vec2 position, glm::vec2 size, Shader* shader)
{
    MenuBox::index += 0.000001;
    this->position = position;
    this->size = size;
    this->visible = false;
    // radius is the 1/5 of the smaller of the two sides
    // float radius = size.x < size.y ? size.x/5 : size.y/5;
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = position.x - size.x / 2;
    float yStart = position.y - size.y / 2;
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, Z_DISTANCE + MenuBox::index }, { size.x - diameter, size.y - diameter }, 0.0, 0.0)); // main box

    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, Z_DISTANCE + MenuBox::index }, { radius, size.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(shader, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + MenuBox::index }, { radius, size.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, Z_DISTANCE + MenuBox::index }, { size.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + MenuBox::index }, { size.x - diameter, radius }, 0.0, 0.0)); // bottom

    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + MenuBox::index }, 0, 90, { 0.f, 0.f, 0.f })); // top right
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + MenuBox::index }, 90, 180, { 0.f, 0.f, 0.f })); // top left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + MenuBox::index }, 180, 270, { 0.f, 0.f, 0.f })); // bottom left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + MenuBox::index }, 270, 360, { 0.f, 0.f, 0.f })); // bottom right

    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size.y, Z_DISTANCE + 0.001 + MenuBox::index }, { xStart + size.x - radius, yStart + size.y, Z_DISTANCE + 0.001 + MenuBox::index })); // top
    this->objects.push_back(new M_Line(shader, { xStart + size.x, yStart + radius, Z_DISTANCE + 0.001 + MenuBox::index }, { xStart + size.x, yStart + size.y - radius, Z_DISTANCE + 0.001 + MenuBox::index })); // right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + MenuBox::index }, { xStart + size.x - radius, yStart, Z_DISTANCE + 0.001 + MenuBox::index })); // bottom
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + MenuBox::index }, { xStart, yStart + size.y - radius, Z_DISTANCE + 0.001 + MenuBox::index })); // left

    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + MenuBox::index })); // top right
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + 0.001 + MenuBox::index })); // bottom right
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + MenuBox::index })); // bottom left
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + MenuBox::index })); // top left

    CubeLog::info("MenuBox created of size: " + std::to_string(size.x) + "x" + std::to_string(size.y) + " at position: " + std::to_string(position.x) + "x" + std::to_string(position.y));
}

/**
 * @brief Destroy the Menu Box:: Menu Box object
 *
 */
MenuBox::~MenuBox()
{
    for (auto object : this->objects) {
        delete object;
    }
    // CubeLog::info("MenuBox destroyed");
}

/**
 * @brief Set the position of the menu box
 *
 * @param position the position of the menu box
 */
void MenuBox::setPosition(glm::vec2 position)
{
    this->position = position;
    // TODO: update the position of all the objects
}

/**
 * @brief Set the size of the menu box
 *
 * @param size the size of the menu box
 */
void MenuBox::setSize(glm::vec2 size)
{
    this->size = size;
    // TODO: update the size and positions of all the objects
}

/**
 * @brief Draw the menu box and all its objects
 *
 */
void MenuBox::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
}

/**
 * @brief Set the visibility of the menu box
 *
 * @param visible the visibility of the menu box
 * @return bool the previous visibility
 */
bool MenuBox::setVisible(bool visible)
{
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

/**
 * @brief Get the visibility of the menu box
 *
 * @return bool the visibility of the menu box
 */
bool MenuBox::getVisible()
{
    return this->visible;
}

//////////////////////////////////////////////////////////////////////////

unsigned int MenuEntry::menuEntryCount = 0;

/**
 * @brief Construct a new Menu Entry:: Menu Entry object
 *
 * @param logger a CubeLog object
 * @param text the text to display
 */
MenuEntry::MenuEntry(Shader* t_shader, Shader* m_shader, const std::string& text, glm::vec2 position, float size, float visibleWidth, EntryType type, std::function<unsigned int(void*)> statusAction, void* statusActionArg)
{
    this->menuEntryIndex = MenuEntry::menuEntryCount++;
    this->textShader = t_shader;
    this->meshShader = m_shader;
    this->type = type;
    this->text = text;
    this->visible = true;
    this->statusReturnData = statusAction(statusActionArg);
    this->statusActionArg = statusActionArg;

    this->textObject = new M_Text(
        textShader,
        text,
        size,
        { 1.f, 1.f, 1.f },
        position);
    this->scrollObjects.push_back(this->textObject);
    this->allObjects.push_back(this->textObject);
    this->scrollObjects.at(0)->capturePosition();
    this->size.x = this->scrollObjects.at(0)->getWidth();
    this->size.y = size;
    this->position = position;
    float clickY = mapRange(float(position.y), SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y, SCREEN_PX_MAX_Y, SCREEN_PX_MIN_Y);
    this->clickArea = ClickableArea(position.x, position.x + visibleWidth, clickY - this->size.y - 3, clickY + 7, this);
    this->originalPosition = { this->clickArea.xMin, this->clickArea.yMin, this->clickArea.xMax, this->clickArea.yMax };
    this->setVisibleWidth(visibleWidth);
    switch (this->type) {
    case EntryType::MENUENTRY_TYPE_ACTION: {
        break;
    }
    case EntryType::MENUENTRY_TYPE_SUBMENU: {
        break;
    }
    case EntryType::MENUENTRY_TYPE_RADIOBUTTON_GROUP: {
        CubeLog::moreInfo("Creating radio button");
        float posX = this->position.x + (this->visibleWidth - size);
        float posY = this->position.y + size;
        auto tempRB = new M_RadioButtonTexture(textShader, size + 10, 20, { 1.f, 1.f, 1.f }, { posX, posY });
        tempRB->setSelected(this->statusReturnData == 1);
        tempRB->capturePosition();
        tempRB->setVisibility(true);
        CubeLog::moreInfo("Radius button getWidth(): " + std::to_string(tempRB->getWidth()));
        this->setVisibleWidth(this->visibleWidth - tempRB->getWidth());
        this->xFixedObjects.push_back(tempRB);
        this->allObjects.push_back(tempRB);
        break;
    }
    case EntryType::MENUENTRY_TYPE_TOGGLE: {
        CubeLog::moreInfo("Creating toggle button");
        float posX = this->position.x + (this->visibleWidth - (size * 2.f - 10.f));
        float posY = this->position.y + size;
        auto tempTB = new M_ToggleTexture(textShader, size * 2.f + 10.f, size + 10, 20, { 1.f, 1.f, 1.f }, { posX, posY });
        tempTB->setSelected(this->statusReturnData == 1);
        tempTB->capturePosition();
        tempTB->setVisibility(true);
        CubeLog::moreInfo("Toggle button getWidth(): " + std::to_string(tempTB->getWidth()));
        this->setVisibleWidth(this->visibleWidth - tempTB->getWidth());
        this->xFixedObjects.push_back(tempTB);
        this->allObjects.push_back(tempTB);
        break;
    }
    case EntryType::MENUENTRY_TYPE_SLIDER: {
        CubeLog::moreInfo("Creating slider");

        break;
    }
        // TODO: add the rest of the MENU_ENTRY_TYPEs
    }
    // this->textStencil = new MenuStencil({ position.x, position.y - 2 }, { this->visibleWidth, size + 4 });
    CubeLog::info("MenuEntry created with text: " + text + " with click area: " + std::to_string(this->clickArea.xMin) + "x" + std::to_string(this->clickArea.yMin) + " to " + std::to_string(this->clickArea.xMax) + "x" + std::to_string(this->clickArea.yMax));
}

void MenuEntry::setEntryText(const std::string& text) 
{ 
    this->text = text; 
    this->textObject->setText(text);
    this->scrollObjects.at(0)->capturePosition();
    this->size.x = this->scrollObjects.at(0)->getWidth();
    this->clickArea.xMax = this->clickArea.xMin + this->size.x;
    this->setVisibleWidth(this->visibleWidth);
}

/**
 * @brief Destroy the Menu Entry:: Menu Entry object
 *
 */
MenuEntry::~MenuEntry()
{
    for (auto object : this->allObjects) {
        delete object;
    }
    // delete this->textStencil;
    CubeLog::info("MenuEntry destroyed");
}

/**
 * @brief Handle the click event. Calls the click action then calls the status action if it exists.
 *
 * @param data the data to pass to the action
 */
void MenuEntry::onClick(void* data)
{
    if (!this->isClickable)
        return;
    CubeLog::info("MenuEntry clicked");
    if (this->actions.size() == 0)
        return;
    if (this->actions.size() == 1) {
        this->clickReturnData = this->actions.at(0)(data) & 0x00ff;
    } else if (this->actions.size() == 2) {
        this->clickReturnData = this->actions.at(0)(data) & 0x00ff;
        this->statusReturnData = this->actions.at(1)(this->statusActionArg);
    }
}

void MenuEntry::onRelease(void* data)
{
    // CubeLog::debugSilly("MenuEntry released");
    this->textObject->setColor({ 1.f, 1.f, 1.f });
}

void MenuEntry::onMouseDown(void* data)
{
    CubeLog::debugSilly("MenuEntry mouse down");
    this->textObject->setColor(MENU_ENTRY_DIM_COLOR);
}

void MenuEntry::onRightClick(void* data)
{
    CubeLog::info("MenuEntry right clicked");
    if (this->rightAction != nullptr && this->visible) {
        this->clickReturnData = this->rightAction(data) & 0xFF00;
    }
}

bool MenuEntry::setVisible(bool visible)
{
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool MenuEntry::getVisible()
{
    return this->visible;
}

bool MenuEntry::setIsClickable(bool isClickable)
{
    bool temp = this->isClickable;
    this->isClickable = isClickable;
    return temp;
}

bool MenuEntry::getIsClickable()
{
    return this->isClickable && this->visible;
}

void MenuEntry::setOnClick(std::function<unsigned int(void*)> action)
{
    CubeLog::info("Setting onClick action for MenuEntry with text: " + this->text);
    if (this->actions.size() == 0)
        this->actions.push_back(action);
    else
        this->actions.at(0) = action;
}

void MenuEntry::setOnRightClick(std::function<unsigned int(void*)> action)
{
    CubeLog::info("Setting onRightClick action for MenuEntry with text: " + this->text);
    this->rightAction = action;
}

/**
 * @brief Set the status action. This action is called when the status of the entry is updated. This must be called after the action is set.
 *
 * @param action
 */
void MenuEntry::setStatusAction(std::function<unsigned int(void*)> action)
{
    if (this->actions.size() == 0)
        return;
    if (this->actions.size() == 1)
        this->actions.push_back(action);
    else
        this->actions.at(1) = action;
}

std::vector<MeshObject*> MenuEntry::getObjects()
{
    return this->allObjects;
}

void MenuEntry::draw()
{
    if (!this->visible) {
        return;
    }

    switch (this->type) {
    case EntryType::MENUENTRY_TYPE_ACTION:
    case EntryType::MENUENTRY_TYPE_SUBMENU: {
        break;
    }
    case EntryType::MENUENTRY_TYPE_TOGGLE:
    case EntryType::MENUENTRY_TYPE_RADIOBUTTON_GROUP: {
        if (this->statusReturnData == 1) {
            ((M_RadioButtonTexture*)this->xFixedObjects.at(0))->setSelected(true);
        } else {
            ((M_RadioButtonTexture*)this->xFixedObjects.at(0))->setSelected(false);
        }
        break;
    }
    case EntryType::MENUENTRY_TYPE_SLIDER: {
        // TODO: add slider drawing
        break;
    }
    }

    if (this->size.x != this->scrollObjects.at(0)->getWidth()) {
        this->size.x = this->scrollObjects.at(0)->getWidth();
        this->clickArea.xMax = this->clickArea.xMin + this->size.x;
        this->setVisibleWidth(this->visibleWidth);
    }
    if (this->scrolling == ScrollingDirection::SCROLL_LEFT && this->scrollWait++ > 60) {
        this->scrollPositionLeft += MENU_ITEM_SCROLL_LEFT_SPEED;
        for (auto object : this->scrollObjects) {
            object->translate({ -MENU_ITEM_SCROLL_LEFT_SPEED, 0.f, 0.f });
        }
        if (this->scrollPositionLeft >= this->size.x - this->visibleWidth) {
            for (auto object : this->scrollObjects) {
                this->scrolling = ScrollingDirection::SCROLL_RIGHT;
            }
            this->scrollWait = 0;
        }
    }
    if (this->scrolling == ScrollingDirection::SCROLL_RIGHT && this->scrollWait++ > 60) {
        this->scrollPositionRight += MENU_ITEM_SCROLL_RIGHT_SPEED;
        const float amount = this->scrollPositionRight < this->scrollPositionLeft ? MENU_ITEM_SCROLL_RIGHT_SPEED : MENU_ITEM_SCROLL_RIGHT_SPEED - this->scrollPositionRight + this->scrollPositionLeft;
        for (auto object : this->scrollObjects) {
            object->translate({ amount, 0.f, 0.f });
        }
        if (this->scrollPositionRight > this->scrollPositionLeft) {
            for (auto object : this->scrollObjects) {
                this->scrolling = ScrollingDirection::SCROLL_LEFT;
            }
            this->scrollPositionRight = 0;
            this->scrollPositionLeft = 0;
            this->scrollWait = 0;
        }
    }
    for (auto object : this->xFixedObjects) {
        object->draw();
    }
    for (auto object : this->scrollObjects) {
        object->draw();
    }
    for (auto object : this->yFixedObjects) {
        object->draw();
    }
    for (auto object : this->fixedObjects) {
        object->draw();
    }
}

void MenuEntry::translate(glm::vec2 translation)
{
    this->position += translation;
    this->clickArea.xMin += translation.x;
    this->clickArea.xMax += translation.x;
    this->clickArea.yMin += translation.y;
    this->clickArea.yMax += translation.y;
    for (auto object : this->scrollObjects) {
        object->translate({ translation.x, translation.y, 0 });
    }
}

void MenuEntry::setVisibleWidth(float width)
{
    this->visibleWidth = width;
    if (width < this->size.x) {
        this->scrolling = ScrollingDirection::SCROLL_LEFT;
    } else {
        this->scrolling = ScrollingDirection::NOT_SCROLLING;
    }
}

void MenuEntry::resetScroll()
{
    if (this->scrolling == ScrollingDirection::NOT_SCROLLING)
        return;
    this->scrolling = ScrollingDirection::SCROLL_LEFT;
    this->scrollPositionLeft = 0;
    this->scrollPositionRight = 0;
    this->scrollWait = 0;
}

ClickableArea* MenuEntry::getClickableArea()
{
    return &this->clickArea;
}

void MenuEntry::setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax)
{
    long xChange = this->clickArea.xMin - xMin;
    long yChange = this->clickArea.yMin - yMin;
    float xRel = mapRange(float(xChange), -SCREEN_PX_MAX_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X);
    float yRel = mapRange(float(yChange), -SCREEN_PX_MAX_Y, SCREEN_PX_MAX_Y, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y);
    for (auto object : this->xFixedObjects) {
        object->translate({ xRel, yRel, 0 });
    }
    this->clickArea.xMin = xMin;
    this->clickArea.xMax = xMax;
    this->clickArea.yMin = yMin;
    this->clickArea.yMax = yMax;
}

void MenuEntry::capturePosition()
{
    for (auto object : this->allObjects) {
        object->capturePosition();
    }
    this->originalPosition = { this->clickArea.xMin, this->clickArea.yMin, this->clickArea.xMax, this->clickArea.yMax };
}

void MenuEntry::restorePosition()
{
    for (auto object : this->allObjects) {
        object->restorePosition();
    }
    this->clickArea.xMin = this->originalPosition.x;
    this->clickArea.yMin = this->originalPosition.y;
    this->clickArea.xMax = this->originalPosition.z;
    this->clickArea.yMax = this->originalPosition.w;
}



//////////////////////////////////////////////////////////////////////////

MenuStencil::MenuStencil(glm::vec2 position, glm::vec2 size, Shader* shader)
{
    CubeLog::info("Creating MenuStencil at position: " + std::to_string(position.x) + "x" + std::to_string(position.y) + " of size: " + std::to_string(size.x) + "x" + std::to_string(size.y));
    this->position = position;
    this->size = size;
    this->shader = shader;
    // create vertices for the stencil
    this->vertices.push_back({ position.x, position.y });
    this->vertices.push_back({ position.x + size.x, position.y });
    this->vertices.push_back({ position.x + size.x, position.y + size.y });
    this->vertices.push_back({ position.x, position.y + size.y });

    this->projectionMatrix = glm::ortho(0.0f, 720.0f, 0.0f, 720.0f);
    this->modelMatrix = glm::mat4(1.0f);
    this->viewMatrix = glm::mat4(1.0f);

    // create the VAO, VBO and EBO
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec2), &this->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CubeLog::info("MenuStencil created");
}

MenuStencil::~MenuStencil()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    glDeleteBuffers(1, &this->EBO);
    CubeLog::info("MenuStencil destroyed");
}

void MenuStencil::setPosition(glm::vec2 position)
{
    this->position = position;
}

void MenuStencil::setSize(glm::vec2 size)
{
    this->size = size;
}

void MenuStencil::draw()
{
    // do nothing. this function is not used in this class.
    return;
}

bool MenuStencil::setVisible(bool visible)
{
    return true;
}

bool MenuStencil::getVisible()
{
    return true;
}

void MenuStencil::enable()
{
    if (false) { // set to true for debugging
        // In debug mode, draw the mask with a solid color to verify its size and position
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        this->shader->use();
        this->shader->setMat4("projection", projectionMatrix);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        return;
    }
    glEnable(GL_STENCIL_TEST);

    // Clear the stencil buffer and set all to 0
    // glClearStencil(0);
    // glClear(GL_STENCIL_BUFFER_BIT);

    // Configure stencil operations to mark the stencil buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    std::bitset<8> bitset;
    bitset.set(this->stencilIndex);
    if (this->stencilIndex == 0) {
        glStencilFunc(GL_ALWAYS, 1, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    } else {
        glStencilFunc(GL_EQUAL, 1, 0xff);
        glStencilOp(GL_ZERO, GL_ZERO, GL_KEEP);
    }
    glStencilMask(0xFF);
    // Draw the mask
    this->shader->use();
    this->shader->setMat4("projection", projectionMatrix);
    this->shader->setMat4("model", modelMatrix);
    this->shader->setMat4("view", viewMatrix);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    // Configure stencil function to only pass where stencil buffer is set to 1
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);
}

void MenuStencil::disable()
{
    glBindVertexBuffer(0, 0, 0, 0);
    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glDisable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
}

void MenuStencil::translate(glm::vec2 translation)
{
    this->modelMatrix = glm::translate(this->modelMatrix, glm::vec3(translation, 0.0f));
}

//////////////////////////////////////////////////////////////////////////

MenuHorizontalRule::MenuHorizontalRule(glm::vec2 position, float size, Shader* shader)
{
    this->position = position;
    this->size = size;
    this->shader = shader;
    this->visible = false;
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = this;
    this->clickArea.xMin = position.x;
    this->clickArea.xMax = position.x + size;
    this->clickArea.yMin = position.y;
    this->clickArea.yMax = position.y;
    // convert the position to screen relative coordinates
    float posX = mapRange(float(position.x), SCREEN_PX_MIN_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X);
    float posY = mapRange(float(position.y), SCREEN_PX_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y);
    glm::vec2 pos = { posX, posY };
    // convert size to screen relative size
    float sizeX = mapRange(size, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH);
    this->objects.push_back(new M_Line(shader, { pos, Z_DISTANCE + MenuBox::index + 0.0001f }, { pos.x + sizeX, pos.y, Z_DISTANCE + MenuBox::index + 0.0001f }));
    MenuBox::index += 0.000001f;
    this->objects.at(0)->type = "MenuHorizontalRule";
    // this->stencil = new MenuStencil({ position.x, position.y }, { size, 4 });
    CubeLog::info("MenuHorizontalRule created at position: " + std::to_string(position.x) + "x" + std::to_string(position.y) + " of size: " + std::to_string(size));
    CubeLog::info("MenuHorizontalRule created with screen relative position: " + std::to_string(pos.x) + "x" + std::to_string(pos.y) + " of size: " + std::to_string(sizeX));
}

MenuHorizontalRule::~MenuHorizontalRule()
{
    for (auto object : this->objects) {
        delete object;
    }
    CubeLog::info("MenuHorizontalRule destroyed");
}

void MenuHorizontalRule::setPosition(glm::vec2 position)
{
    float xChange = position.x - this->clickArea.xMin;
    float yChange = position.y - this->clickArea.yMin;
    this->position = position;
}

void MenuHorizontalRule::setSize(glm::vec2 size)
{
    this->size = size.x;
}

void MenuHorizontalRule::setSize(float size)
{
    this->size = size;
}

void MenuHorizontalRule::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
}

bool MenuHorizontalRule::setVisible(bool visible)
{
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool MenuHorizontalRule::getVisible()
{
    return this->visible;
}

void MenuHorizontalRule::onClick(void* data)
{
    CubeLog::info("MenuHorizontalRule clicked");
}

void MenuHorizontalRule::onRelease(void* data)
{
    // CubeLog::info("MenuHorizontalRule released");
    // noop
}

void MenuHorizontalRule::onMouseDown(void* data)
{
    CubeLog::info("MenuHorizontalRule mouse down");
}

void MenuHorizontalRule::onRightClick(void* data)
{
    CubeLog::info("MenuHorizontalRule right clicked");
}

std::vector<MeshObject*> MenuHorizontalRule::getObjects()
{
    return this->objects;
}

void MenuHorizontalRule::setOnClick(std::function<unsigned int(void*)> action)
{
}

void MenuHorizontalRule::setOnRightClick(std::function<unsigned int(void*)> action)
{
}

ClickableArea* MenuHorizontalRule::getClickableArea()
{
    return &this->clickArea;
}

void MenuHorizontalRule::capturePosition()
{
    for (auto object : this->objects) {
        object->capturePosition();
    }
}

void MenuHorizontalRule::restorePosition()
{
    for (auto object : this->objects) {
        object->restorePosition();
    }
}

//////////////////////////////////////////////////////////////////////////

float screenPxToScreenRelative(float screenPx)
{
    return mapRange(screenPx, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X);
}

float screenRelativeToScreenPx(float screenRelative)
{
    return mapRange(screenRelative, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
}

float screenPxToScreenRelativeWidth(float screenPx)
{
    return mapRange(screenPx, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH);
}

float screenRelativeToScreenPxWidth(float screenRelative)
{
    return mapRange(screenRelative, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
}
} // namespace MENUS
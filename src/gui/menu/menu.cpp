#include "menu.h"

float screenRelativeToScreenPx(float screenRelative);
float screenPxToScreenRelative(float screenPx);
float screenPxToScreenRelativeWidth(float screenPx);
float screenRelativeToScreenPxWidth(float screenRelative);
bool Menu::mainMenuSet = false;

int stencilIndex = 1;

/**
 * @brief Construct a new Menu object
 * 
 * @param renderer - the renderer object
 * 
 * @return Menu* - the new Menu object
 * 
 * @note This constructor will create a Menu object without a CountingLatch. "this->latch" will be set to nullptr.
 */
Menu::Menu(Renderer* renderer) : Menu(renderer, *this->latch){}

Menu::Menu(Renderer* renderer, CountingLatch& latch): Menu(renderer, latch, 0, 720, 0, 720)
{
    CubeLog::info("Creating Menu class object");
    this->latch = &latch;   
}

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
    CubeLog::info("Menu destroyed");
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
void Menu::addMenuEntry(std::string text, MenuEntry::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> statusAction, void* statusActionData)
{
    // TODO: add the ability to have an entry be fixed to the top of the menu. This will need a stencil so that other entries can be scrolled under it.
    // TODO: add icon support
    // TODO: add checkbox support
    // TODO: add radio button support
    // TODO: add slider support
    CubeLog::debug("Adding MenuEntry with text: " + text);
    float startY = (((menuItemTextSize * 1.2) + MENU_ITEM_PADDING_PX) * this->childrenClickables.size()) + MENU_TOP_PADDING_PX;
    float textX = mapRange(MENU_POSITION_SCREEN_RELATIVE_X_LEFT, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X) + (STENCIL_INSET_PX * 2);
    float textY = mapRange(MENU_POSITION_SCREEN_RELATIVE_Y_TOP, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y) - startY - (STENCIL_INSET_PX * 2) - this->menuItemTextSize;
    float menuWidthPx = mapRange(MENU_WIDTH_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
    float menuWidthAdjusted = menuWidthPx - (STENCIL_INSET_PX * 2) - (MENU_ITEM_PADDING_PX * 2);
    auto entry = new MenuEntry(text, new Shader("shaders/text.vs", "shaders/text.fs"), this->renderer->getShader(), { textX, textY }, menuItemTextSize, menuWidthAdjusted, type, statusAction, statusActionData);
    entry->setStatusAction(statusAction);
    entry->getClickableArea()->yMin -= (MENU_ITEM_PADDING_PX);
    entry->getClickableArea()->yMax += (MENU_ITEM_PADDING_PX);
    entry->setVisible(true);
    entry->setOnClick(action);
    for (auto object : entry->getObjects()) {
        object->capturePosition();
    }
    this->childrenClickables.push_back(entry);
    if (this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax > this->maxScrollY)
        this->maxScrollY = this->getClickableAreas().at(this->getClickableAreas().size() - 1)->yMax;
    
    CubeLog::debug("MenuEntry added with text: " + text + " and clickable area: " + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->xMin) + "x" + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->yMin) + " to " + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->xMax) + "x" + std::to_string(this->childrenClickables.at(this->childrenClickables.size() - 1)->getClickableArea()->yMax));
}

/**
 * @brief Add a menu entry to the menu with a right click action
 *
 * @param text The text to display
 * @param action The action to take when the menu entry is clicked
 * @param rightAction The action to take when the menu entry is right clicked
 */
void Menu::addMenuEntry(std::string text, MenuEntry::EntryType type, std::function<unsigned int(void*)> action, std::function<unsigned int(void*)> rightAction, std::function<unsigned int(void*)> statusAction, void* statusActionData)
{
    this->addMenuEntry(text, type, action, statusAction, statusActionData);
    this->childrenClickables.at(this->childrenClickables.size() - 1)->setOnRightClick(rightAction);
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
    this->childrenClickables.push_back(new MenuHorizontalRule({ startX, startY }, 350, this->renderer->getShader()));
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
void Menu::setMenuName(std::string name)
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
    Menu::mainMenuSet = true;
}

/**
 * @brief Setup the menu. This must be called by the renderer thread. 
 *
 */
void Menu::setup()
{
    this->objects.push_back(new MenuBox({ MENU_POSITION_SCREEN_RELATIVE_X_CENTER, MENU_POSITION_SCREEN_RELATIVE_Y_CENTER }, { MENU_WIDTH_SCREEN_RELATIVE, MENU_HEIGHT_SCREEN_RELATIVE }, this->renderer->getShader()));
    this->objects.at(0)->setVisible(true);
    Shader* stencilShader = new Shader("shaders/menuStencil.vs", "shaders/menuStencil.fs");
    float stencilX_start_temp = MENU_POSITION_SCREEN_RELATIVE_X_CENTER - MENU_WIDTH_SCREEN_RELATIVE / 2;
    float stencilY_start_temp = MENU_POSITION_SCREEN_RELATIVE_Y_CENTER - MENU_HEIGHT_SCREEN_RELATIVE / 2;
    float stencilX_start = mapRange(stencilX_start_temp, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X);
    float stencilY_start = mapRange(stencilY_start_temp, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y);
    float stencilWidth = mapRange(MENU_WIDTH_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X) - (STENCIL_INSET_PX * 2);
    float stencilHeight = mapRange(MENU_HEIGHT_SCREEN_RELATIVE, SCREEN_RELATIVE_MIN_HEIGHT, SCREEN_RELATIVE_MAX_HEIGHT, SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y) - (STENCIL_INSET_PX * 2);
    this->stencil = new MenuStencil({ stencilX_start + STENCIL_INSET_PX, stencilY_start + STENCIL_INSET_PX }, { stencilWidth, stencilHeight }, stencilShader);
    /////// TESTING //////////// 
    // for (auto font : GlobalSettings::fontPaths) {
    //     FT_Library ft;
    //     FT_Face face;
    //     FT_Init_FreeType(&ft);
    //     if (FT_New_Face(ft, font.c_str(), 0, &face)) {
    //         CubeLog::error("Could not load font: " + font);
    //     } else {
    //         CubeLog::info("Loaded font: " + std::string(face->family_name) + ":" + std::string(face->style_name));
    //         std::string fontName = std::string(face->family_name) + ":" + std::string(face->style_name);
    //         this->addMenuEntry(fontName, [&, fontName, font](void* data) { // copy fontName and font
    //             CubeLog::info("Changing to font: " + fontName);
    //             if (GlobalSettings::setSetting("selectedFontPath", font)) {
    //                 CubeLog::info("SelectedFontPath: " + GlobalSettings::selectedFontPath);
    //             }
    //         });
    //     }
    //     FT_Done_Face(face);
    //     FT_Done_FreeType(ft);
    // }
    ////////// END TESTING //////////

    // for (auto clickArea : this->getClickableAreas()) {
    //     if (clickArea->yMax > this->maxScrollY)
    //         this->maxScrollY = clickArea->yMax;
    // }
    std::lock_guard<std::mutex> lock(this->mutex);
    this->ready = true;
    if(this->latch != nullptr) this->latch->count_down();
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
    for(auto clickable : this->childrenClickables){
        clickable->setVisible(visible);
    }
    if (this->visible) {
        // TODO: create a timeout that will hide the menu after a certain amount of time. will need to adjust clickable area so that it catches clicks
        // for the entire menu area to reset the timer. Will also need to fix the issue where clicks don't propagate to all areas that are possible,
    }
    return temp;
}

void Menu::setIsClickable(bool isClickable)
{
    this->isClickable = isClickable;
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
 * @brief Draw the menu
 *
 */
void Menu::draw()
{
    if (!this->visible) return;
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
    return this->isClickable;
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
    this->index += 0.001;
    this->position = position;
    this->size = size;
    this->visible = false;
    // radius is the 1/5 of the smaller of the two sides
    // float radius = size.x < size.y ? size.x/5 : size.y/5;
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = position.x - size.x / 2;
    float yStart = position.y - size.y / 2;
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, { size.x - diameter, size.y - diameter }, 0.0, 0.0)); // main box

    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, Z_DISTANCE + this->index }, { radius, size.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(shader, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + this->index }, { radius, size.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, Z_DISTANCE + this->index }, { size.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + this->index }, { size.x - diameter, radius }, 0.0, 0.0)); // bottom

    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + this->index }, 0, 90, {0.f,0.f,0.f})); // top right
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + this->index }, 90, 180, {0.f,0.f,0.f})); // top left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, 180, 270, {0.f,0.f,0.f})); // bottom left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + this->index }, 270, 360, {0.f,0.f,0.f})); // bottom right

    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size.y, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x - radius, yStart + size.y, Z_DISTANCE + 0.001 + this->index })); // top
    this->objects.push_back(new M_Line(shader, { xStart + size.x, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + this->index }, { xStart + size.x - radius, yStart, Z_DISTANCE + 0.001 + this->index })); // bottom
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // left

    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size.x - radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // top right
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size.x - radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom right
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom left
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size.y - radius, Z_DISTANCE + 0.001 + this->index })); // top left

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
    CubeLog::info("MenuBox destroyed");
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

/**
 * @brief Construct a new Menu Entry:: Menu Entry object
 *
 * @param logger a CubeLog object
 * @param text the text to display
 */
MenuEntry::MenuEntry(std::string text, Shader* textShader, Shader* meshShader, glm::vec2 position, float size, float visibleWidth, EntryType type, std::function<unsigned int(void*)> statusAction, void* statusActionArg)
{
    
    this->type = type;
    this->text = text;
    this->visible = true;
    this->textShader = textShader;
    
    this->statusReturnData = statusAction(statusActionArg);
    this->statusActionArg = statusActionArg;

    auto temp = new M_Text(textShader, text, size, {1.f,1.f,1.f,}, position);
    this->scrollObjects.push_back(temp);
    this->allObjects.push_back(temp);
    this->scrollObjects.at(0)->capturePosition();

    this->size.x = this->scrollObjects.at(0)->getWidth();
    this->size.y = size;
    this->position = position;
    float clickY = mapRange(float(position.y), SCREEN_PX_MIN_Y, SCREEN_PX_MAX_Y, SCREEN_PX_MAX_Y, SCREEN_PX_MIN_Y);
    this->clickArea = ClickableArea(position.x, position.x + this->size.x, clickY - this->size.y - 3, clickY + 7, this);
    this->originalPosition = { this->clickArea.xMin, this->clickArea.yMin, this->clickArea.xMax, this->clickArea.yMax };
    this->setVisibleWidth(visibleWidth);    
    switch(this->type){
        case EntryType::MENUENTRY_TYPE_ACTION:
        {
            // This is the default type
            // TODO: add some sort of feedback to the user when this type is clicked / tapped.
            break;
        }
        case EntryType::MENUENTRY_TYPE_SUBMENU:
        {
            break;
        }
        case EntryType::MENUENTRY_TYPE_CHECKBOX:
        {
            break;
        }
        case EntryType::MENUENTRY_TYPE_RADIOBUTTON:
        {
            this->size.x -= 5;
            float posX = screenPxToScreenRelative(position.x + size / 2);
            float posY = screenPxToScreenRelative(position.y + size / 2);
            float radius = mapRange(size, SCREEN_PX_MIN_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_WIDTH, SCREEN_RELATIVE_MAX_WIDTH / 2);
            auto tempRB = new M_RadioButton(meshShader, { posX, posY, Z_DISTANCE + 0.01}, radius, size, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f });
            float translateX = screenPxToScreenRelativeWidth(this->visibleWidth - (size));
            tempRB->translate({translateX, 0.f, 0.f});
            tempRB->capturePosition();
            tempRB->setVisibility(true);
            CubeLog::moreInfo("Radius button getWidth(): " + std::to_string(tempRB->getWidth()));
            this->setVisibleWidth(this->visibleWidth - tempRB->getWidth());
            this->xFixedObjects.push_back(tempRB);
            this->allObjects.push_back(tempRB);
            break;
        }
    }
    this->textStencil = new MenuStencil({ position.x, position.y - 2 }, { this->visibleWidth, size + 4 }, new Shader("shaders/menuStencil.vs", "shaders/menuStencil.fs"));
    CubeLog::info("MenuEntry created with text: " + text + " with click area: " + std::to_string(this->clickArea.xMin) + "x" + std::to_string(this->clickArea.yMin) + " to " + std::to_string(this->clickArea.xMax) + "x" + std::to_string(this->clickArea.yMax));
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
    delete this->textStencil;
    CubeLog::info("MenuEntry destroyed");
}

/**
 * @brief Handle the click event
 *
 * @param data the data to pass to the action
 */
void MenuEntry::onClick(void* data)
{
    CubeLog::info("MenuEntry clicked");
    if (this->action != nullptr && this->visible) {
        this->clickReturnData = this->action(data) & 0x00FF;
    }
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

void MenuEntry::setOnClick(std::function<unsigned int(void*)> action)
{
    CubeLog::info("Setting onClick action for MenuEntry with text: " + this->text);
    this->action = action;
}

void MenuEntry::setOnRightClick(std::function<unsigned int(void*)> action)
{
    CubeLog::info("Setting onRightClick action for MenuEntry with text: " + this->text);
    this->rightAction = action;
}

void MenuEntry::setStatusAction(std::function<unsigned int(void*)> action)
{
    this->statusAction = action;
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
    
    if (this->size.x != this->scrollObjects.at(0)->getWidth()) {
        this->size.x = this->scrollObjects.at(0)->getWidth();
        this->clickArea.xMax = this->clickArea.xMin + this->size.x;
        this->setVisibleWidth(this->visibleWidth);
    }
    if (this->scrolling == SCROLL_LEFT && this->scrollWait++ > 60) {
        this->scrollPositionLeft += MENU_ITEM_SCROLL_LEFT_SPEED;
        for (auto object : this->scrollObjects) {
            object->translate({ -MENU_ITEM_SCROLL_LEFT_SPEED, 0.f, 0.f });
        }
        if (this->scrollPositionLeft >= this->size.x - this->visibleWidth) {
            for (auto object : this->scrollObjects) {
                this->scrolling = SCROLL_RIGHT;
            }
            this->scrollWait = 0;
        }
    }
    if (this->scrolling == SCROLL_RIGHT && this->scrollWait++ > 60) {
        this->scrollPositionRight += MENU_ITEM_SCROLL_RIGHT_SPEED;
        const float amount = this->scrollPositionRight < this->scrollPositionLeft ? MENU_ITEM_SCROLL_RIGHT_SPEED : MENU_ITEM_SCROLL_RIGHT_SPEED - this->scrollPositionRight + this->scrollPositionLeft;
        for (auto object : this->scrollObjects) {
            object->translate({ amount, 0.f, 0.f });
        }
        if (this->scrollPositionRight > this->scrollPositionLeft) {
            for (auto object : this->scrollObjects) {
                this->scrolling = SCROLL_LEFT;
            }
            this->scrollPositionRight = 0;
            this->scrollPositionLeft = 0;
            this->scrollWait = 0;
        }
    }
    this->textStencil->enable();
    for (auto object : this->scrollObjects) {
        object->draw();
    }
    this->textStencil->disable();
    for(auto object : this->xFixedObjects){
        object->draw();
    }
    for(auto object : this->yFixedObjects){
        object->draw();
    }
    for(auto object : this->fixedObjects){
        object->draw();
    }
}

void MenuEntry::translate(glm::vec2 translation){
    this->position += translation;
    this->clickArea.xMin += translation.x;
    this->clickArea.xMax += translation.x;
    this->clickArea.yMin += translation.y;
    this->clickArea.yMax += translation.y;
    for(auto object : this->scrollObjects){
        object->translate({translation.x, translation.y, 0});
    }
    this->textStencil->translate({translation.x, translation.y});
}

void MenuEntry::setVisibleWidth(float width)
{
    this->visibleWidth = width;
    if (width < this->size.x) {
        this->scrolling = SCROLL_LEFT;
    } else {
        this->scrolling = NOT_SCROLLING;
    }
}

void MenuEntry::resetScroll()
{
    if(this->scrolling == NOT_SCROLLING)
        return;
    this->scrolling = SCROLL_LEFT;
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
    this->textStencil->translate({xChange, yChange});
    float xRel = mapRange(float(xChange), -SCREEN_PX_MAX_X, SCREEN_PX_MAX_X, SCREEN_RELATIVE_MIN_X, SCREEN_RELATIVE_MAX_X);
    float yRel = mapRange(float(yChange), -SCREEN_PX_MAX_Y, SCREEN_PX_MAX_Y, SCREEN_RELATIVE_MIN_Y, SCREEN_RELATIVE_MAX_Y);
    for(auto object : this->xFixedObjects){
        object->translate({xRel, yRel, 0});
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

bool MenuEntry::getIsClickable()
{
    return this->getVisible();
}

//////////////////////////////////////////////////////////////////////////

MenuStencil::MenuStencil(glm::vec2 position, glm::vec2 size, Shader* shader)
{
    CubeLog::info("Creating MenuStencil at position: " + std::to_string(position.x) + "x" + std::to_string(position.y) + " of size: " + std::to_string(size.x) + "x" + std::to_string(size.y));
    this->position = position;
    this->size = size;
    this->shader = shader;
    this->stencilIndex = stencilIndex++;
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
        shader->setMat4("projection", projectionMatrix);
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
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    
    // Configure stencil operations to mark the stencil buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    checkGLError("glStencilFunc_errorCheck_1");
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    // Draw the mask
    shader->use();
    shader->setMat4("projection", projectionMatrix);
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
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
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
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
    this->objects.push_back(new M_Line(shader, { pos, Z_DISTANCE + 0.01 }, { pos.x + sizeX, pos.y, Z_DISTANCE + 0.01 }));
    this->objects.at(0)->type = "MenuHorizontalRule";
    this->stencil = new MenuStencil({ position.x, position.y }, { size, 4 }, new Shader("shaders/menuStencil.vs", "shaders/menuStencil.fs"));
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
    this->stencil->translate({xChange, yChange});
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
    this->stencil->enable();
    for (auto object : this->objects) {
        object->draw();
    }
    this->stencil->disable();
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
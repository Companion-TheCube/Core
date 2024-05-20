#include "menu.h"

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05

/**
 * @brief Construct a new Menu:: Menu object
 * 
 * @param logger a CubeLog object
 */
Menu::Menu(CubeLog* logger, std::string filename, Shader* shader){
    this->logger = logger;
    this->filename = filename;
    this->shader = shader;
    this->visible = false;
    this->logger->log("Menu created", true);
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = this;
    this->clickArea.xMin = 0;
    this->clickArea.xMax = 720;
    this->clickArea.yMin = 0;
    this->clickArea.yMax = 720;
    this->setOnClick([&](void* data){
        this->setVisible(!this->getVisible());
    });
}

/**
 * @brief Destroy the Menu:: Menu object
 * 
 */
Menu::~Menu(){
    this->logger->log("Menu destroyed", true);
    for(auto object: this->objects){
        delete object;
    }
}

void Menu::addMenuEntry(std::string text, std::function<void(void*)> action){
    // TODO: fix this to dynamically adjust y position
    Shader* textShader = new Shader("shaders/text.vs", "shaders/text.fs", logger);
    float textX = mapRange(0.f, -1.f, 1.f, 0.f, 720.f);
    float textY = mapRange(0.f, 1.f, -1.f, 0.f, 720.f);
    this->childrenClickables.push_back(new MenuEntry(logger, text, textShader, {textX, textY}, menuItemTextSize));
    this->childrenClickables.at(this->childrenClickables.size()-1)->setVisible(true);
    this->childrenClickables.at(this->childrenClickables.size()-1)->setOnClick(action);
}

void Menu::addMenuEntry(std::string text, std::function<void(void*)> action, std::function<void(void*)> rightAction){
    // TODO: fix this to dynamically adjust y position
    float textX = mapRange(0.f, -1.f, 1.f, 0.f, 720.f);
    float textY = mapRange(0.f, 1.f, -1.f, 0.f, 720.f);
    this->childrenClickables.push_back(new MenuEntry(logger, text, textShader, {textX, textY}, menuItemTextSize));
    this->childrenClickables.at(this->childrenClickables.size()-1)->setVisible(true);
    this->childrenClickables.at(this->childrenClickables.size()-1)->setOnClick(action);
    this->childrenClickables.at(this->childrenClickables.size()-1)->setOnRightClick(rightAction);
}

void Menu::addHorizontalRule(){
    Shader* textShader = new Shader("shaders/text.vs", "shaders/text.fs", logger);
    float textX = mapRange(0.f, -1.f, 1.f, 0.f, 720.f);
    float textY = mapRange(0.f, 1.f, -1.f, 0.f, 720.f);
    // TODO: add a box that is the width of the screen and 1 pixel high
}

void Menu::setup(){
    this->loadObjects(filename);
    this->objects.push_back(new MenuBox(logger, {0.4, 0.0}, {1.2, 2.0}, shader));
    this->objects.at(0)->setVisible(true);

    this->textShader = new Shader("shaders/text.vs", "shaders/text.fs", logger);

    Shader* stencilShader = new Shader("shaders/menuStencil.vs", "shaders/menuStencil.fs", logger);
    float stencilX_start_temp = 0.4f - 1.2f/2;
    float stencilY_start_temp = 0.0f - 2.0f/2;
    float stencilX_start = mapRange(stencilX_start_temp, -1.f, 1.f, 0.f, 720.f);
    float stencilY_start = mapRange(stencilY_start_temp, 1.f, -1.f, 0.f, 720.f);
    float stencilWidth = mapRange(1.2f, -1.f, 1.f, 0.f, 720.f);
    float stencilHeight = mapRange(2.0f, 1.f, -1.f, 0.f, 720.f);
    this->stencil = new MenuStencil(logger, {0, 0}, {720, 720}, stencilShader);
    
    float textX = mapRange(0.f, -1.f, 1.f, 0.f, 720.f);
    float textY = mapRange(0.f, 1.f, -1.f, 0.f, 720.f);
    this->childrenClickables.push_back(new MenuEntry(logger, "Test", textShader, {textX, textY}, menuItemTextSize));
    this->childrenClickables.at(0)->setVisible(true);
    this->childrenClickables.push_back(new MenuEntry(logger, "Test2", textShader, {textX, textY+menuItemTextSize}, menuItemTextSize));
    this->childrenClickables.at(1)->setVisible(true);
    this->childrenClickables.push_back(new MenuEntry(logger, "Test3", textShader, {textX, textY+350}, menuItemTextSize));
    this->childrenClickables.at(2)->setVisible(true);
    std::lock_guard<std::mutex> lock(this->mutex);
    this->ready = true;
    this->logger->log("Menu setup done", true);
}

void Menu::onClick(void* data){
    // this->logger->log("Menu clicked", true);
    // if(this->action != nullptr && this->visible){
        this->action(data);
    // }
}

void Menu::onRightClick(void* data){
    this->logger->log("Menu right clicked", true);
    if(this->rightAction != nullptr && this->visible){
        this->rightAction(data);
    }
}

bool Menu::setVisible(bool visible){
    bool temp = this->visible;
    this->visible = visible;
    if(this->visible){
        this->clickArea.xMin = 595;
        this->clickArea.xMax = 720;
        this->clickArea.yMin = 0;
        this->clickArea.yMax = 125;
    }else{
        this->clickArea.xMin = 0;
        this->clickArea.xMax = 720;
        this->clickArea.yMin = 0;
        this->clickArea.yMax = 720;
    }
    return temp;
}

bool Menu::getVisible(){
    return this->visible;
}

void Menu::setOnClick(std::function<void(void*)> action){
    this->action = action;
}

void Menu::setOnRightClick(std::function<void(void*)> action){
    this->rightAction = action;
}

bool Menu::loadObjects(std::string filename){
    this->logger->log("Loading objects as defined in file: " + filename, true);
    // check to see if the file exists
    if(!std::filesystem::exists(filename)){
        this->logger->error("File does not exist: " + filename);
        return false;
    }
    // open the file
    std::ifstream file(filename);
    std::string line;
    while(std::getline(file, line)){
        std::istringstream iss(line);
        // TODO:
        // parse the file loading all the objects
    }
    return true;
}

std::vector<MeshObject*> Menu::getObjects(){
    // create a vector of objects
    std::vector<MeshObject*> objects;
    // add all the objects to the vector
    
    // return the vector
    return objects;
}

bool Menu::isReady(){
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->ready;
}

void Menu::draw(){
    if(!this->visible){
        return;
    }
    
    for(auto object: this->objects){
        object->draw();
    }
    this->stencil->enable();
    for(auto clickable: this->childrenClickables){
        clickable->draw();
    }
    this->stencil->disable();
}

std::vector<ClickableArea*> Menu::getClickableAreas(){
    std::vector<ClickableArea*> areas;
    areas.push_back(&this->clickArea);
    for(auto clickable: this->childrenClickables){
        areas.push_back(&clickable->clickArea);
    }
    return areas;
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
MenuBox::MenuBox(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader){
    this->index+=0.001;
    this->logger = logger;
    this->position = position;
    this->size = size;
    this->visible = false;
    // radius is the 1/5 of the smaller of the two sides
    // float radius = size.x < size.y ? size.x/5 : size.y/5;
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = position.x - size.x/2;
    float yStart = position.y - size.y/2;
    this->objects.push_back(new M_Rect(logger, shader, {xStart + radius, yStart + radius, Z_DISTANCE+this->index}, {size.x - diameter, size.y - diameter}, 0.0, 0.0)); // main box

    this->objects.push_back(new M_Rect(logger, shader, {xStart, yStart+radius, Z_DISTANCE+this->index}, {radius, size.y - diameter}, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(logger, shader, {xStart+size.x-radius, yStart+radius, Z_DISTANCE+this->index}, {radius, size.y - diameter}, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(logger, shader, {xStart+radius, yStart, Z_DISTANCE+this->index}, {size.x - diameter, radius}, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(logger, shader, {xStart+radius, yStart+size.y-radius, Z_DISTANCE+this->index}, {size.x - diameter, radius}, 0.0, 0.0)); // bottom

    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, {xStart+size.x-radius, yStart+size.y-radius, Z_DISTANCE+this->index}, 0, 90, 0.0)); // top right
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, {xStart+radius, yStart+size.y-radius, Z_DISTANCE+this->index}, 90, 180, 0.0)); // top left
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, {xStart+radius, yStart+radius, Z_DISTANCE+this->index}, 180, 270, 0.0)); // bottom left
    this->objects.push_back(new M_PartCircle(logger, shader, 50, radius, {xStart+size.x-radius, yStart+radius, Z_DISTANCE+this->index}, 270, 360, 0.0)); // bottom right
    
    this->objects.push_back(new M_Line(logger, shader, {xStart + radius, yStart + size.y, Z_DISTANCE+0.001+this->index}, {xStart + size.x - radius, yStart + size.y, Z_DISTANCE+0.001+this->index})); // top
    this->objects.push_back(new M_Line(logger, shader, {xStart + size.x, yStart + radius, Z_DISTANCE+0.001+this->index}, {xStart + size.x, yStart + size.y - radius, Z_DISTANCE+0.001+this->index})); // right
    this->objects.push_back(new M_Line(logger, shader, {xStart + radius, yStart, Z_DISTANCE+0.001+this->index}, {xStart + size.x - radius, yStart, Z_DISTANCE+0.001+this->index})); // bottom
    this->objects.push_back(new M_Line(logger, shader, {xStart, yStart + radius, Z_DISTANCE+0.001+this->index}, {xStart, yStart + size.y - radius, Z_DISTANCE+0.001+this->index})); // left

    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 0, 90, {xStart+size.x-radius, yStart+size.y-radius, Z_DISTANCE+0.001+this->index})); // top right
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 360, 270, {xStart+size.x-radius, yStart+radius, Z_DISTANCE+0.001+this->index})); // bottom right
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 180, 270, {xStart+radius, yStart+radius, Z_DISTANCE+0.001+this->index})); // bottom left
    this->objects.push_back(new M_Arc(logger, shader, 50, radius, 180, 90, {xStart+radius, yStart+size.y-radius, Z_DISTANCE+0.001+this->index})); // top left

    this->logger->log("MenuBox created of size: " + std::to_string(size.x) + "x" + std::to_string(size.y) + " at position: " + std::to_string(position.x) + "x" + std::to_string(position.y), true);
}

/**
 * @brief Destroy the Menu Box:: Menu Box object
 * 
 */
MenuBox::~MenuBox(){
    for(auto object: this->objects){
        delete object;
    }
    this->logger->log("MenuBox destroyed", true);
}

void MenuBox::setPosition(glm::vec2 position){
    this->position = position;
    // TODO: update the position of all the objects
}

void MenuBox::setSize(glm::vec2 size){
    this->size = size;
    // TODO: update the size and positions of all the objects
}

void MenuBox::draw(){
    if(!this->visible){
        return;
    }
    for(auto object: this->objects){
        object->draw();
    }
}

bool MenuBox::setVisible(bool visible){
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool MenuBox::getVisible(){
    return this->visible;
}

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new Menu Entry:: Menu Entry object
 * 
 * @param logger a CubeLog object
 * @param text the text to display
 */
MenuEntry::MenuEntry(CubeLog* logger, std::string text, Shader* shader, glm::vec2 position, float size){
    this->logger = logger;
    this->text = text;
    this->visible = false;
    this->shader = shader;
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = this;
    this->clickArea.xMin = position.x;
    this->clickArea.xMax = position.x + text.size() * size * 0.6;
    this->clickArea.yMin = position.y;
    this->clickArea.yMax = position.y + size;
    this->objects.push_back(new M_Text(logger, shader, text, size, {1.f,1.f,1.f,}, position));
    this->logger->log("MenuEntry created with text: " + text, true);
}

MenuEntry::~MenuEntry(){
    for(auto object: this->objects){
        delete object;
    }
    this->logger->log("MenuEntry destroyed", true);
}

void MenuEntry::onClick(void* data){
    this->logger->log("MenuEntry clicked", true);
    if(this->action != nullptr && this->visible){
        this->action(data);
    }
}

void MenuEntry::onRightClick(void* data){
    this->logger->log("MenuEntry right clicked", true);
    if(this->rightAction != nullptr && this->visible){
        this->rightAction(data);
    }
}

bool MenuEntry::setVisible(bool visible){
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool MenuEntry::getVisible(){
    return this->visible;
}

void MenuEntry::setOnClick(std::function<void(void*)> action){
    this->action = action;
}

void MenuEntry::setOnRightClick(std::function<void(void*)> action){
    this->rightAction = action;
}

std::vector<MeshObject*> MenuEntry::getObjects(){
    return this->objects;
}

void MenuEntry::draw(){
    if(!this->visible){
        return;
    }
    for(auto object: this->objects){
        object->draw();
    }
}

void MenuEntry::setPosition(glm::vec2 position){
    this->position = position;
    // TODO: update the position of all the objects
}

void MenuEntry::setVisibleWidth(float width){
    // TODO: update the position of the text so that it scrolls if the visible width is less than 100% (1.f)
}

//////////////////////////////////////////////////////////////////////////

MenuStencil::MenuStencil(CubeLog* logger, glm::vec2 position, glm::vec2 size, Shader* shader){
    this->logger = logger;
    this->position = position;
    this->size = size;
    this->shader = shader;

    // position is the center of the box
    // glm::vec2 pos = {position.x - size.x/2, position.y - size.y/2};
    glm::vec2 pos = position;

    // create vertices for the stencil
    this->vertices.push_back({pos.x, pos.y, 1.0, 1.0});
    this->vertices.push_back({pos.x + size.x, pos.y, 1.0, 1.0});
    this->vertices.push_back({pos.x + size.x, pos.y + size.y, 1.0, 1.0});
    this->vertices.push_back({pos.x, pos.y + size.y, 1.0, 1.0});

    // this->modelMatrix = glm::mat4(1.0f);
    // this->viewMatrix = glm::mat4(1.0f);
    this->projectionMatrix = glm::ortho(0.0f, 720.0f, 720.0f, 0.0f, 0.f, 1.0f);
    
    // create the VAO, VBO and EBO    
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->logger->log("MenuStencil created", true);
}

MenuStencil::~MenuStencil(){
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    glDeleteBuffers(1, &this->EBO);
    this->logger->log("MenuStencil destroyed", true);
}

void MenuStencil::setPosition(glm::vec2 position){
    this->position = position;
}

void MenuStencil::setSize(glm::vec2 size){
    this->size = size;
}

void MenuStencil::draw(){
    
}

bool MenuStencil::setVisible(bool visible){
    return true;
}

bool MenuStencil::getVisible(){
    return true;
}

void MenuStencil::enable(){
    if (true) {
        // In debug mode, draw the mask with a solid color to verify its size and position
        glDisable(GL_STENCIL_TEST);
        this->shader->use();
        // shader->setMat4("model", modelMatrix);
        // shader->setMat4("view", viewMatrix);
        shader->setMat4("projection", projectionMatrix);
        glBindVertexArray(VAO);
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
        return;
    }
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    glDepthMask(GL_FALSE);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glStencilFunc(GL_LESS, 1, 0xFF);
    
    glUseProgram(this->shader->ID);
    // this->shader->setMat4("model", this->modelMatrix);
    // this->shader->setMat4("view", this->viewMatrix);
    this->shader->setMat4("projection", this->projectionMatrix);
    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0x00);
}

void MenuStencil::disable(){
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glDisable(GL_STENCIL_TEST);
}


//////////////////////////////////////////////////////////////////////////
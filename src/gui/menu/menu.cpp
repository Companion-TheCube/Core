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

void Menu::setup(){
    this->loadObjects(filename);
    this->objects.push_back(new MenuBox(logger, {0.4, 0.0}, {1.2, 2.0}, shader));
    this->objects.at(0)->setVisible(true);
    Shader* textShader = new Shader("shaders/text.vs", "shaders/text.fs", logger);
    float textX = mapRange(0.f, -1.f, 1.f, 0.f, 720.f);
    float textY = mapRange(0.f, 1.f, -1.f, 0.f, 720.f);
    this->childrenClickables.push_back(new MenuEntry(logger, "Test", textShader, {textX, textY}, 24));
    this->childrenClickables.at(0)->setVisible(true);
    std::lock_guard<std::mutex> lock(this->mutex);
    this->ready = true;
    this->logger->log("Menu setup done", true);
}

void Menu::onClick(void* data){
    this->logger->log("Menu clicked", true);
    if(this->action != nullptr && this->visible){
        this->action(data);
    }
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
    for(auto clickable: this->childrenClickables){
        clickable->draw();
    }
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
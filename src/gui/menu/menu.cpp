#include "menu.h"
/**
 * @brief Construct a new Menu:: Menu object
 * 
 * @param logger a CubeLog object
 */

Menu::Menu(CubeLog* logger, std::string filename){
    this->logger = logger;
    this->loadObjects(filename);
}

/**
 * @brief Destroy the Menu:: Menu object
 * 
 */
Menu::~Menu(){
    this->logger->log("Menu destroyed", true);
}

void Menu::onClick(void* data){
    this->logger->log("Menu clicked", true);
}

void Menu::onRightClick(void* data){
    this->logger->log("Menu right clicked", true);
}

void Menu::setVisible(bool visible){
    this->visible = visible;
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
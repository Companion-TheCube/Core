#include "characterManager.h"

CharacterManager::CharacterManager(Shader* sh)
{
    this->shader = sh;
    CubeLog::log("CharacterManager Started. Loading built-in characters.", true);
    this->loadBuiltInCharacters();
    CubeLog::log("Built-in characters loaded. Loading app characters.", true);
    if(!this->loadAppCharacters()){
        CubeLog::log("No app characters found.", true);
    }
    CubeLog::log("All characters loaded. Character manager ready.", true);
}

CharacterManager::~CharacterManager()
{
    for(auto character: this->characters){
        delete character;
    }
}

C_Character* CharacterManager::getCharacter()
{
    return this->currentCharacter;
}

void CharacterManager::setCharacter(C_Character* character)
{
    this->currentCharacter = character;
}

bool CharacterManager::loadAppCharacters()
{
    // TODO: This will need to interface with the list of registered apps and find the ones
    // that have characters to load.
    return false;
}

bool CharacterManager::loadBuiltInCharacters()
{
    this->characters.push_back(new TheCube(this->shader));
    return false;
}

bool CharacterManager::setCharacterByName(std::string name)
{
    return false;
}

C_Character* CharacterManager::getCharacterByName(std::string name)
{
    for(auto character: this->characters){
        if(character->getName().compare(name) == 0){
            CubeLog::log("getCharacterByName(" + name + "): found!", true);
            character->setVisible(true);
            return character;
        }
    }
    CubeLog::log("getCharacterByName(" + name + "): NOT found!", true);
    return nullptr;
}

std::vector<std::string> CharacterManager::getCharacterNames()
{
    std::vector<std::string> names;
    for(auto character: this->characters){
        names.push_back(character->getName());
    }
    CubeLog::log("getCharacterNames(): Returning " + std::to_string(names.size()) + " names.", true);
    return names;
}


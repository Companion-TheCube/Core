#include "characterManager.h"

CharacterManager::CharacterManager(Shader* sh, CubeLog* lgr)
{
    this->logger = lgr;
    this->shader = sh;
    this->logger->log("CharacterManager Started. Loading built-in characters.", true);
    this->loadBuiltInCharacters();
    this->logger->log("Built-in characters loaded. Loading app characters.", true);
    if(!this->loadAppCharacters()){
        this->logger->log("No app characters found.", true);
    }
    this->logger->log("All characters loaded. Character manager ready.", true);
}

CharacterManager::~CharacterManager()
{
    for(auto character: this->characters){
        delete character;
    }
}

Character* CharacterManager::getCharacter()
{
    return this->currentCharacter;
}

void CharacterManager::setCharacter(Character* character)
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
    this->characters.push_back(new TheCube(this->shader, this->logger));
    return false;
}

bool CharacterManager::setCharacterByName(std::string name)
{
    return false;
}

Character* CharacterManager::getCharacterByName(std::string name)
{
    for(auto character: this->characters){
        if(character->getName().compare(name) == 0){
            this->logger->log("getCharacterByName(" + name + "): found!", true);
            return character;
        }
    }
    this->logger->log("getCharacterByName(" + name + "): NOT found!", true);
    return nullptr;
}

std::vector<std::string> CharacterManager::getCharacterNames()
{
    std::vector<std::string> names;
    for(auto character: this->characters){
        names.push_back(character->getName());
    }
    this->logger->log("getCharacterNames(): Returning " + std::to_string(names.size()) + " names.", true);
    return names;
}

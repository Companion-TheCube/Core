#include "characterManager.h"

CharacterManager::CharacterManager(Shader* sh)
{
    this->shader = sh;
    CubeLog::info("CharacterManager Started. Loading built-in characters.");
    this->loadBuiltInCharacters();
    CubeLog::info("Built-in characters loaded. Loading app characters.");
    if (!this->loadAppCharacters()) {
        CubeLog::info("No app characters found.");
    }
    CubeLog::info("All characters loaded. Character manager ready.");
}

CharacterManager::~CharacterManager()
{
    for (auto character : this->characters) {
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
    for (auto character : this->characters) {
        if (character->getName().compare(name) == 0) {
            CubeLog::info("getCharacterByName(" + name + "): found!");
            character->setVisible(true);
            return character;
        }
    }
    CubeLog::info("getCharacterByName(" + name + "): NOT found!");
    return nullptr;
}

std::vector<std::string> CharacterManager::getCharacterNames()
{
    std::vector<std::string> names;
    for (auto character : this->characters) {
        names.push_back(character->getName());
    }
    CubeLog::info("getCharacterNames(): Returning " + std::to_string(names.size()) + " names.");
    return names;
}

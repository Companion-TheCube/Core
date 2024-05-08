#include "character.h"
#include "characters/cube.h"

CharacterManager::CharacterManager()
{
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
    this->characters.push_back(new Cube());
    return false;
}

bool CharacterManager::setCharacterByName(std::string name)
{
    return false;
}

Character* CharacterManager::getCharacterByName(std::string name)
{
    for(auto character: this->characters){
        if(character->name == name){
            return character;
        }
    }
}

std::vector<std::string> CharacterManager::getCharacterNames()
{
    std::vector<std::string> names;
    for(auto character: this->characters){
        names.push_back(character->name);
    }
    return names;
}
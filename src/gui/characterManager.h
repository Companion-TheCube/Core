#pragma once
#include <vector>
#include "objects.h"
#include "renderables/theCube.h"
#include <iostream>
#include "shader.h"
#include <logger.h>

class CharacterManager{
    private:
        std::vector<Character*> characters;
        Character* currentCharacter;
        Shader* shader;
        CubeLog* logger;
    public:
        CharacterManager(Shader* sh, CubeLog* lgr);
        ~CharacterManager();
        Character* getCharacter();
        void setCharacter(Character*);
        bool loadAppCharacters();
        bool loadBuiltInCharacters();
        bool setCharacterByName(std::string name);
        Character* getCharacterByName(std::string name);
        std::vector<std::string> getCharacterNames();
};

#pragma once
#include <vector>
#include "objects.h"
#include "renderables/theCube.h"
#include <iostream>
#include "shader.h"
#include <logger.h>

class CharacterManager{
    private:
        std::vector<C_Character*> characters;
        C_Character* currentCharacter;
        Shader* shader;
        CubeLog* logger;
    public:
        CharacterManager(Shader* sh, CubeLog* lgr);
        ~CharacterManager();
        C_Character* getCharacter();
        void setCharacter(C_Character*);
        bool loadAppCharacters();
        bool loadBuiltInCharacters();
        bool setCharacterByName(std::string name);
        C_Character* getCharacterByName(std::string name);
        std::vector<std::string> getCharacterNames();
};

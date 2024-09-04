#pragma once
#include <vector>
#include "objects.h"
// #include "renderables/theCube.h"
#include "meshObject.h"
#include "meshLoader.h"
#include "shapes.h"
#include <iostream>
#include "shader.h"
#include <logger.h>

class CharacterManager{
    private:
        std::vector<C_Character*> characters;
        C_Character* currentCharacter;
        Shader* shader;
    public:
        CharacterManager(Shader* sh);
        ~CharacterManager();
        C_Character* getCharacter();
        void setCharacter(C_Character*);
        bool loadAppCharacters();
        bool loadBuiltInCharacters();
        bool setCharacterByName(std::string name);
        C_Character* getCharacterByName(std::string name);
        std::vector<std::string> getCharacterNames();
};

struct CharacterPart {
    std::string name;
    std::vector<MeshObject*> objects;
    glm::vec3 centerPoint;
};

class Character_generic : public C_Character {
private:
    void rotate(float angle, float x, float y, float z);
    void translate(float x, float y, float z);
    void scale(float x, float y, float z);
    std::string name;
    Shader* shader;
    std::vector<MeshObject*> objects;
    std::vector<Animation> animations;
    std::vector<ExpressionDefinition> expressions;
    std::vector<CharacterPart*> parts;
    unsigned long long animationFrame;
    bool visible;
    ExpressionNames_enum currentExpression;
    ExpressionNames_enum currentFunnyExpression;
public:
    Character_generic(Shader* sh, std::string folder); // load character data from folder
    Character_generic(Shader* sh, unsigned long id); // load character data from database
    ~Character_generic();
    void draw();
    bool animateRandomFunny();
    bool animateJumpUp();
    bool animateJumpLeft();
    bool animateJumpRight();
    bool animateJumpLeftThroughWall();
    bool animateJumpRightThroughWall();
    void expression(ExpressionNames_enum);
    std::string getName();
    CharacterPart* getPartByName(std::string name);
    bool setVisible(bool visible);
    bool getVisible();
};
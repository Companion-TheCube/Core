#pragma once
#include "objects.h"
#include <vector>
// #include "renderables/theCube.h"
#include "renderables/meshLoader.h"
#include "renderables/meshObject.h"
#include "renderables/shapes.h"
#include "shader.h"
#include <../database/cubeDB.h>
#include <iostream>
#include <logger.h>
#include <thread>


struct CharacterPart {
    std::string name;
    std::vector<MeshObject*> objects;
    glm::vec3 centerPoint;
};

class Character_generic : public Object {
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
    Expressions::ExpressionNames_enum currentExpression;
    Animations::AnimationNames_enum currentAnimationName;
    Animation* currentAnimation;
    ExpressionDefinition* currentExpressionDef;

public:
    Character_generic(Shader* sh, std::string folder); // load character data from folder
    Character_generic(Shader* sh, unsigned long id); // load character data from database
    ~Character_generic();
    void draw();
    void animate();
    void expression();
    void triggerExpression(Expressions::ExpressionNames_enum);
    void triggerAnimation(Animations::AnimationNames_enum);
    std::string getName();
    CharacterPart* getPartByName(std::string name);
    bool setVisible(bool visible);
    bool getVisible();
};

class CharacterManager {
private:
    std::vector<Character_generic*> characters;
    Character_generic* currentCharacter;
    Shader* shader;
    
    
    std::condition_variable animationCV;
    std::condition_variable expressionCV;
    bool exitThreads = false;
    bool animationThreadReady = false;
    bool expressionThreadReady = false;
public:
    std::mutex animationMutex;
    std::mutex expressionMutex;
    std::mutex drawMutex;
    std::condition_variable drawCV;
    std::jthread* animationThread;
    std::jthread* expressionThread;
    CharacterManager(Shader* sh);
    ~CharacterManager();
    Character_generic* getCharacter();
    void setCharacter(Character_generic*);
    bool loadAppCharacters();
    bool loadBuiltInCharacters();
    bool setCharacterByName(std::string name);
    Character_generic* getCharacterByName(std::string name);
    std::vector<std::string> getCharacterNames();
    void triggerAnimationAndExpressionThreads();
};
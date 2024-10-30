#pragma once
#ifndef CHARACTERMANAGER_H
#define CHARACTERMANAGER_H
#ifndef OBJECTS_H
#include "objects.h"
#endif // OBJECTS_H
#ifndef MESHLOADER_H
#include "renderables/meshLoader.h"
#endif // MESHLOADER_H
#ifndef MESHOBJECT_H
#include "renderables/meshObject.h"
#endif // MESHOBJECT_H
#ifndef SHAPES_H
#include "renderables/shapes.h"
#endif // SHAPES_H
#ifndef SHADER_H
#include "shader.h"
#endif // SHADER_H
#ifndef CUBEDB_H
#include <../database/cubeDB.h>
#endif // CUBEDB_H
#ifndef API_I_H
#include "../api_i.h"
#endif // API_I_H
#include <filesystem>
#include <iostream>
#include <logger.h>
#include <thread>
#include <vector>

struct CharacterSystemError : public std::exception {
    static uint16_t errorCount;
    std::string message;
    enum ERROR_TYPES {
        CHARACTER_NOT_FOUND,
        CHARACTER_LOAD_ERROR,
        CHARACTER_PART_NOT_FOUND,
        CHARACTER_PART_LOAD_ERROR,
        CHARACTER_ANIMATION_NOT_FOUND,
        CHARACTER_ANIMATION_LOAD_ERROR,
        CHARACTER_EXPRESSION_NOT_FOUND,
        CHARACTER_EXPRESSION_LOAD_ERROR,

        CHARACTER_ERROR_COUNT
    };
    CharacterSystemError(const std::string& message)
        : message(message)
    {
        errorCount++;
    }
    const char* what() const noexcept override
    {
        return message.c_str();
    }
    uint16_t getErrorCount()
    {
        return errorCount;
    }
};

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
    AnimationLoader* animationLoader;
    ExpressionLoader* expressionLoader;
    std::vector<CharacterPart*> parts;
    unsigned long long animationFrame;
    bool visible;
    Expressions::ExpressionNames_enum currentExpression;
    Expressions::ExpressionNames_enum nextExpression;
    Animations::AnimationNames_enum currentAnimationName;
    Animations::AnimationNames_enum nextAnimationName;
    Animation currentAnimation;
    Animation nextAnimation;
    ExpressionDefinition currentExpressionDef;
    ExpressionDefinition nextExpressionDef;
    std::mutex currentMutex;
    std::mutex nextMutex;

public:
    Character_generic(Shader* sh, const std::string& folder); // load character data from folder
    Character_generic(Shader* sh, unsigned long id); // load character data from database
    ~Character_generic();
    void draw();
    void animate();
    void expression();
    void triggerExpression(Expressions::ExpressionNames_enum);
    void triggerAnimation(Animations::AnimationNames_enum);
    std::string getName();
    CharacterPart* getPartByName(const std::string& name);
    bool setVisible(bool visible);
    bool getVisible();
};

class CharacterManager : public I_API_Interface {
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
    bool setCharacterByName(const std::string& name);
    Character_generic* getCharacterByName(const std::string& name);
    std::vector<std::string> getCharacterNames();
    void triggerAnimationAndExpressionThreads();
    std::string getInterfaceName() const override;
    HttpEndPointData_t getHttpEndpointData() override;
};

#endif // CHARACTERMANAGER_H

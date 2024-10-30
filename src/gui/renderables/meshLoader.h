#pragma once
#ifndef MESHLOADER_H
#define MESHLOADER_H
#ifndef MESHOBJECT_H
#include "meshObject.h"
#endif // MESHOBJECT_H
#ifndef SHAPES_H
#include "shapes.h"
#endif // SHAPES_H
#include <algorithm>
#include <fstream>
#include <iostream>
#include <logger.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ObjectCollection {
    std::vector<MeshObject*> objects;
    std::string name;
};

class MeshLoader {
private:
    std::string folderName;

public:
    std::vector<ObjectCollection*> collections;
    Shader* shader;
    MeshLoader(Shader* shdr, const std::string& folderName, std::vector<std::string> toLoad);
    ~MeshLoader();
    std::vector<MeshObject*> loadMesh(const std::string& path);
    std::vector<std::string> getFileNames();
    std::vector<MeshObject*> getObjects();
    std::vector<ObjectCollection*> getCollections();
};

namespace Expressions {
enum class ExpressionNames_enum {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    FRUSTRATED,
    SURPRISED,
    SCARED,
    DISGUSTED,
    DIZZY,
    SICK,
    SLEEPY,
    CONFUSED,
    SHOCKED,
    INJURED,
    DEAD,
    CRYING,
    SCREAMING,
    TALKING,
    LISTENING,
    SLEEPING,
    FUNNY_INDEX,
    FUNNY_BOUNCE,
    FUNNY_SPIN,
    FUNNY_SHRINK,
    FUNNY_EXPAND,
    FUNNY_JUMP,
    NULL_EXPRESSION,
    COUNT
};
}

namespace Animations {
enum class AnimationNames_enum {
    NEUTRAL,
    JUMP_RIGHT_THROUGH_WALL,
    JUMP_LEFT_THROUGH_WALL,
    JUMP_UP_THROUGH_CEILING,
    JUMP_RIGHT,
    JUMP_LEFT,
    JUMP_UP,
    JUMP_DOWN,
    JUMP_FORWARD,
    JUMP_BACKWARD,
    FUNNY_INDEX,
    FUNNY_BOUNCE,
    FUNNY_SPIN,
    FUNNY_SHRINK,
    FUNNY_EXPAND,
    FUNNY_JUMP,
    MOVE_TO_LOWER_LEFT,
    MOVE_TO_LOWER_RIGHT,
    MOVE_TO_UPPER_LEFT,
    MOVE_TO_UPPER_RIGHT,
    COUNT
};

enum class AnimationType {
    TRANSLATE, // uses type, value, axis, and time
    ROTATE, // uses type, value, axis, and time
    SCALE_XYZ, // uses type, value, axis, and time
    UNIFORM_SCALE, // uses type, value, and time
    ROTATE_ABOUT, // uses type, value, point, and time
    RETURN_HOME // uses type, and time
};
}

struct AnimationKeyframe {
    Animations::AnimationType type;
    float value;
    unsigned int timeStart; // in frames
    unsigned int timeEnd; // in frames
    std::function<double(double)> easingFunction;
    glm::vec3 axis;
    glm::vec3 point; // relative to the object's center
};

struct Animation {
    Animations::AnimationNames_enum name;
    std::string expression;
    std::vector<AnimationKeyframe> keyframes;
};

class AnimationLoader {
private:
    std::map<Animations::AnimationNames_enum, Animation> animationsMap;
    std::vector<std::string> getFileNames();
    void loadAnimations(std::vector<std::string> fileNames);
    Animation loadAnimation(const std::string& fileName);
    AnimationKeyframe loadKeyframe(nlohmann::json keyframe);
    std::string folderName;

public:
    AnimationLoader(const std::string& folderName, std::vector<std::string> animationFileNames);
    ~AnimationLoader();
    std::vector<Animation> getAnimationsVector();
    Animation getAnimationByName(std::string name);
    Animation getAnimationByEnum(Animations::AnimationNames_enum name);
    std::map<Animations::AnimationNames_enum, Animation> getAnimations();
    std::vector<std::string> getAnimationNames();
};

class AnimationLoaderException : public std::exception {
private:
    static int count;
    std::string message;

public:
    AnimationLoaderException(const std::string& message);
    const char* what() const throw();
};

struct ExpressionDefinition {
    Expressions::ExpressionNames_enum name;
    std::string expression;
    std::vector<std::string> objects;
    std::vector<bool> visibility;
    std::vector<AnimationKeyframe> animations;
};

class ExpressionLoader {
private:
    std::map<Expressions::ExpressionNames_enum, ExpressionDefinition> expressionsMap;
    std::vector<std::string> getFileNames();
    std::vector<ExpressionDefinition> loadExpressions(std::vector<std::string> fileNames);
    std::string folderName;

public:
    ExpressionLoader(const std::string& folderName, std::vector<std::string> expressionFileNames);
    ~ExpressionLoader();
    std::vector<ExpressionDefinition> getExpressionsVector();
    std::map<Expressions::ExpressionNames_enum, ExpressionDefinition> getExpressions();
    ExpressionDefinition getExpressionByEnum(Expressions::ExpressionNames_enum name);
    ExpressionDefinition getExpressionByName(std::string name);
};

#endif // MESHLOADER_H
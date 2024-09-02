#pragma once

#include "../../shader.h"
#include "../meshObject.h"
#include "shapes.h"
#include <fstream>
#include <iostream>
#include <logger.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

struct ObjectCollection {
    std::vector<MeshObject*> objects;
    std::string name;
};

class MeshLoader {
public:
    std::vector<ObjectCollection*> collections;
    Shader* shader;
    MeshLoader(Shader* shdr, std::vector<std::string> toLoad);
    ~MeshLoader();
    std::vector<MeshObject*> loadMesh(std::string path);
    std::vector<std::string> getFileNames();
    std::vector<MeshObject*> getObjects();
    std::vector<ObjectCollection*> getCollections();
};

enum AnimationType {
    TRANSLATE, // uses type, axis, and time
    ROTATE, // uses type, axis, and time
    SCALE_XYZ, // uses type, axis, and time
    UNIFORM_SCALE, // uses type, value, and time
    ROTATE_ABOUT // uses type, axis, point, and time
};

struct AnimationKeyframe {
    AnimationType type;
    float value;
    unsigned int time; // in frames
    std::function<float(float)> easingFunction;
    glm::vec3 axis;
    glm::vec3 point; // relative to the object's center
};

struct Animation {
    std::string name;
    std::vector<AnimationKeyframe> keyframes;
};

class AnimationLoader {
private:
    std::vector<Animation> animations;
    std::vector<std::string> animationNames;
    std::vector<std::string> getFileNames();
    std::vector<Animation> loadAnimations(std::vector<std::string> fileNames);
    Animation loadAnimation(std::string fileName);
    std::vector<std::string> getAnimationNames();
    AnimationKeyframe loadKeyframe(nlohmann::json keyframe);
public:
    AnimationLoader(std::vector<std::string> animationNames);
    ~AnimationLoader();
    std::vector<Animation> getAnimations();
};

class AnimationLoaderException : public std::exception {
private:
    static int count;
    std::string message;
public:
    AnimationLoaderException(std::string message);
    const char* what() const throw();
};
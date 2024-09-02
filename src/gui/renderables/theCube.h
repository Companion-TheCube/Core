#pragma once
#include <vector>
#include "../objects.h"
#include "../shader.h"
#include "meshObject.h"
#include "meshes/shapes.h"
#include "cmath"
#include <logger.h>
#include "meshes/meshLoader.h"

struct CharacterPart{
    std::string name;
    std::vector<MeshObject*> objects;
    glm::vec3 centerPoint;
};

class TheCube: public C_Character{
    private:
        void rotate(float angle, float x, float y, float z);
        void translate(float x, float y, float z);
        void scale(float x, float y, float z);
        std::string name;
        Shader* shader;
        std::vector<MeshObject*> objects;
        std::vector<Animation> animations;
        std::vector<CharacterPart*> parts;
        unsigned long long animationFrame;
        bool visible;
        Expression currentExpression;
        Expression currentFunnyExpression;
    public:
        TheCube(Shader* sh);
        ~TheCube();
        void draw();
        bool animateRandomFunny();
        bool animateJumpUp();
        bool animateJumpLeft();
        bool animateJumpRight();
        bool animateJumpLeftThroughWall();
        bool animateJumpRightThroughWall();
        void expression(Expression);
        std::string getName();
        CharacterPart* getPartByName(std::string name);
        bool setVisible(bool visible);
        bool getVisible();
};
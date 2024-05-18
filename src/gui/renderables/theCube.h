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
        CubeLog* logger;
        MeshLoader* loader;
        std::vector<CharacterPart*> parts;
        unsigned long long animationFrame;
        bool visible;
    public:
        TheCube(Shader* sh, CubeLog* lgr);
        ~TheCube();
        void draw();
        void animateRandomFunny();
        void animateJumpUp();
        void animateJumpLeft();
        void animateJumpRight();
        void animateJumpLeftThroughWall();
        void animateJumpRightThroughWall();
        void expression(Expression);
        std::string getName();
        CharacterPart* getPartByName(std::string name);
        bool setVisible(bool visible);
        bool getVisible();
};
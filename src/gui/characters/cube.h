#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "../character.h"
#include "cmath"


struct Vertex{
    float x, y, z;
};

class Cube: public Character{
    private:
        std::vector<sf::Vertex> drawables;
        std::vector<Vertex> cubeVertices;
        float angleX, angleY, angleZ;
        void rotateX(float angle);
        void rotateY(float angle);
        void rotateZ(float angle);
        void project();
        void translate(float x, float y, float z);
        // std::string name;
    public:
        Cube();
        ~Cube();
        std::vector<sf::Vertex> getDrawables();
        void animateRandomFunny();
        void animateJumpUp();
        void animateJumpLeft();
        void animateJumpRight();
        void animateJumpLeftThroughWall();
        void animateJumpRightThroughWall();
        void expression(Expression);
};
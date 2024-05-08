#include "cube.h"

Cube::Cube()
{
    this->angleX = 0;
    this->angleY = 0;
    this->angleZ = 0;
    this->cubeVertices = {
        {1, 1, 1},
        {1, 1, -1},
        {1, -1, 1},
        {1, -1, -1},
        {-1, 1, 1},
        {-1, 1, -1},
        {-1, -1, 1},
        {-1, -1, -1}
    };
    this->name = "Cube";
}

Cube::~Cube()
{
}

std::vector<sf::Vertex> Cube::getDrawables()
{
    return this->drawables;
}

void Cube::animateRandomFunny()
{
    this->rotateX(0.01);
    this->rotateY(0.01);
    this->rotateZ(0.01);
    this->project();
}

void Cube::animateJumpUp()
{
    this->translate(0, 0.1, 0);
    this->project();
}

void Cube::animateJumpLeft()
{
    this->translate(-0.1, 0, 0);
    this->project();
}

void Cube::animateJumpRight()
{
    this->translate(0.1, 0, 0);
    this->project();
}

void Cube::animateJumpLeftThroughWall()
{
    this->translate(-0.1, 0, 0);
    this->project();
}

void Cube::animateJumpRightThroughWall()
{
    this->translate(0.1, 0, 0);
    this->project();
}

void Cube::expression(Expression e)
{
}

void Cube::rotateX(float angle)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) {
        float ynew = this->cubeVertices[i].y * c - this->cubeVertices[i].z * s;
        float znew = this->cubeVertices[i].y * s + this->cubeVertices[i].z * c;
        this->cubeVertices[i].y = ynew;
        this->cubeVertices[i].z = znew;
    }
}

void Cube::rotateY(float angle)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) {
        float xnew = this->cubeVertices[i].x * c - this->cubeVertices[i].z * s;
        float znew = this->cubeVertices[i].x * s + this->cubeVertices[i].z * c;
        this->cubeVertices[i].x = xnew;
        this->cubeVertices[i].z = znew;
    }
}

void Cube::rotateZ(float angle)
{
    float s = std::sin(angle);
    float c = std::cos(angle);

    for (int i = 0; i < 8; ++i) {
        float xnew = this->cubeVertices[i].x * c - this->cubeVertices[i].y * s;
        float ynew = this->cubeVertices[i].x * s + this->cubeVertices[i].y * c;
        this->cubeVertices[i].x = xnew;
        this->cubeVertices[i].y = ynew;
    }
}

void Cube::project()
{
    
}

void Cube::translate(float x, float y, float z)
{
    for (int i = 0; i < 8; ++i) {
        this->cubeVertices[i].x += x;
        this->cubeVertices[i].y += y;
        this->cubeVertices[i].z += z;
    }
}

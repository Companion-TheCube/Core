#pragma once
#ifndef __GLEW_H__
#include "GL/glew.h"
#endif
#include <thread>
#include "SFML/Graphics.hpp"
#include <iostream>
#include "../logger/logger.h"
#include <cmath>
#include <vector>
#include "character.h"
#include <mutex>

#include <SFML/OpenGL.hpp>
// #include <GL/gl.h>
// #include <GL/glu.h>
#include <SFML/Window.hpp>
#include "shader.h"

struct Vertex{
    float x, y, z;
    float r, g, b;
};

struct Face{
    Vertex a, b, c, d;
    float colorR, colorG, colorB;
};

class Renderer{
    private:
        int thread();
        sf::RenderWindow window;
        sf::Font font;
        std::thread t;
        CubeLog *logger;
        float viewer_distance = 5;
        bool viewer_distance_increment = true;
        sf::Vector2f project(Vertex& v);
        std::vector<Vertex> rotateY(float angle, std::vector<Vertex> cubeVertices);
        std::vector<Vertex> rotateX(float angle, std::vector<Vertex> cubeVertices);
        std::vector<Vertex> rotateZ(float angle, std::vector<Vertex> cubeVertices);
    public:
        Renderer(CubeLog *logger);
        ~Renderer();
};

struct Vector3 {
    float x, y, z;

    // Constructor for convenience
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Cross product
    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }

    // Subtract
    Vector3 operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }
};
#pragma once
#include <thread>
#include "SFML/Graphics.hpp"
#include <iostream>
#include "../logger/logger.h"
#include <cmath>
#include <vector>

struct Vertex{
    float x, y, z;
};

class Renderer{
    private:
        int thread();
        sf::RenderWindow window;
        sf::Font font;
        std::thread t;
        CubeLog *logger;
        sf::Vector2f project(Vertex& v);
        std::vector<Vertex> rotateY(float angle, std::vector<Vertex> cubeVertices);
        std::vector<Vertex> rotateX(float angle, std::vector<Vertex> cubeVertices);
        std::vector<Vertex> rotateZ(float angle, std::vector<Vertex> cubeVertices);
    public:
        Renderer(CubeLog *logger);
        ~Renderer();
};


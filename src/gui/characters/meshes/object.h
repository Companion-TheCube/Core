#pragma once
#include <vector>
#include "../../character.h"
#include "../../shader.h"
#include "GL/glew.h"
#include <glm/glm.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>

class MeshObject {
public:
    virtual void draw() = 0;
    virtual void setProjectionMatrix(glm::mat4 projectionMatrix) = 0;
    virtual void setViewMatrix(glm::vec3 viewMatrix) = 0;
    virtual void setModelMatrix(glm::mat4 modelMatrix) = 0;
    virtual void translate(glm::vec3 translation) = 0;
    virtual void rotate(float angle, glm::vec3 axis) = 0;
    virtual void scale(glm::vec3 scale) = 0;
    virtual void uniformScale(float scale) = 0;
    virtual void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point) = 0;
};
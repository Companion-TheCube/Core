#pragma once
#include "../meshObject.h"

class M_Arc: public MeshObject{
private:
    CubeLog* logger;
    std::vector<glm::vec3> vertices;
    unsigned int numSegments;
    float radius;
    float startAngle;
    float endAngle;
    glm::vec3 centerPoint;
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO[1], VBO[1];
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
public:
    M_Arc(CubeLog* logger, Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint);
    ~M_Arc();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    glm::vec3 getCenterPoint();
};
#include "arc.h"

M_Arc::M_Arc(CubeLog* logger, Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint){
    this->logger = logger;
    this->shader = sh;
    this->numSegments = numSegments;
    this->radius = radius;
    this->startAngle = startAngle;
    this->endAngle = endAngle;
    this->centerPoint = centerPoint;
    for(unsigned int i = 0; i <= this->numSegments; i++){
        float theta = glm::radians(this->startAngle) + i * (glm::radians(this->endAngle) - glm::radians(this->startAngle)) / this->numSegments;
        float x = this->centerPoint.x + this->radius * cos(theta);
        float y = this->centerPoint.y + this->radius * sin(theta);
        float z = this->centerPoint.z;
        this->vertexData.push_back({x, y, z, 1.f});
    }
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, this->vertexData.size() * sizeof(Vertex), &this->vertexData[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    setProjectionMatrix(glm::perspective(glm::radians(45.0f), 720.0f / 720.0f, 0.1f, 100.0f));
    setViewMatrix(glm::vec3(0.0f, 0.0f, 6.0f));
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
    setModelMatrix(modelMatrix);
    this->logger->log("Created Arc", true);
}

M_Arc::~M_Arc(){
    this->logger->log("Destroyed Arc", true);
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Arc::draw(){
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    // glDrawElements(GL_LINE_STRIP, this->vertexData.size(), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_LINE_STRIP, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_Arc::setProjectionMatrix(glm::mat4 projectionMatrix){
    this->projectionMatrix = projectionMatrix;
}
void M_Arc::setViewMatrix(glm::vec3 viewMatrix){
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}
void M_Arc::setModelMatrix(glm::mat4 modelMatrix){
    this->modelMatrix = modelMatrix;
}
void M_Arc::translate(glm::vec3 translation){
    this->centerPoint += translation;
}

void M_Arc::rotate(float angle, glm::vec3 axis){
    this->startAngle += angle;
    this->endAngle += angle;
}

void M_Arc::scale(glm::vec3 scale){
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the radius
    this->radius *= avgScale;
}

void M_Arc::uniformScale(float scale){
    this->radius *= scale;
}

void M_Arc::rotateAbout(float angle, glm::vec3 point){}
void M_Arc::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point){}
glm::vec3 M_Arc::getCenterPoint(){
    return this->centerPoint;
}

// Path: src/gui/renderables/meshes/arc.h

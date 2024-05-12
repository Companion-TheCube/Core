#include "cube.h"

Cube::Cube(Shader* sh){
    this->shader = sh;
    glGenVertexArrays(7, VAOs);
    glGenBuffers(7, VBOs);
    glGenBuffers(7, EBOs);

    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 8, cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 36, indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    Vertex face[4];
    for(int i = 0; i < 6; i++){
        face[0] = edgeVertices[i*4];
        face[1] = edgeVertices[i*4+1];
        face[2] = edgeVertices[i*4+2];
        face[3] = edgeVertices[i*4+3];
        glBindVertexArray(VAOs[i+1]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[i+1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(face) * 12, face, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i+1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faceIndices) * 6, faceIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    setProjectionMatrix(glm::perspective(glm::radians(45.0f), 720.0f / 720.0f, 0.1f, 100.0f));
    setViewMatrix(glm::vec3(0.0f, 0.0f, 6.0f));
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
    setModelMatrix(modelMatrix);
}

void Cube::draw(){
    shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    // glDepthMask(GL_FALSE);
    glBindVertexArray(VAOs[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    for(int i = 1; i <= 6; i++){
        glBindVertexArray(VAOs[i]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);    
    }
    // glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void Cube::setProjectionMatrix(glm::mat4 projection){
    this->projectionMatrix = projection;
}

void Cube::setViewMatrix(glm::vec3 view){
    this->viewMatrix = glm::lookAt(view, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Cube::setModelMatrix(glm::mat4 model){
    this->modelMatrix = model;
}

void Cube::rotate(float angle, glm::vec3 axis){
    modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), axis);
}

void Cube::translate(glm::vec3 translation){
    modelMatrix = glm::translate(modelMatrix, translation);
}

void Cube::scale(glm::vec3 scale){
    // adjust scale vectors to avoid scaling to zero and modify them to make sure the cube is not deformed
    if(scale.x == 0.0f) scale.x = 0.01f;
    if(scale.y == 0.0f) scale.y = 0.01f;
    if(scale.z == 0.0f) scale.z = 0.01f;

    modelMatrix = glm::scale(modelMatrix, scale);
}

Cube::~Cube(){
    glDeleteVertexArrays(7, VAOs);
    glDeleteBuffers(7, VBOs);
    glDeleteBuffers(7, EBOs);
}

void Cube::uniformScale(float scale){
    this->scale(glm::vec3(scale, scale, scale));
}

void Cube::rotateAbout(float angle, glm::vec3 point) {
    // calculate the axis of rotation based on the models current position and the point of rotation
    glm::vec3 axis = point - getCenterPoint();
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    modelMatrix = tempMat * modelMatrix; // Apply the new transformation to the existing model matrix
}

void Cube::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point) {
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    modelMatrix = tempMat * modelMatrix; // Apply the new transformation to the existing model matrix
}

glm::vec3 Cube::getCenterPoint(){
    // get the center of the cube form the model matrix
    glm::vec4 center = modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return glm::vec3(center.x, center.y, center.z);
}

// Path: src/gui/characters/meshes/cube.h
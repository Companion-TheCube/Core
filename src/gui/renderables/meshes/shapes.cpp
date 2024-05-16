#include "shapes.h"

M_PartCircle::M_PartCircle(CubeLog* logger, Shader* sh, unsigned int numSegments, float radius, glm::vec3 centerPoint, float startAngle, float endAngle, float fillColor){
    this->logger = logger;
    this->shader = sh;
    this->numSegments = numSegments;
    this->radius = radius;
    this->centerPoint = centerPoint; 
    this->startAngle = startAngle;
    this->endAngle = endAngle;
    // ensure start angle is greater than end angle
    if(this->startAngle < this->endAngle){
        float temp = this->startAngle;
        this->startAngle = this->endAngle;
        this->endAngle = temp;
    }
    // push the center point as the first vertex
    this->vertexData.push_back({centerPoint.x, centerPoint.y, centerPoint.z, fillColor});
    for(unsigned int i = 0; i <= this->numSegments; i++){
        float theta = glm::radians(this->startAngle) + i * (glm::radians(this->endAngle) - glm::radians(this->startAngle)) / this->numSegments;
        float x = this->centerPoint.x + this->radius * cos(theta);
        float y = this->centerPoint.y + this->radius * sin(theta);
        float z = this->centerPoint.z;
        this->vertexData.push_back({x, y, z, fillColor});
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
    this->logger->log("Created PartCircle", true);
}

M_PartCircle::~M_PartCircle(){
    this->logger->log("Destroyed PartCircle", true);
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_PartCircle::draw(){
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_PartCircle::setProjectionMatrix(glm::mat4 projectionMatrix){
    this->projectionMatrix = projectionMatrix;
}

void M_PartCircle::setViewMatrix(glm::vec3 viewMatrix){
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_PartCircle::setModelMatrix(glm::mat4 modelMatrix){
    this->modelMatrix = modelMatrix;
}

void M_PartCircle::translate(glm::vec3 translation){
    for(int i = 0; i < this->vertexData.size(); i++){
        this->vertexData[i].x += translation.x;
        this->vertexData[i].y += translation.y;
        this->vertexData[i].z += translation.z;
    }
}

void M_PartCircle::rotate(float angle, glm::vec3 axis){
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    for(int i = 0; i < this->vertexData.size(); i++){
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = {point.x, point.y, point.z, 1.f};
    }
}

void M_PartCircle::scale(glm::vec3 scale){
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the circle
    for(int i = 0; i < this->vertexData.size(); i++){
        this->vertexData[i].x *= avgScale;
        this->vertexData[i].y *= avgScale;
        this->vertexData[i].z *= avgScale;
    }
}

void M_PartCircle::uniformScale(float scale){
    for(int i = 0; i < this->vertexData.size(); i++){
        this->vertexData[i].x *= scale;
        this->vertexData[i].y *= scale;
        this->vertexData[i].z *= scale;
    }
}

void M_PartCircle::rotateAbout(float angle, glm::vec3 point){
    glm::vec3 axis = point - glm::vec3(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for(int i = 0; i < this->vertexData.size(); i++){
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = {point.x, point.y, point.z, 1.f};
    }
}

void M_PartCircle::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point){
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for(int i = 0; i < this->vertexData.size(); i++){
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = {point.x, point.y, point.z, 1.f};
    }
}

glm::vec3 M_PartCircle::getCenterPoint(){
    return this->centerPoint;
}


//////////////////////////////////////////////////////////////////////////////////////////

M_Rect::M_Rect(CubeLog* logger, Shader* sh, glm::vec3 position, glm::vec2 size, float fillColor, float borderColor){
    this->logger = logger;
    this->shader = sh;
    this->vertexDataFill.push_back({position.x + size.x, position.y, position.z, fillColor});
    this->vertexDataFill.push_back({position.x, position.y, position.z, fillColor});
    this->vertexDataFill.push_back({position.x + size.x, position.y + size.y, position.z, fillColor});
    this->vertexDataFill.push_back({position.x, position.y + size.y, position.z, fillColor});
    this->vertexDataBorder.push_back({position.x, position.y, position.z, borderColor});
    this->vertexDataBorder.push_back({position.x + size.x, position.y, position.z, borderColor});
    this->vertexDataBorder.push_back({position.x + size.x, position.y + size.y, position.z, borderColor});
    this->vertexDataBorder.push_back({position.x, position.y + size.y, position.z, borderColor});
    glGenVertexArrays(2, VAO);
    glGenBuffers(2, VBO);
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, this->vertexDataFill.size() * sizeof(Vertex), &this->vertexDataFill[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, this->vertexDataBorder.size() * sizeof(Vertex), &this->vertexDataBorder[0], GL_STATIC_DRAW);
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
    this->logger->log("Created Rect", true);
}

M_Rect::~M_Rect(){
    this->logger->log("Destroyed Rect", true);
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Rect::draw(){
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, this->vertexDataFill.size());
    glBindVertexArray(VAO[1]);
    glDrawArrays(GL_LINE_LOOP, 0, this->vertexDataBorder.size());
    glBindVertexArray(0);
}

void M_Rect::setProjectionMatrix(glm::mat4 projectionMatrix){
    this->projectionMatrix = projectionMatrix;
}

void M_Rect::setViewMatrix(glm::vec3 viewMatrix){
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_Rect::setModelMatrix(glm::mat4 modelMatrix){
    this->modelMatrix = modelMatrix;
}

void M_Rect::translate(glm::vec3 translation){
    for(int i = 0; i < 4; i++){
        this->vertexDataFill[i].x += translation.x;
        this->vertexDataFill[i].y += translation.y;
        this->vertexDataFill[i].z += translation.z;
        this->vertexDataBorder[i].x += translation.x;
        this->vertexDataBorder[i].y += translation.y;
        this->vertexDataBorder[i].z += translation.z;
    }
}

void M_Rect::rotate(float angle, glm::vec3 axis){
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    for(int i = 0; i < 4; i++){
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = {fill.x, fill.y, fill.z, 1.f};
        this->vertexDataBorder[i] = {border.x, border.y, border.z, 1.f};
    }
}

void M_Rect::scale(glm::vec3 scale){
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the rectangle
    for(int i = 0; i < 4; i++){
        this->vertexDataFill[i].x *= avgScale;
        this->vertexDataFill[i].y *= avgScale;
        this->vertexDataFill[i].z *= avgScale;
        this->vertexDataBorder[i].x *= avgScale;
        this->vertexDataBorder[i].y *= avgScale;
        this->vertexDataBorder[i].z *= avgScale;
    }
}

void M_Rect::uniformScale(float scale){
    for(int i = 0; i < 4; i++){
        this->vertexDataFill[i].x *= scale;
        this->vertexDataFill[i].y *= scale;
        this->vertexDataFill[i].z *= scale;
        this->vertexDataBorder[i].x *= scale;
        this->vertexDataBorder[i].y *= scale;
        this->vertexDataBorder[i].z *= scale;
    }
}

void M_Rect::rotateAbout(float angle, glm::vec3 point){
    glm::vec3 axis = point - glm::vec3(this->vertexDataFill[0].x, this->vertexDataFill[0].y, this->vertexDataFill[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for(int i = 0; i < 4; i++){
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = {fill.x, fill.y, fill.z, 1.f};
        this->vertexDataBorder[i] = {border.x, border.y, border.z, 1.f};
    }
}

void M_Rect::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point){
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for(int i = 0; i < 4; i++){
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = {fill.x, fill.y, fill.z, 1.f};
        this->vertexDataBorder[i] = {border.x, border.y, border.z, 1.f};
    }
}

glm::vec3 M_Rect::getCenterPoint(){
    return glm::vec3((this->vertexDataFill[0].x + this->vertexDataFill[2].x) / 2, (this->vertexDataFill[0].y + this->vertexDataFill[2].y) / 2, (this->vertexDataFill[0].z + this->vertexDataFill[2].z) / 2);
}

//////////////////////////////////////////////////////////////////////////////////////////

M_Line::M_Line(CubeLog* logger, Shader* sh, glm::vec3 start, glm::vec3 end){
    this->logger = logger;
    this->shader = sh;
    this->vertexData.push_back({start.x, start.y, start.z, 1.f});
    this->vertexData.push_back({end.x, end.y, end.z, 1.f});
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
    this->logger->log("Created Line", true);
}

M_Line::~M_Line(){
    this->logger->log("Destroyed Line", true);
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Line::draw(){
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_LINES, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_Line::setProjectionMatrix(glm::mat4 projectionMatrix){
    this->projectionMatrix = projectionMatrix;
}

void M_Line::setViewMatrix(glm::vec3 viewMatrix){
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_Line::setModelMatrix(glm::mat4 modelMatrix){
    this->modelMatrix = modelMatrix;
}

void M_Line::translate(glm::vec3 translation){
    this->vertexData[0].x += translation.x;
    this->vertexData[0].y += translation.y;
    this->vertexData[0].z += translation.z;
    this->vertexData[1].x += translation.x;
    this->vertexData[1].y += translation.y;
    this->vertexData[1].z += translation.z;
}

void M_Line::rotate(float angle, glm::vec3 axis){
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = {start.x, start.y, start.z, 1.f};
    this->vertexData[1] = {end.x, end.y, end.z, 1.f};
}

void M_Line::scale(glm::vec3 scale){
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the line
    this->vertexData[1].x *= avgScale;
    this->vertexData[1].y *= avgScale;
    this->vertexData[1].z *= avgScale;
}

void M_Line::uniformScale(float scale){
    this->vertexData[1].x *= scale;
    this->vertexData[1].y *= scale;
    this->vertexData[1].z *= scale;
}

void M_Line::rotateAbout(float angle, glm::vec3 point){
    glm::vec3 axis = point - glm::vec3(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = {start.x, start.y, start.z, 1.f};
    this->vertexData[1] = {end.x, end.y, end.z, 1.f};
}

void M_Line::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point){
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = {start.x, start.y, start.z, 1.f};
    this->vertexData[1] = {end.x, end.y, end.z, 1.f};
}

glm::vec3 M_Line::getCenterPoint(){
    return glm::vec3((this->vertexData[0].x + this->vertexData[1].x) / 2, (this->vertexData[0].y + this->vertexData[1].y) / 2, (this->vertexData[0].z + this->vertexData[1].z) / 2);
}

//////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////////////

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

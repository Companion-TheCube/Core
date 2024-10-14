// TODO: the font is being loaded for each instance of an M_Text object and should be made static so that it only gets loaded once and is shared between all instances of M_Text

#include "shapes.h"

void checkGLError(const std::string& location)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << location << ": " << error << std::endl;
    }
}
////////////////////////////////////////////////////////////////
FT_Library M_Text::ft;
FT_Face M_Text::face;
bool M_Text::faceInitialized = false;

M_Text::M_Text(Shader* sh, std::string text, float fontSize, glm::vec3 color, glm::vec2 position)
{
    this->shader = sh;
    this->text = text;
    this->fontSize = fontSize;
    this->color = color;
    this->position = position;
    this->projectionMatrix = glm::ortho(0.0f, 720.0f, 0.0f, 720.0f, -1.f, 1.f);
    this->buildText();
    GlobalSettings::setSettingCB("selectedFontPath", [&]() {
        this->reloadFace = true;
    });
    CubeLog::debug("Created Text: " + text);
}

void M_Text::reloadFont()
{
    CubeLog::debug("Reloading font");
    FT_Done_Face(M_Text::face);
    this->characters.clear();
    this->vertexData.clear();
    this->vertexData.shrink_to_fit();
    this->width = 0;
    M_Text::faceInitialized = false;
    // rebuild the text
    this->buildText();
    CubeLog::debug("Reloaded font");
}

void M_Text::buildText()
{
    if (!M_Text::faceInitialized) {
        if (FT_Init_FreeType(&M_Text::ft)) {
            CubeLog::error("ERROR::FREETYPE: Could not init FreeType Library");
        }
        std::string fontPath = GlobalSettings::selectedFontPath;
        if (!std::filesystem::exists(fontPath)) {
            CubeLog::error("ERROR::FREETYPE: Font file does not exist");
            fontPath = "fonts/Roboto/Roboto-Regular.ttf";
        }
        if (FT_New_Face(M_Text::ft, fontPath.c_str(), 0, &M_Text::face)) {
            CubeLog::error("ERROR::FREETYPE: Failed to load font");
        }
        M_Text::faceInitialized = true;
    }
    if (FT_Set_Pixel_Sizes(M_Text::face, 0, this->fontSize)) {
        CubeLog::error("ERROR::FREETYPE: Failed to set pixel size");
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    checkGLError("0.1");

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(M_Text::face, c, FT_LOAD_RENDER)) {
            CubeLog::error("ERROR::FREETYPE: Failed to load Glyph");
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        checkGLError("0.2");
        glBindTexture(GL_TEXTURE_2D, texture);
        checkGLError("0.3");
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            M_Text::face->glyph->bitmap.width,
            M_Text::face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            M_Text::face->glyph->bitmap.buffer);
        if (M_Text::face->glyph->bitmap.buffer != nullptr) {
            // std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX sample glyph buffer value: " + std::to_string(M_Text::face->glyph->bitmap.buffer[0]);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        checkGLError("0.4");
        Character newCharacter = {
            texture,
            glm::ivec2(M_Text::face->glyph->bitmap.width, M_Text::face->glyph->bitmap.rows),
            glm::ivec2(M_Text::face->glyph->bitmap_left, M_Text::face->glyph->bitmap_top),
            M_Text::face->glyph->advance.x
        };
        this->characters.insert(std::pair<char, Character>(c, newCharacter));
    }
    // This loop adds up the widths of each character in the text
    std::string::const_iterator c;
    for (c = this->text.begin(); c != this->text.end(); c++) {
        Character ch = this->characters[*c]; // Get the character from the map
        this->width += (ch.advance >> 6); // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError("0.5");
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    checkGLError("0.6");
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    checkGLError("0.7");
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // setProjectionMatrix(glm::ortho(0.0f, 720.f, 0.0f, 720.f));
    checkGLError("0.8");
    this->reloadFace = false;
}

M_Text::~M_Text()
{
    FT_Done_Face(M_Text::face);
    FT_Done_FreeType(M_Text::ft);
    // delete all the openGl stuff
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    for (unsigned char c = 0; c < 128; c++) {
        glDeleteTextures(1, &this->characters[c].textureID);
    }
    CubeLog::info("Destroyed Text");
}

void M_Text::draw()
{
    if (this->reloadFace) {
        reloadFont();
        this->reloadFace = false;
    }
    if (!this->faceInitialized) {
        return;
    }
    this->shader->use();
    shader->setVec3("textColor", this->color.x, this->color.y, this->color.z);
    shader->setMat4("projection", projectionMatrix);
    shader->setFloat("zindex", 0.0f);
    shader->setFloat("alpha", 1.0f);
    shader->setFloat("bg_alpha", 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(this->VAO);
    std::string::const_iterator c;
    float xTemp = this->position.x;
    for (c = this->text.begin(); c != this->text.end(); c++) {
        Character ch = this->characters[*c];
        float xpos = xTemp + ch.bearing.x;
        float ypos = this->position.y - (ch.size.y - ch.bearing.y);
        float w = ch.size.x;
        float h = ch.size.y;
        float vertices[6][4] = {
            // Positions            // Texture Coords
            { xpos, ypos + h, 0.0f, 0.0f }, // Top-left
            { xpos + w, ypos, 1.0f, 1.0f }, // Bottom-right
            { xpos, ypos, 0.0f, 1.0f }, // Bottom-left

            { xpos, ypos + h, 0.0f, 0.0f }, // Top-left
            { xpos + w, ypos + h, 1.0f, 0.0f }, // Top-right
            { xpos + w, ypos, 1.0f, 1.0f } // Bottom-right
        };
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        xTemp += (ch.advance >> 6);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void M_Text::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}

void M_Text::setViewMatrix(glm::vec3 viewMatrix)
{
    return; // do nothing
}

void M_Text::setViewMatrix(glm::mat4 viewMatrix)
{
    return; // do nothing
}

void M_Text::setModelMatrix(glm::mat4 modelMatrix)
{
    return; // do nothing
}

void M_Text::translate(glm::vec3 translation)
{
    this->position.x += translation.x;
    this->position.y += translation.y;
}

void M_Text::rotate(float angle, glm::vec3 axis)
{
    // TODO: Implement rotation of text
}

void M_Text::scale(glm::vec3 scale)
{
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the text
    this->fontSize *= avgScale;
}

void M_Text::uniformScale(float scale)
{
    this->fontSize *= scale;
}

void M_Text::rotateAbout(float angle, glm::vec3 point)
{
    glm::vec3 axis = point - glm::vec3(this->position.x, this->position.y, 0.f);
    rotateAbout(angle, axis, point);
}

void M_Text::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), glm::normalize(axis));
    tempMat = glm::translate(tempMat, -point);
    for (int i = 0; i < this->vertexData.size(); i++) {
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = { point.x, point.y, point.z, 1.f };
    }
}

glm::vec3 M_Text::getCenterPoint()
{
    return { this->position.x, this->position.y, 0.f };
}

std::vector<Vertex> M_Text::getVertices()
{
    std::vector<Vertex> vertices;
    return vertices;
}

void M_Text::setPosition(glm::vec2 position)
{
    this->position = position;
}

void M_Text::setText(std::string text)
{
    for (unsigned char c = 0; c < 128; c++) {
        glDeleteTextures(1, &this->characters[c].textureID);
    }
    this->text = text;
    this->buildText();
}

void M_Text::setColor(glm::vec3 color)
{
    this->color = color;
}

float M_Text::getWidth()
{
    return this->width;
}

void M_Text::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
    this->capturedPosition = this->position;
}

void M_Text::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
    this->position = this->capturedPosition;
}

void M_Text::setVisibility(bool visible)
{
    this->visible = visible;
}

void M_Text::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_Text::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 M_Text::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 M_Text::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////

M_PartCircle::M_PartCircle(Shader* sh, unsigned int numSegments, float radius, glm::vec3 centerPoint, float startAngle, float endAngle, glm::vec3 fillColor)
{
    this->shader = sh;
    this->numSegments = numSegments;
    this->radius = radius;
    this->centerPoint = centerPoint;
    this->startAngle = startAngle;
    this->endAngle = endAngle;
    // ensure start angle is greater than end angle
    if (this->startAngle < this->endAngle) {
        float temp = this->startAngle;
        this->startAngle = this->endAngle;
        this->endAngle = temp;
    }
    // push the center point as the first vertex
    this->vertexData.push_back({ centerPoint.x, centerPoint.y, centerPoint.z, fillColor.r, fillColor.g, fillColor.b });
    for (unsigned int i = 0; i <= this->numSegments; i++) {
        float theta = glm::radians(this->startAngle) + i * (glm::radians(this->endAngle) - glm::radians(this->startAngle)) / this->numSegments;
        float x = this->centerPoint.x + this->radius * cos(theta);
        float y = this->centerPoint.y + this->radius * sin(theta);
        float z = this->centerPoint.z;
        this->vertexData.push_back({ x, y, z, fillColor.r, fillColor.g, fillColor.b });
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
    CubeLog::info("Created PartCircle");
}

M_PartCircle::~M_PartCircle()
{
    CubeLog::info("Destroyed PartCircle");
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_PartCircle::draw()
{
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_PartCircle::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}

void M_PartCircle::setViewMatrix(glm::vec3 viewMatrix)
{
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_PartCircle::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void M_PartCircle::setModelMatrix(glm::mat4 modelMatrix)
{
    this->modelMatrix = modelMatrix;
}

void M_PartCircle::translate(glm::vec3 translation)
{
    this->modelMatrix = glm::translate(this->modelMatrix, translation);
}

void M_PartCircle::rotate(float angle, glm::vec3 axis)
{
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    for (int i = 0; i < this->vertexData.size(); i++) {
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = { point.x, point.y, point.z, 1.f };
    }
}

void M_PartCircle::scale(glm::vec3 scale)
{
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the circle
    for (int i = 0; i < this->vertexData.size(); i++) {
        this->vertexData[i].x *= avgScale;
        this->vertexData[i].y *= avgScale;
        this->vertexData[i].z *= avgScale;
    }
}

void M_PartCircle::uniformScale(float scale)
{
    for (int i = 0; i < this->vertexData.size(); i++) {
        this->vertexData[i].x *= scale;
        this->vertexData[i].y *= scale;
        this->vertexData[i].z *= scale;
    }
}

void M_PartCircle::rotateAbout(float angle, glm::vec3 point)
{
    glm::vec3 axis = point - glm::vec3(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for (int i = 0; i < this->vertexData.size(); i++) {
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = { point.x, point.y, point.z, 1.f };
    }
}

void M_PartCircle::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for (int i = 0; i < this->vertexData.size(); i++) {
        glm::vec4 point = tempMat * glm::vec4(this->vertexData[i].x, this->vertexData[i].y, this->vertexData[i].z, 1.0f);
        this->vertexData[i] = { point.x, point.y, point.z, 1.f };
    }
}

glm::vec3 M_PartCircle::getCenterPoint()
{
    return this->centerPoint;
}

std::vector<Vertex> M_PartCircle::getVertices()
{
    return this->vertexData;
}

float M_PartCircle::getWidth()
{
    return this->radius;
}

void M_PartCircle::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
}

void M_PartCircle::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
}

void M_PartCircle::setVisibility(bool visible)
{
    this->visible = visible;
}

void M_PartCircle::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_PartCircle::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 M_PartCircle::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 M_PartCircle::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////

unsigned char* createRadioButtonTexture(unsigned int size, unsigned int padding, bool selected)
{
    float overallWidthHeight = size + padding * 2.f;
    unsigned char* data = new unsigned char[(unsigned int)(overallWidthHeight * overallWidthHeight)];
    float center_x = overallWidthHeight / 2.0f;
    float center_y = overallWidthHeight / 2.0f;
    float circle_outer_radius = size / 2.0f;
    float circle_inner_radius = circle_outer_radius * RADIOBUTTON_INNER_OUTER_RATIO;
    float dot_radius = circle_inner_radius * RADIOBUTTON_DOT_INNER_RATIO;
    for (unsigned int y = 0; y < overallWidthHeight; ++y) {
        for (unsigned int x = 0; x < overallWidthHeight; ++x) {
            float dx = x - center_x;
            float dy = y - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);
            unsigned char pixel_color = 0;
            if (dist >= circle_inner_radius && dist <= circle_outer_radius)
                pixel_color = 255;
            else if (selected && dist < dot_radius)
                pixel_color = 255;
            data[(unsigned int)((float)y * overallWidthHeight + x)] = pixel_color;
            for (float i = -(size * 0.1f); i <= (size * 0.1f); i++) {
                for (float j = -(size * 0.1f); j <= (size * 0.1f); j++) {
                    unsigned int yIdx = y + i;
                    unsigned int xIdx = x + j;
                    if (yIdx < 0 || yIdx >= overallWidthHeight || xIdx < 0 || xIdx >= overallWidthHeight) {
                        continue;
                    }
                    if((((unsigned int)data[(unsigned int)((float)yIdx * overallWidthHeight + xIdx)]) + pixel_color / ((size * 0.2f) *(size * 0.2f))) > 255) {
                        data[(unsigned int)((float)yIdx * overallWidthHeight + xIdx)] = 255;
                    } else {
                        data[(unsigned int)((float)yIdx * overallWidthHeight + xIdx)] += pixel_color / ((size * 0.2f) * (size * 0.2f));
                    }
                }
            }
        }
    }
    return data;
}

M_RadioButtonTexture::M_RadioButtonTexture(Shader* sh, float radioSize, unsigned int padding, glm::vec3 color, glm::vec2 position)
{
    this->shader = sh;
    this->position = position;
    this->selected = false;
    this->radioSize = radioSize;
    this->padding = padding;
    this->color = color;
    this->scale_ = 1.0f;
    this->selectedTextureBitmap = createRadioButtonTexture(radioSize, padding, true);
    this->unselectedTextureBitmap = createRadioButtonTexture(radioSize, padding, false);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   
    glGenTextures(1, &this->textureSelected);
    glBindTexture(GL_TEXTURE_2D, this->textureSelected);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        radioSize + padding * 2,
        radioSize + padding * 2,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        this->selectedTextureBitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &this->textureUnselected);
    glBindTexture(GL_TEXTURE_2D, this->textureUnselected);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        radioSize + padding * 2,
        radioSize + padding * 2,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        this->unselectedTextureBitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    setProjectionMatrix(glm::ortho(0.0f, 720.f, 0.0f, 720.f, -1.f, 1.f));
    glBindTexture(GL_TEXTURE_2D, 0);
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    CubeLog::info("Created RadioButtonTexture");
}

M_RadioButtonTexture::~M_RadioButtonTexture()
{
    CubeLog::info("Destroyed RadioButtonTexture");
    glDeleteTextures(1, &this->textureSelected);
    glDeleteTextures(1, &this->textureUnselected);
}

void M_RadioButtonTexture::draw()
{
    this->shader->use();
    shader->setVec3("textColor", this->color.x, this->color.y, this->color.z);
    shader->setMat4("projection", projectionMatrix);
    shader->setFloat("zindex", 0.1f);
    shader->setFloat("alpha", 1.0f);
    shader->setFloat("bg_alpha", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    float xPos = this->position.x;
    float yPos = this->position.y - this->radioSize;
    float vertices[6][4] = {
        // Positions            // Texture Coords
        { xPos, yPos + this->radioSize, 0.0f, 0.0f }, // Top-left
        { xPos + this->radioSize, yPos, 1.0f, 1.0f }, // Bottom-right
        { xPos, yPos, 0.0f, 1.0f }, // Bottom-left

        { xPos, yPos + this->radioSize, 0.0f, 0.0f }, // Top-left
        { xPos + this->radioSize, yPos + this->radioSize, 1.0f, 0.0f }, // Top-right
        { xPos + this->radioSize, yPos, 1.0f, 1.0f } // Bottom-right
    };
    if (this->selected) {
        glBindTexture(GL_TEXTURE_2D, this->textureSelected);
    } else {
        glBindTexture(GL_TEXTURE_2D, this->textureUnselected);
    }
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void M_RadioButtonTexture::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}

void M_RadioButtonTexture::setViewMatrix(glm::vec3 viewMatrix)
{
    return; // do nothing
}

void M_RadioButtonTexture::setViewMatrix(glm::mat4 viewMatrix)
{
    return; // do nothing
}

void M_RadioButtonTexture::setModelMatrix(glm::mat4 modelMatrix)
{
    return; // do nothing
}

void M_RadioButtonTexture::translate(glm::vec3 translation)
{
    this->position.x += translation.x;
    this->position.y += translation.y;
}

void M_RadioButtonTexture::rotate(float angle, glm::vec3 axis)
{
    // do nothing
}

void M_RadioButtonTexture::scale(glm::vec3 scale)
{
    this->radioSize = this->radioSize * scale.x;
}

void M_RadioButtonTexture::uniformScale(float scale)
{
    this->scale_ *= scale;
}

void M_RadioButtonTexture::rotateAbout(float angle, glm::vec3 point)
{
    // do nothing
}

void M_RadioButtonTexture::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    // do nothing
}

glm::vec3 M_RadioButtonTexture::getCenterPoint()
{
    return { this->position.x, this->position.y, 0.f };
}

std::vector<Vertex> M_RadioButtonTexture::getVertices()
{
    std::vector<Vertex> vertices;
    return vertices;
}

void M_RadioButtonTexture::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
    this->capturedPosition = this->position;
}

void M_RadioButtonTexture::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
    this->position = this->capturedPosition;
}

void M_RadioButtonTexture::setVisibility(bool visible)
{
    this->visible = visible;
}

void M_RadioButtonTexture::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_RadioButtonTexture::getModelMatrix()
{
    return this->modelMatrix;
}

glm::mat4 M_RadioButtonTexture::getViewMatrix()
{
    return this->viewMatrix;
}

glm::mat4 M_RadioButtonTexture::getProjectionMatrix()
{
    return this->projectionMatrix;
}

void M_RadioButtonTexture::setSelected(bool selected)
{
    this->selected = selected;
}

void M_RadioButtonTexture::setPosition(glm::vec2 position)
{
    this->position = position;
}

float M_RadioButtonTexture::getWidth() { return this->radioSize; };

//////////////////////////////////////////////////////////////////////////////////////////

M_Rect::M_Rect(Shader* sh, glm::vec3 position, glm::vec2 size, float fillColor, float borderColor)
{
    this->shader = sh;
    this->vertexDataFill.push_back({ position.x + size.x, position.y, position.z, fillColor });
    this->vertexDataFill.push_back({ position.x, position.y, position.z, fillColor });
    this->vertexDataFill.push_back({ position.x + size.x, position.y + size.y, position.z, fillColor });
    this->vertexDataFill.push_back({ position.x, position.y + size.y, position.z, fillColor });
    this->vertexDataBorder.push_back({ position.x, position.y, position.z, borderColor });
    this->vertexDataBorder.push_back({ position.x + size.x, position.y, position.z, borderColor });
    this->vertexDataBorder.push_back({ position.x + size.x, position.y + size.y, position.z, borderColor });
    this->vertexDataBorder.push_back({ position.x, position.y + size.y, position.z, borderColor });
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
    CubeLog::info("Created Rect");
}

M_Rect::~M_Rect()
{
    CubeLog::info("Destroyed Rect");
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Rect::draw()
{
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

void M_Rect::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}

void M_Rect::setViewMatrix(glm::vec3 viewMatrix)
{
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_Rect::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void M_Rect::setModelMatrix(glm::mat4 modelMatrix)
{
    this->modelMatrix = modelMatrix;
}

void M_Rect::translate(glm::vec3 translation)
{
    // for (int i = 0; i < 4; i++) {
    //     this->vertexDataFill[i].x += translation.x;
    //     this->vertexDataFill[i].y += translation.y;
    //     this->vertexDataFill[i].z += translation.z;
    //     this->vertexDataBorder[i].x += translation.x;
    //     this->vertexDataBorder[i].y += translation.y;
    //     this->vertexDataBorder[i].z += translation.z;
    // }
    this->modelMatrix = glm::translate(this->modelMatrix, translation);
}

void M_Rect::rotate(float angle, glm::vec3 axis)
{
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    for (int i = 0; i < 4; i++) {
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = { fill.x, fill.y, fill.z, 1.f };
        this->vertexDataBorder[i] = { border.x, border.y, border.z, 1.f };
    }
}

void M_Rect::scale(glm::vec3 scale)
{
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the rectangle
    for (int i = 0; i < 4; i++) {
        this->vertexDataFill[i].x *= avgScale;
        this->vertexDataFill[i].y *= avgScale;
        this->vertexDataFill[i].z *= avgScale;
        this->vertexDataBorder[i].x *= avgScale;
        this->vertexDataBorder[i].y *= avgScale;
        this->vertexDataBorder[i].z *= avgScale;
    }
}

void M_Rect::uniformScale(float scale)
{
    for (int i = 0; i < 4; i++) {
        this->vertexDataFill[i].x *= scale;
        this->vertexDataFill[i].y *= scale;
        this->vertexDataFill[i].z *= scale;
        this->vertexDataBorder[i].x *= scale;
        this->vertexDataBorder[i].y *= scale;
        this->vertexDataBorder[i].z *= scale;
    }
}

void M_Rect::rotateAbout(float angle, glm::vec3 point)
{
    glm::vec3 axis = point - glm::vec3(this->vertexDataFill[0].x, this->vertexDataFill[0].y, this->vertexDataFill[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for (int i = 0; i < 4; i++) {
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = { fill.x, fill.y, fill.z, 1.f };
        this->vertexDataBorder[i] = { border.x, border.y, border.z, 1.f };
    }
}

void M_Rect::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    for (int i = 0; i < 4; i++) {
        glm::vec4 fill = tempMat * glm::vec4(this->vertexDataFill[i].x, this->vertexDataFill[i].y, this->vertexDataFill[i].z, 1.0f);
        glm::vec4 border = tempMat * glm::vec4(this->vertexDataBorder[i].x, this->vertexDataBorder[i].y, this->vertexDataBorder[i].z, 1.0f);
        this->vertexDataFill[i] = { fill.x, fill.y, fill.z, 1.f };
        this->vertexDataBorder[i] = { border.x, border.y, border.z, 1.f };
    }
}

glm::vec3 M_Rect::getCenterPoint()
{
    return glm::vec3((this->vertexDataFill[0].x + this->vertexDataFill[2].x) / 2, (this->vertexDataFill[0].y + this->vertexDataFill[2].y) / 2, (this->vertexDataFill[0].z + this->vertexDataFill[2].z) / 2);
}

std::vector<Vertex> M_Rect::getVertices()
{
    return this->vertexDataFill;
}

float M_Rect::getWidth()
{
    return this->vertexDataFill[0].x - this->vertexDataFill[1].x;
}

void M_Rect::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
}

void M_Rect::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
}

void M_Rect::setVisibility(bool visible)
{
    this->visible = visible;
}

void M_Rect::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_Rect::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 M_Rect::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 M_Rect::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////

M_Line::M_Line(Shader* sh, glm::vec3 start, glm::vec3 end)
{
    this->shader = sh;
    this->vertexData.push_back({ start.x, start.y, start.z, 1.f });
    this->vertexData.push_back({ end.x, end.y, end.z, 1.f });
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
    CubeLog::info("Created Line");
}

M_Line::~M_Line()
{
    CubeLog::info("Destroyed Line");
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Line::draw()
{
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_LINES, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_Line::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}

void M_Line::setViewMatrix(glm::vec3 viewMatrix)
{
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_Line::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void M_Line::setModelMatrix(glm::mat4 modelMatrix)
{
    this->modelMatrix = modelMatrix;
}

void M_Line::translate(glm::vec3 translation)
{
    // this->vertexData[0].x += translation.x;
    // this->vertexData[0].y += translation.y;
    // this->vertexData[0].z += translation.z;
    // this->vertexData[1].x += translation.x;
    // this->vertexData[1].y += translation.y;
    // this->vertexData[1].z += translation.z;
    // update the matrixes
    this->modelMatrix = glm::translate(this->modelMatrix, translation);
}

void M_Line::rotate(float angle, glm::vec3 axis)
{
    glm::mat4 tempMat = glm::mat4(1.0f);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = { start.x, start.y, start.z, 1.f };
    this->vertexData[1] = { end.x, end.y, end.z, 1.f };
    this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(angle), axis);
}

void M_Line::scale(glm::vec3 scale)
{
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the line
    this->vertexData[1].x *= avgScale;
    this->vertexData[1].y *= avgScale;
    this->vertexData[1].z *= avgScale;
    this->modelMatrix = glm::scale(this->modelMatrix, scale);
}

void M_Line::uniformScale(float scale)
{
    this->vertexData[1].x *= scale;
    this->vertexData[1].y *= scale;
    this->vertexData[1].z *= scale;
    this->modelMatrix = glm::scale(this->modelMatrix, glm::vec3(scale, scale, scale));
}

void M_Line::rotateAbout(float angle, glm::vec3 point)
{
    glm::vec3 axis = point - glm::vec3(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z);
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = { start.x, start.y, start.z, 1.f };
    this->vertexData[1] = { end.x, end.y, end.z, 1.f };
    this->modelMatrix = glm::translate(this->modelMatrix, point);
    this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(angle), axis);
    this->modelMatrix = glm::translate(this->modelMatrix, -point);
}

void M_Line::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);
    glm::vec4 start = tempMat * glm::vec4(this->vertexData[0].x, this->vertexData[0].y, this->vertexData[0].z, 1.0f);
    glm::vec4 end = tempMat * glm::vec4(this->vertexData[1].x, this->vertexData[1].y, this->vertexData[1].z, 1.0f);
    this->vertexData[0] = { start.x, start.y, start.z, 1.f };
    this->vertexData[1] = { end.x, end.y, end.z, 1.f };
    this->modelMatrix = glm::translate(this->modelMatrix, point);
    this->modelMatrix = glm::rotate(this->modelMatrix, glm::radians(angle), axis);
    this->modelMatrix = glm::translate(this->modelMatrix, -point);
}

glm::vec3 M_Line::getCenterPoint()
{
    return glm::vec3((this->vertexData[0].x + this->vertexData[1].x) / 2, (this->vertexData[0].y + this->vertexData[1].y) / 2, (this->vertexData[0].z + this->vertexData[1].z) / 2);
}

std::vector<Vertex> M_Line::getVertices()
{
    return this->vertexData;
}

float M_Line::getWidth()
{
    return this->vertexData[0].x - this->vertexData[1].x;
}

void M_Line::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
}

void M_Line::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
}

void M_Line::setVisibility(bool visibility)
{
    this->visible = visibility;
}

void M_Line::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_Line::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 M_Line::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 M_Line::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////

M_Arc::M_Arc(Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint, glm::vec3 fillColor)
    : M_Arc(sh, numSegments, radius, startAngle, endAngle, centerPoint)
{
    this->fillColor = fillColor;
}

M_Arc::M_Arc(Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint)
{
    this->shader = sh;
    this->numSegments = numSegments;
    this->radius = radius;
    this->startAngle = startAngle;
    this->endAngle = endAngle;
    this->centerPoint = centerPoint;
    for (unsigned int i = 0; i <= this->numSegments; i++) {
        float theta = glm::radians(this->startAngle) + i * (glm::radians(this->endAngle) - glm::radians(this->startAngle)) / this->numSegments;
        float x = this->centerPoint.x + this->radius * cos(theta);
        float y = this->centerPoint.y + this->radius * sin(theta);
        float z = this->centerPoint.z;
        this->vertexData.push_back({ x, y, z, this->fillColor.r, this->fillColor.g, this->fillColor.b });
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
    CubeLog::info("Created Arc");
}

M_Arc::~M_Arc()
{
    CubeLog::info("Destroyed Arc");
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);
}

void M_Arc::draw()
{
    this->shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO[0]);
    // glDrawElements(GL_LINE_STRIP, this->vertexData.size(), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_LINE_STRIP, 0, this->vertexData.size());
    glBindVertexArray(0);
}

void M_Arc::setProjectionMatrix(glm::mat4 projectionMatrix)
{
    this->projectionMatrix = projectionMatrix;
}
void M_Arc::setViewMatrix(glm::vec3 viewMatrix)
{
    this->viewMatrix = glm::lookAt(viewMatrix, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void M_Arc::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void M_Arc::setModelMatrix(glm::mat4 modelMatrix)
{
    this->modelMatrix = modelMatrix;
}
void M_Arc::translate(glm::vec3 translation)
{
    this->centerPoint += translation;
    this->modelMatrix = glm::translate(this->modelMatrix, translation);
}

void M_Arc::rotate(float angle, glm::vec3 axis)
{
    this->startAngle += angle;
    this->endAngle += angle;
}

void M_Arc::scale(glm::vec3 scale)
{
    // normalize scale
    glm::vec3 normalizedScale = glm::normalize(scale);
    // get the average of the scale
    float avgScale = (normalizedScale.x + normalizedScale.y + normalizedScale.z) / 3;
    // scale the radius
    this->radius *= avgScale;
}

void M_Arc::uniformScale(float scale)
{
    this->radius *= scale;
}

void M_Arc::rotateAbout(float angle, glm::vec3 point) { }
void M_Arc::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point) { }
glm::vec3 M_Arc::getCenterPoint()
{
    return this->centerPoint;
}

std::vector<Vertex> M_Arc::getVertices()
{
    return this->vertexData;
}

float M_Arc::getWidth()
{
    return this->radius;
}

void M_Arc::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
}

void M_Arc::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
}

void M_Arc::setVisibility(bool visibility)
{
    this->visible = visibility;
}

void M_Arc::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 M_Arc::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 M_Arc::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 M_Arc::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////

Cube::Cube(Shader* sh)
{
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
    for (int i = 0; i < 6; i++) {
        face[0] = edgeVertices[i * 4];
        face[1] = edgeVertices[i * 4 + 1];
        face[2] = edgeVertices[i * 4 + 2];
        face[3] = edgeVertices[i * 4 + 3];
        glBindVertexArray(VAOs[i + 1]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[i + 1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(face) * 12, face, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i + 1]);
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

void Cube::draw()
{
    shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    // glDepthMask(GL_FALSE);
    glBindVertexArray(VAOs[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    for (int i = 1; i <= 6; i++) {
        glBindVertexArray(VAOs[i]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    // glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void Cube::setProjectionMatrix(glm::mat4 projection)
{
    this->projectionMatrix = projection;
}

void Cube::setViewMatrix(glm::vec3 view)
{
    this->viewMatrix = glm::lookAt(view, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Cube::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void Cube::setModelMatrix(glm::mat4 model)
{
    this->modelMatrix = model;
}

void Cube::rotate(float angle, glm::vec3 axis)
{
    modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), axis);
}

void Cube::translate(glm::vec3 translation)
{
    modelMatrix = glm::translate(modelMatrix, translation);
}

void Cube::scale(glm::vec3 scale)
{
    // adjust scale vectors to avoid scaling to zero and modify them to make sure the cube is not deformed
    if (scale.x == 0.0f)
        scale.x = 0.01f;
    if (scale.y == 0.0f)
        scale.y = 0.01f;
    if (scale.z == 0.0f)
        scale.z = 0.01f;

    modelMatrix = glm::scale(modelMatrix, scale);
}

Cube::~Cube()
{
    glDeleteVertexArrays(7, VAOs);
    glDeleteBuffers(7, VBOs);
    glDeleteBuffers(7, EBOs);
}

void Cube::uniformScale(float scale)
{
    this->scale(glm::vec3(scale, scale, scale));
}

void Cube::rotateAbout(float angle, glm::vec3 point)
{
    glm::mat4 originalModelMatrix = modelMatrix; // Save the original model matrix
    modelMatrix = glm::mat4(1.0f); // Reset model matrix to identity

    glm::vec3 axis = glm::normalize(point - getCenterPoint()); // Normalize the axis
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);

    modelMatrix = tempMat * originalModelMatrix; // Apply the rotation to the original matrix
}

void Cube::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    glm::mat4 originalModelMatrix = modelMatrix; // Save the original model matrix
    modelMatrix = glm::mat4(1.0f); // Reset model matrix to identity

    glm::vec3 normalizedAxis = glm::normalize(axis); // Normalize the axis
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), normalizedAxis);
    tempMat = glm::translate(tempMat, -point);

    modelMatrix = tempMat * originalModelMatrix; // Apply the rotation to the original matrix
}

glm::vec3 Cube::getCenterPoint()
{
    // get the center of the cube form the model matrix
    glm::vec4 center = modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return glm::vec3(center.x, center.y, center.z);
}

std::vector<Vertex> Cube::getVertices()
{
    std::vector<Vertex> vertices;
    for (int i = 0; i < 8; i++) {
        vertices.push_back(cubeVertices[i]);
    }
    return vertices;
}

float Cube::getWidth()
{
    return cubeVertices[0].x - cubeVertices[1].x;
}

void Cube::capturePosition()
{
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
}

void Cube::restorePosition()
{
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
}

void Cube::setVisibility(bool visible)
{
    this->visible = visible;
}

void Cube::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 Cube::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 Cube::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 Cube::getProjectionMatrix()
{
    return this->projectionMatrix;
}
//////////////////////////////////////////////////////////////////////////////////////////

OBJObject::OBJObject(Shader* sh, std::vector<Vertex> vertices)
{
    this->shader = sh;
    this->vertexData = vertices;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, this->vertexData.size() * sizeof(Vertex), &this->vertexData[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    setProjectionMatrix(glm::perspective(glm::radians(45.0f), 720.0f / 720.0f, 0.1f, 100.0f));
    setViewMatrix(glm::vec3(0.0f, 0.0f, 6.0f));
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    // modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(1.f, 1.f, 1.f));
    // modelMatrix = glm::rotate(modelMatrix, glm::radians(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
    setModelMatrix(modelMatrix);
    CubeLog::info("Created OBJObject");
}

void OBJObject::draw()
{
    if (!visible)
        return;
    this->mutex.lock();
    shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, this->vertexData.size());
    glBindVertexArray(0);
    this->mutex.unlock();
}

void OBJObject::setProjectionMatrix(glm::mat4 projection)
{
    this->mutex.lock();
    this->projectionMatrix = projection;
    this->mutex.unlock();
}

void OBJObject::setViewMatrix(glm::vec3 view)
{
    this->mutex.lock();
    this->viewMatrix = glm::lookAt(view, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    this->mutex.unlock();
}

void OBJObject::setViewMatrix(glm::mat4 viewMatrix)
{
    this->mutex.lock();
    this->viewMatrix = viewMatrix;
    this->mutex.unlock();
}

void OBJObject::setModelMatrix(glm::mat4 model)
{
    this->mutex.lock();
    this->modelMatrix = model;
    this->mutex.unlock();
}

void OBJObject::rotate(float angle, glm::vec3 axis)
{
    this->mutex.lock();
    modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), axis);
    this->mutex.unlock();
}

void OBJObject::translate(glm::vec3 translation)
{
    this->mutex.lock();
    modelMatrix = glm::translate(modelMatrix, translation);
    this->mutex.unlock();
}

void OBJObject::scale(glm::vec3 scale)
{
    this->mutex.lock();
    modelMatrix = glm::scale(modelMatrix, scale);
    this->mutex.unlock();
}

OBJObject::~OBJObject()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void OBJObject::uniformScale(float scale)
{
    this->scale(glm::vec3(scale, scale, scale));
}

void OBJObject::rotateAbout(float angle, glm::vec3 point)
{
    this->mutex.lock();
    glm::mat4 originalModelMatrix = modelMatrix; // Save the original model matrix
    modelMatrix = glm::mat4(1.0f); // Reset model matrix to identity

    glm::vec3 axis = glm::normalize(point - getCenterPoint()); // Normalize the axis
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), axis);
    tempMat = glm::translate(tempMat, -point);

    modelMatrix = tempMat * originalModelMatrix; // Apply the rotation to the original matrix
    this->mutex.unlock();
}

void OBJObject::rotateAbout(float angle, glm::vec3 axis, glm::vec3 point)
{
    this->mutex.lock();
    glm::mat4 originalModelMatrix = modelMatrix; // Save the original model matrix
    modelMatrix = glm::mat4(1.0f); // Reset model matrix to identity

    glm::vec3 normalizedAxis = glm::normalize(axis); // Normalize the axis
    glm::mat4 tempMat = glm::mat4(1.0f); // Start with an identity matrix
    tempMat = glm::translate(tempMat, point);
    tempMat = glm::rotate(tempMat, glm::radians(angle), normalizedAxis);
    tempMat = glm::translate(tempMat, -point);

    modelMatrix = tempMat * originalModelMatrix; // Apply the rotation to the original matrix
    this->mutex.unlock();
}

glm::vec3 OBJObject::getCenterPoint()
{
    // get the center of the object by parsing the vertex data
    float x = 0, y = 0, z = 0;
    for (int i = 0; i < this->vertexData.size(); i++) {
        x += this->vertexData[i].x;
        y += this->vertexData[i].y;
        z += this->vertexData[i].z;
    }
    x /= this->vertexData.size();
    y /= this->vertexData.size();
    z /= this->vertexData.size();

    // move this point to incorporate the model matrix
    glm::vec4 center = modelMatrix * glm::vec4(x, y, z, 1.0f);
    return glm::vec3(center.x, center.y, center.z);
}

std::vector<Vertex> OBJObject::getVertices()
{
    return this->vertexData;
}

float OBJObject::getWidth()
{
    return this->vertexData[0].x - this->vertexData[1].x;
}

void OBJObject::capturePosition()
{
    this->mutex.lock();
    this->capturedModelMatrix = this->modelMatrix;
    this->capturedViewMatrix = this->viewMatrix;
    this->capturedProjectionMatrix = this->projectionMatrix;
    this->mutex.unlock();
}

void OBJObject::restorePosition()
{
    this->mutex.lock();
    this->modelMatrix = this->capturedModelMatrix;
    this->viewMatrix = this->capturedViewMatrix;
    this->projectionMatrix = this->capturedProjectionMatrix;
    this->mutex.unlock();
}

void OBJObject::setVisibility(bool visible)
{
    this->visible = visible;
}

void OBJObject::getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix)
{
    this->mutex.lock();
    *modelMatrix = this->capturedModelMatrix - this->modelMatrix;
    *viewMatrix = this->capturedViewMatrix - this->viewMatrix;
    *projectionMatrix = this->capturedProjectionMatrix - this->projectionMatrix;
    this->mutex.unlock();
}

glm::mat4 OBJObject::getModelMatrix()
{
    return this->modelMatrix;
}
glm::mat4 OBJObject::getViewMatrix()
{
    return this->viewMatrix;
}
glm::mat4 OBJObject::getProjectionMatrix()
{
    return this->projectionMatrix;
}

//////////////////////////////////////////////////////////////////////////////////////////
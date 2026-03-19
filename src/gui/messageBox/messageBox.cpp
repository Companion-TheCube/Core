/*
███╗   ███╗███████╗███████╗███████╗ █████╗  ██████╗ ███████╗██████╗  ██████╗ ██╗  ██╗    ██████╗██████╗ ██████╗ 
████╗ ████║██╔════╝██╔════╝██╔════╝██╔══██╗██╔════╝ ██╔════╝██╔══██╗██╔═══██╗╚██╗██╔╝   ██╔════╝██╔══██╗██╔══██╗
██╔████╔██║█████╗  ███████╗███████╗███████║██║  ███╗█████╗  ██████╔╝██║   ██║ ╚███╔╝    ██║     ██████╔╝██████╔╝
██║╚██╔╝██║██╔══╝  ╚════██║╚════██║██╔══██║██║   ██║██╔══╝  ██╔══██╗██║   ██║ ██╔██╗    ██║     ██╔═══╝ ██╔═══╝ 
██║ ╚═╝ ██║███████╗███████║███████║██║  ██║╚██████╔╝███████╗██████╔╝╚██████╔╝██╔╝ ██╗██╗╚██████╗██║     ██║     
╚═╝     ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// TODO: make a "Do you want to authorize this app? Yes/No" message box
// TODO: add auto wrapping of text in message box
// TODO: close message bos when clicked / touched
// TODO: disable menu click listener when message box is visible

#include "messageBox.h"

/**
 * @brief Construct a new Cube Message Box object
 *
 * @param shader - shader for the box
 * @param textShader - shader for the text
 * @param renderer - reference to the renderer
 * @param latch - a latch to synchronize the setup of the message box
 */
CubeMessageBox::CubeMessageBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch)
{
    this->shader = shader;
    this->textShader = textShader;
    this->visible = false;
    this->latch = &latch;
    this->renderer = renderer;
    CubeLog::info("MessageBox initialized");
    this->callback = [&]() { return; };
    // Initialize sensible defaults so text renders in the box
    // The box geometry in setup() currently draws a ~1x1 area centered using normalized coords
    // Map that roughly to pixel coordinates for text placement: position (252,252), size (432,432)
    this->position = { 252, 252 };
    this->size = { 432, 432 };
    // Initialize a reasonable clickable area matching the pixel box
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = nullptr;
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

/**
 * @brief Destroy the Cube Message Box object
 *
 */
CubeMessageBox::~CubeMessageBox()
{
    // TODO: Manual delete across message-box object arrays is brittle because ownership is spread across setup/update paths; convert these collections to std::unique_ptr-owned renderables.
    for (auto object : this->objects) {
        delete object;
    }
    for (auto object : this->textObjects) {
        delete object;
    }
    CubeLog::info("MessageBox destroyed");
}

/**
 * @brief Setup the message box
 *
 */
void CubeMessageBox::setup()
{

    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = -0.3;
    float yStart = -0.3;
    glm::vec2 size_ = { 1.2f, 1.2f };
    // TODO: make the outline more like a speech bubble
    // TODO: the character to needs to be aligned with the box in a way that makes it look like a speech bubble
    // TODO: This setup builds the widget tree with raw new() calls and relies on later manual cleanup; use std::unique_ptr-backed storage so partial setup and exceptions cannot leak UI objects.
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, { size_.x - diameter, size_.y - diameter }, 0.0, 0.0)); // main box
    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(shader, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // bottom
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 0, 90, { 0.f, 0.f, 0.f })); // top right
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 90, 180, { 0.f, 0.f, 0.f })); // top left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, 180, 270, { 0.f, 0.f, 0.f })); // bottom left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, 270, 360, { 0.f, 0.f, 0.f })); // bottom right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index })); // top
    this->objects.push_back(new M_Line(shader, { xStart + size_.x, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart, Z_DISTANCE + 0.001 + this->index })); // bottom
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // left
    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top right
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom right
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom left
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top left
    std::lock_guard<std::mutex> lock(this->mutex);
    this->latch->count_down();
    CubeLog::info("MessageBox setup");
}

/**
 * @brief Set the text of the message box. This adds a lambda to the renderer's setup tasks to set the text.
 *
 * @param text - the text to display
 */
void CubeMessageBox::setText(const std::string& text, const std::string& title)
{
    auto fn = [&, text, title]() {
        CubeLog::info("Objects size: " + std::to_string(this->objects.size()));
        CubeLog::info("textObjects size: " + std::to_string(this->textObjects.size()));
        CubeLog::info("Objects size in memory: " + std::to_string(this->objects.size() * sizeof(MeshObject)));
        // TODO: Clearing and rebuilding textObjects with manual delete/new is fragile because ownership crosses async renderer tasks; store text objects as std::unique_ptr and replace the vector atomically.
        for (size_t index = 0; index < this->textObjects.size(); index++) {
            delete this->textObjects[index];
        }
        this->textObjects.clear();
        auto titleText = new M_Text(textShader, title, (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT), { 1.f, 1.f, 1.f }, { 0.f, 0.f });
        float titleWidth = titleText->getWidth();
        float titleX = this->position.x + (this->size.x - titleWidth) / 2.f;
        float titleY = (this->position.y + this->size.y) - this->messageTextSize - STENCIL_INSET_PX;
        titleText->setPosition({ titleX, titleY });
        this->textObjects.push_back(titleText);
        std::vector<std::string> lines;
        size_t maxCharsPerLine = (size_t)((this->size.x - (2 * STENCIL_INSET_PX)) / (this->messageTextSize * 0.55f));
        std::istringstream textStream(text);
        std::string paragraph;
        while (std::getline(textStream, paragraph)) {
            if (paragraph.empty()) {
                lines.push_back("");
                continue;
            }
            std::istringstream wordStream(paragraph);
            std::string word;
            std::string current;
            while (wordStream >> word) {
                if (current.empty()) {
                    current = word;
                } else if ((current.size() + 1 + word.size()) <= maxCharsPerLine) {
                    current += " " + word;
                } else {
                    lines.push_back(current);
                    current = word;
                }
            }
            if (!current.empty())
                lines.push_back(current);
        }
        if (lines.size() == 0)
            lines.push_back(text);
        for (size_t i = 0; i < lines.size(); i++) {
            float shiftForPreviousLines = ((float)(i + 1) * this->messageTextSize) + (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT);
            float shiftForMargin = (float)(i + 1) * MESSAGEBOX_LINE_SPACING * this->messageTextSize;
            this->textObjects.push_back(new M_Text(textShader, lines[i], this->messageTextSize, { 1.f, 1.f, 1.f }, { this->position.x + STENCIL_INSET_PX, (this->position.y + this->size.y) - STENCIL_INSET_PX - shiftForPreviousLines - shiftForMargin }));
        }
        CubeLog::info("TextBox text set");
    };
    this->renderer->addSetupTask(fn);
}

/**
 * @brief Set the visibility of the message box
 *
 * @param visible - the visibility of the message box
 * @return bool - the previous visibility of the message box
 */
bool CubeMessageBox::setVisible(bool visible)
{
    CubeLog::debug("MessageBox visibility set to " + std::to_string(visible));
    bool temp = this->visible;
    this->visible = visible;
    if (this->callback != nullptr)
        if (this->visible)
            this->callback();
    return temp;
}

/**
 * @brief Get the visibility of the message box
 *
 * @return bool - the visibility of the message box
 */
bool CubeMessageBox::getVisible()
{
    return this->visible;
}

/**
 * @brief Draw the message box. This function should be called by the renderer thread.
 *
 */
void CubeMessageBox::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
    for (auto object : this->textObjects) {
        object->draw();
    }
}

/**
 * @brief Set the position of the message box
 *
 * @param position - the position to set the message box to
 */
void CubeMessageBox::setPosition(glm::vec2 position)
{
    this->position = position;
    // keep clickable area in sync
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

/**
 * @brief Set the size of the message box
 *
 * @param size - the size to set the message box to
 */
void CubeMessageBox::setSize(glm::vec2 size)
{
    this->size = size;
    // keep clickable area in sync
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

/**
 * @brief Get the objects that make up the message box
 *
 * @return std::vector<MeshObject*> - the objects of the message box
 */
std::vector<MeshObject*> CubeMessageBox::getObjects()
{
    return this->objects;
}

void CubeMessageBox::setCallback(std::function<void()> callback)
{
    this->callback = callback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CubeTextBox::CubeTextBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch)
{
    this->visible = false;
    this->shader = shader;
    this->textShader = textShader;
    this->visible = false;
    this->latch = &latch;
    this->renderer = renderer;
    this->position = { 0, 0 };
    this->size = { 720, 720 };
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = nullptr;
    this->clickArea.xMin = 0;
    this->clickArea.xMax = 720;
    this->clickArea.yMin = 0;
    this->clickArea.yMax = 720;
    CubeLog::info("TextBox initialized");
    this->callback = [&]() { return; };
}

CubeTextBox::~CubeTextBox()
{
    for (auto object : this->objects) {
        delete object;
    }
    for (auto object : this->textObjects) {
        delete object;
    }
    CubeLog::info("TextBox destroyed");
}

void CubeTextBox::setup()
{
    if (!this->needsSetup)
        return;
    this->needsSetup = false;
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = mapRange(this->position.x, 0.f, 720.f, -1.f, 1.f);
    float yStart = mapRange(this->position.y, 0.f, 720.f, -1.f, 1.f);
    float xSize = mapRange(this->size.x, 0.f, 720.f, 0.f, 2.f);
    float ySize = mapRange(this->size.y - 1, 0.f, 720.f, 0.f, 2.f);
    glm::vec2 size_ = { xSize, ySize };
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, { size_.x - diameter, size_.y - diameter }, 0.0, 0.0)); // main box
    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(shader, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // bottom
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 0, 90, { 0.f, 0.f, 0.f })); // top right
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 90, 180, { 0.f, 0.f, 0.f })); // top left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, 180, 270, { 0.f, 0.f, 0.f })); // bottom left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, 270, 360, { 0.f, 0.f, 0.f })); // bottom right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index })); // top
    this->objects.push_back(new M_Line(shader, { xStart + size_.x, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart, Z_DISTANCE + 0.001 + this->index })); // bottom
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // left
    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top right
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom right
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom left
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top left
    textMeshCount = this->objects.size();
    std::lock_guard<std::mutex> lock(this->mutex);
    this->latch->count_down();
    CubeLog::info("TextBox setup");
}

bool CubeTextBox::setVisible(bool visible)
{
    CubeLog::debug("TextBox visibility set to " + std::to_string(visible));
    bool temp = this->visible;
    this->visible = visible;
    if (this->callback != nullptr)
        if (!this->visible)
            this->callback();
    return temp;
}

bool CubeTextBox::getVisible()
{
    return this->visible;
}

void CubeTextBox::draw()
{
    if (this->needsSetup) {
        this->renderer->addSetupTask([&]() { this->setup(); });
        return;
    }
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
    for (auto object : this->textObjects) {
        object->draw();
    }
}

void CubeTextBox::setPosition(glm::vec2 position)
{
    this->needsSetup = true;
    this->position = position;
    for (auto object : this->objects) {
        delete object;
    }
    this->objects.clear();
    this->index = 0.001;
}

void CubeTextBox::setSize(glm::vec2 size)
{
    this->needsSetup = true;
    this->size = size;
    for (auto object : this->objects) {
        delete object;
    }
    this->objects.clear();
    this->index = 0.001;
}

void CubeTextBox::setTextSize(float size)
{
    this->messageTextSize = size;
}

std::vector<MeshObject*> CubeTextBox::getObjects()
{
    // concat objects and textObjects
    std::vector<MeshObject*> allObjects;
    allObjects.insert(allObjects.end(), this->objects.begin(), this->objects.end());
    allObjects.insert(allObjects.end(), this->textObjects.begin(), this->textObjects.end());
    return allObjects;
}

void CubeTextBox::setCallback(std::function<void()> callback)
{
    this->callback = callback;
}

void CubeTextBox::setText(const std::string& text, const std::string& title)
{
    auto fn = [&, text, title]() {
        CubeLog::info("Objects size: " + std::to_string(this->objects.size()));
        CubeLog::info("textObjects size: " + std::to_string(this->textObjects.size()));
        CubeLog::info("Objects size in memory: " + std::to_string(this->objects.size() * sizeof(MeshObject)));
        for (size_t index = 0; index < this->textObjects.size(); index++) {
            delete this->textObjects[index];
        }
        this->textObjects.clear();
        this->textObjects.push_back(new M_Text(textShader, title, (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT), { 1.f, 1.f, 1.f }, { this->position.x + STENCIL_INSET_PX, (this->position.y + this->size.y) - this->messageTextSize - STENCIL_INSET_PX }));
        std::vector<std::string> lines;
        std::string line;
        std::istringstream textStream(text);
        while (std::getline(textStream, line)) {
            lines.push_back(line);
        }
        if (lines.size() == 0)
            lines.push_back(text);
        if (lines[0].size() == 0)
            lines[0] = text;
        for (size_t i = 0; i < lines.size(); i++) {
            float shiftForPreviousLines = ((float)(i + 1) * this->messageTextSize) + (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT);
            float shiftForMargin = (float)(i + 1) * MESSAGEBOX_LINE_SPACING * this->messageTextSize;
            this->textObjects.push_back(new M_Text(textShader, lines[i], this->messageTextSize, { 1.f, 1.f, 1.f }, { this->position.x + STENCIL_INSET_PX, (this->position.y + this->size.y) - STENCIL_INSET_PX - shiftForPreviousLines - shiftForMargin }));
        }
        CubeLog::info("TextBox text set");
    };
    this->renderer->addSetupTask(fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CubeNotificaionBox::CubeNotificaionBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch)
{
    this->shader = shader;
    this->textShader = textShader;
    this->visible = false;
    this->latch = &latch;
    this->renderer = renderer;
    CubeLog::info("NotificationBox initialized");
    this->callbackYes = [&]() { return; };
    this->callbackNo = [&]() { return; };
    // Default geometry in pixel space
    this->position = { 144, 144 };
    this->size = { 432, 432 };
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = nullptr;
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

CubeNotificaionBox::~CubeNotificaionBox()
{
    for (auto object : this->objects) {
        delete object;
    }
    for (auto object : this->textObjects) {
        delete object;
    }
    CubeLog::info("NotificationBox destroyed");
}

void CubeNotificaionBox::setup()
{
    float radius = BOX_RADIUS;
    float diameter = radius * 2;
    float xStart = -0.2;
    float yStart = -0.2;
    glm::vec2 size_ = { 1.f, 1.f };
    // Fullscreen blackout backdrop to ensure content beneath is hidden
    this->objects.push_back(new M_Rect(shader, { -1.f, -1.f, Z_DISTANCE + this->index - 0.0005f }, { 2.f, 2.f }, 0.0, 0.0));
    // Main rounded box with outline
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, { size_.x - diameter, size_.y - diameter }, 0.0, 0.0)); // main box
    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // left
    this->objects.push_back(new M_Rect(shader, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, { radius, size_.y - diameter }, 0.0, 0.0)); // right
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // top
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, { size_.x - diameter, radius }, 0.0, 0.0)); // bottom
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 0, 90, { 0.f, 0.f, 0.f })); // top right
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + this->index }, 90, 180, { 0.f, 0.f, 0.f })); // top left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, Z_DISTANCE + this->index }, 180, 270, { 0.f, 0.f, 0.f })); // bottom left
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + this->index }, 270, 360, { 0.f, 0.f, 0.f })); // bottom right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart + size_.y, Z_DISTANCE + 0.001 + this->index })); // top
    this->objects.push_back(new M_Line(shader, { xStart + size_.x, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // right
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, Z_DISTANCE + 0.001 + this->index }, { xStart + size_.x - radius, yStart, Z_DISTANCE + 0.001 + this->index })); // bottom
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, Z_DISTANCE + 0.001 + this->index }, { xStart, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // left
    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size_.x - radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top right
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size_.x - radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom right
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, Z_DISTANCE + 0.001 + this->index })); // bottom left
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size_.y - radius, Z_DISTANCE + 0.001 + this->index })); // top left
    textMeshCount = this->objects.size();
    std::lock_guard<std::mutex> lock(this->mutex);
    this->latch->count_down();
    CubeLog::info("NotificationBox setup");
}

bool CubeNotificaionBox::setVisible(bool visible)
{
    CubeLog::debug("NotificationBox visibility set to " + std::to_string(visible));
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool CubeNotificaionBox::getVisible()
{
    return this->visible;
}

void CubeNotificaionBox::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
    for (auto object : this->textObjects) {
        object->draw();
    }
}

void CubeNotificaionBox::setPosition(glm::vec2 position)
{
    this->needsSetup = true;
    this->position = position;
    for (auto object : this->objects) {
        delete object;
    }
    this->objects.clear();
    this->index = 0.001;
    // keep clickable area in sync
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

void CubeNotificaionBox::setSize(glm::vec2 size)
{
    this->needsSetup = true;
    this->size = size;
    for (auto object : this->objects) {
        delete object;
    }
    this->objects.clear();
    this->index = 0.001;
    // keep clickable area in sync
    this->clickArea.xMin = (unsigned int)this->position.x;
    this->clickArea.xMax = (unsigned int)(this->position.x + this->size.x);
    this->clickArea.yMin = (unsigned int)this->position.y;
    this->clickArea.yMax = (unsigned int)(this->position.y + this->size.y);
}

void CubeNotificaionBox::setTextSize(float size)
{
    this->messageTextSize = size;
}

std::vector<MeshObject*> CubeNotificaionBox::getObjects()
{
    // concat objects and textObjects
    std::vector<MeshObject*> allObjects;
    allObjects.insert(allObjects.end(), this->objects.begin(), this->objects.end());
    allObjects.insert(allObjects.end(), this->textObjects.begin(), this->textObjects.end());
    return allObjects;
}

void CubeNotificaionBox::setCallbackNo(std::function<void()> callback)
{
    this->callbackNo = callback;
}

void CubeNotificaionBox::setCallbackYes(std::function<void()> callback)
{
    this->callbackYes = callback;
}

void CubeNotificaionBox::setText(const std::string& text, const std::string& title)
{
    // Precompute button rectangles synchronously so click handling works immediately
    unsigned int btnHeight = (unsigned int)(this->messageTextSize * 2.2f);
    unsigned int btnWidth = (unsigned int)((this->size.x - 3 * STENCIL_INSET_PX) / 2.f);
    unsigned int yMin = (unsigned int)(this->position.y + STENCIL_INSET_PX);
    unsigned int yMax = yMin + btnHeight;
    unsigned int xLeft = (unsigned int)(this->position.x + STENCIL_INSET_PX);
    unsigned int xRight = (unsigned int)(this->position.x + this->size.x - STENCIL_INSET_PX - btnWidth);
    this->yesBtn_ = { xLeft, xLeft + btnWidth, yMin, yMax };
    this->noBtn_ = { xRight, xRight + btnWidth, yMin, yMax };

    this->renderer->addSetupTask([&, text, title]() {
        // Clear old text
        // TODO: Notification text rebuilds manually delete and recreate heap objects inside a queued task, which is easy to race or leak; use std::unique_ptr and swap in a rebuilt object list.
        for (auto* t : this->textObjects) delete t;
        this->textObjects.clear();
        // Title centered
        auto titleText = new M_Text(textShader, title, (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT), { 1.f, 1.f, 1.f }, { 0.f, 0.f });
        float titleWidth = titleText->getWidth();
        float titleX = this->position.x + (this->size.x - titleWidth) / 2.f;
        float titleY = (this->position.y + this->size.y) - this->messageTextSize - STENCIL_INSET_PX;
        titleText->setPosition({ titleX, titleY });
        this->textObjects.push_back(titleText);
        // Body text with simple wrapping
        std::vector<std::string> lines;
        size_t maxCharsPerLine = (size_t)((this->size.x - (2 * STENCIL_INSET_PX)) / (this->messageTextSize * 0.55f));
        std::istringstream textStream(text);
        std::string paragraph;
        while (std::getline(textStream, paragraph)) {
            if (paragraph.empty()) { lines.push_back(""); continue; }
            std::istringstream wordStream(paragraph);
            std::string word; std::string current;
            while (wordStream >> word) {
                if (current.empty()) current = word;
                else if ((current.size() + 1 + word.size()) <= maxCharsPerLine) current += " " + word;
                else { lines.push_back(current); current = word; }
            }
            if (!current.empty()) lines.push_back(current);
        }
        if (lines.empty()) lines.push_back(text);
        for (size_t i = 0; i < lines.size(); ++i) {
            float shiftForPrevious = ((float)(i + 1) * this->messageTextSize) + (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT);
            float margin = (float)(i + 1) * MESSAGEBOX_LINE_SPACING * this->messageTextSize;
            this->textObjects.push_back(new M_Text(textShader, lines[i], this->messageTextSize, { 1.f, 1.f, 1.f }, { this->position.x + STENCIL_INSET_PX, (this->position.y + this->size.y) - STENCIL_INSET_PX - shiftForPrevious - margin }));
        }
        // Use precomputed buttons rects for label placement
        unsigned int btnHeight = this->yesBtn_.yMax - this->yesBtn_.yMin;
        unsigned int btnWidth = this->yesBtn_.xMax - this->yesBtn_.xMin;
        unsigned int xLeft = this->yesBtn_.xMin;
        unsigned int xRight = this->noBtn_.xMin;
        unsigned int yMin = this->yesBtn_.yMin;
        // Button labels
        auto yesText = new M_Text(textShader, std::string("Approve"), this->messageTextSize, { 0.1f, 1.f, 0.1f }, { 0.f, 0.f });
        float yW = yesText->getWidth();
        float yesX = xLeft + (btnWidth - yW) / 2.f;
        float yesY = yMin + (btnHeight - this->messageTextSize) / 2.f;
        yesText->setPosition({ yesX, yesY });
        this->textObjects.push_back(yesText);
        auto noText = new M_Text(textShader, std::string("Deny"), this->messageTextSize, { 1.f, 0.1f, 0.1f }, { 0.f, 0.f });
        float nW = noText->getWidth();
        float noX = xRight + (btnWidth - nW) / 2.f;
        float noY = yesY;
        noText->setPosition({ noX, noY });
        this->textObjects.push_back(noText);
        CubeLog::info("NotificationBox text set");
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CubeSliderBox::CubeSliderBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch)
{
    this->shader = shader;
    this->textShader = textShader;
    this->renderer = renderer;
    this->latch = &latch;
    this->visible = false;
    this->position = { 144, 144 };
    this->size = { 432, 432 };
    this->clickArea = ClickableArea();
    this->clickArea.clickableObject = nullptr;
    this->clickArea.xMin = 0;
    this->clickArea.xMax = 720;
    this->clickArea.yMin = 0;
    this->clickArea.yMax = 720;
    CubeLog::info("SliderBox initialized");
}

CubeSliderBox::~CubeSliderBox()
{
    clearTextObjects();
    clearObjects();
    CubeLog::info("SliderBox destroyed");
}

void CubeSliderBox::clearTextObjects()
{
    for (auto* object : this->textObjects) {
        delete object;
    }
    this->textObjects.clear();
}

void CubeSliderBox::clearObjects()
{
    for (auto* object : this->objects) {
        delete object;
    }
    this->objects.clear();
    this->sliderObject = nullptr;
}

int CubeSliderBox::clampAndSnapLocked(int value) const
{
    if (this->minValue >= this->maxValue) {
        return this->minValue;
    }
    const int safeStep = std::max(this->stepValue, 1);
    value = std::clamp(value, this->minValue, this->maxValue);
    const float stepIndex = static_cast<float>(value - this->minValue) / static_cast<float>(safeStep);
    int snapped = this->minValue + static_cast<int>(std::lround(stepIndex)) * safeStep;
    if (snapped > this->maxValue) {
        snapped = this->maxValue;
    }
    if (snapped < this->minValue) {
        snapped = this->minValue;
    }
    return snapped;
}

float CubeSliderBox::getNormalizedDraftValueLocked() const
{
    if (this->maxValue <= this->minValue) {
        return 0.0f;
    }
    const float numerator = static_cast<float>(this->draftValue - this->minValue);
    const float denominator = static_cast<float>(this->maxValue - this->minValue);
    return std::clamp(numerator / denominator, 0.0f, 1.0f);
}

int CubeSliderBox::valueFromScreenXLocked(int x) const
{
    if (this->trackRect_.xMax <= this->trackRect_.xMin || this->maxValue <= this->minValue) {
        return this->minValue;
    }
    const float normalized = std::clamp(
        static_cast<float>(x - static_cast<int>(this->trackRect_.xMin)) /
            static_cast<float>(this->trackRect_.xMax - this->trackRect_.xMin),
        0.0f,
        1.0f);
    const int rawValue = this->minValue + static_cast<int>(std::lround(normalized * static_cast<float>(this->maxValue - this->minValue)));
    return clampAndSnapLocked(rawValue);
}

void CubeSliderBox::updateRectsLocked()
{
    this->popupRect_ = {
        static_cast<unsigned int>(this->position.x),
        static_cast<unsigned int>(this->position.x + this->size.x),
        static_cast<unsigned int>(this->position.y),
        static_cast<unsigned int>(this->position.y + this->size.y)
    };

    const unsigned int buttonHeight = static_cast<unsigned int>(this->messageTextSize * 2.2f);
    const unsigned int buttonWidth = static_cast<unsigned int>((this->size.x - 3 * STENCIL_INSET_PX) / 2.f);
    const unsigned int buttonsYMin = static_cast<unsigned int>(this->position.y + STENCIL_INSET_PX);
    const unsigned int buttonsYMax = buttonsYMin + buttonHeight;
    const unsigned int okXMin = static_cast<unsigned int>(this->position.x + STENCIL_INSET_PX);
    const unsigned int cancelXMin = static_cast<unsigned int>(this->position.x + this->size.x - STENCIL_INSET_PX - buttonWidth);

    this->okBtn_ = { okXMin, okXMin + buttonWidth, buttonsYMin, buttonsYMax };
    this->cancelBtn_ = { cancelXMin, cancelXMin + buttonWidth, buttonsYMin, buttonsYMax };

    const unsigned int trackInset = static_cast<unsigned int>(STENCIL_INSET_PX * 2);
    const unsigned int trackHeight = static_cast<unsigned int>(std::max(this->messageTextSize * 0.9f, 24.0f));
    const unsigned int trackYMin = static_cast<unsigned int>(this->position.y + (this->size.y * 0.42f));
    this->trackRect_ = {
        static_cast<unsigned int>(this->position.x + trackInset),
        static_cast<unsigned int>(this->position.x + this->size.x - trackInset),
        trackYMin,
        trackYMin + trackHeight
    };

    const unsigned int thumbWidth = this->trackRect_.yMax - this->trackRect_.yMin;
    const unsigned int usableTravel = (this->trackRect_.xMax - this->trackRect_.xMin) > thumbWidth
        ? (this->trackRect_.xMax - this->trackRect_.xMin - thumbWidth)
        : 0;
    const unsigned int thumbXMin = this->trackRect_.xMin +
        static_cast<unsigned int>(std::lround(static_cast<float>(usableTravel) * getNormalizedDraftValueLocked()));
    this->thumbRect_ = { thumbXMin, thumbXMin + thumbWidth, this->trackRect_.yMin, this->trackRect_.yMax };
}

void CubeSliderBox::refreshVisualStateLocked()
{
    updateRectsLocked();
    clearTextObjects();

    if (this->sliderObject == nullptr) {
        this->sliderObject = new M_SliderTexture(
            this->textShader,
            static_cast<float>(this->trackRect_.xMax - this->trackRect_.xMin),
            static_cast<float>(this->trackRect_.yMax - this->trackRect_.yMin),
            10,
            { 1.f, 1.f, 1.f },
            { static_cast<float>(this->trackRect_.xMin), static_cast<float>(this->trackRect_.yMax) });
        this->objects.push_back(this->sliderObject);
    } else {
        this->sliderObject->setPosition({ static_cast<float>(this->trackRect_.xMin), static_cast<float>(this->trackRect_.yMax) });
    }
    this->sliderObject->setSliderPosition(getNormalizedDraftValueLocked());
    this->sliderObject->setVisibility(this->visible);

    auto* titleText = new M_Text(this->textShader, this->title, (this->messageTextSize * MESSAGEBOX_TITLE_TEXT_MULT), { 1.f, 1.f, 1.f }, { 0.f, 0.f });
    const float titleWidth = titleText->getWidth();
    const float titleX = this->position.x + (this->size.x - titleWidth) / 2.f;
    const float titleY = (this->position.y + this->size.y) - this->messageTextSize - STENCIL_INSET_PX;
    titleText->setPosition({ titleX, titleY });
    this->textObjects.push_back(titleText);

    auto* valueText = new M_Text(this->textShader, std::to_string(this->draftValue), this->messageTextSize * 1.2f, { 1.f, 1.f, 1.f }, { 0.f, 0.f });
    const float valueWidth = valueText->getWidth();
    const float valueX = this->position.x + (this->size.x - valueWidth) / 2.f;
    const float valueY = static_cast<float>(this->trackRect_.yMax + this->messageTextSize * 1.8f);
    valueText->setPosition({ valueX, valueY });
    this->textObjects.push_back(valueText);

    auto* okText = new M_Text(this->textShader, "OK", this->messageTextSize, { 0.1f, 1.f, 0.1f }, { 0.f, 0.f });
    const float okWidth = okText->getWidth();
    const float okX = this->okBtn_.xMin + (this->okBtn_.xMax - this->okBtn_.xMin - okWidth) / 2.f;
    const float okY = this->okBtn_.yMin + ((this->okBtn_.yMax - this->okBtn_.yMin) - this->messageTextSize) / 2.f;
    okText->setPosition({ okX, okY });
    this->textObjects.push_back(okText);

    auto* cancelText = new M_Text(this->textShader, "Cancel", this->messageTextSize, { 1.f, 0.2f, 0.2f }, { 0.f, 0.f });
    const float cancelWidth = cancelText->getWidth();
    const float cancelX = this->cancelBtn_.xMin + (this->cancelBtn_.xMax - this->cancelBtn_.xMin - cancelWidth) / 2.f;
    const float cancelY = this->cancelBtn_.yMin + ((this->cancelBtn_.yMax - this->cancelBtn_.yMin) - this->messageTextSize) / 2.f;
    cancelText->setPosition({ cancelX, cancelY });
    this->textObjects.push_back(cancelText);

    this->needsRefresh = false;
}

void CubeSliderBox::setup()
{
    if (!this->needsSetup) {
        return;
    }
    this->needsSetup = false;

    clearObjects();
    const float radius = BOX_RADIUS;
    const float diameter = radius * 2.0f;
    const float xStart = mapRange(this->position.x, 0.f, 720.f, -1.f, 1.f);
    const float yStart = mapRange(this->position.y, 0.f, 720.f, -1.f, 1.f);
    const float xSize = mapRange(this->size.x, 0.f, 720.f, 0.f, 2.f);
    const float ySize = mapRange(this->size.y, 0.f, 720.f, 0.f, 2.f);
    glm::vec2 size_ = { xSize, ySize };
    const float overlayZ = Z_DISTANCE + 0.02f;
    const float panelZ = Z_DISTANCE + 0.03f;
    const float outlineZ = Z_DISTANCE + 0.031f;
    const float panelFill = 0.0f;

    this->objects.push_back(new M_Rect(shader, { -1.f, -1.f, overlayZ }, { 2.f, 2.f }, 0.0, 0.0));
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + radius, panelZ }, { size_.x - diameter, size_.y - diameter }, panelFill, panelFill));
    this->objects.push_back(new M_Rect(shader, { xStart, yStart + radius, panelZ }, { radius, size_.y - diameter }, panelFill, panelFill));
    this->objects.push_back(new M_Rect(shader, { xStart + size_.x - radius, yStart + radius, panelZ }, { radius, size_.y - diameter }, panelFill, panelFill));
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart, panelZ }, { size_.x - diameter, radius }, panelFill, panelFill));
    this->objects.push_back(new M_Rect(shader, { xStart + radius, yStart + size_.y - radius, panelZ }, { size_.x - diameter, radius }, panelFill, panelFill));
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + size_.y - radius, panelZ }, 0, 90, { panelFill, panelFill, panelFill }));
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + size_.y - radius, panelZ }, 90, 180, { panelFill, panelFill, panelFill }));
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + radius, yStart + radius, panelZ }, 180, 270, { panelFill, panelFill, panelFill }));
    this->objects.push_back(new M_PartCircle(shader, 50, radius, { xStart + size_.x - radius, yStart + radius, panelZ }, 270, 360, { panelFill, panelFill, panelFill }));
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart + size_.y, outlineZ }, { xStart + size_.x - radius, yStart + size_.y, outlineZ }));
    this->objects.push_back(new M_Line(shader, { xStart + size_.x, yStart + radius, outlineZ }, { xStart + size_.x, yStart + size_.y - radius, outlineZ }));
    this->objects.push_back(new M_Line(shader, { xStart + radius, yStart, outlineZ }, { xStart + size_.x - radius, yStart, outlineZ }));
    this->objects.push_back(new M_Line(shader, { xStart, yStart + radius, outlineZ }, { xStart, yStart + size_.y - radius, outlineZ }));
    this->objects.push_back(new M_Arc(shader, 50, radius, 0, 90, { xStart + size_.x - radius, yStart + size_.y - radius, outlineZ }));
    this->objects.push_back(new M_Arc(shader, 50, radius, 360, 270, { xStart + size_.x - radius, yStart + radius, outlineZ }));
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 270, { xStart + radius, yStart + radius, outlineZ }));
    this->objects.push_back(new M_Arc(shader, 50, radius, 180, 90, { xStart + radius, yStart + size_.y - radius, outlineZ }));

    if (!this->hasCountedDown) {
        this->hasCountedDown = true;
        this->latch->count_down();
    }
    this->needsRefresh = true;
    CubeLog::info("SliderBox setup");
}

bool CubeSliderBox::setVisible(bool visible)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    bool previous = this->visible;
    this->visible = visible;
    this->dragging = false;
    this->needsRefresh = true;
    if (this->sliderObject != nullptr) {
        this->sliderObject->setVisibility(visible);
    }
    return previous;
}

bool CubeSliderBox::getVisible()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->visible;
}

void CubeSliderBox::draw()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (this->needsSetup) {
        setup();
    }
    if (this->needsRefresh) {
        refreshVisualStateLocked();
    }
    if (!this->visible) {
        return;
    }
    for (auto* object : this->objects) {
        object->draw();
    }
    for (auto* object : this->textObjects) {
        object->draw();
    }
}

void CubeSliderBox::setPosition(glm::vec2 position)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->position = position;
    this->needsSetup = true;
    this->needsRefresh = true;
}

void CubeSliderBox::setSize(glm::vec2 size)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->size = size;
    this->needsSetup = true;
    this->needsRefresh = true;
}

std::vector<MeshObject*> CubeSliderBox::getObjects()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    std::vector<MeshObject*> allObjects;
    allObjects.insert(allObjects.end(), this->objects.begin(), this->objects.end());
    allObjects.insert(allObjects.end(), this->textObjects.begin(), this->textObjects.end());
    return allObjects;
}

void CubeSliderBox::setTitle(const std::string& title)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->title = title;
    this->needsRefresh = true;
}

void CubeSliderBox::setRange(int minValue, int maxValue, int stepValue)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (minValue >= maxValue) {
        CubeLog::warning("SliderBox: invalid range requested");
        return;
    }
    this->minValue = minValue;
    this->maxValue = maxValue;
    this->stepValue = std::max(stepValue, 1);
    this->committedValue = clampAndSnapLocked(this->committedValue);
    this->draftValue = clampAndSnapLocked(this->draftValue);
    this->needsRefresh = true;
}

void CubeSliderBox::setCommittedValue(int value)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->committedValue = clampAndSnapLocked(value);
    this->draftValue = this->committedValue;
    this->needsRefresh = true;
}

void CubeSliderBox::setDraftValue(int value)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->draftValue = clampAndSnapLocked(value);
    this->needsRefresh = true;
}

int CubeSliderBox::getDraftValue()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->draftValue;
}

void CubeSliderBox::setConfirmCallback(std::function<void(int)> callback)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->confirmCallback = callback;
}

void CubeSliderBox::setCancelCallback(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->cancelCallback = callback;
}

bool CubeSliderBox::beginDrag(unsigned int x, unsigned int y)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (!this->visible) {
        return false;
    }
    if (!(isInsideTrack(x, y) || isInsideThumb(x, y))) {
        this->dragging = false;
        return false;
    }
    this->dragging = true;
    this->dragStartPointerX = static_cast<int>(x);
    this->dragStartValue = this->draftValue;
    this->draftValue = valueFromScreenXLocked(static_cast<int>(x));
    this->needsRefresh = true;
    return true;
}

void CubeSliderBox::handleTrackInteraction(unsigned int x)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->draftValue = valueFromScreenXLocked(static_cast<int>(x));
    this->needsRefresh = true;
}

void CubeSliderBox::handleDragDelta(int deltaX)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (!this->visible || !this->dragging) {
        return;
    }
    this->draftValue = valueFromScreenXLocked(this->dragStartPointerX + deltaX);
    this->needsRefresh = true;
}

void CubeSliderBox::handlePointerMove(unsigned int x, unsigned int y)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (!this->visible || !this->dragging) {
        return;
    }
    if (!(isInsideTrack(x, y) || isInsideThumb(x, y) || isInsidePopup(x, y))) {
        return;
    }
    this->draftValue = valueFromScreenXLocked(static_cast<int>(x));
    this->needsRefresh = true;
}

void CubeSliderBox::endDrag()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->dragging = false;
}

void CubeSliderBox::confirmSelection()
{
    std::function<void(int)> callback;
    int value = 0;
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->committedValue = this->draftValue;
        this->visible = false;
        this->dragging = false;
        this->needsRefresh = true;
        callback = this->confirmCallback;
        value = this->committedValue;
    }
    if (callback) {
        callback(value);
    }
}

void CubeSliderBox::cancelSelection()
{
    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->draftValue = this->committedValue;
        this->visible = false;
        this->dragging = false;
        this->needsRefresh = true;
        callback = this->cancelCallback;
    }
    if (callback) {
        callback();
    }
}

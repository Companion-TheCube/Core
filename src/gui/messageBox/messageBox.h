/*
███╗   ███╗███████╗███████╗███████╗ █████╗  ██████╗ ███████╗██████╗  ██████╗ ██╗  ██╗   ██╗  ██╗
████╗ ████║██╔════╝██╔════╝██╔════╝██╔══██╗██╔════╝ ██╔════╝██╔══██╗██╔═══██╗╚██╗██╔╝   ██║  ██║
██╔████╔██║█████╗  ███████╗███████╗███████║██║  ███╗█████╗  ██████╔╝██║   ██║ ╚███╔╝    ███████║
██║╚██╔╝██║██╔══╝  ╚════██║╚════██║██╔══██║██║   ██║██╔══╝  ██╔══██╗██║   ██║ ██╔██╗    ██╔══██║
██║ ╚═╝ ██║███████╗███████║███████║██║  ██║╚██████╔╝███████╗██████╔╝╚██████╔╝██╔╝ ██╗██╗██║  ██║
╚═╝     ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
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

#pragma once
#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <latch>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#ifndef SHAPES_H
#include "./../renderables/shapes.h"
#endif
#ifndef OBJECTS_H
#include "./../objects.h"
#endif
#ifndef SHADER_H
#include "./../shader.h"
#endif
#ifndef RENDERER_H
#include "./../renderer.h"
#endif

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
#define MESSAGEBOX_ITEM_TEXT_SIZE 32.f
#define MESSAGEBOX_LINE_SPACING 0.3f
#define MESSAGEBOX_TITLE_TEXT_MULT 1.3f

// TODO: make a generic message box class that can be inherited from to make different types of message boxes

class CubeMessageBox : public M_Box {
private:
    bool visible;
    std::vector<MeshObject*> objects;
    std::vector<MeshObject*> textObjects;
    Shader* shader;
    Shader* textShader;
    float messageTextSize = MESSAGEBOX_ITEM_TEXT_SIZE;
    long scrollVertPosition = 0;
    float index = 0.001;
    CountingLatch* latch;
    std::mutex mutex;
    std::vector<size_t> textMeshIndices;
    Renderer* renderer;
    std::function<void()> callback = nullptr;
    ClickableArea clickArea;
    bool needsSetup = true;
    glm::vec2 position;
    glm::vec2 size;

public:
    CubeMessageBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch);
    ~CubeMessageBox();
    void setup();
    bool setVisible(bool visible);
    bool getVisible();
    void draw();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    std::vector<MeshObject*> getObjects();
    void setText(const std::string& text, const std::string& title);
    void setCallback(std::function<void()> callback);
    void call_callback()
    {
        if (this->callback != nullptr)
            this->callback();
    };
    ClickableArea getClickableArea_() { return this->clickArea; };
};

class CubeTextBox : public M_Box {
private:
    bool visible;
    std::vector<MeshObject*> objects;
    std::vector<MeshObject*> textObjects;
    Shader* shader;
    Shader* textShader;
    float messageTextSize = MESSAGEBOX_ITEM_TEXT_SIZE;
    long scrollVertPosition = 0;
    float index = 0.001;
    CountingLatch* latch;
    std::mutex mutex;
    std::vector<size_t> textMeshIndices;
    Renderer* renderer;
    std::function<void()> callback = nullptr;
    ClickableArea clickArea;
    bool needsSetup = true;
    glm::vec2 position;
    glm::vec2 size;
    int textMeshCount = 0;

public:
    CubeTextBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch);
    ~CubeTextBox();
    void setup();
    bool setVisible(bool visible);
    bool getVisible();
    void draw();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setTextSize(float size);
    std::vector<MeshObject*> getObjects();
    void setText(const std::string& text, const std::string& title);
    void setCallback(std::function<void()> callback);
    void call_callback()
    {
        if (this->callback != nullptr)
            this->callback();
    };
    ClickableArea getClickableArea_() { return this->clickArea; };
};

// TODO: This class is not rendering correctly. fix.

class CubeNotificaionBox : public M_Box {
private:
    bool visible;
    std::vector<MeshObject*> objects;
    std::vector<MeshObject*> textObjects;
    Shader* shader;
    Shader* textShader;
    float messageTextSize = MESSAGEBOX_ITEM_TEXT_SIZE;
    long scrollVertPosition = 0;
    float index = 0.001;
    CountingLatch* latch;
    std::mutex mutex;
    Renderer* renderer;
    std::function<void()> callbackYes = nullptr;
    std::function<void()> callbackNo = nullptr;
    ClickableArea clickArea;
    bool needsSetup = true;
    glm::vec2 position;
    glm::vec2 size;
    int textMeshCount = 0;
    // Button rectangles (pixel coordinates in 720x720 space)
    struct Rect { unsigned int xMin, xMax, yMin, yMax; };
    Rect yesBtn_{};
    Rect noBtn_{};

public:
    CubeNotificaionBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch);
    ~CubeNotificaionBox();
    void setup();
    bool setVisible(bool visible);
    bool getVisible();
    void draw();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    void setTextSize(float size);
    std::vector<MeshObject*> getObjects();
    void setText(const std::string& text, const std::string& title);
    void setCallbackYes(std::function<void()> callback);
    void setCallbackNo(std::function<void()> callback);
    void call_callback()
    {
        if (this->callbackNo != nullptr)
            this->callbackNo();
    };
    ClickableArea getClickableArea_() { return this->clickArea; };
    // Helpers for click handling
    bool isInsideYes(unsigned int x, unsigned int y) const
    {
        return x < yesBtn_.xMax && x > yesBtn_.xMin && y < yesBtn_.yMax && y > yesBtn_.yMin;
    }
    bool isInsideNo(unsigned int x, unsigned int y) const
    {
        return x < noBtn_.xMax && x > noBtn_.xMin && y < noBtn_.yMax && y > noBtn_.yMin;
    }
    void triggerYes()
    {
        if (callbackYes) callbackYes();
    }
    void triggerNo()
    {
        if (callbackNo) callbackNo();
    }
    Rect getYesRect() const { return yesBtn_; }
    Rect getNoRect() const { return noBtn_; }
};

template <class T>
class MakeCubeBoxClickable : public Clickable {
public:
    MakeCubeBoxClickable(T* object)
        : object(object)
    {
        clickAreaPtr = new ClickableArea();
        clickAreaPtr->clickableObject = this;
        clickAreaPtr->xMin = object->getClickableArea_().xMin;
        clickAreaPtr->xMax = object->getClickableArea_().xMax;
        clickAreaPtr->yMin = object->getClickableArea_().yMin;
        clickAreaPtr->yMax = object->getClickableArea_().yMax;
    };
    void onClick(void* data) override { object->setVisible(!object->getVisible()); };
    void onRelease(void* data) override {};
    void onMouseDown(void* data) override {};
    void onRightClick(void* data) override {};
    ClickableArea* getClickableArea() override
    {

        clickAreaPtr->clickableObject = this;
        clickAreaPtr->xMin = object->getClickableArea_().xMin;
        clickAreaPtr->xMax = object->getClickableArea_().xMax;
        clickAreaPtr->yMin = object->getClickableArea_().yMin;
        clickAreaPtr->yMax = object->getClickableArea_().yMax;
        return this->clickAreaPtr;
    };
    void setOnClick(std::function<unsigned int(void*)> action) override {};
    void setOnRightClick(std::function<unsigned int(void*)> action) override {};
    bool getIsClickable() override { return object->getVisible(); };
    void setVisibleWidth(float width) override {};
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) override {};
    void capturePosition() override {};
    void restorePosition() override {};
    void resetScroll() override {};
    std::vector<MeshObject*> getObjects() override { return object->getObjects(); };
    void draw() override {};
    bool setVisible(bool visible) override { return object->setVisible(visible); };
    bool getVisible() override { return object->getVisible(); };
    bool setIsClickable(bool isClickable) override { return object->getVisible(); };
    T* object;
    ClickableArea* clickAreaPtr;
};

class NotificationBoxClickable : public Clickable {
public:
    explicit NotificationBoxClickable(CubeNotificaionBox* box)
        : box_(box)
    {
        clickAreaPtr = new ClickableArea();
        clickAreaPtr->clickableObject = this;
    }
    ~NotificationBoxClickable() override { delete clickAreaPtr; }
    void onClick(void* data) override
    {
        if (!box_->getVisible()) return;
        auto* ev = static_cast<sf::Event*>(data);
        unsigned int x = ev ? static_cast<unsigned int>(ev->mouseButton.x) : 0;
        unsigned int y = ev ? static_cast<unsigned int>(ev->mouseButton.y) : 0;
        if (box_->isInsideYes(x, y)) {
            box_->setVisible(false);
            box_->triggerYes();
        } else if (box_->isInsideNo(x, y)) {
            box_->setVisible(false);
            box_->triggerNo();
        }
    }
    void onRelease(void* /*data*/) override {}
    void onMouseDown(void* /*data*/) override {}
    void onRightClick(void* /*data*/) override {}
    ClickableArea* getClickableArea() override
    {
        // Sync area with box bounds
        auto area = box_->getClickableArea_();
        clickAreaPtr->clickableObject = this;
        clickAreaPtr->xMin = area.xMin;
        clickAreaPtr->xMax = area.xMax;
        clickAreaPtr->yMin = area.yMin;
        clickAreaPtr->yMax = area.yMax;
        return clickAreaPtr;
    }
    void setOnClick(std::function<unsigned int(void*)> /*action*/) override {}
    void setOnRightClick(std::function<unsigned int(void*)> /*action*/) override {}
    bool getIsClickable() override { return box_->getVisible(); }
    bool setIsClickable(bool /*isClickable*/) override { return box_->getVisible(); }
    std::vector<MeshObject*> getObjects() override { return box_->getObjects(); }
    void setVisibleWidth(float /*width*/) override {}
    void setClickAreaSize(unsigned int /*xMin*/, unsigned int /*xMax*/, unsigned int /*yMin*/, unsigned int /*yMax*/) override {}
    void capturePosition() override {}
    void restorePosition() override {}
    void resetScroll() override {}
    bool setVisible(bool v) override { return box_->setVisible(v); }
    bool getVisible() override { return box_->getVisible(); }
    void draw() override {}

private:
    CubeNotificaionBox* box_;
    ClickableArea* clickAreaPtr;
};

class NotificationYesClickable : public Clickable {
public:
    explicit NotificationYesClickable(CubeNotificaionBox* box)
        : box_(box)
    {
        clickAreaPtr = new ClickableArea();
        clickAreaPtr->clickableObject = this;
    }
    ~NotificationYesClickable() override { delete clickAreaPtr; }
    void onClick(void* /*data*/) override { if (!box_->getVisible()) return; box_->setVisible(false); box_->triggerYes(); }
    void onRelease(void* /*data*/) override {}
    void onMouseDown(void* /*data*/) override {}
    void onRightClick(void* /*data*/) override {}
    ClickableArea* getClickableArea() override
    {
        auto r = box_->getYesRect();
        clickAreaPtr->clickableObject = this;
        clickAreaPtr->xMin = r.xMin;
        clickAreaPtr->xMax = r.xMax;
        clickAreaPtr->yMin = r.yMin;
        clickAreaPtr->yMax = r.yMax;
        return clickAreaPtr;
    }
    void setOnClick(std::function<unsigned int(void*)> /*action*/) override {}
    void setOnRightClick(std::function<unsigned int(void*)> /*action*/) override {}
    bool getIsClickable() override { return box_->getVisible(); }
    bool setIsClickable(bool /*isClickable*/) override { return box_->getVisible(); }
    std::vector<MeshObject*> getObjects() override { return {}; }
    void setVisibleWidth(float /*width*/) override {}
    void setClickAreaSize(unsigned int /*xMin*/, unsigned int /*xMax*/, unsigned int /*yMin*/, unsigned int /*yMax*/) override {}
    void capturePosition() override {}
    void restorePosition() override {}
    void resetScroll() override {}
    bool setVisible(bool v) override { return box_->setVisible(v); }
    bool getVisible() override { return box_->getVisible(); }
    void draw() override {}

private:
    CubeNotificaionBox* box_;
    ClickableArea* clickAreaPtr;
};

class NotificationNoClickable : public Clickable {
public:
    explicit NotificationNoClickable(CubeNotificaionBox* box)
        : box_(box)
    {
        clickAreaPtr = new ClickableArea();
        clickAreaPtr->clickableObject = this;
    }
    ~NotificationNoClickable() override { delete clickAreaPtr; }
    void onClick(void* /*data*/) override { if (!box_->getVisible()) return; box_->setVisible(false); box_->triggerNo(); }
    void onRelease(void* /*data*/) override {}
    void onMouseDown(void* /*data*/) override {}
    void onRightClick(void* /*data*/) override {}
    ClickableArea* getClickableArea() override { return clickAreaPtr; }
    ClickableArea* getClickableArea() const { return clickAreaPtr; }
    void setOnClick(std::function<unsigned int(void*)> /*action*/) override {}
    void setOnRightClick(std::function<unsigned int(void*)> /*action*/) override {}
    bool getIsClickable() override { return box_->getVisible(); }
    bool setIsClickable(bool /*isClickable*/) override { return box_->getVisible(); }
    std::vector<MeshObject*> getObjects() override { return {}; }
    void setVisibleWidth(float /*width*/) override {}
    void setClickAreaSize(unsigned int /*xMin*/, unsigned int /*xMax*/, unsigned int /*yMin*/, unsigned int /*yMax*/) override {}
    void capturePosition() override {}
    void restorePosition() override {}
    void resetScroll() override {}
    bool setVisible(bool v) override { return box_->setVisible(v); }
    bool getVisible() override { return box_->getVisible(); }
    void draw() override {}

private:
    CubeNotificaionBox* box_;
    ClickableArea* clickAreaPtr;
};

#endif // MESSAGEBOX_H

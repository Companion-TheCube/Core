#ifndef CHARACTER_H
#define CHARACTER_H
#include <vector>
#include <SFML/Graphics.hpp>
#include <logger.h>
#include <glm/glm.hpp>
#include <functional>

class Clickable;

struct Vertex{
    float x, y, z;
    float r, g, b;
};

struct ClickableArea{
    ClickableArea(){};
    ClickableArea(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax, Clickable* clickableObject){
        this->xMin = xMin;
        this->xMax = xMax;
        this->yMin = yMin;
        this->yMax = yMax;
        this->clickableObject = clickableObject;
    }
    Clickable* clickableObject;
    unsigned int xMin, xMax, yMin, yMax;
};

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
    virtual void rotateAbout(float angle, glm::vec3 point) = 0;
    virtual void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point) = 0;
    virtual glm::vec3 getCenterPoint() = 0;
    virtual std::vector<Vertex> getVertices() = 0;
    virtual float getWidth() = 0;
    virtual ~MeshObject(){};
    virtual void capturePosition() = 0;
    virtual void restorePosition() = 0;
    virtual void setVisibility(bool visible) = 0;
};



class Object{
    public:
        virtual ~Object(){};
        virtual void draw() = 0;
        virtual bool setVisible(bool visible) = 0;
        virtual bool getVisible() = 0;
};

class M_Box: public Object{
public:
    virtual void setPosition(glm::vec2 position) = 0;
    virtual void setSize(glm::vec2 size) = 0;
    virtual bool setVisible(bool visible) = 0;
    virtual ~M_Box(){};
};

class Clickable: public Object{
public:
    virtual ~Clickable(){};
    ClickableArea clickArea;
    virtual void onClick(void*) = 0;
    virtual void onRightClick(void*) = 0;
    virtual std::vector<MeshObject*> getObjects() = 0;
    virtual void setOnClick(std::function<void(void*)> action) = 0;
    virtual void setOnRightClick(std::function<void(void*)> action) = 0;
    virtual ClickableArea* getClickableArea() = 0;
    virtual void setVisibleWidth(float width) = 0;
    virtual void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) = 0;
};



template<typename T>
T mapRange(T value, T fromLow, T fromHigh, T toLow, T toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

#endif
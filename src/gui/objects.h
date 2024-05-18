#ifndef CHARACTER_H
#define CHARACTER_H
#include <vector>
#include <SFML/Graphics.hpp>
#include <logger.h>
#include <glm/glm.hpp>

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
};

enum Expression{
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    FRUSTRATED,
    SURPRISED,
    SCARED,
    DISGUSTED,
    DIZZY,
    SICK,
    SLEEPY,
    CONFUSED,
    SHOCKED,
    INJURED,
    DEAD,
    NULL_EXPRESSION,
    COUNT
};

class Object{
    public:
        virtual ~Object(){};
        virtual void draw() = 0;
        virtual bool setVisible(bool visible) = 0;
        virtual bool getVisible() = 0;
};

class C_Character : public Object{
    public:
        virtual ~C_Character(){};
        virtual void animateRandomFunny() = 0;
        virtual void animateJumpUp() = 0;
        virtual void animateJumpLeft() = 0;
        virtual void animateJumpRight() = 0;
        virtual void animateJumpLeftThroughWall() = 0;
        virtual void animateJumpRightThroughWall() = 0;
        virtual void expression(Expression) = 0;
        virtual std::string getName() = 0;
        virtual bool setVisible(bool visible) = 0;
};

class M_Box: public Object{
public:
    virtual void setPosition(glm::vec2 position) = 0;
    virtual void setSize(glm::vec2 size) = 0;
    virtual bool setVisible(bool visible) = 0;
};

class Clickable: public Object{
public:
    virtual void onClick(void*) = 0;
    virtual void onRightClick(void*) = 0;
    virtual std::vector<MeshObject*> getObjects() = 0;
};

struct Vertex{
    float x, y, z;
    float r, g, b;
};

template<typename T>
T mapRange(T value, T fromLow, T fromHigh, T toLow, T toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

#endif
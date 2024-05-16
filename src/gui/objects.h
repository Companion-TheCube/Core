#ifndef CHARACTER_H
#define CHARACTER_H
#include <vector>
#include <SFML/Graphics.hpp>
#include <logger.h>
#include <glm/glm.hpp>

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
    DEAD
};

class Object{
    public:
        virtual ~Object(){};
        virtual void draw() = 0;
        virtual bool setVisible(bool visible) = 0;
};

class Character : public Object{
    public:
        virtual ~Character(){};
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

class Clickable{
public:
    virtual void onClick(void*) = 0;
    virtual void onRightClick(void*) = 0;
};

struct Vertex{
    float x, y, z;
    float r, g, b;
};

#endif
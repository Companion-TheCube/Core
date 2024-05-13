#ifndef CHARACTER_H
#define CHARACTER_H
#include <vector>
#include <SFML/Graphics.hpp>

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
};

struct Vertex{
    float x, y, z;
    float r, g, b;
};

#endif
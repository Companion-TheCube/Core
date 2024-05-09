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

class Character{
    public:
        virtual ~Character(){};
        std::string name;
        virtual std::vector<sf::Vertex> getDrawables() = 0;
        virtual void animateRandomFunny() = 0;
        virtual void animateJumpUp() = 0;
        virtual void animateJumpLeft() = 0;
        virtual void animateJumpRight() = 0;
        virtual void animateJumpLeftThroughWall() = 0;
        virtual void animateJumpRightThroughWall() = 0;
        virtual void expression(Expression) = 0;
};

class CharacterManager{
    private:
        std::vector<Character*> characters;
        Character* currentCharacter;
    public:
        CharacterManager();
        ~CharacterManager();
        Character* getCharacter();
        void setCharacter(Character*);
        bool loadAppCharacters();
        bool loadBuiltInCharacters();
        bool setCharacterByName(std::string name);
        Character* getCharacterByName(std::string name);
        std::vector<std::string> getCharacterNames();
};

#endif
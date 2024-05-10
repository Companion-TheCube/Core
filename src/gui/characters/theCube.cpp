#include "theCube.h"

TheCube::TheCube(Shader* sh, CubeLog* lgr)
{
    this->logger = lgr;
    this->shader = sh;

    this->objects.push_back(new Cube(this->shader));
    this->objects.at(0)->translate(glm::vec3(-1.f, 1.0f, -0.5f));
    this->objects.at(0)->uniformScale(0.1f);

    
    this->objects.push_back(new Cube(this->shader));
    this->objects.at(objects.size()-1)->translate(glm::vec3(1.f, 1.0f, -0.5f));
    this->objects.at(objects.size()-1)->uniformScale(0.5f);

    this->objects.push_back(new Cube(this->shader));
    this->objects.at(objects.size()-1)->translate(glm::vec3(1.f, -1.f, -0.5f));
    this->objects.at(objects.size()-1)->uniformScale(0.5f);

    this->objects.push_back(new Cube(this->shader));
    this->objects.at(objects.size()-1)->translate(glm::vec3(-1.f, -1.f, -0.5f));
    this->objects.at(objects.size()-1)->uniformScale(0.25f);


    this->loader = new MeshLoader(this->logger, this->shader);
    // find the "shoe" collection in loader
    for(auto collection: this->loader->collections){
        this->parts.push_back(new CharacterPart());
        this->parts.at(this->parts.size()-1)->name = collection->name;
        for(auto object: collection->objects){
            this->parts.at(this->parts.size()-1)->objects.push_back(object);
            this->objects.push_back(object);
        }
    }

    for(auto part:this->parts){
        for(auto object: part->objects){
            object->translate(glm::vec3(0.0f, 0.0f, -4.0f));
        }
    }

    // this->objects.push_back(new Cube(this->shader));
    this->name = "TheCube";
    this->logger->log("Created character " + this->name, true);
}

TheCube::~TheCube()
{
    for(auto object: this->objects){
        delete object;
    }
    delete this->loader;
}


void TheCube::animateRandomFunny()
{
    // this->rotate(1.f, 1.0f, 0.0f, 0.0f);
    this->objects.at(0)->rotate(2.f, glm::vec3(0.0f, 1.0f, 0.0f));
    this->objects.at(1)->rotate(2.f, glm::vec3(0.0f, 0.0f, 1.0f));
    this->objects.at(2)->rotate(2.f, glm::vec3(0.0f, -1.0f, 0.0f));
    // this->objects.at(3)->rotate(2.f, glm::vec3(1.0f, 0.0f, 0.0f));

    this->objects.at(3)->rotateAbout(2.f, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    for(auto object: this->parts.at(0)->objects){
        object->rotateAbout(2.1f, glm::vec3(0.0f, 2.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    }
}

void TheCube::animateJumpUp()
{
}

void TheCube::animateJumpLeft()
{
}

void TheCube::animateJumpRight()
{
}

void TheCube::animateJumpLeftThroughWall()
{
}

void TheCube::animateJumpRightThroughWall()
{
}

void TheCube::expression(Expression e)
{
}

std::string TheCube::getName(){
    return name;
}

void TheCube::draw(){
    for(auto object: this->objects){
        object->draw();
    }
}

void TheCube::rotate(float angle, float x, float y, float z)
{
    for(auto object: this->objects){
        glm::vec3 axis = glm::vec3(x, y, z);
        object->rotate(angle, axis);
    }
}

void TheCube::translate(float x, float y, float z)
{
    for(auto object: this->objects){
        glm::vec3 axis = glm::vec3(x, y, z);
        object->translate(axis);
    }
}

void TheCube::scale(float x, float y, float z)
{
    for(auto object: this->objects){
        glm::vec3 axis = glm::vec3(x, y, z);
        object->scale(axis);
    }
}
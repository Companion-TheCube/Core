#include "theCube.h"

TheCube::TheCube(Shader* sh, CubeLog* lgr)
{
    this->logger = lgr;
    this->shader = sh;
    this->visible = true;
    this->animationFrame = 0;

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

    std::vector<std::string> toLoad;
    toLoad.push_back("shoeL");
    toLoad.push_back("shoeR");
    this->loader = new MeshLoader(this->logger, this->shader, toLoad);
    // find the "shoe" collection in loader
    for(auto collection: this->loader->collections){
        this->parts.push_back(new CharacterPart());
        this->parts.at(this->parts.size()-1)->name = collection->name;
        // average the center points of all objects in the collection
        float x = 0.0f, y = 0.0f, z = 0.0f;
        for(auto object: collection->objects){
            this->parts.at(this->parts.size()-1)->objects.push_back(object);
            this->objects.push_back(object);
            glm::vec3 center = object->getCenterPoint();
            x += center.x;
            y += center.y;
            z += center.z;
        }
        x /= collection->objects.size();
        y /= collection->objects.size();
        z /= collection->objects.size();
        this->parts.at(this->parts.size()-1)->centerPoint = glm::vec3(x, y, z);
    }

    for(auto part:this->parts){
        for(auto object: part->objects){
            object->translate(glm::vec3(0.0f, 0.0f, -4.0f));
        }
        if(part->name == "shoeL" || part->name == "shoeR"){
            for(auto object: part->objects){
                object->translate(glm::vec3(0.0f, 5.0f, 0.0f));
                object->rotateAbout(-90.f, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                object->rotateAbout(30.f, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
            }
        }
    }

    for(auto object: this->getPartByName("shoeL")->objects){
        object->translate(glm::vec3(6.0f, 0.0f, 0.0f));
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

    this->animationFrame++;
    // this->animationFrame continuously counts up. We need to calculate an angle based on this frame
    // count that results in a smooth animation. We can use sin() to achieve this.
    float angle = sin((double)this->animationFrame/10.f) * 0.7f;
    for(auto object: this->getPartByName("shoeR")->objects){
        object->rotateAbout(angle, glm::vec3(2.f/3.f, 0.0f, -1.0f), this->parts[0]->centerPoint);;
    }
    // calculate the angle for the second part of the character, several frames behind the first part
    angle = sin((double)(this->animationFrame + 45)/10.f ) * 0.7f;
    for(auto object: this->getPartByName("shoeL")->objects){
        object->rotateAbout(angle, glm::vec3(2.f/3.f, 0.f, -1.0f), this->parts[1]->centerPoint);
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
    if(!this->visible){
        return;
    }
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

CharacterPart* TheCube::getPartByName(std::string name){
    for(auto part: this->parts){
        if(part->name == name){
            return part;
        }
    }
    return nullptr;
}

bool TheCube::setVisible(bool visible){
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool TheCube::getVisible(){
    return this->visible;
}

#include "theCube.h"

#define M_PI 3.14159265358979323846

TheCube::TheCube(Shader* sh)
{
    this->shader = sh;
    this->visible = true;
    this->animationFrame = 0;

    std::vector<std::string> toLoad;
    toLoad.push_back("TheCubeMain");
    toLoad.push_back("OpenEyesSmile");
    toLoad.push_back("ClosedEyesSmile");
    this->loader = new MeshLoader(this->shader, toLoad);
    // find the "shoe" collection in loader
    for (auto collection : this->loader->collections) {
        this->parts.push_back(new CharacterPart());
        this->parts.at(this->parts.size() - 1)->name = collection->name;
        // average the center points of all objects in the collection
        float x = 0.0f, y = 0.0f, z = 0.0f;
        for (auto object : collection->objects) {
            this->parts.at(this->parts.size() - 1)->objects.push_back(object);
            this->objects.push_back(object);
            glm::vec3 center = object->getCenterPoint();
            x += center.x;
            y += center.y;
            z += center.z;
        }

        x /= collection->objects.size();
        y /= collection->objects.size();
        z /= collection->objects.size();
        CubeLog::warning("Center point for " + collection->name + " is " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z));
        this->parts.at(this->parts.size() - 1)->centerPoint = glm::vec3(x, y, z);
    }

    for (auto object : this->objects) {
        // object->rotate(90.f, glm::vec3(1.0f, 0.0f, 0.0f));
        object->translate(glm::vec3(0.0f, -2.5f, -10.0f));
        object->uniformScale(5.f);
        object->rotate(-80.f, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    for (auto object : this->getPartByName("ClosedEyesSmile")->objects) {
        object->setVisibility(false);
    }

    for (auto object : this->objects) {
        object->capturePosition();
    }
    this->name = "TheCube";
    CubeLog::info("Created character " + this->name);
}

TheCube::~TheCube()
{
    for (auto object : this->objects) {
        delete object;
    }
    delete this->loader;
}

void TheCube::animateRandomFunny()
{
    this->animationFrame++;
    if (this->animationFrame % 300 == 0) {
        for (auto object : this->getPartByName("OpenEyesSmile")->objects) {
            object->setVisibility(false);
        }
        for (auto object : this->getPartByName("ClosedEyesSmile")->objects) {
            object->setVisibility(true);
        }
    }
    if ((this->animationFrame % 300) - 15 == 0) {
        for (auto object : this->getPartByName("OpenEyesSmile")->objects) {
            object->setVisibility(true);
        }
        for (auto object : this->getPartByName("ClosedEyesSmile")->objects) {
            object->setVisibility(false);
        }
    }
    // this->animationFrame continuously counts up. We need to calculate an angle based on this frame
    // count that results in a smooth animation. We can use sin() to achieve this.
    float angle = sin(glm::radians((double)(this->animationFrame) + 90));
    float angleDegrees = glm::degrees(angle);
    for (auto object : this->objects) {
        object->rotateAbout(angleDegrees / 70, glm::vec3(0.0f, 1.0f, 0.0f), this->objects.at(1)->getCenterPoint());
    }
    if (fmod(this->animationFrame, 360.f) < 0.2) {
        // reset all the objects positions to prevent drift
        for (auto object : this->objects)
            object->restorePosition();
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

std::string TheCube::getName()
{
    return name;
}

void TheCube::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
}

void TheCube::rotate(float angle, float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->rotate(angle, axis);
    }
}

void TheCube::translate(float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->translate(axis);
    }
}

void TheCube::scale(float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->scale(axis);
    }
}

CharacterPart* TheCube::getPartByName(std::string name)
{
    for (auto part : this->parts) {
        if (part->name == name) {
            return part;
        }
    }
    return nullptr;
}

bool TheCube::setVisible(bool visible)
{
    bool temp = this->visible;
    this->visible = visible;
    return temp;
}

bool TheCube::getVisible()
{
    return this->visible;
}

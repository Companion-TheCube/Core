#include "characterManager.h"

// TODO: Character manager needs some static methods that handle changing / triggering animations and expressions.
// These methods should be called from the GUI and should be able to handle any character that is loaded.
CharacterManager::CharacterManager(Shader* sh)
{
    this->shader = sh;
    CubeLog::info("CharacterManager Started. Loading built-in characters.");
    if (this->loadBuiltInCharacters()) {
        CubeLog::info("Built-in characters loaded.");
    } else {
        CubeLog::error("Failed to load one or more built-in characters.");
    }
    CubeLog::info("Built-in characters loaded. Loading app characters.");
    if (!this->loadAppCharacters()) {
        CubeLog::info("No app characters found.");
    }
    CubeLog::info("All characters loaded. Character manager ready.");

    this->animationThread = new std::jthread([&]() {
        while (true) {
            std::unique_lock<std::mutex> lock(this->animationMutex);
            this->animationCV.wait(lock, [&] { return this->animationThreadReady || this->exitThreads; });
            if (this->exitThreads) {
                break;
            }
            this->animationThreadReady = false;
            lock.unlock();
            this->drawCV.notify_one();

            CubeLog::critical("CharacterManager::animationThread: Calling animate()");
            this->getCharacter()->animate();
        }
    });
    this->expressionThread = new std::jthread([&]() {
        while (true) {
            std::unique_lock<std::mutex> lock(this->expressionMutex);
            this->expressionCV.wait(lock, [&] { return this->expressionThreadReady || this->exitThreads; });
            if (this->exitThreads) {
                break;
            }
            this->expressionThreadReady = false;
            lock.unlock();
            this->drawCV.notify_one();

            CubeLog::critical("CharacterManager::expressionThread: Calling expression()");
            this->getCharacter()->expression();
        }
    });
}

CharacterManager::~CharacterManager()
{
    for (auto character : this->characters) {
        delete character;
    }

    this->animationMutex.lock();
    this->expressionMutex.lock();
    this->exitThreads = true;
    this->animationCV.notify_one();
    this->expressionCV.notify_one();
    this->animationThread->join();
    this->expressionThread->join();
}

Character_generic* CharacterManager::getCharacter()
{
    return this->currentCharacter;
}

void CharacterManager::setCharacter(Character_generic* character)
{
    this->currentCharacter = character;
}

bool CharacterManager::loadAppCharacters()
{
    // TODO: This will need to interface with the list of registered apps and find the ones
    // that have characters to load. Their character data should be stored in the database.
    return false;
}

bool CharacterManager::loadBuiltInCharacters()
{
    // TODO: There should be a list of built in characters in the database that we can load.
    // Then, we load the characters and if one fails or is not found, we return false.
    // For now, we will just load TheCube.
    this->characters.push_back(new Character_generic(this->shader, "Character_TheCube"));
    return true;
}

bool CharacterManager::setCharacterByName(std::string name)
{
    return false;
}

Character_generic* CharacterManager::getCharacterByName(std::string name)
{
    for (auto character : this->characters) {
        if (character->getName().compare(name) == 0) {
            CubeLog::info("getCharacterByName(" + name + "): found!");
            character->setVisible(true);
            return character;
        }
    }
    CubeLog::info("getCharacterByName(" + name + "): NOT found!");
    return nullptr;
}

std::vector<std::string> CharacterManager::getCharacterNames()
{
    std::vector<std::string> names;
    for (auto character : this->characters) {
        names.push_back(character->getName());
    }
    CubeLog::info("getCharacterNames(): Returning " + std::to_string(names.size()) + " names.");
    return names;
}

void CharacterManager::triggerAnimationAndExpressionThreads()
{
    std::lock_guard<std::mutex> lock(this->animationMutex);
    std::lock_guard<std::mutex> lock2(this->expressionMutex);
    this->expressionThreadReady = true;
    this->animationThreadReady = true;
    this->animationCV.notify_one();
    this->expressionCV.notify_one();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Construct a new Character:: Character object
 *
 * @param sh The shader to use
 * @param folder The folder to load the character from
 */
Character_generic::Character_generic(Shader* sh, std::string folder)
{
    this->shader = sh;
    this->visible = false;
    this->animationFrame = 0;
    this->currentExpression = Expressions::NEUTRAL;
    this->currentAnimation = nullptr;

    // Load character.json from the folder. This file should contain the list of objects, animations and expressions to load.
    std::ifstream file("meshes/" + folder + "/character.json");
    if (!file.is_open()) {
        CubeLog::error("Failed to open " + folder + "/character.json");
        return;
    }
    nlohmann::json json;
    try {
        file >> json;
    } catch (nlohmann::json::parse_error& e) {
        CubeLog::error("Failed to parse " + folder + "/character.json");
        CubeLog::error(e.what());
        return;
    }

    // The "objects" key should contain an array of object filenames to load
    std::vector<std::string> objectsToLoad;
    for (auto object : json["objects"]) {
        objectsToLoad.push_back(object);
    }

    // The "animations" key should contain an array of animation filenames to load
    std::vector<std::string> animationsToLoad;
    for (auto animation : json["animations"]) {
        animationsToLoad.push_back(animation);
    }

    // The "expressions" key should contain an array of expression filenames to load
    std::vector<std::string> expressionsToLoad;
    for (auto expression : json["expressions"]) {
        expressionsToLoad.push_back(expression);
    }

    auto meshLoader = new MeshLoader(this->shader, folder, objectsToLoad);

    for (auto collection : meshLoader->collections) {
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

    auto animationLoader = new AnimationLoader(folder, animationsToLoad);
    for (auto animation : animationLoader->getAnimations()) {
        this->animations.push_back(animation);
    }

    auto expressionLoader = new ExpressionLoader(folder, expressionsToLoad);
    for (auto expression : expressionLoader->getAllExpressions()) {
        this->expressions.push_back(expression);
    }

    // set object to initial position
    for (auto object : this->objects) {
        object->translate(glm::vec3(1.9f, -2.5f, -10.0f));
        object->uniformScale(5.f);
        object->rotate(-80.f, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    for (auto object : this->objects) {
        object->capturePosition();
    }

    this->name = json["name"];

    CubeLog::info("Created character " + this->name);
}

Character_generic::~Character_generic()
{
    for (auto object : this->objects) {
        delete object;
    }
}

void Character_generic::animate()
{
    // 1. If the current animation is nullptr or AnimationNames_enum::NUEUTRAL, return true
    //

    // The below is old. Remove at some point.
    // TODO: Implement bounce animation. Return true when animation is complete.
    // 1. scale x,z up while translating y down in order to keep the bottom of the TheCube on the ground
    // 2. scale x,z down while translating y up to return to original position
    // 3. translate y up to simulate a bounce
    // 4. translate y down to return to original position
    // 5. scale x,z up while translating y down in order to keep the bottom of the TheCube on the ground
    // 6. scale x,z down while translating y up to return to original position
    //
    // set this->currenFunnyExpression to FUNNY_INDEX when the animation is complete

    // if(this->getPartByName("OpenEyesSmile") == nullptr || this->getPartByName("ClosedEyesSmile") == nullptr) {
    //     CubeLog::error("Failed to find OpenEyesSmile or ClosedEyesSmile");
    //     return true;
    // }
    // this->animationFrame++;
    // if (this->animationFrame % 300 == 0) {
    //     for (auto object : this->getPartByName("OpenEyesSmile")->objects) {
    //         object->setVisibility(false);
    //     }
    //     for (auto object : this->getPartByName("ClosedEyesSmile")->objects) {
    //         object->setVisibility(true);
    //     }
    // }
    // if ((this->animationFrame % 300) - 15 == 0) {
    //     for (auto object : this->getPartByName("OpenEyesSmile")->objects) {
    //         object->setVisibility(true);
    //     }
    //     for (auto object : this->getPartByName("ClosedEyesSmile")->objects) {
    //         object->setVisibility(false);
    //     }
    // }
    // this->animationFrame continuously counts up. We need to calculate an angle based on this frame
    // count that results in a smooth animation. We can use sin() to achieve this.
    // float angle = sin(glm::radians((double)(this->animationFrame) + 90));
    // float angleDegrees = glm::degrees(angle);
    // for (auto object : this->objects) {
    //     object->rotateAbout(angleDegrees / 70, glm::vec3(0.0f, 1.0f, 0.0f), this->objects.at(1)->getCenterPoint());
    // }
    // if (fmod(this->animationFrame, 360.f) < 0.2) {
    //     // reset all the objects positions to prevent drift
    //     for (auto object : this->objects)
    //         object->restorePosition();
    // }
}

void Character_generic::expression()
{
    // TODO: do the stuff that needs to be done to change the expression of the character based on the currentExpression
}

CharacterPart* Character_generic::getPartByName(std::string name)
{
    for (auto part : this->parts) {
        if (part->name.compare(name) == 0) {
            return part;
        }
    }
    return nullptr;
}

bool Character_generic::setVisible(bool visible)
{
    this->visible = visible;
    return this->visible;
}

bool Character_generic::getVisible()
{
    return this->visible;
}

void Character_generic::draw()
{
    if (!this->visible) {
        return;
    }
    for (auto object : this->objects) {
        object->draw();
    }
}

std::string Character_generic::getName()
{
    return this->name;
}

void Character_generic::rotate(float angle, float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->rotate(angle, axis);
    }
}

void Character_generic::translate(float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->translate(axis);
    }
}

void Character_generic::scale(float x, float y, float z)
{
    for (auto object : this->objects) {
        glm::vec3 axis = glm::vec3(x, y, z);
        object->scale(axis);
    }
}

void Character_generic::triggerAnimation(Animations::AnimationNames_enum name)
{
    for (auto animation : this->animations) {
        if (animation.name == name) {
            this->currentAnimation = &animation;
            this->currentAnimationName = name;
            return;
        }
    }
}

void Character_generic::triggerExpression(Expressions::ExpressionNames_enum e)
{
    for (auto expression : this->expressions) {
        if (expression.name == e) {
            this->currentExpression = e;
            this->currentExpressionDef = &expression;
            return;
        }
    }
}

////////////////////////////////////////////////////////////
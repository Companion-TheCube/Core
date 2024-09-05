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

            // CubeLog::critical("CharacterManager::animationThread: Calling animate()");
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

            // CubeLog::critical("CharacterManager::expressionThread: Calling expression()");
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
    // this->getCharacter()->triggerAnimation(Animations::NEUTRAL);
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
    this->currentExpressionDef = { Expressions::ExpressionNames_enum::COUNT, "", {}, {} };
    this->currentAnimation = { Animations::AnimationNames_enum::COUNT, "", {} };
    this->currentAnimationName = Animations::NEUTRAL;

    // Load character.json from the folder. This file should contain the list of objects, animations and expressions to load.
    std::ifstream file("meshes/" + folder + "/character.json");
    if (!file.is_open()) {
        CubeLog::error("Failed to open " + folder + "/character.json");
        return;
    }
    nlohmann::json characterData;
    try {
        file >> characterData;
    } catch (nlohmann::json::parse_error& e) {
        CubeLog::error("Failed to parse " + folder + "/character.json");
        CubeLog::error(e.what());
        return;
    }

    // The "objects" key should contain an array of object filenames to load
    std::vector<std::string> objectsToLoad;
    for (auto object : characterData["objects"]) {
        objectsToLoad.push_back(object);
    }

    // The "animations" key should contain an array of animation filenames to load
    std::vector<std::string> animationsToLoad;
    for (auto animation : characterData["animations"]) {
        animationsToLoad.push_back(animation);
    }

    // The "expressions" key should contain an array of expression filenames to load
    std::vector<std::string> expressionsToLoad;
    for (auto expression : characterData["expressions"]) {
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

    this->animationLoader = new AnimationLoader(folder, animationsToLoad);    
    this->expressionLoader = new ExpressionLoader(folder, expressionsToLoad);

    float x, y, z;
    try{
        x = characterData["initPos"]["x"];
        y = characterData["initPos"]["y"];
        z = characterData["initPos"]["z"];
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Character_generic: Failed to load character position from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    } catch (std::exception& e) {
        CubeLog::error("Character_generic: Failed to load character position from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
    for(auto object : this->objects) {
        object->translate(glm::vec3(x, y, z));
    }

    try{
        x = characterData["initScale"]["x"];
        y = characterData["initScale"]["y"];
        z = characterData["initScale"]["z"];
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Character_generic: Failed to load character scale from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 1.0f;
        y = 1.0f;
        z = 1.0f;
    } catch (std::exception& e) {
        CubeLog::error("Character_generic: Failed to load character scale from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 1.0f;
        y = 1.0f;
        z = 1.0f;
    }
    for(auto object : this->objects) {
        object->scale(glm::vec3(x, y, z));
    }

    float val;
    try{
        x = characterData["initRot"]["x"];
        y = characterData["initRot"]["y"];
        z = characterData["initRot"]["z"];
        val = characterData["initRot"]["value"];
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Character_generic: Failed to load character rotation from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        val = 0.f;
    } catch (std::exception& e) {
        CubeLog::error("Character_generic: Failed to load character rotation from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        val = 0.f;
    }
    for(auto object : this->objects) {
        object->rotate(val, glm::vec3(x, y, z));
    }

    for (auto object : this->objects) {
        object->capturePosition();
    }

    try{
        this->name = characterData["name"];
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("Character_generic: Failed to load character name from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        this->name = "unknown";
    } catch (std::exception& e) {
        CubeLog::error("Character_generic: Failed to load character name from file: " + folder + "/character.json");
        CubeLog::error(e.what());
        this->name = "unknown";
    }

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
    // get a high accuracy time point
    // std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(this->currentMutex);
    if (this->currentAnimationName == Animations::AnimationNames_enum::COUNT || this->nextAnimationName != Animations::AnimationNames_enum::COUNT) {
        CubeLog::info("Character_generic::animate: No current animation. Setting animation to NEUTRAL");
        std::lock_guard<std::mutex> lock(this->nextMutex);
        this->currentAnimation = this->nextAnimation;
        this->currentAnimationName = this->nextAnimationName;
        this->nextAnimation = { Animations::AnimationNames_enum::COUNT, "", {} };
        this->nextAnimationName = Animations::AnimationNames_enum::COUNT;
        this->animationFrame = 0;
    }
    bool foundKeyframe = false;
    for (auto keyframe : this->currentAnimation.keyframes) {
        if (keyframe.timeStart <= this->animationFrame && keyframe.timeEnd > this->animationFrame) {
            foundKeyframe = true;
            double f = this->animationFrame;
            double s = keyframe.timeStart;
            double e = keyframe.timeEnd;
            double f_normal = (f - s) / (e - s);
            double f2_normal = (f>0)?(f - s - 1) / (e - s):0.f;
            double fn_eased = keyframe.easingFunction(f_normal);
            double fn2_eased = keyframe.easingFunction(f2_normal);
            double calcValue = (double)keyframe.value * (fn_eased - fn2_eased);
            switch (keyframe.type) {
            case Animations::TRANSLATE:
                {
                    this->translate(keyframe.axis.x * calcValue, keyframe.axis.y * calcValue, keyframe.axis.z * calcValue);
                    break;
                }
            case Animations::ROTATE:
                {
                    this->rotate(calcValue, keyframe.axis.x, keyframe.axis.y, keyframe.axis.z);
                    break;
                }
            case Animations::SCALE_XYZ:
                {
                    calcValue = calcValue + 1.f;
                    this->scale((keyframe.axis.x>0?1*calcValue:1) , (keyframe.axis.y>0?1*calcValue:1), (keyframe.axis.z>0?1*calcValue:1));
                    break;
                }
            case Animations::UNIFORM_SCALE:
                {
                    this->scale(calcValue, calcValue, calcValue);
                    break;
                }
            case Animations::ROTATE_ABOUT:
                {
                    for(auto object : this->objects) {
                        object->rotateAbout(calcValue, keyframe.axis, keyframe.point );
                    }
                    break;
                }
            default:
                CubeLog::error("Character_generic::animate: Invalid animation type");
                break;
            }
        }
    }
    this->animationFrame++;
    if(!foundKeyframe) {
        this->triggerAnimation(Animations::NEUTRAL);
        this->animationFrame = 0;
        for(auto object : this->objects) {
            object->restorePosition();
        }
    }
    // std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    // CubeLog::info("Character_generic::animate: Animation took " + std::to_string(time_span.count()) + " seconds."); // about 0.000030 seconds
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
        object->rotate(angle, glm::vec3(x, y, z));
    }
}

void Character_generic::translate(float x, float y, float z)
{
    for (auto object : this->objects) {
        object->translate(glm::vec3(x, y, z));
    }
}

void Character_generic::scale(float x, float y, float z)
{
    for (auto object : this->objects) {
        object->scale(glm::vec3(x, y, z));
    }
}

void Character_generic::triggerAnimation(Animations::AnimationNames_enum name)
{
    std::lock_guard<std::mutex> lock(this->nextMutex);
    for (auto animation : this->animationLoader->getAnimationsVector()) {
        if (animation.name == name) {
            this->nextAnimation = this->animationLoader->getAnimationByEnum(name);
            this->nextAnimationName = name;
            return;
        }
    }
}

void Character_generic::triggerExpression(Expressions::ExpressionNames_enum e)
{
    std::lock_guard<std::mutex> lock(this->nextMutex);
    for (auto expression : this->expressionLoader->getExpressionsVector()) {
        if (expression.name == e) {
            this->nextExpression = e;
            this->nextExpressionDef = this->expressionLoader->getExpressionByEnum(e);
            return;
        }
    }
}

////////////////////////////////////////////////////////////
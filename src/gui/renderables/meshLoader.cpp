#include "meshLoader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define M_PI 3.14159265358979323846

MeshLoader::MeshLoader(Shader* shdr, std::string folderName, std::vector<std::string> toLoad)
{
    this->folderName = folderName;
    this->shader = shdr;
    std::vector<std::string> filenames = this->getFileNames();
    for (auto name : filenames) {
        CubeLog::info("MeshLoader: Found file: " + name);
    }
    // load .mesh files
    for (auto filename : filenames) {
        // ensure filetype is .mesh
        if (filename.find(".mesh") == std::string::npos || filename.find(".mesh") != filename.length() - 5) {
            continue;
        }
        // replace "\" with "/" for windows
        std::string _filename = std::regex_replace(filename, std::regex("\\"), "/");
        std::string name = _filename.substr(_filename.find_last_of('/') + 1, _filename.find(".mesh") - _filename.find_last_of('/') - 1);
        if (std::find(toLoad.begin(), toLoad.end(), name) == toLoad.end()) {
            continue;
        }
        CubeLog::info("MeshLoader: Found mesh file: " + _filename);
        CubeLog::info("Attempting to load mesh file: " + _filename);
        // concatenate the folder name with the filename
        _filename = "meshes/" + this->folderName + "/" + name + ".mesh";
        std::vector<MeshObject*> objects = this->loadMesh(_filename);
        CubeLog::info("MeshLoader: Loaded " + std::to_string(objects.size()) + " objects from file: " + _filename);
        this->collections.push_back(new ObjectCollection());
        // collection name is filename minus ".mesh"
        this->collections.at(collections.size() - 1)->name = filename.substr(_filename.find_last_of('/') + 1, _filename.find(".mesh") - _filename.find_last_of('/') - 1);
        for (auto object : objects) {
            this->collections.at(collections.size() - 1)->objects.push_back(object);
        }
    }

    // load .obj files
    for (auto filename : filenames) {
        // ensure filetype is .obj
        if (filename.find(".obj") == std::string::npos || filename.find(".obj") != filename.length() - 4) {
            continue;
        }
        // replace "\" with "/" for windows
        std::string _filename = std::regex_replace(filename, std::regex("\\\\"), "/");
        std::string name = _filename.substr(_filename.find_last_of('/') + 1, _filename.find(".obj") - _filename.find_last_of('/') - 1);
        if (std::find(toLoad.begin(), toLoad.end(), name) == toLoad.end()) {
            continue;
        }
        CubeLog::info("MeshLoader: Found obj file: " + _filename);
        CubeLog::info("Attempting to load obj file: " + _filename);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> mats;
        std::string warn, err;
        std::map<int, tinyobj::material_t> materials;
        std::vector<OBJObject*> objects;
        std::vector<Vertex> vertices;

        // Load the OBJ file and its materials
        bool success = tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, _filename.c_str(), nullptr, true);

        if (!warn.empty()) {
            CubeLog::warning("MeshLoader: " + warn);
        }

        if (!err.empty()) {
            CubeLog::error("MeshLoader: Failed to load obj file: " + _filename);
            CubeLog::error(err);
            continue;
        }

        if (!success) {
            CubeLog::error("MeshLoader: Failed to load obj file: " + _filename);
            continue;
        }

        // Store materials in a map for easy access
        for (size_t i = 0; i < mats.size(); ++i) {
            materials[i] = mats[i];
        }

        // Extract vertex positions and colors from the shapes
        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.indices.size(); f++) {
                const tinyobj::index_t& index = shape.mesh.indices[f];
                Vertex vertex = {
                    attrib.vertices[3 * index.vertex_index + 2],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 0],
                    1.0f, 1.0f, 1.0f // Default white color if no material
                };

                // Get the material ID associated with the face
                int material_id = shape.mesh.material_ids[f / 3]; // Assumes triangles (3 vertices per face)

                // Check if the material exists
                if (materials.count(material_id) > 0) {
                    tinyobj::material_t material = materials[material_id];
                    // Assign diffuse color from the material
                    vertex.r = material.diffuse[0];
                    vertex.g = material.diffuse[1];
                    vertex.b = material.diffuse[2];
                }

                vertices.push_back(vertex);
            }
        }

        if (!err.empty()) {
            CubeLog::info("MeshLoader: " + err);
        }
        CubeLog::info("MeshLoader: Loaded " + std::to_string(shapes.size()) + " shapes from obj file: " + _filename);
        for (auto shape : shapes) {
            objects.push_back(new OBJObject(this->shader, vertices));
        }
        CubeLog::info("MeshLoader: Loaded " + std::to_string(objects.size()) + " objects from obj file: " + _filename);
        this->collections.push_back(new ObjectCollection());
        // collection name is filename minus ".obj"
        this->collections.at(collections.size() - 1)->name = _filename.substr(_filename.find_last_of('/') + 1, _filename.find(".obj") - _filename.find_last_of('/') - 1);
        for (auto object : objects) {
            this->collections.at(collections.size() - 1)->objects.push_back(object);
        }
    }

    CubeLog::info("MeshLoader: Loaded " + std::to_string(this->collections.size()) + " collections");
}

MeshLoader::~MeshLoader()
{
    for (auto collection : this->collections) {
        delete collection;
    }
}

std::vector<MeshObject*> MeshLoader::getObjects()
{
    std::vector<MeshObject*> objects;
    for (auto collection : this->collections) {
        for (auto object : collection->objects) {
            objects.push_back(object);
        }
    }
    return objects;
}

std::vector<ObjectCollection*> MeshLoader::getCollections()
{
    return this->collections;
}

std::vector<std::string> MeshLoader::getFileNames()
{
    std::vector<std::string> names;
    for (auto& p : std::filesystem::directory_iterator("meshes/" + this->folderName)) {
        if (p.is_directory()) {
            continue;
        }
        // check that the file is a .mesh or .obj file
        if (p.path().has_extension() && (p.path().extension() == ".mesh" || p.path().extension() == ".obj")) {
            CubeLog::info("MeshLoader: Found file: " + p.path().string());
            names.push_back(p.path().string());
        }
    }
    CubeLog::info("MeshLoader: Found " + std::to_string(names.size()) + " mesh files");
    return names;
}

std::vector<MeshObject*> MeshLoader::loadMesh(std::string path)
{
    std::vector<MeshObject*> objects;
    std::ifstream file(path);
    if (!file.is_open()) {
        CubeLog::info("MeshLoader: Failed to open file: " + path);
        return objects;
    }
    float scale = 1.0f;
    std::string line;
    while (std::getline(file, line)) {
        // if line starts with a "#" its a comment, skip it
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }
        // the file is broken down into sections for each type of primitive: cubes, pyramids, etc
        // each section starts with a name such as $CUBES
        // each line in the section is an object of that type and is in the format:
        // X Y Z RRGGBB
        // We will ignore color. We will only use the X Y Z values. these indicate the 0,0,0 point of
        // each object and each object takes up a 1.0x1.0x1.0 space. We'll have to adjust the translation
        // to account for this.
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (line.find("@SCALE") != std::string::npos) {
            // the scale value is the next token on the line
            std::string floatStr = line.substr(6);
            std::istringstream iss(floatStr);
            float sc;
            iss >> sc;
            scale = sc;
            CubeLog::info("MeshLoader: Scaling all objects by: " + std::to_string(scale));
        }
        // TODO: change this to use RRGGBB data for shape type
        else if (type == "$CUBES") {
            CubeLog::info("MeshLoader: Loading cubes...");
            unsigned int count = 0;
            while (std::getline(file, line)) {
                if (line.length() == 0 || line[0] == '#' || line[0] == '$') {
                    break;
                }
                // std::cout<<".";
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // adjust translation
                x -= 0.5;
                y -= 0.5;
                z -= 0.5;
                x *= scale;
                y *= scale;
                z *= scale;
                objects.push_back(new Cube(this->shader));
                objects.at(objects.size() - 1)->translate(glm::vec3(x, y, z));
                objects.at(objects.size() - 1)->uniformScale(0.5f * scale);
                count++;
            }
            CubeLog::info("MeshLoader: Loaded " + std::to_string(count) + " cubes");
        } else if (type == "$PYRAMIDS") {
            while (std::getline(file, line)) {
                if (line[0] == '$') {
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new PyramidMeshObject(x, y, z));
            }
        } else if (type == "$SPHERES") {
            while (std::getline(file, line)) {
                if (line[0] == '$') {
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new SphereMeshObject(x, y, z));
            }
        } else if (type == "$CYLINDERS") {
            while (std::getline(file, line)) {
                if (line[0] == '$') {
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new CylinderMeshObject(x, y, z));
            }
        } else if (type == "$CONE") {
            while (std::getline(file, line)) {
                if (line[0] == '$') {
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new ConeMeshObject(x, y, z));
            }
        } else if (type == "$TORUS") {
            while (std::getline(file, line)) {
                if (line[0] == '$') {
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new TorusMeshObject(x, y, z));
            }
        }
    }
    return objects;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimationLoader::AnimationLoader(std::string folderName, std::vector<std::string> animationFileNames)
{
    this->folderName = folderName;
    // cross reference the animation names with the files in the folder
    std::vector<std::string> fileNames = this->getFileNames();
    for (auto name : animationFileNames) {
        if (std::find(fileNames.begin(), fileNames.end(), name) == fileNames.end()) {
            CubeLog::warning("AnimationLoader: Animation file not found: " + name);
        }
        if (name == "character.json") {
            CubeLog::warning("AnimationLoader: Skipping character.json");
            continue;
        }
        // if the filename is not in the animationNames list, remove it
        fileNames.erase(std::remove(fileNames.begin(), fileNames.end(), name), fileNames.end());
    }
    this->loadAnimations(fileNames);
}

AnimationLoader::~AnimationLoader()
{
}

std::vector<std::string> AnimationLoader::getFileNames()
{
    std::vector<std::string> names;
    try {
        for (auto& p : std::filesystem::directory_iterator("meshes/" + this->folderName)) {
            if (p.is_directory()) {
                continue;
            }
            // check that the file is a .json file and starts with "anim_" or "animation_"
            if (p.path().has_extension() && p.path().extension() == ".json" && (p.path().filename().string().find("anim_") != std::string::npos || p.path().filename().string().find("animation_") != std::string::npos)) {
                CubeLog::info("AnimationLoader: Found file: " + p.path().string());
                names.push_back(p.path().string());
            }
        }
        CubeLog::info("AnimationLoader: Found " + std::to_string(names.size()) + " animation files");
    } catch (std::filesystem::filesystem_error& e) {
        CubeLog::error("AnimationLoader: Failed to load animation files");
        CubeLog::error(e.what());
    } catch (std::exception& e) {
        CubeLog::error("AnimationLoader: Failed to load animation files");
        CubeLog::error(e.what());
    }
    return names;
}

void AnimationLoader::loadAnimations(std::vector<std::string> fileNames)
{
    std::vector<Animation> animations;
    for (auto filename : fileNames) {
        CubeLog::info("AnimationLoader: Loading animation file: " + filename);
        Animation animation = this->loadAnimation(filename);
        animationsMap[animation.name] = animation;
    }
}

Animation AnimationLoader::loadAnimation(std::string fileName)
{
    Animation animation;
    // use lohmann json to load the file
    std::ifstream file(fileName);
    if (!file.is_open()) {
        CubeLog::info("AnimationLoader: Failed to open file: " + fileName);
        return animation;
    }
    nlohmann::json jsonObject;
    try {
        file >> jsonObject;
    } catch (nlohmann::json::parse_error& e) {
        CubeLog::error("AnimationLoader: Failed to parse json file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    }
    // jsonObject is an object with properties "name, "expression", and "frames"
    try {
        std::string name = jsonObject["name"]; // the name of the animation
        static const std::unordered_map<std::string, Animations::AnimationNames_enum> nameToAnimation = {
            {"neutral", Animations::NEUTRAL},
            {"jump_right_through_wall", Animations::JUMP_RIGHT_THROUGH_WALL},
            {"jump_left_through_wall", Animations::JUMP_LEFT_THROUGH_WALL},
            {"jump_up_through_ceiling", Animations::JUMP_UP_THROUGH_CEILING},
            {"jump_right", Animations::JUMP_RIGHT},
            {"jump_left", Animations::JUMP_LEFT},
            {"jump_up", Animations::JUMP_UP},
            {"jump_down", Animations::JUMP_DOWN},
            {"jump_forward", Animations::JUMP_FORWARD},
            {"jump_backward", Animations::JUMP_BACKWARD},
            {"funny_index", Animations::FUNNY_INDEX},
            {"funny_bounce", Animations::FUNNY_BOUNCE},
            {"funny_spin", Animations::FUNNY_SPIN},
            {"funny_shrink", Animations::FUNNY_SHRINK},
            {"funny_expand", Animations::FUNNY_EXPAND},
            {"funny_jump", Animations::FUNNY_JUMP},
        };
        auto it = nameToAnimation.find(name);
        if (it == nameToAnimation.end()) {
            CubeLog::error("AnimationLoader: Invalid animation name: " + name);
            return animation;
        } else {
            animation.name = it->second;
        }
        animation.expression = jsonObject["expression"]; // the expression to be used with this animation
    } catch (nlohmann::json::parse_error e) {
        CubeLog::error("AnimationLoader: Failed to parse animation file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    } catch (nlohmann::json::type_error e) {
        CubeLog::error("AnimationLoader: Failed to load animation from file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    } catch (nlohmann::json::other_error e) {
        CubeLog::error("AnimationLoader: Failed to load animation from file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    } catch (nlohmann::json::exception& e) {
        CubeLog::error("AnimationLoader: Failed to load animation from file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    } catch (std::exception& e) {
        CubeLog::error("AnimationLoader: Failed to load animation from file: " + fileName);
        CubeLog::error(e.what());
        return animation;
    }
    for (auto keyframe : jsonObject["frames"]) {
        try {
            animation.keyframes.push_back(loadKeyframe(keyframe));
        } catch (nlohmann::json::type_error& e) {
            CubeLog::error("AnimationLoader: Failed to load keyframe from file: " + fileName);
            CubeLog::error(e.what());
            return animation;
        } catch (nlohmann::json::parse_error& e) {
            CubeLog::error("AnimationLoader: Failed to load keyframe from file: " + fileName);
            CubeLog::error(e.what());
            return animation;
        } catch (nlohmann::json::other_error& e) {
            CubeLog::error("AnimationLoader: Failed to load keyframe from file: " + fileName);
            CubeLog::error(e.what());
            return animation;
        } catch (AnimationLoaderException& e) {
            std::string mes = e.what();
            CubeLog::error("AnimationLoader encountered an error: \n" + mes);
            return animation;
        } catch (std::exception& e) {
            CubeLog::error("AnimationLoader: Failed to load keyframe from file: " + fileName);
            CubeLog::error(e.what());
            return animation;
        }
    }
    CubeLog::info("AnimationLoader: Loaded " + std::to_string(animation.keyframes.size()) + " keyframes from file: " + fileName);
    return animation;
}

/**
 * @brief Parse a json object into an AnimationKeyframe object
 * 
 * @param keyframe - The json object to parse
 * @return AnimationKeyframe - The parsed keyframe
 */
AnimationKeyframe AnimationLoader::loadKeyframe(nlohmann::json keyframe)
{
    static const std::unordered_map<std::string, Animations::AnimationType> nameToAnimType = {
        {"TRANSLATE", Animations::TRANSLATE},
        {"ROTATE", Animations::ROTATE},
        {"SCALE_XYZ", Animations::SCALE_XYZ},
        {"UNIFORM_SCALE", Animations::UNIFORM_SCALE},
        {"ROTATE_ABOUT", Animations::ROTATE_ABOUT},
    };
    std::string type;
    try{
        type = keyframe["type"];
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing type");
    }
    AnimationKeyframe newKeyFrame;
    auto it = nameToAnimType.find(type);
    if (it == nameToAnimType.end()) {
        throw AnimationLoaderException("Invalid animation type: " + type);
    } else {
        newKeyFrame.type = it->second;
    }
    float value;
    try {
        value = keyframe["value"];
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing value");
    }
    newKeyFrame.value = value;
    unsigned int timeStart;
    unsigned int timeEnd;
    try {
        timeStart = keyframe["time"]["start"];
        timeEnd = keyframe["time"]["end"];
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing time");
    }
    newKeyFrame.timeStart = timeStart;
    newKeyFrame.timeEnd = timeEnd;
    glm::vec3 axis;
    try {
        axis = glm::vec3(keyframe["axis"]["x"], keyframe["axis"]["y"], keyframe["axis"]["z"]);
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing axis");
    }
    newKeyFrame.axis = axis;
    glm::vec3 point;
    try {
        point = glm::vec3(keyframe["point"]["x"], keyframe["point"]["y"], keyframe["point"]["z"]);
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing point");
    }
    newKeyFrame.point = point;
    static const std::unordered_map<std::string, std::function<double(double)>> easingFunctions = {
        {"linear", [](double t) { return t; }},
        {"easeIn", [](double t) { return t * t; }},
        {"easeOut", [](double t) { return t * (2 - t); }},
        {"easeInOut", [](double t) { return 0.5f * (1 - cos(M_PI * t)); }},
    };
    std::string easing;
    try {
        easing = keyframe["easing"];
    } catch (nlohmann::json::exception& e) {
        throw AnimationLoaderException("Keyframe missing easing");
    }
    auto easingFunction = easingFunctions.find(easing);
    if (easingFunction == easingFunctions.end()) {
        newKeyFrame.easingFunction = [](double t) { return t; }; // default to linear
    } else {
        newKeyFrame.easingFunction = easingFunction->second;
    }
    return newKeyFrame;
}

/**
 * @brief Get the names of all loaded animations
 * 
 * @return std::vector<std::string> 
 */
std::vector<std::string> AnimationLoader::getAnimationNames()
{
    static const std::unordered_map<Animations::AnimationNames_enum, std::string> animToName = {
        {Animations::NEUTRAL, "NEUTRAL"},
        {Animations::JUMP_RIGHT_THROUGH_WALL, "JUMP_RIGHT_THROUGH_WALL"},
        {Animations::JUMP_LEFT_THROUGH_WALL, "JUMP_LEFT_THROUGH_WALL"},
        {Animations::JUMP_UP_THROUGH_CEILING, "JUMP_UP_THROUGH_CEILING"},
        {Animations::JUMP_RIGHT, "JUMP_RIGHT"},
        {Animations::JUMP_LEFT, "JUMP_LEFT"},
        {Animations::JUMP_UP, "JUMP_UP"},
        {Animations::JUMP_DOWN, "JUMP_DOWN"},
        {Animations::JUMP_FORWARD, "JUMP_FORWARD"},
        {Animations::JUMP_BACKWARD, "JUMP_BACKWARD"},
        {Animations::FUNNY_INDEX, "FUNNY_INDEX"},
        {Animations::FUNNY_BOUNCE, "FUNNY_BOUNCE"},
        {Animations::FUNNY_SPIN, "FUNNY_SPIN"},
        {Animations::FUNNY_SHRINK, "FUNNY_SHRINK"},
        {Animations::FUNNY_EXPAND, "FUNNY_EXPAND"},
        {Animations::FUNNY_JUMP, "FUNNY_JUMP"},
    };
    std::vector<std::string> names;
    for (auto& [key, value] : animToName) {
        names.push_back(value);
    }
    return names;
}

/**
 * @brief Get all loaded animations as a vector
 * 
 * @return std::vector<Animation> 
 */
std::vector<Animation> AnimationLoader::getAnimationsVector()
{
    std::vector<Animation> animations;
    for (auto& [key, value] : animationsMap) {
        animations.push_back(value);
    }
    return animations;
}

/**
 * @brief Get an animation by name
 * 
 * @param name - The name of the animation
 * @return Animation - The animation
 */
Animation AnimationLoader::getAnimationByName(std::string name)
{
    // Convert name to lowercase
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    // Map from string to Animations enum
    static const std::unordered_map<std::string, Animations::AnimationNames_enum> nameToAnimation = {
        {"neutral", Animations::NEUTRAL},
        {"jump_right_through_wall", Animations::JUMP_RIGHT_THROUGH_WALL},
        {"jump_left_through_wall", Animations::JUMP_LEFT_THROUGH_WALL},
        {"jump_up_through_ceiling", Animations::JUMP_UP_THROUGH_CEILING},
        {"jump_right", Animations::JUMP_RIGHT},
        {"jump_left", Animations::JUMP_LEFT},
        {"jump_up", Animations::JUMP_UP},
        {"jump_down", Animations::JUMP_DOWN},
        {"jump_forward", Animations::JUMP_FORWARD},
        {"jump_backward", Animations::JUMP_BACKWARD},
        {"funny_index", Animations::FUNNY_INDEX},
        {"funny_bounce", Animations::FUNNY_BOUNCE},
        {"funny_spin", Animations::FUNNY_SPIN},
        {"funny_shrink", Animations::FUNNY_SHRINK},
        {"funny_expand", Animations::FUNNY_EXPAND},
        {"funny_jump", Animations::FUNNY_JUMP},
    };

    // Find the animation in the map
    auto it = nameToAnimation.find(name);
    if (it != nameToAnimation.end()) {
        return animationsMap[it->second];
    }else{
        CubeLog::error("AnimationLoader: Animation not found: " + name);
    }

    // Default return neutral animation if no match is found
    return animationsMap[Animations::NEUTRAL];
}

/**
 * @brief Get an animation by Animations::AnimationNames_enum
 * 
 * @param name - The enum of the animation
 * @return Animation - The animation
 */
Animation AnimationLoader::getAnimationByEnum(Animations::AnimationNames_enum name)
{
    return animationsMap[name];
}

/**
 * @brief Get all loaded animations as a map
 * 
 * @return std::map<Animations::AnimationNames_enum, Animation> 
 */
std::map<Animations::AnimationNames_enum, Animation> AnimationLoader::getAnimations()
{
    return animationsMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExpressionLoader::ExpressionLoader(std::string folderName, std::vector<std::string> expressionFileNames)
{
    this->folderName = folderName;
    std::vector<std::string> fileNames = this->getFileNames();
    for (auto name : expressionFileNames) {
        if (std::find(fileNames.begin(), fileNames.end(), name) == fileNames.end()) {
            CubeLog::warning("ExpressionLoader: Expression file not found: " + name);
        }
        // if the filename is not in the expressionFilesNames list, remove it from fileNames

    }
    this->loadExpressions(fileNames);
}

ExpressionLoader::~ExpressionLoader()
{
}

std::vector<std::string> ExpressionLoader::getFileNames()
{
    std::vector<std::string> names;
    for (auto& p : std::filesystem::directory_iterator("meshes/" + this->folderName)) {
        if (p.is_directory()) {
            continue;
        }
        // check that the file is a .json file and starts with "expr_" (expression file)
        if(p.path().has_extension() && p.path().extension() == ".json" && p.path().filename().string().find("expr_") == 0){
            CubeLog::info("ExpressionLoader: Found file: " + p.path().string());
            names.push_back(p.path().string());
        }
    }
    CubeLog::info("ExpressionLoader: Found " + std::to_string(names.size()) + " expression files");
    return names;
}

std::vector<ExpressionDefinition> ExpressionLoader::loadExpressions(std::vector<std::string> fileNames)
{
    std::vector<ExpressionDefinition> expressions;
    for (auto filename : fileNames) {
        CubeLog::info("ExpressionLoader: Loading expression file: " + filename);
        std::ifstream file(filename);
        if (!file.is_open()) {
            CubeLog::info("ExpressionLoader: Failed to open file: " + filename);
            continue;
        }
        nlohmann::json jsonObject;
        try {
            file >> jsonObject;
        } catch (nlohmann::json::parse_error& e) {
            CubeLog::error("ExpressionLoader: Failed to parse json file: " + filename);
            CubeLog::error(e.what());
            continue;
        } catch (nlohmann::json::exception& e) {
            CubeLog::error("ExpressionLoader: Failed to load expression from file: " + filename);
            CubeLog::error(e.what());
            continue;
        } catch (std::exception& e) {
            CubeLog::error("ExpressionLoader: Failed to load expression from file: " + filename);
            CubeLog::error(e.what());
            continue;
        }
        for (auto expression : jsonObject) {
            expressions.push_back({ expression["name"], expression["expression"], expression["objects"], expression["visibility"] });
        }
    }
    return expressions;
}

std::vector<ExpressionDefinition> ExpressionLoader::getExpressionsVector()
{
    std::vector<ExpressionDefinition> expressions;
    for (auto& [key, value] : expressionsMap) {
        expressions.push_back(value);
    }
    return expressions;
}

ExpressionDefinition ExpressionLoader::getExpressionByName(std::string name)
{
    // Convert name to lowercase
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    // Map from string to Expressions enum
    static const std::unordered_map<std::string, Expressions::ExpressionNames_enum> nameToExpression = {
        {"neutral", Expressions::NEUTRAL},
        {"default", Expressions::NEUTRAL},
        {"happy", Expressions::HAPPY},
        {"sad", Expressions::SAD},
        {"angry", Expressions::ANGRY},
        {"surprised", Expressions::SURPRISED},
        {"disgusted", Expressions::DISGUSTED},
        {"scared", Expressions::SCARED},
        {"confused", Expressions::CONFUSED},
        {"dizzy", Expressions::DIZZY},
        {"sick", Expressions::SICK},
        {"sleepy", Expressions::SLEEPY},
        {"confused", Expressions::CONFUSED},
        {"shocked", Expressions::SHOCKED},
        {"injured", Expressions::INJURED},
        {"dead", Expressions::DEAD},
        {"screaming", Expressions::SCREAMING},
        {"talking", Expressions::TALKING},
        {"listening", Expressions::LISTENING},
        {"sleeping", Expressions::SLEEPING},
        {"dead", Expressions::DEAD},
        {"funny_index", Expressions::FUNNY_INDEX},
        {"funny_bounce", Expressions::FUNNY_BOUNCE},
        {"funny_spin", Expressions::FUNNY_SPIN},
        {"funny_shrink", Expressions::FUNNY_SHRINK},
        {"funny_expand", Expressions::FUNNY_EXPAND},
        {"funny_jump", Expressions::FUNNY_JUMP},
    };

    // Find the expression in the map
    auto it = nameToExpression.find(name);
    if (it != nameToExpression.end()) {
        return expressionsMap[it->second];
    }

    // Default return value if no match is found
    return { Expressions::NEUTRAL, "", {}, {} };
}

ExpressionDefinition ExpressionLoader::getExpressionByEnum(Expressions::ExpressionNames_enum name)
{
    return expressionsMap[name];
}

std::map<Expressions::ExpressionNames_enum, ExpressionDefinition> ExpressionLoader::getExpressions()
{
    return expressionsMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AnimationLoaderException::count = 0;

AnimationLoaderException::AnimationLoaderException(std::string message)
{
    AnimationLoaderException::count++;
    this->message = "Error Count: " + std::to_string(AnimationLoaderException::count) + "\n" + message;
}

const char* AnimationLoaderException::what() const throw()
{
    return this->message.c_str();
}

// Path: src/gui/characters/meshes/meshObject.h
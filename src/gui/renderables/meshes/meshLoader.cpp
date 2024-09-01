#include "meshLoader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

MeshLoader::MeshLoader(Shader* shdr, std::vector<std::string> toLoad)
{
    // TODO: Add support for loading .obj files
    this->shader = shdr;
    std::vector<std::string> filenames = this->getFileNames();
    // load .mesh files
    for (auto filename : filenames) {
        // ensure filetype is .mesh
        if (filename.find(".mesh") == std::string::npos || filename.find(".mesh") != filename.length() - 5) {
            continue;
        }
        std::string name = filename.substr(filename.find_last_of('/') + 1, filename.find(".mesh") - filename.find_last_of('/') - 1);
        if (std::find(toLoad.begin(), toLoad.end(), name) == toLoad.end()) {
            continue;
        }
        CubeLog::info("MeshLoader: Found mesh file: " + filename);
        CubeLog::info("Attempting to load mesh file: " + filename);
        std::vector<MeshObject*> objects = this->loadMesh(filename);
        CubeLog::info("MeshLoader: Loaded " + std::to_string(objects.size()) + " objects from file: " + filename);
        this->collections.push_back(new ObjectCollection());
        // collection name is filename minus ".mesh"
        this->collections.at(collections.size() - 1)->name = filename.substr(filename.find_last_of('/') + 1, filename.find(".mesh") - filename.find_last_of('/') - 1);
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
        std::string name = filename.substr(filename.find_last_of('/') + 1, filename.find(".obj") - filename.find_last_of('/') - 1);
        if (std::find(toLoad.begin(), toLoad.end(), name) == toLoad.end()) {
            continue;
        }
        CubeLog::info("MeshLoader: Found obj file: " + filename);
        CubeLog::info("Attempting to load obj file: " + filename);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> mats;
        std::string warn, err;
        std::map<int, tinyobj::material_t> materials;
        std::vector<OBJObject*> objects;
        std::vector<Vertex> vertices;

        // Load the OBJ file and its materials
        bool success = tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, filename.c_str(), nullptr, true);

        if (!warn.empty()) {
            CubeLog::warning("MeshLoader: " + warn);
        }

        if (!err.empty()) {
            CubeLog::error("MeshLoader: Failed to load obj file: " + filename);
            CubeLog::error(err);
            continue;
        }

        if (!success) {
            CubeLog::error("MeshLoader: Failed to load obj file: " + filename);
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
        CubeLog::info("MeshLoader: Loaded " + std::to_string(shapes.size()) + " shapes from obj file: " + filename);
        for (auto shape : shapes) {
            objects.push_back(new OBJObject(this->shader, vertices));
        }
        CubeLog::info("MeshLoader: Loaded " + std::to_string(objects.size()) + " objects from obj file: " + filename);
        this->collections.push_back(new ObjectCollection());
        // collection name is filename minus ".obj"
        this->collections.at(collections.size() - 1)->name = filename.substr(filename.find_last_of('/') + 1, filename.find(".obj") - filename.find_last_of('/') - 1);
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
    for (auto& p : std::filesystem::directory_iterator("meshes/")) {
        CubeLog::info("MeshLoader: Found file: " + p.path().string());
        names.push_back(p.path().string());
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

// Path: src/gui/characters/meshes/meshObject.h
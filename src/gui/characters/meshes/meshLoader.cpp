#include "meshLoader.h"

MeshLoader::MeshLoader(CubeLog* lgr, Shader* shdr)
{
    this->logger = lgr;
    this->shader = shdr;
    std::vector<std::string> filenames = this->getMeshFileNames();
    for(auto filename: filenames){
        this->logger->log("MeshLoader: Found mesh file: " + filename, true);
        this->logger->log("Attempting to load mesh file: " + filename, true);
        std::vector<MeshObject*> objects = this->loadMesh(filename);
        this->logger->log("MeshLoader: Loaded " + std::to_string(objects.size()) + " objects from file: " + filename, true);
        this->collections.push_back(new ObjectCollection());
        // collection name is filename minus ".mesh"
        this->collections.at(collections.size() - 1)->name = filename.substr(filename.find_last_of('/') + 1, filename.find(".mesh") - filename.find_last_of('/') - 1);
        for(auto object: objects){
            this->collections.at(collections.size() - 1)->objects.push_back(object);
        }
    }
}

MeshLoader::~MeshLoader()
{
    for(auto collection: this->collections){
        delete collection;
    }
}

std::vector<MeshObject*> MeshLoader::getObjects()
{
    std::vector<MeshObject*> objects;
    for(auto collection: this->collections){
        for(auto object: collection->objects){
            objects.push_back(object);
        }
    }
    return objects;
}

std::vector<ObjectCollection*> MeshLoader::getCollections()
{
    return this->collections;
}

std::vector<std::string> MeshLoader::getMeshFileNames()
{
    std::vector<std::string> names;
    for(auto& p: std::filesystem::directory_iterator("meshes/")){
        this->logger->log("MeshLoader: Found file: " + p.path().string(), true);
        names.push_back(p.path().string());
    }
    this->logger->log("MeshLoader: Found " + std::to_string(names.size()) + " mesh files", true);
    return names;
}

std::vector<MeshObject*> MeshLoader::loadMesh(std::string path)
{
    std::vector<MeshObject*> objects;
    std::ifstream
    file(path);
    if(!file.is_open()){
        this->logger->log("MeshLoader: Failed to open file: " + path, true);
        return objects;
    }
    float scale = 1.0f;
    std::string line;
    while(std::getline(file, line)){
        // if line starts with a "#" its a comment, skip it
        if(line[0] == '#'){
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
        if(line.find("@SCALE") != std::string::npos){
            // the scale value is the next token on the line
            std::string floatStr = line.substr(6);
            std::istringstream iss(floatStr);
            float sc;
            iss >> sc;
            scale = sc;
            this->logger->log("MeshLoader: Scaling all objects by: " + std::to_string(scale), true);
        }
        else if(type == "$CUBES"){
            this->logger->log("MeshLoader: Loading cubes...", true);
            unsigned int count = 0;
            while(std::getline(file, line)){
                if(line[0] == '$'){
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
            this->logger->log("MeshLoader: Loaded " + std::to_string(count) + " cubes", true);
        }
        else if(type == "$PYRAMIDS"){
            while(std::getline(file, line)){
                if(line[0] == '$'){
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new PyramidMeshObject(x, y, z));
            }
        }
        else if(type == "$SPHERES"){
            while(std::getline(file, line)){
                if(line[0] == '$'){
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new SphereMeshObject(x, y, z));
            }
        }
        else if(type == "$CYLINDERS"){
            while(std::getline(file, line)){
                if(line[0] == '$'){
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new CylinderMeshObject(x, y, z));
            }
        }
        else if(type == "$CONE"){
            while(std::getline(file, line)){
                if(line[0] == '$'){
                    break;
                }
                std::istringstream iss(line);
                float x, y, z;
                iss >> x >> y >> z;
                // objects.push_back(new ConeMeshObject(x, y, z));
            }
        }
        else if(type == "$TORUS"){
            while(std::getline(file, line)){
                if(line[0] == '$'){
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
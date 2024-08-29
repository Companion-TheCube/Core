#pragma once

#include "../../shader.h"
#include "../meshObject.h"
#include "shapes.h"
#include <fstream>
#include <iostream>
#include <logger.h>
#include <vector>

struct ObjectCollection {
    std::vector<MeshObject*> objects;
    std::string name;
};

class MeshLoader {
public:
    std::vector<ObjectCollection*> collections;
    Shader* shader;
    MeshLoader(Shader* shdr, std::vector<std::string> toLoad);
    ~MeshLoader();
    std::vector<MeshObject*> loadMesh(std::string path);
    std::vector<std::string> getFileNames();
    std::vector<MeshObject*> getObjects();
    std::vector<ObjectCollection*> getCollections();
};

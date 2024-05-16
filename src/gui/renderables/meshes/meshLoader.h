#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <logger.h>
#include "shapes.h"
#include "../meshObject.h"
#include "../../shader.h"

struct ObjectCollection{
    std::vector<MeshObject*> objects;
    std::string name;
};

class MeshLoader{
public:
    std::vector<ObjectCollection*> collections;
    CubeLog* logger;
    Shader* shader;
    MeshLoader(CubeLog* lgr, Shader* shdr, std::vector<std::string> toLoad);
    ~MeshLoader();
    std::vector<MeshObject*> loadMesh(std::string path);
    std::vector<std::string> getMeshFileNames();
    std::vector<MeshObject*> getObjects();
    std::vector<ObjectCollection*> getCollections();
};


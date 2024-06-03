#pragma once
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <latch>
#include <typeinfo>
#include "./../renderables/meshObject.h"
#include "./../renderables/meshes/shapes.h"
#include "./../objects.h"
#include "./../shader.h"
#include "./../renderer.h"

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
#define STENCIL_INSET_PX 6
#define MESSAGEBOX_ITEM_TEXT_SIZE 32.f

class CubeMessageBox: public M_Box{
private:
    bool visible;
    std::vector<MeshObject*> objects;
    Shader* shader;
    Shader* textShader;
    float messageTextSize = MESSAGEBOX_ITEM_TEXT_SIZE;
    long scrollVertPosition = 0;
    float index = 0.001;
    std::latch* latch;
    std::mutex mutex;
    std::vector<size_t> textMeshIndices;
    Renderer* renderer;
public:
    CubeMessageBox(Shader* shader, Shader* textShader, Renderer* renderer, std::latch& latch);
    ~CubeMessageBox();
    void setup();
    bool setVisible(bool visible);
    bool getVisible();
    void draw();
    void setPosition(glm::vec2 position);
    void setSize(glm::vec2 size);
    std::vector<MeshObject*> getObjects();
    void setText(std::string text);
};
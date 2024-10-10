#pragma once
#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <latch>
#include <typeinfo>
#ifndef SHAPES_H
#include "./../renderables/shapes.h"
#endif
#ifndef OBJECTS_H
#include "./../objects.h"
#endif
#ifndef SHADER_H
#include "./../shader.h"
#endif
#ifndef RENDERER_H
#include "./../renderer.h"
#endif

#define Z_DISTANCE 3.57
#define BOX_RADIUS 0.05
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
    CountingLatch* latch;
    std::mutex mutex;
    std::vector<size_t> textMeshIndices;
    Renderer* renderer;
public:
    CubeMessageBox(Shader* shader, Shader* textShader, Renderer* renderer, CountingLatch& latch);
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

#endif// MESSAGEBOX_H

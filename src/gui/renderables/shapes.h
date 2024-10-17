#ifndef SHAPES_H
#define SHAPES_H
#ifndef MESHOBJECT_H
#include "meshObject.h"
#endif // SHAPES_H
#ifndef GLOBAL_SETTINGS_H
#include "settings/globalSettings.h"
#endif // GLOBAL_SETTINGS_H
#include <ft2build.h>
#include FT_FREETYPE_H

#define STENCIL_INSET_PX 10

struct Character {
    unsigned int textureID; // ID handle of the glyph texture
    glm::ivec2 size; // Size of glyph
    glm::ivec2 bearing; // Offset from baseline to left/top of glyph
    FT_Pos advance; // Offset to advance to next glyph
};

class M_Text : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO, VBO;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    std::string text;
    float fontSize;
    glm::vec3 color;
    glm::vec3 centerPoint;
    glm::vec2 position;
    float scale_;
    std::map<char, Character> characters;
    float width = 0.f;
    void buildText();
    static FT_Library ft;
    static FT_Face face;
    static bool faceInitialized;
    bool reloadFace = false;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    glm::vec2 capturedPosition;
    bool visible = true;
    std::mutex mutex;

public:
    M_Text(Shader* sh, std::string text, float fontSize, glm::vec3 color, glm::vec2 position);
    ~M_Text();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    void setPosition(glm::vec2 position);
    void setText(std::string text);
    float getWidth();
    void reloadFont();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
    void setColor(glm::vec3 color);
};

class M_PartCircle : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO[1], VBO[1];
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    float radius;
    glm::vec3 centerPoint;
    unsigned int numSegments;
    float startAngle;
    float endAngle;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;

public:
    M_PartCircle(Shader* sh, unsigned int numSegments, float radius, glm::vec3 centerPoint, float startAngle, float endAngle, glm::vec3 fillColor);
    ~M_PartCircle();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

class M_Rect : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexDataFill;
    std::vector<Vertex> vertexDataBorder;
    glm::vec3 fillColor;
    glm::vec3 borderColor;
    GLuint VAO[2], VBO[2];
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;

public:
    M_Rect(Shader* sh, glm::vec3 position, glm::vec2 size, float fillColor, float borderColor);
    ~M_Rect();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

class M_Line : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO[1], VBO[1];
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;

public:
    M_Line(Shader* sh, glm::vec3 start, glm::vec3 end);
    ~M_Line();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

class M_Arc : public MeshObject {
private:
    unsigned int numSegments;
    float radius;
    float startAngle;
    float endAngle;
    glm::vec3 centerPoint;
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO[1], VBO[1];
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;
    glm::vec3 fillColor = { 1.f, 1.f, 1.f };

public:
    M_Arc(Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint, glm::vec3 fillColor);
    M_Arc(Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint);
    ~M_Arc();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

#define RADIOBUTTON_INNER_OUTER_RATIO 0.8f
#define RADIOBUTTON_DOT_INNER_RATIO 0.75f

class M_RadioButtonTexture : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO, VBO;
    GLuint textureSelected, textureUnselected;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    bool selected;
    float radioSize;
    unsigned int padding;
    glm::vec3 color;
    glm::vec2 position;
    float scale_;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    glm::vec2 capturedPosition;
    bool visible = true;
    std::mutex mutex;
    unsigned char* selectedTextureBitmap;
    unsigned char* unselectedTextureBitmap;

public:
    M_RadioButtonTexture(Shader* sh, float radioSize, unsigned int padding, glm::vec3 color, glm::vec2 position);
    ~M_RadioButtonTexture();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    void setPosition(glm::vec2 position);
    void setSelected(bool selected);
    void setVisibility(bool visible);
    void capturePosition();
    void restorePosition();
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
    float getWidth();
};

#define TOGGLE_INNER_OUTER_RATIO 0.8f
#define TOGGLE_DOT_INNER_RATIO 0.75f

class M_ToggleTexture : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    GLuint VAO, VBO;
    GLuint textureSelected, textureUnselected;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    bool selected;
    float toggleWidth;
    float toggleHeight;
    unsigned int padding;
    glm::vec3 color;
    glm::vec2 position;
    float scale_;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    glm::vec2 capturedPosition;
    bool visible = true;
    std::mutex mutex;
    unsigned char* selectedTextureBitmap;
    unsigned char* unselectedTextureBitmap;

public:
    M_ToggleTexture(Shader* sh, float toggleWidth, float toggleHeight, unsigned int padding, glm::vec3 color, glm::vec2 position);
    ~M_ToggleTexture();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    void setPosition(glm::vec2 position);
    void setSelected(bool selected);
    void setVisibility(bool visible);
    void capturePosition();
    void restorePosition();
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
    float getWidth();
};

#define CUBE_VERTICES_CONST 1.0f
#define EDGE_VERTICES_OFFSET 0.02f
#define BLACK_FLOATS 0.0f
#define WHITE_FLOATS 1.f

class Cube : public MeshObject {
private:
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;

public:
    const Vertex cubeVertices[8] = {
        // Front face
        { -CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, WHITE_FLOATS },
        { CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, WHITE_FLOATS },
        { CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, WHITE_FLOATS },
        { -CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, WHITE_FLOATS },

        // Back face
        { -CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, WHITE_FLOATS },
        { CUBE_VERTICES_CONST, -CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, WHITE_FLOATS },
        { CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, WHITE_FLOATS },
        { -CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, CUBE_VERTICES_CONST, WHITE_FLOATS }
    };
    const Vertex edgeVertices[24] = {
        // front face
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },

        // back face
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), BLACK_FLOATS },

        // left face
        { -(CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },

        // right face
        { (CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },

        // bottom face
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), -(CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },

        // top face
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS },
        { -(CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), (CUBE_VERTICES_CONST + 0.01f), (CUBE_VERTICES_CONST - EDGE_VERTICES_OFFSET), BLACK_FLOATS }
    };
    const unsigned int indices[36] = {
        0, 1, 2, 2, 3, 0, // Front face
        7, 6, 5, 5, 4, 7, // Back face
        4, 0, 3, 3, 7, 4, // bottom face
        5, 6, 2, 2, 1, 5, // top face
        1, 0, 4, 4, 5, 1, // left face
        3, 2, 6, 6, 7, 3 // right face
    };
    GLuint VAOs[7], VBOs[7], EBOs[7];
    const unsigned int faceIndices[6] = { 0, 1, 2, 2, 3, 0 };
    Cube(Shader* sh);
    ~Cube();
    Shader* shader;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;

    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

// TODO: create a generic object class that can utilize vertex and face data loaded from a file

class OBJObject : public MeshObject {
private:
    Shader* shader;
    std::vector<Vertex> vertexData;
    std::vector<unsigned int> faceData;
    GLuint VAO, VBO, EBO;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;
    bool visible = true;
    std::mutex mutex;

public:
    OBJObject(Shader* sh, std::vector<Vertex> vertices);
    ~OBJObject();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setViewMatrix(glm::mat4 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
    glm::mat4 getModelMatrix();
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void translate(glm::vec3 translation);
    void rotate(float angle, glm::vec3 axis);
    void scale(glm::vec3 scale);
    void uniformScale(float scale);
    void rotateAbout(float angle, glm::vec3 axis, glm::vec3 point);
    void rotateAbout(float angle, glm::vec3 point);
    glm::vec3 getCenterPoint();
    std::vector<Vertex> getVertices();
    float getWidth();
    void capturePosition();
    void restorePosition();
    void setVisibility(bool visible);
    void getRestorePositionDiff(glm::mat4* modelMatrix, glm::mat4* viewMatrix, glm::mat4* projectionMatrix);
};

#endif // SHAPES_H

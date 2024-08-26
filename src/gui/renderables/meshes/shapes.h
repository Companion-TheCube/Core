#pragma once
#include "../meshObject.h"
#include <ft2build.h>
#include <globalSettings.h>
#include FT_FREETYPE_H

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size; // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    unsigned int Advance; // Offset to advance to next glyph
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
    std::map<char, Character> Characters;
    float width = 0.f;
    void buildText();
    static FT_Library ft;
    static FT_Face face;
    static bool faceInitialized;
    glm::mat4 capturedProjectionMatrix;
    glm::mat4 capturedViewMatrix;
    glm::mat4 capturedModelMatrix;

public:
    M_Text(Shader* sh, std::string text, float fontSize, glm::vec3 color, glm::vec2 position);
    ~M_Text();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
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

public:
    M_PartCircle(Shader* sh, unsigned int numSegments, float radius, glm::vec3 centerPoint, float startAngle, float endAngle, float fillColor);
    ~M_PartCircle();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
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

public:
    M_Rect(Shader* sh, glm::vec3 position, glm::vec2 size, float fillColor, float borderColor);
    ~M_Rect();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
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

public:
    M_Line(Shader* sh, glm::vec3 start, glm::vec3 end);
    ~M_Line();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
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

public:
    M_Arc(Shader* sh, unsigned int numSegments, float radius, float startAngle, float endAngle, glm::vec3 centerPoint);
    ~M_Arc();
    void draw();
    void setProjectionMatrix(glm::mat4 projectionMatrix);
    void setViewMatrix(glm::vec3 viewMatrix);
    void setModelMatrix(glm::mat4 modelMatrix);
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
    void setModelMatrix(glm::mat4 modelMatrix);
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
};

// TODO: create a generic object class that can utilize vertex and face data loaded from a file
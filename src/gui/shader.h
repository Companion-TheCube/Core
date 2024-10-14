#ifndef SHADER_H
#define SHADER_H
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include <string>
#include <filesystem>
#include "GL/glew.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include "SFML/Graphics.hpp"
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <logger.h>
#include <mutex>

class Shader{
    private:
        std::string vertexShader;
        std::string fragmentShader;
        std::string readShader(std::string path);
        std::mutex mutex;
    public:
        unsigned int ID;
        Shader(){};
        Shader(std::string vertexShaderPath, std::string fragmentShaderPath);
        ~Shader();
        void use();
        void setBool(const std::string &name, bool value) const;
        void setInt(const std::string &name, int value) const;
        void setFloat(const std::string &name, float value) const;
        void setVec2(const std::string &name, float x, float y) const;
        void setVec3(const std::string &name, float x, float y, float z) const;
        void setVec4(const std::string &name, float x, float y, float z, float w) const;
        void setMat4(const std::string &name, glm::mat4 value) const;
};

#endif
#pragma once
#ifndef MESHOBJECT_H
#define MESHOBJECT_H
#ifndef WIN32_INCLUDED
#define WIN32_INCLUDED
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#endif
#include <vector>
#ifndef OBJECTS_H
#include "../objects.h"
#endif // OBJECTS_H
#ifndef SHADER_H
#include "../shader.h"
#endif // SHADER_H
#include "GL/glew.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#endif
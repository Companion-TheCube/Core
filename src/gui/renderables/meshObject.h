/*
███╗   ███╗███████╗███████╗██╗  ██╗ ██████╗ ██████╗      ██╗███████╗ ██████╗████████╗██╗  ██╗
████╗ ████║██╔════╝██╔════╝██║  ██║██╔═══██╗██╔══██╗     ██║██╔════╝██╔════╝╚══██╔══╝██║  ██║
██╔████╔██║█████╗  ███████╗███████║██║   ██║██████╔╝     ██║█████╗  ██║        ██║   ███████║
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║██║   ██║██╔══██╗██   ██║██╔══╝  ██║        ██║   ██╔══██║
██║ ╚═╝ ██║███████╗███████║██║  ██║╚██████╔╝██████╔╝╚█████╔╝███████╗╚██████╗   ██║██╗██║  ██║
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝  ╚════╝ ╚══════╝ ╚═════╝   ╚═╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License

Copyright (c) 2025 A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
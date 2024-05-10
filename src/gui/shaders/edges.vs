#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;   // Correct use: specifies the location of vertex attribute
layout(location = 1) in vec3 aColor; // Correct use: specifies the location of another vertex attribute

out vec3 vertexColor; // Pass color to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
#version 300 es

in vec4 vertex;
out vec2 TexCoords;

uniform float zindex;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, zindex, 1.0);
    TexCoords = vertex.zw;
}

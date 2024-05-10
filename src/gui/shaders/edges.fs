#version 300 es

in mediump vec3 vertexColor;
out mediump vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0); // Set fragment color to white
} 
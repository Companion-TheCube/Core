#version 300 es
precision mediump float;

in mediump vec3 vertexColor;
out mediump vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}

#version 300 core
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in vec3 normals[];
out vec3 outColor;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main() {
    // Calculate the edge vectors
    vec3 edge1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 edge2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 faceNormal = normalize(cross(edge1, edge2));

    // Emit edges if the face normal is facing towards the camera
    if (dot(faceNormal, (inverse(viewMatrix) * vec4(0.0, 0.0, 1.0, 0.0)).xyz) > 0) {
        for (int i = 0; i < 3; ++i) {
            gl_Position = projectionMatrix * viewMatrix * modelMatrix * gl_in[i].gl_Position;
            outColor = vec3(1.0, 1.0, 1.0); // White color for edges
            EmitVertex();
            
            int next = (i + 1) % 3;
            gl_Position = projectionMatrix * viewMatrix * modelMatrix * gl_in[next].gl_Position;
            EmitVertex();
            EndPrimitive();
        }
    }
}

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 WorldNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Project the coordinate directly using our matrices
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    WorldNormal = mat3(model) * aNormal;
}
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Project the coordinate directly using our matrices
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
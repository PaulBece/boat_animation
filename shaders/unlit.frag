#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;

void main() {
    // Output the pure color of the light with full intensity
    FragColor = vec4(lightColor, 1.0);
}
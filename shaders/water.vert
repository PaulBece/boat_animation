#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time; // Linked to your running CPU elapsed execution time

void main() {
    vec3 currentPos = aPos;

    // --- WAVE PARAMETERS ---
    // Wave 1: Rolling swell moving along the X-axis
    float amp1 = 0.5;       // Amplitude (height of wave)
    float freq1 = 0.15;     // Frequency (width of wave)
    float speed1 = 1.6;     // Velocity over time
    float wave1 = amp1 * sin(currentPos.x * freq1 + time * speed1);

    // Wave 2: Criss-crossing choppy wave moving along the Z-axis
    float amp2 = 0.3;       
    float freq2 = 0.25;     
    float speed2 = 2.2;     
    float wave2 = amp2 * cos(currentPos.z * freq2 + time * speed2);

    // 1. Displace the vertex Y position dynamically
    currentPos.y += wave1 + wave2;

    // 2. Compute the exact surface derivatives (slopes) to find the new normal
    // This is necessary because the original flat grid normal (0,1,0) is no longer accurate
    float dy_dx = amp1 * freq1 * cos(currentPos.x * freq1 + time * speed1);
    float dy_dz = -amp2 * freq2 * sin(currentPos.z * freq2 + time * speed2);
    
    // Tangent vectors along the curving mesh surface
    vec3 tangent = vec3(1.0, dy_dx, 0.0);
    vec3 bitangent = vec3(0.0, dy_dz, 1.0);
    
    // The cross product gives us a vector perfectly perpendicular to the shifting wave surface
    vec3 computedNormal = normalize(cross(bitangent, tangent));

    // 3. Output standard world and clip matrices for our scene pipeline
    FragPos = vec3(model * vec4(currentPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * computedNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * model * vec4(currentPos, 1.0);
}
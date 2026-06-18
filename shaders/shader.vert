#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float FogDepth;     // NEW: distancia en view space al fragmento

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Transform vertex position into world space for accurate light calculations
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform normal vectors safely, handling any custom object scaling factors
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoords = aTexCoords;

    // Compute view-space position and extract depth (z) for fog distance
    vec4 viewSpacePos = view * model * vec4(aPos, 1.0);
    FogDepth = abs(viewSpacePos.z);   // z es negativo en OpenGL, abs() lo corrige

    gl_Position = projection * viewSpacePos;
}

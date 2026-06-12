#version 330 core
out vec4 FragColor;

// --- LIGHT TYPE STRUCTURES ---
struct DirLight {
    vec3 direction;
    vec3 color;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float cutOff;
    float outerCutOff;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
};

// --- UNIFORMS ---
uniform vec3 viewPos;
uniform sampler2D diffuseTexture;

uniform DirLight dirLight;
uniform bool hasDirLight;

#define MAX_POINT_LIGHTS 4
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int activePointLightCount;

#define MAX_SPOT_LIGHTS 4
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int activeSpotLightCount;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// --- LIGHT CALCULATOR FUNCTIONS ---

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    
    vec3 ambient  = light.ambientStrength * light.color;
    vec3 diffuse  = light.diffuseStrength * diff * light.color;
    vec3 specular = light.specularStrength * spec * light.color;
    return (ambient + diffuse + specular);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    
    vec3 ambient  = light.ambientStrength * light.color;
    vec3 diffuse  = light.diffuseStrength * diff * light.color;
    vec3 specular = light.specularStrength * spec * light.color;
    return (ambient + diffuse + specular);
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    
    // Spotlight intensity check (using soft edge interpolation formula)
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient  = light.ambientStrength * light.color;
    vec3 diffuse  = light.diffuseStrength * diff * light.color * intensity;
    vec3 specular = light.specularStrength * spec * light.color * intensity;
    return (ambient + diffuse + specular);
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 totalLighting = vec3(0.0);

    // 1. Accumulate Global Sun Illumination if present
    if (hasDirLight) {
        totalLighting += CalculateDirLight(dirLight, norm, viewDir);
    }

    // 2. Accumulate Point Lights (Boats lanterns)
    for (int i = 0; i < activePointLightCount; i++) {
        totalLighting += CalculatePointLight(pointLights[i], norm, FragPos, viewDir);
    }

    // 3. Accumulate Spotlights (Lighthouses / Searchlights)
    for (int i = 0; i < activeSpotLightCount; i++) {
        totalLighting += CalculateSpotLight(spotLights[i], norm, FragPos, viewDir);
    }

    vec3 textureColor = texture(diffuseTexture, TexCoords).rgb;
    FragColor = vec4(totalLighting * textureColor, 1.0);
}
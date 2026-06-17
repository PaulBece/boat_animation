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

uniform DirLight dirLight;
uniform bool hasDirLight;

#define MAX_POINT_LIGHTS 4
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int activePointLightCount;

#define MAX_SPOT_LIGHTS 4
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int activeSpotLightCount;

uniform float waterSurfaceY; // Passed dynamically from C++ (-8.0f)

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Constant distance attenuation variables 
const float constantK  = 1.0;
const float linearK    = 0.04;
const float quadraticK = 0.007;

// --- MULTI-LIGHT SHADING ROUTINES ---

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);

    vec3 ambient  = light.ambientStrength * light.color;
    vec3 diffuse  = light.diffuseStrength * diff * light.color;
    vec3 specular = light.specularStrength * spec * light.color;
    return (ambient + diffuse + specular);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    float distance = length(light.position - fragPos);
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);

    float attenuation = 1.0 / (constantK + linearK * distance + quadraticK * (distance * distance));

    vec3 ambient  = light.ambientStrength * light.color * attenuation;
    vec3 diffuse  = light.diffuseStrength * diff * light.color * attenuation;
    vec3 specular = light.specularStrength * spec * light.color * attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    float distance = length(light.position - fragPos);
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float attenuation = 1.0 / (constantK + linearK * distance + quadraticK * (distance * distance));

    vec3 ambient  = light.ambientStrength * light.color * attenuation;
    vec3 diffuse  = light.diffuseStrength * diff * light.color * intensity * attenuation;
    vec3 specular = light.specularStrength * spec * light.color * intensity * attenuation;
    return (ambient + diffuse + specular);
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 totalLighting = vec3(0.0);

    // 1. Process standard light caster blocks
    if (hasDirLight) totalLighting += CalculateDirLight(dirLight, norm, viewDir);
    for (int i = 0; i < activePointLightCount; i++) totalLighting += CalculatePointLight(pointLights[i], norm, FragPos, viewDir);
    for (int i = 0; i < activeSpotLightCount; i++) totalLighting += CalculateSpotLight(spotLights[i], norm, FragPos, viewDir);

    // 2. PROCEDURAL WATER COLOR CODES (Corrected to support height offsets)
    vec3 deepWaterColor  = vec3(0.01, 0.05, 0.15); // Rich deep navy blue
    vec3 shallowCrestColor = vec3(0.0, 0.45, 0.55); // Vibrant turquoise

    // FIX 1: Subtract waterSurfaceY to look at relative wave height local displacement instead of absolute coordinates!
    float localWaveHeight = FragPos.y - waterSurfaceY;
    
    // Smoothly map relative wave heights [-0.8, 0.8] to a clean [0.0, 1.0] mix factor
    float heightMixFactor = clamp((localWaveHeight + 0.8) / 1.6, 0.0, 1.0);
    vec3 waterBaseColor = mix(deepWaterColor, shallowCrestColor, heightMixFactor);

    // Calculate final shaded surface water color
    vec3 finalWaterSurfaceColor = totalLighting * waterBaseColor;

    // ====================================================================
    // FIX 2: HORI-DISTANCE ATMOSPHERIC WATER FOG
    // ====================================================================
    // To cleanly obscure objects into the boundless distance, we track 
    // the physical distance between your camera lens eye position and this fragment.
    float viewDistance = length(viewPos - FragPos);
    
    // Calibrated density for an immersive open-sea horizon fade
    float fogDensity = 0.015; 
    float visibilityFactor = exp(-fogDensity * viewDistance);
    
    // As you look further toward the clipping boundary, the water thickens 
    // into an opaque sheet, seamlessly blocking out objects hiding past the horizon.
    float baseAlpha = 0.75; // Surface clarity base
    float dynamicAlpha = mix(1.0, baseAlpha, visibilityFactor);

    // 3. Output our final shaded pixel block straight to the screen buffers
    FragColor = vec4(finalWaterSurfaceColor, dynamicAlpha);
}
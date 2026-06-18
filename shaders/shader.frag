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

// --- FOG UNIFORMS ---
uniform vec3  fogColor;     // Color de la niebla (debe coincidir con glClearColor)
uniform float fogDensity;   // Densidad: 0.01 poca niebla, 0.04 niebla densa

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoords;
in float FogDepth;          // NEW: distancia del fragmento a la camara

// --- ATTENUATION COEFFICIENTS ---
// Industry standard multipliers calibrated for a ~50 unit clear rendering range
const float constantK  = 1.0;
const float linearK    = 0.09;
const float quadraticK = 0.032;

// --- LIGHT CALCULATOR FUNCTIONS ---

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

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
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

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
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

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
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 totalLighting = vec3(0.0);

    // 1. Accumulate Global Moonlight if present
    if (hasDirLight) {
        totalLighting += CalculateDirLight(dirLight, norm, viewDir);
    }

    // 2. Accumulate Point Lights (Boat lanterns)
    for (int i = 0; i < activePointLightCount; i++) {
        totalLighting += CalculatePointLight(pointLights[i], norm, FragPos, viewDir);
    }

    // 3. Accumulate Spotlights (Lighthouses / Searchlights)
    for (int i = 0; i < activeSpotLightCount; i++) {
        totalLighting += CalculateSpotLight(spotLights[i], norm, FragPos, viewDir);
    }

    vec3 textureColor = texture(diffuseTexture, TexCoords).rgb;
    vec3 litColor     = totalLighting * textureColor;

    // --- FOG: formula exponencial cuadratica (suave y natural para ambientes maritimos) ---
    // fogFactor = 1.0 -> color original (cerca de la camara)
    // fogFactor = 0.0 -> color puro de niebla (lejos de la camara)
    float fogExponent = fogDensity * FogDepth;
    float fogFactor   = exp(-(fogExponent * fogExponent));
    fogFactor         = clamp(fogFactor, 0.0, 1.0);

	//este cambio hace que en vez de tornarse del color del fondo basico, se vaya haciendo transparente
	FragColor = vec4(litColor, fogFactor);
	// Esta es la version anterior:
	//vec3 finalColor = mix(fogColor, litColor, fogFactor);
	//FragColor = vec4(finalColor, 1.0);
}

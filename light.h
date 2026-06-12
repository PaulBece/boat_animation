#ifndef LIGHT_H
#define LIGHT_H

#include "myglm.h"

enum class LightType {
    DIRECTIONAL = 0, // Like the sun (infinite distance, parallel rays)
    POINT       = 1, // Like a light bulb (has a position, shoots 360°)
    SPOTLIGHT   = 2  // Like a flashlight or lighthouse beam (has a cone)
};

class Light {
public:
    LightType type;
    
    // Core properties shared by all lights
    myglm::vec3 color;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;

    // Spatial properties (used depending on the type)
    myglm::vec3 position;  // For Point and Spotlight
    myglm::vec3 direction; // For Directional and Spotlight

    // Spotlight specific cone variables (stored as cosines for fast shader math)
    float cutOff;      // Inner cone angle
    float outerCutOff; // Outer smooth-fading cone angle

    // Constructor initializing a standard warm point light by default
    Light(LightType lightType = LightType::POINT, myglm::vec3 startPos = myglm::vec3(0.0f, 15.0f, 0.0f))
        : type(lightType), position(startPos), color(1.0f, 0.95f, 0.85f),
          ambientStrength(0.15f), diffuseStrength(0.8f), specularStrength(0.4f),
          direction(0.0f, -1.0f, 0.0f), cutOff(std::cos(myglm::radians(12.5f))), outerCutOff(std::cos(myglm::radians(17.5f))) {}

    // Method to move the light dynamically over time
    void translate(const myglm::vec3& offset) {
        position += offset;
    }

    void setDirection(const myglm::vec3& dir) {
        direction = myglm::normalize(dir);
    }
};

#endif
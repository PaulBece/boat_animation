#ifndef ANIMATION_GROUP_H
#define ANIMATION_GROUP_H

#include "myglm.h"
#include "scene_element.h"
#include "light_element.h"
#include "camera.h"
#include <vector>

enum class AnimationType {
    STATIC,          // Node cluster is stationary in the world
    ANCHORED_WATER,  // Node cluster bobs and tilts uniformly together on waves
    SAILING_ORBIT    // Node cluster translates along a circular orbit while floating and rocking
};

// --- Group Membership Component Wrappers ---
// These pair raw pointers with their local relative space offsets from the group pivot
struct SceneMember {
    SceneElement* element;
    myglm::vec3 localOffsetPos;
    myglm::vec3 localOffsetRot;
};

struct LightMember {
    LightElement* element;
    myglm::vec3 localOffsetPos;
    myglm::vec3 localOffsetRot; 
};

struct CameraMember {
    Camera* camera;
    myglm::vec3 localOffsetPos;
    float localOffsetYaw;       
    float localOffsetPitch;
};

class AnimationGroup {
public:
    AnimationType type;

    myglm::vec3 position;
    myglm::vec3 rotation;       // Unified group orientation state (Pitch, Yaw, Roll)

    // Trajectory parameters read by the stateless AnimationSystem
    float orbitRadius;
    float orbitSpeed;
    float currentAngle;

    // Separated component lists to keep render and physics updates linear and clean
    std::vector<SceneMember> sceneElements;
    std::vector<LightMember> lightElements;
    std::vector<CameraMember> cameras;

    AnimationGroup(AnimationType animType, myglm::vec3 startPivotPos = myglm::vec3(0.0f))
        : type(animType), position(startPivotPos), rotation(0.0f),
          orbitRadius(0.0f), orbitSpeed(0.0f), currentAngle(0.0f) {}

    
    void addMember(SceneElement* element, myglm::vec3 localPos = myglm::vec3(0.0f), myglm::vec3 localRot = myglm::vec3(0.0f)) {
        sceneElements.push_back({element, localPos, localRot});
    }

    void addMember(LightElement* element, myglm::vec3 localPos = myglm::vec3(0.0f), myglm::vec3 localRot = myglm::vec3(0.0f)) {
        lightElements.push_back({element, localPos, localRot});
    }

    void addMember(Camera* camera, myglm::vec3 localPos = myglm::vec3(0.0f), float localYaw = 0.0f, float localPitch = 0.0f) {
        cameras.push_back({camera, localPos, localYaw, localPitch});
    }
};

#endif
#ifndef CAMERA_H
#define CAMERA_H

#include "myglm.h"
#include <cmath>

enum class CameraMode {
    DRONE,   // Free-flight mode: complete movement and looking input flexibility
    STATIC,  // Stationary viewpoint: position locked in place, but user can look around
    ATTACHED // Group-linked mode: position and view tracking driven by the group animation loop
};

class Camera {
public:
    // Core spatial state vectors used to generate the View Matrix
    myglm::vec3 position;
    myglm::vec3 forward;
    myglm::vec3 up;
    myglm::vec3 right;

    // Relative user look angle variables (stored in radians)
    float yaw;   
    float pitch; 

    // The current behavioral control policy
    CameraMode mode;

    Camera(CameraMode initialMode = CameraMode::DRONE, myglm::vec3 startPos = myglm::vec3(0.0f, 8.0f, 30.0f))
        : position(startPos), yaw(3.14159f), pitch(-0.2f), up(0.0f, 1.0f, 0.0f), mode(initialMode) {
        updateCameraVectors();
    }

    // Computes directional look orientations from standard user inputs
    void updateCameraVectors() {
        // Only build vectors locally if the camera is fully self-directed
        if (mode == CameraMode::DRONE || mode == CameraMode::STATIC) {
            myglm::vec3 front;
            front.x = std::cos(pitch) * std::sin(yaw);
            front.y = std::sin(pitch);
            front.z = std::cos(pitch) * std::cos(yaw);
            forward = myglm::normalize(front);

            myglm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            right = myglm::normalize(myglm::cross(forward, worldUp));
            up    = myglm::normalize(myglm::cross(right, forward));
        }
    }

    // --- Absolute State Injection Interface ---
    // Called exclusively by the AnimationSystem to lock spatial state overrides
    void setWorldTransform(const myglm::vec3& worldPos, const myglm::vec3& worldForward, const myglm::vec3& worldUp) {
        position = worldPos;
        forward  = worldForward;
        up       = worldUp;
        right    = myglm::normalize(myglm::cross(forward, up));
    }

    myglm::mat4 getViewMatrix() {
        return myglm::lookAt(position, position + forward, up);
    }

    // --- Relative User Inputs (Key / Mouse Tracking) ---
    void rotateView(float dYaw, float dPitch) {
        yaw += dYaw;
        pitch += dPitch;

        // Clamp pitching limits to avoid upside-down neck-snapping flips
        if (pitch > 1.4f)  pitch = 1.4f;
        if (pitch < -1.4f) pitch = -1.4f;

        // For Drone and Static, rebuild vectors immediately.
        // For Attached, we do nothing here—the AnimationSystem will read these updated angles 
        // and combine them with the boat's rolling matrix next frame!
        if (mode != CameraMode::ATTACHED) {
            updateCameraVectors();
        }
    }

    // Relative keyboard flight translations are strictly restricted to DRONE type
    void moveForward(float amount) {
        if (mode == CameraMode::DRONE) {
            position += forward * amount;
        }
    }

    void moveStrafe(float amount) {
        if (mode == CameraMode::DRONE) {
            position += right * amount;
        }
    }
};

#endif
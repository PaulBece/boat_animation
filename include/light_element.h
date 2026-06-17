#ifndef LIGHTELEMENT_H
#define LIGHTELEMENT_H

#include "myglm.h"
#include "model.h"
#include "light.h"

class LightElement {
private:
    myglm::vec3 position;
    myglm::vec3 rotation; // Euler angles in radians (Pitch, Yaw, Roll)
    myglm::vec3 scale;
    myglm::mat4 modelMatrix;

public:
    Model* asset;         // Pointer to shared passive indicator model (e.g., ballModel)
    Light light;          // The functional mathematical light parameters packet
    bool isVisible;       // Visibility flag matching SceneElement behavior

    // Constructor maps spatial attributes to the class fields and passes non-spatial attributes to the Light component
    LightElement(Model* modelAsset, LightType type, myglm::vec3 startPos = myglm::vec3(0.0f), myglm::vec3 scaleSize = myglm::vec3(1.0f))
        : asset(modelAsset), light(type, startPos), position(startPos), scale(scaleSize), rotation(0.0f), modelMatrix(1.0f), isVisible(true) {
        updateTransform(); // Compute initial baseline states immediately
    }

    // --- Visibility Control ---
    void setVisibility(bool visible) { isVisible = visible; }
    void toggleVisibility()          { isVisible = !isVisible; }

    // --- Active Transformation Interface (Automatically keeps light & model synced) ---
    void setPosition(const myglm::vec3& newPos) {
        position = newPos;
        updateTransform();
    }

    void translate(const myglm::vec3& delta) {
        position += delta;
        updateTransform();
    }

    void setRotation(const myglm::vec3& newRot) {
        rotation = newRot;
        updateTransform();
    }

    void rotate(const myglm::vec3& angularDelta) {
        rotation += angularDelta;
        updateTransform();
    }

    void setScale(const myglm::vec3& newScale) {
        scale = newScale;
        updateTransform();
    }

    // --- Internal State Synchronization System ---
    void updateTransform() {
        // 1. Compile the clean, commutative rotation matrix sequence (Yaw -> Pitch -> Roll)
        myglm::mat4 rotMat = myglm::mat4(1.0f);
        if (rotation.y != 0.0f) rotMat = myglm::rotate(rotMat, rotation.y, myglm::vec3(0.0f, 1.0f, 0.0f));
        if (rotation.x != 0.0f) rotMat = myglm::rotate(rotMat, rotation.x, myglm::vec3(1.0f, 0.0f, 0.0f));
        if (rotation.z != 0.0f) rotMat = myglm::rotate(rotMat, rotation.z, myglm::vec3(0.0f, 0.0f, 1.0f));

        // 2. Sync functional shader calculation position
        light.position = position;

        // 3. Sync functional spotlight direction vector
        // Native directional vector pointing down along the negative Y-axis from light.h
        myglm::vec4 nativeDir = myglm::vec4(0.0f, -1.0f, 0.0f, 0.0f); 
        myglm::vec4 transformedDir = rotMat * nativeDir;
        
        // Update focus tracking with the computed direction vector
        light.setDirection(myglm::vec3(transformedDir.x, transformedDir.y, transformedDir.z));

        // 4. Build the clean pre-cached model matrix for the drawing pipeline pass
        myglm::mat4 mat = myglm::mat4(1.0f);
        mat = myglm::translate(mat, position);
        mat = mat * rotMat;
        mat = myglm::scale(mat, scale);
        
        modelMatrix = mat;
    }

    void setWorldTransform(const myglm::mat4& completeMatrix, const myglm::vec3& worldPos) {
        modelMatrix = completeMatrix;
        position = worldPos;
        light.position = worldPos; // Sync shader uniform calculation position

        // Transform the native downward vector by the complete matrix to capture wave rocking
        myglm::vec4 nativeDir = myglm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        myglm::vec4 transformedDir = completeMatrix * nativeDir;
        
        light.setDirection(myglm::normalize(myglm::vec3(transformedDir.x, transformedDir.y, transformedDir.z)));
    }

    // --- Read-Only Component Inspectors ---
    const myglm::mat4& getModelMatrix() const { return modelMatrix; }
    const myglm::vec3& getPosition() const    { return position; }
    const myglm::vec3& getRotation() const    { return rotation; }
    const myglm::vec3& getScale() const       { return scale; }
};

#endif
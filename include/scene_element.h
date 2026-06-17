#ifndef SCENEELEMENT_H
#define SCENEELEMENT_H

#include "myglm.h"
#include "model.h"



class SceneElement {
private:
    myglm::vec3 position;
    myglm::vec3 rotation; // Euler angles (Pitch, Yaw, Roll)
    myglm::vec3 scale;
    myglm::mat4 modelMatrix;

public:
    Model* asset;
    bool isVisible;       // ADOPTED: Clean visibility tracking flag

    SceneElement(Model* modelAsset, myglm::vec3 startPos = myglm::vec3(0.0f), myglm::vec3 scaleSize = myglm::vec3(1.0f))
        : asset(modelAsset), position(startPos), scale(scaleSize), rotation(0.0f), modelMatrix(1.0f), isVisible(true) {
        updateMatrix(); 
    }

    // ADOPTED: Simple visibility modifier function interface
    void setVisibility(bool visible) { isVisible = visible; }
    void toggleVisibility()          { isVisible = !isVisible; }

    // --- Active Transformation Control Interface ---
    void setPosition(const myglm::vec3& newPos) {
        position = newPos;
        updateMatrix();
    }

    void translate(const myglm::vec3& delta) {
        position += delta;
        updateMatrix();
    }

    void setRotation(const myglm::vec3& newRot) {
        rotation = newRot;
        updateMatrix();
    }

    void rotate(const myglm::vec3& angularDelta) {
        rotation += angularDelta;
        updateMatrix();
    }

    void setScale(const myglm::vec3& newScale) {
        scale = newScale;
        updateMatrix();
    }

    // Rebuilds a clean, drift-free matrix state from independent vectors
    void updateMatrix() {
        myglm::mat4 mat = myglm::mat4(1.0f);
        mat = myglm::translate(mat, position);

        if (rotation.y != 0.0f) mat = myglm::rotate(mat, rotation.y, myglm::vec3(0.0f, 1.0f, 0.0f));
        if (rotation.x != 0.0f) mat = myglm::rotate(mat, rotation.x, myglm::vec3(1.0f, 0.0f, 0.0f));
        if (rotation.z != 0.0f) mat = myglm::rotate(mat, rotation.z, myglm::vec3(0.0f, 0.0f, 1.0f));

        mat = myglm::scale(mat, scale);
        modelMatrix = mat;
    }

    void setWorldTransform(const myglm::mat4& completeMatrix, const myglm::vec3& worldPos) {
        modelMatrix = completeMatrix;
        position = worldPos;
    }

    // Read-only parameters for physics loops and shader uniforms
    const myglm::mat4& getModelMatrix() const { return modelMatrix; }
    const myglm::vec3& getPosition() const    { return position; }
    const myglm::vec3& getRotation() const    { return rotation; }
    const myglm::vec3& getScale() const       { return scale; }
};

#endif
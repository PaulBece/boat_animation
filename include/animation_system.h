#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "myglm.h"
#include "animation_group.h"
#include <vector>
#include <cmath>

class AnimationSystem {
public:
    static void update(std::vector<AnimationGroup*>& groups, float currentTime, float deltaTime, float baseSeaLevel) {
        for (auto* groupPtr : groups) {
            if (!groupPtr) continue;
            AnimationGroup& group = *groupPtr;

            // --- Step 1: Update Group Pivot Coordinates & Orientations ---
            if (group.type == AnimationType::STATIC) {
                group.rotation = myglm::vec3(0.0f);
            }
            else if (group.type == AnimationType::ANCHORED_WATER) {
                float wave1 = 0.5f * std::sin(group.position.x * 0.15f + currentTime * 1.6f);
                float wave2 = 0.3f * std::cos(group.position.z * 0.25f + currentTime * 2.2f);
                group.position.y = baseSeaLevel + wave1 + wave2;

                float dy_dx = 0.5f * 0.15f * std::cos(group.position.x * 0.15f + currentTime * 1.6f);
                float dy_dz = -0.3f * 0.25f * std::sin(group.position.z * 0.25f + currentTime * 2.2f);
                
                group.rotation.x = std::atan(dy_dx); 
                group.rotation.y = 0.0f;             
                group.rotation.z = std::atan(dy_dz); 
            }
            else if (group.type == AnimationType::SAILING_ORBIT) {
                group.currentAngle += group.orbitSpeed * deltaTime;

                group.position.x = group.orbitRadius * std::cos(group.currentAngle);
                group.position.z = group.orbitRadius * std::sin(group.currentAngle);

                float wave1 = 0.5f * std::sin(group.position.x * 0.15f + currentTime * 1.6f);
                float wave2 = 0.3f * std::cos(group.position.z * 0.25f + currentTime * 2.2f);
                group.position.y = baseSeaLevel + wave1 + wave2;

                float dy_dx = 0.5f * 0.15f * std::cos(group.position.x * 0.15f + currentTime * 1.6f);
                float dy_dz = -0.3f * 0.25f * std::sin(group.position.z * 0.25f + currentTime * 2.2f);

                group.rotation.x = std::atan(dy_dx);
                group.rotation.y = -group.currentAngle + 1.5708f;
                group.rotation.z = std::atan(dy_dz);
            }

            // --- Step 2: Build the Master Unified Group Coordinate Frame Matrix ---
            myglm::mat4 groupMatrix = myglm::mat4(1.0f);
            groupMatrix = myglm::translate(groupMatrix, group.position);
            if (group.rotation.y != 0.0f) groupMatrix = myglm::rotate(groupMatrix, group.rotation.y, myglm::vec3(0.0f, 1.0f, 0.0f));
            if (group.rotation.x != 0.0f) groupMatrix = myglm::rotate(groupMatrix, group.rotation.x, myglm::vec3(1.0f, 0.0f, 0.0f));
            if (group.rotation.z != 0.0f) groupMatrix = myglm::rotate(groupMatrix, group.rotation.z, myglm::vec3(0.0f, 0.0f, 1.0f));

            // --- Step 3: Transform Scene Elements via Matrix Composition ---
            for (auto& member : group.sceneElements) {
                myglm::mat4 localMatrix = myglm::mat4(1.0f);
                localMatrix = myglm::translate(localMatrix, member.localOffsetPos);
                if (member.localOffsetRot.y != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.y, myglm::vec3(0.0f, 1.0f, 0.0f));
                if (member.localOffsetRot.x != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.x, myglm::vec3(1.0f, 0.0f, 0.0f));
                if (member.localOffsetRot.z != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.z, myglm::vec3(0.0f, 0.0f, 1.0f));
                localMatrix = myglm::scale(localMatrix, member.element->getScale());

                myglm::mat4 finalMatrix = groupMatrix * localMatrix;
                
                // Extract translation values directly out of the matrix .m array fourth column
                myglm::vec3 worldPos = myglm::vec3(finalMatrix.m[3][0], finalMatrix.m[3][1], finalMatrix.m[3][2]);

                member.element->setWorldTransform(finalMatrix, worldPos);
            }

            // --- Step 4: Transform Light Elements via Matrix Composition ---
            for (auto& member : group.lightElements) {
                myglm::mat4 localMatrix = myglm::mat4(1.0f);
                localMatrix = myglm::translate(localMatrix, member.localOffsetPos);
                if (member.localOffsetRot.y != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.y, myglm::vec3(0.0f, 1.0f, 0.0f));
                if (member.localOffsetRot.x != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.x, myglm::vec3(1.0f, 0.0f, 0.0f));
                if (member.localOffsetRot.z != 0.0f) localMatrix = myglm::rotate(localMatrix, member.localOffsetRot.z, myglm::vec3(0.0f, 0.0f, 1.0f));
                localMatrix = myglm::scale(localMatrix, member.element->getScale());

                myglm::mat4 finalMatrix = groupMatrix * localMatrix;
                myglm::vec3 worldPos = myglm::vec3(finalMatrix.m[3][0], finalMatrix.m[3][1], finalMatrix.m[3][2]);

                member.element->setWorldTransform(finalMatrix, worldPos);
            }

            // --- Step 5: Transform Group-Attached Viewport Cameras ---
            for (auto& member : group.cameras) {
                myglm::vec4 worldPos4 = groupMatrix * myglm::vec4(member.localOffsetPos, 1.0f);
                myglm::vec3 worldPos = myglm::vec3(worldPos4.x, worldPos4.y, worldPos4.z);

                // Isolate pure rotation vectors by stripping positions via the .m data grid arrays
                myglm::mat4 groupRotMat = groupMatrix;
                groupRotMat.m[3][0] = 0.0f;
                groupRotMat.m[3][1] = 0.0f;
                groupRotMat.m[3][2] = 0.0f;
                groupRotMat.m[3][3] = 1.0f;

                float totalYaw   = member.localOffsetYaw   + member.camera->yaw;
                float totalPitch = member.localOffsetPitch + member.camera->pitch;

                myglm::vec3 localFront;
                localFront.x = std::cos(totalPitch) * std::sin(totalYaw);
                localFront.y = std::sin(totalPitch);
                localFront.z = std::cos(totalPitch) * std::cos(totalYaw);

                myglm::vec4 worldFront = groupRotMat * myglm::vec4(localFront, 0.0f);
                myglm::vec4 worldUp    = groupRotMat * myglm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

                member.camera->setWorldTransform(
                    worldPos,
                    myglm::normalize(myglm::vec3(worldFront.x, worldFront.y, worldFront.z)),
                    myglm::normalize(myglm::vec3(worldUp.x, worldUp.y, worldUp.z))
                );
            }
        }
    }
};

#endif
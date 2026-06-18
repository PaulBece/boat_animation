#ifndef STORM_H
#define STORM_H

#include "myglm.h"
#include "light.h"
#include "scene_element.h"

#include <cstdlib>

class Storm {
public:
    SceneElement* boltElement;
    Light* globalLight;

    bool isFlashing;
    float nextStrikeTime;
    float flashStartTime;
    float flashDuration;
    float minInterval;
    float maxInterval;

    myglm::vec3 baseColor;
    float baseAmbientStrength;
    float baseDiffuseStrength;
    float baseSpecularStrength;

    myglm::vec3 flashColor;
    float flashAmbientStrength;
    float flashDiffuseStrength;
    float flashSpecularStrength;

    Storm(SceneElement* lightningModel = nullptr, Light* sceneGlobalLight = nullptr)
        : boltElement(lightningModel), globalLight(sceneGlobalLight), isFlashing(false),
          nextStrikeTime(0.0f), flashStartTime(0.0f), flashDuration(0.16f),
          minInterval(3.5f), maxInterval(8.0f),
          baseColor(0.12f, 0.18f, 0.32f), baseAmbientStrength(0.05f),
          baseDiffuseStrength(0.25f), baseSpecularStrength(0.40f),
          flashColor(0.70f, 0.82f, 1.0f), flashAmbientStrength(0.42f),
          flashDiffuseStrength(1.35f), flashSpecularStrength(1.60f) {
        if (globalLight != nullptr) {
            captureBaseLight();
        }

        if (boltElement != nullptr) {
            boltElement->setVisibility(false);
        }
    }

    void update(float currentTime) {
        if (globalLight == nullptr) return;

        if (!isFlashing && currentTime >= nextStrikeTime) {
            beginFlash(currentTime);
        }

        if (isFlashing) {
            float elapsed = currentTime - flashStartTime;
            float t = elapsed / flashDuration;

            if (t >= 1.0f) {
                endFlash(currentTime);
            } else {
                applyFlash(1.0f - t);
            }
        }
    }

    void forceStrike(float currentTime) {
        if (globalLight == nullptr) return;
        beginFlash(currentTime);
    }

    void captureBaseLight() {
        if (globalLight == nullptr) return;

        baseColor = globalLight->color;
        baseAmbientStrength = globalLight->ambientStrength;
        baseDiffuseStrength = globalLight->diffuseStrength;
        baseSpecularStrength = globalLight->specularStrength;
    }

private:
    float randomRange(float minValue, float maxValue) {
        float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return minValue + (maxValue - minValue) * t;
    }

    void beginFlash(float currentTime) {
        isFlashing = true;
        flashStartTime = currentTime;

        if (boltElement != nullptr) {
            boltElement->setVisibility(true);
        }

        applyFlash(1.0f);
    }

    void endFlash(float currentTime) {
        isFlashing = false;
        restoreBaseLight();

        if (boltElement != nullptr) {
            boltElement->setVisibility(false);
        }

        nextStrikeTime = currentTime + randomRange(minInterval, maxInterval);
    }

    void applyFlash(float intensity) {
        float clamped = intensity;
        if (clamped < 0.0f) clamped = 0.0f;
        if (clamped > 1.0f) clamped = 1.0f;

        globalLight->color = mixVec3(baseColor, flashColor, clamped);
        globalLight->ambientStrength = mixFloat(baseAmbientStrength, flashAmbientStrength, clamped);
        globalLight->diffuseStrength = mixFloat(baseDiffuseStrength, flashDiffuseStrength, clamped);
        globalLight->specularStrength = mixFloat(baseSpecularStrength, flashSpecularStrength, clamped);
    }

    void restoreBaseLight() {
        globalLight->color = baseColor;
        globalLight->ambientStrength = baseAmbientStrength;
        globalLight->diffuseStrength = baseDiffuseStrength;
        globalLight->specularStrength = baseSpecularStrength;
    }

    float mixFloat(float a, float b, float t) {
        return a + (b - a) * t;
    }

    myglm::vec3 mixVec3(const myglm::vec3& a, const myglm::vec3& b, float t) {
        return myglm::vec3(
            mixFloat(a.x, b.x, t),
            mixFloat(a.y, b.y, t),
            mixFloat(a.z, b.z, t)
        );
    }
};

#endif

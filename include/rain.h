#ifndef RAIN_H
#define RAIN_H

#include <glad/gl.h>
#include "myglm.h"

#include <cmath>
#include <cstdlib>
#include <vector>

class Rain
{
public:
    unsigned int VAO, VBO;
    unsigned int vertexCount;

    int dropCount;
    float areaSize;
    float topY;
    float bottomY;
    float fallSpeed;
    float dropLength;
    myglm::vec3 wind;

    Rain(int count = 1800, float spawnAreaSize = 90.0f, float spawnTopY = 35.0f, float spawnBottomY = -8.0f)
        : VAO(0), VBO(0), vertexCount(0), dropCount(count), areaSize(spawnAreaSize),
          topY(spawnTopY), bottomY(spawnBottomY), fallSpeed(42.0f), dropLength(1.8f),
          wind(0.35f, 0.0f, -0.12f)
    {
        generateDrops(myglm::vec3(0.0f));
        setupBuffers();
    }

    void update(float deltaTime, const myglm::vec3 &cameraPosition)
    {
        float halfArea = areaSize * 0.5f;

        for (int i = 0; i < dropCount; ++i)
        {
            int base = i * 6;

            drops[base + 1] -= fallSpeed * deltaTime;
            drops[base + 4] -= fallSpeed * deltaTime;

            float x = drops[base + 0];
            float y = drops[base + 1];
            float z = drops[base + 2];

            bool outsideX = std::fabs(x - cameraPosition.x) > halfArea;
            bool outsideZ = std::fabs(z - cameraPosition.z) > halfArea;

            if (y < bottomY)
            {
                resetDrop(i, cameraPosition, true);
            }
            else if (outsideX || outsideZ)
            {
                resetDrop(i, cameraPosition, false);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, drops.size() * sizeof(float), drops.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw(unsigned int shaderProgram, const myglm::mat4 &view, const myglm::mat4 &projection)
    {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, myglm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, myglm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, vertexCount);
        glBindVertexArray(0);
    }

    ~Rain()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    std::vector<float> drops;

    float randomRange(float minValue, float maxValue)
    {
        float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return minValue + (maxValue - minValue) * t;
    }

    void generateDrops(const myglm::vec3 &center)
    {
        drops.resize(static_cast<size_t>(dropCount) * 6);

        for (int i = 0; i < dropCount; ++i)
        {
            resetDrop(i, center, false);
        }

        vertexCount = static_cast<unsigned int>(dropCount * 2);
    }

    void resetDrop(int index, const myglm::vec3 &center, bool forceTop)
    {
        float halfArea = areaSize * 0.5f;
        float x = center.x + randomRange(-halfArea, halfArea);
        float y = forceTop ? randomRange(topY - 4.0f, topY) : randomRange(bottomY, topY);
        float z = center.z + randomRange(-halfArea, halfArea);

        int base = index * 6;
        drops[base + 0] = x;
        drops[base + 1] = y;
        drops[base + 2] = z;

        drops[base + 3] = x + wind.x;
        drops[base + 4] = y - dropLength;
        drops[base + 5] = z + wind.z;
    }

    void setupBuffers()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, drops.size() * sizeof(float), drops.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }
};

#endif

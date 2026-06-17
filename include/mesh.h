#ifndef MESH_H
#define MESH_H

#include <glad/gl.h>
#include "myglm.h"
#include <vector>
#include <string>

// The unified, interleaved layout for modern GPU cache efficiency
struct Vertex {
    myglm::vec3 Position;
    myglm::vec3 Normal;
    myglm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    unsigned int textureID; // The GPU hardware texture handle for this sub-mesh

    Mesh(const std::vector<Vertex>& vertData, unsigned int texID) {
        this->vertices = vertData;
        this->textureID = texID;
        setupMesh();
    }

    void draw(unsigned int shaderProgram) {
        // 1. Bind this specific sub-mesh's texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTexture"), 0);

        // 2. Draw the geometry
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);
    }

    ~Mesh() {
        // Clean up memory allocations inside the GPU
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    unsigned int VAO, VBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Upload the entire interleaved vertex vector in one stream
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Attribute 1: Positions (Location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

        // Attribute 2: Normals (Location = 1) -> Prepped and ready for Phase 4 lighting
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // Attribute 3: Texture Coordinates / UVs (Location = 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

#endif
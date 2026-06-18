#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/gl.h>
#include "myglm.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>

class Skybox {
public:
    unsigned int VAO, VBO, EBO;
    unsigned int cubemapTexture;
    unsigned int indexCount;

    Skybox(const std::vector<std::string>& faces)
        : VAO(0), VBO(0), EBO(0), cubemapTexture(0), indexCount(36) {
        generateMesh();
        cubemapTexture = loadCubemap(faces);
    }

    void draw(unsigned int shaderProgram, const myglm::mat4& view, const myglm::mat4& projection) {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(shaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, myglm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, myglm::value_ptr(projection));
        glUniform1i(glGetUniformLocation(shaderProgram, "skybox"), 0);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);
    }

    ~Skybox() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &cubemapTexture);
    }

private:
    void generateMesh() {
        const float vertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,

            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f
        };

        const unsigned int indices[] = {
            0, 1, 2,  2, 3, 0,
            7, 6, 5,  5, 4, 7,
            4, 5, 1,  1, 0, 4,
            3, 2, 6,  6, 7, 3,
            4, 0, 3,  3, 7, 4,
            1, 5, 6,  6, 2, 1
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    unsigned int loadCubemap(const std::vector<std::string>& faces) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        stbi_set_flip_vertically_on_load(false);

        for (unsigned int i = 0; i < faces.size(); ++i) {
            int width, height, channels;
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

            if (data) {
                GLenum format = GL_RGB;
                if (channels == 1) format = GL_RED;
                if (channels == 3) format = GL_RGB;
                if (channels == 4) format = GL_RGBA;

                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0,
                    format,
                    width,
                    height,
                    0,
                    format,
                    GL_UNSIGNED_BYTE,
                    data
                );
                stbi_image_free(data);
            } else {
                std::cerr << "ERROR::SKYBOX::FAILED_TO_LOAD_TEXTURE: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }
};

#endif

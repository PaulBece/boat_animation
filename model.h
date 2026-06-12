#ifndef MODEL_H
#define MODEL_H

#include <glad/gl.h>
#include "myglm.h"
#include "mesh.h"

// Clean inclusions WITHOUT any implementation macro flags
#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>

// High-performance image loading using stb_image.h
unsigned int loadTextureFromFile(const std::string& path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // Tell stb_image to flip textures along the Y-axis because OpenGL expects pixel (0,0) at the bottom-left
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        // Upload the decoded raw pixel bytes directly into VRAM
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Configure standard texture wrapping/filtering controls
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "   -> Successfully loaded texture map file: " << path << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cerr << "   -> [TEXTURE ERROR]: Failed to load image at path: " << path << std::endl;
        stbi_image_free(data);
        
        // Fallback texture generation so the engine doesn't crash if an asset is missing
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char fallbackPixel[] = { 200, 200, 200, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixel);
    }

    return textureID;
}

class Model {
public:
    std::vector<Mesh*> meshes;
    std::string directory;

    Model(const std::string& folderName, const std::string& objFilename) {
#ifdef MODELS_PATH
        this->directory = std::string(MODELS_PATH) + folderName + "/";
#else
        this->directory = "models/" + folderName + "/";
#endif
        loadModel(directory + objFilename);
    }

    void draw(unsigned int shaderProgram) {
        for (size_t i = 0; i < meshes.size(); ++i) {
            meshes[i]->draw(shaderProgram);
        }
    }

    ~Model() {
        for (Mesh* mesh : meshes) delete mesh;
    }

private:
    void loadModel(const std::string& path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), directory.c_str());

        if (!warn.empty()) std::cout << "[TINYOBJ WARN]: " << warn << std::endl;
        if (!ret) {
            std::cerr << "[TINYOBJ FATAL ERROR]: " << err << " Path: " << path << std::endl;
            return;
        }

        std::cout << "[ENGINE] Loaded geometry file: " << path << std::endl;
        std::cout << " -> Sub-meshes count: " << shapes.size() << " | Materials count: " << materials.size() << std::endl;

        // Process materials and load real textures
        std::vector<unsigned int> materialTextureIDs;
        
        if (materials.empty()) {
            materialTextureIDs.push_back(loadTextureFromFile("")); // Triggers generic fallback pixel
        } else {
            for (size_t m = 0; m < materials.size(); m++) {
                std::string textureName = materials[m].diffuse_texname;
                if (!textureName.empty()) {
                    materialTextureIDs.push_back(loadTextureFromFile(directory + textureName));
                } else {
                    // Fallback to solid color texture map from diffuse settings if no image file is defined
                    unsigned int texID;
                    glGenTextures(1, &texID);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    unsigned char r = static_cast<unsigned char>(materials[m].diffuse[0] * 255.0f);
                    unsigned char g = static_cast<unsigned char>(materials[m].diffuse[1] * 255.0f);
                    unsigned char b = static_cast<unsigned char>(materials[m].diffuse[2] * 255.0f);
                    unsigned char pixel[] = { r, g, b, 255 };
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                    materialTextureIDs.push_back(texID);
                }
            }
        }

        // Parse geometry layouts and build sub-meshes
        for (size_t s = 0; s < shapes.size(); s++) {
            std::vector<Vertex> meshVertices;
            size_t index_offset = 0;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    Vertex vertex;

                    vertex.Position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    vertex.Position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    vertex.Position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                    if (idx.normal_index >= 0) {
                        vertex.Normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                        vertex.Normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                        vertex.Normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    } else {
                        vertex.Normal = myglm::vec3(0.0f, 1.0f, 0.0f);
                    }

                    if (idx.texcoord_index >= 0) {
                        vertex.TexCoords.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        vertex.TexCoords.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    } else {
                        vertex.TexCoords = myglm::vec2(0.0f, 0.0f);
                    }

                    meshVertices.push_back(vertex);
                }
                index_offset += fv;
            }

            unsigned int assignedTexID = materialTextureIDs[0];
            if (!shapes[s].mesh.material_ids.empty() && shapes[s].mesh.material_ids[0] >= 0) {
                assignedTexID = materialTextureIDs[shapes[s].mesh.material_ids[0]];
            }

            meshes.push_back(new Mesh(meshVertices, assignedTexID));
        }
    }
};

#endif
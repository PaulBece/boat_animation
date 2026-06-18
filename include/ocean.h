class Ocean {
public:
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;

    int resolution; // Dynamically calculated 
    float size;     // Desired physical size of the plane
    float height;   // Base sea level height
    float cellSize; // The locked spacing size of a single grid cell (density ratio)

    // Constructor accepts desired cell spacing width instead of segment counts
    Ocean(float totalSize = 400.0f, float targetCellSize = 0.5f, float initialHeight = 0.0f) 
        : size(totalSize), cellSize(targetCellSize), height(initialHeight), VAO(0), VBO(0), EBO(0) {
        
        // 1. Automatically calculate the resolution needed to fill the size at this density
        resolution = static_cast<int>(std::round(size / cellSize));
        
        // Safety guard: ensure resolution is at least 1 segment
        if (resolution < 1) resolution = 1;

        generateMesh();
    }

    void generateMesh() {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        // Recalculate exact bounds based on our finalized resolution grid
        // This ensures the mesh dimensions perfectly match our cell alignments
        float actualSize = static_cast<float>(resolution) * cellSize;
        float halfSize = actualSize / 2.0f;

        for (int z = 0; z <= resolution; ++z) {
            for (int x = 0; x <= resolution; ++x) {
                // Calculate vertex positions using our constant cellSize spacing stride
                float posX = -halfSize + static_cast<float>(x) * cellSize;
                float posY = 0.0f; 
                float posZ = -halfSize + static_cast<float>(z) * cellSize;

                // Location 0: Positions
                vertices.push_back(posX);
                vertices.push_back(posY);
                vertices.push_back(posZ);

                // Location 1: Normals
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);

                // Location 2: UVs 
                float u = static_cast<float>(x) / static_cast<float>(resolution);
                float v = static_cast<float>(z) / static_cast<float>(resolution);
                vertices.push_back(u);
                vertices.push_back(v);
            }
        }

        // Stitching faces remains exactly the same as our quad layout loop
        for (int z = 0; z < resolution; ++z) {
            for (int x = 0; x < resolution; ++x) {
                unsigned int topLeft     = z * (resolution + 1) + x;
                unsigned int topRight    = topLeft + 1;
                unsigned int bottomLeft  = (z + 1) * (resolution + 1) + x;
                unsigned int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        indexCount = static_cast<unsigned int>(indices.size());

        // Standard OpenGL Buffer Streaming
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        GLsizei stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    void draw(unsigned int shaderProgram) {
        myglm::mat4 modelMat = myglm::translate(myglm::mat4(1.0f), myglm::vec3(0.0f, height, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, myglm::value_ptr(modelMat));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~Ocean() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};
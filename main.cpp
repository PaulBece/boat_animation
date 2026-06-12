#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "myglm.h"
#include "model.h"
#include "light.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define WIDTH 1000
#define HEIGHT 800

class Scene;
Scene *g_SceneInstance = nullptr;

// Expanded global input tracking array
bool g_LeftPressed  = false;
bool g_RightPressed = false;
bool g_UpPressed    = false;
bool g_DownPressed  = false;
bool g_WPressed     = false;
bool g_SPressed     = false;
bool g_APressed     = false;
bool g_DPressed     = false;

unsigned int loadShaders(const char* vertexFile, const char* fragmentFile) {
#ifdef SHADER_PATH
    std::string vertexPath   = std::string(SHADER_PATH) + vertexFile;
    std::string fragmentPath = std::string(SHADER_PATH) + fragmentFile;
#else
    std::string vertexPath   = vertexFile;
    std::string fragmentPath = fragmentFile;
#endif

    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\nPaths:\n -> " 
                  << vertexPath << "\n -> " << fragmentPath << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    int success;
    char infoLog[512];

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertex, 512, NULL, infoLog); std::cerr << "VERTEX_COMP_FAIL:\n" << infoLog << std::endl; }

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragment, 512, NULL, infoLog); std::cerr << "FRAGMENT_COMP_FAIL:\n" << infoLog << std::endl; }

    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(programID, 512, NULL, infoLog); std::cerr << "LINK_FAIL:\n" << infoLog << std::endl; }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return programID;
}

class Camera {
public:
    myglm::vec3 position;
    myglm::vec3 forward;
    myglm::vec3 up;
    myglm::vec3 right;

    // View Angle States (Initialized to look slightly down the center channel)
    float yaw;   
    float pitch; 

    Camera(myglm::vec3 startPos = myglm::vec3(0.0f, 8.0f, 30.0f))
        : position(startPos), yaw(3.14f), pitch(-0.2f), up(0.0f, 1.0f, 0.0f) {
        updateCameraVectors();
    }

    void updateCameraVectors() {
        // Calculate the modern Direction Vector explicitly through Trigonometry 
        myglm::vec3 front;
        front.x = std::cos(pitch) * std::sin(yaw);
        front.y = std::sin(pitch);
        front.z = std::cos(pitch) * std::cos(yaw);
        forward = myglm::normalize(front);

        // Re-calculate the local Coordinate Space coordinate frames
        myglm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        right = myglm::normalize(myglm::cross(forward, worldUp));
        up    = myglm::normalize(myglm::cross(right, forward));
    }

    myglm::mat4 getViewMatrix() {
        // LookTarget is exactly one vector length step directly ahead of our position eye
        return myglm::lookAt(position, position + forward, up);
    }

    // Rotates camera direction in place (Look around)
    void rotateView(float dYaw, float dPitch) {
        yaw += dYaw;
        pitch += dPitch;

        // Clamp pitching to prevent vertical neck-snapping flips
        if (pitch > 1.4f)  pitch = 1.4f;
        if (pitch < -1.4f) pitch = -1.4f;

        updateCameraVectors();
    }

    // Moves camera position in 3D space relative to its view direction
    void moveForward(float amount) {
        position += forward * amount;
    }

    void moveStrafe(float amount) {
        position += right * amount;
    }
};

class Scene {
public:
    std::vector<Model*> models;
    std::vector<myglm::mat4> modelMatrices;
    Camera camera;
    myglm::mat4 projection;

    std::vector<Light*> sceneLights;

    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    unsigned int lastShaderProgram = 0;
    bool isSceneDirty = true;

    unsigned int modelLoc = 0;
    unsigned int viewLoc = 0;
    unsigned int projLoc = 0;

    Scene(float fovy = myglm::radians(45.0f), float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), float zNear = 0.1f, float zFar = 200.0f) {
        projection = myglm::perspective(fovy, aspect, zNear, zFar);
        lastTime = static_cast<float>(glfwGetTime());
    }

    void addModel(Model* model, const myglm::mat4& transform = myglm::mat4(1.0f)) {
        models.push_back(model);
        modelMatrices.push_back(transform);
    }

    void addLight(Light* light) {
        sceneLights.push_back(light);
    }

    void updateTime() {
        float currentTime = static_cast<float>(glfwGetTime());
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;
    }

    void processInputs() {
        float rotationSpeed = 2.0f * deltaTime;
        float flightSpeed   = 18.0f * deltaTime; // Velocity vector stride bounds

        // 1. Handle CAMERA ROTATION (Looking around via Arrow Keys)
        if (g_LeftPressed)  { camera.rotateView(rotationSpeed, 0.0f);   isSceneDirty = true; }
        if (g_RightPressed) { camera.rotateView(-rotationSpeed, 0.0f);  isSceneDirty = true; }
        if (g_UpPressed)    { camera.rotateView(0.0f, rotationSpeed);   isSceneDirty = true; }
        if (g_DownPressed)  { camera.rotateView(0.0f, -rotationSpeed);  isSceneDirty = true; }

        // 2. Handle CAMERA MOVEMENT (Flying through the space via W/A/S/D)
        if (g_WPressed) { camera.moveForward(flightSpeed);  isSceneDirty = true; }
        if (g_SPressed) { camera.moveForward(-flightSpeed); isSceneDirty = true; }
        if (g_APressed) { camera.moveStrafe(-flightSpeed);  isSceneDirty = true; }
        if (g_DPressed) { camera.moveStrafe(flightSpeed);   isSceneDirty = true; }
    }

    void run(unsigned int shaderProgram) {
        updateTime();
        processInputs();
        updateGlobalUniforms(shaderProgram);

        for (size_t i = 0; i < models.size(); ++i) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, myglm::value_ptr(modelMatrices[i]));
            models[i]->draw(shaderProgram);
        }
    }

    void updateGlobalUniforms(unsigned int shaderProgram) {
        if (lastShaderProgram != shaderProgram) {
            glUseProgram(shaderProgram);
            lastShaderProgram = shaderProgram;

            modelLoc = glGetUniformLocation(shaderProgram, "model");
            viewLoc  = glGetUniformLocation(shaderProgram, "view");
            projLoc  = glGetUniformLocation(shaderProgram, "projection");

            isSceneDirty = true;
        }

        myglm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, myglm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, myglm::value_ptr(projection));

        // Counter limits to organize incoming signals safely
        int pointCount = 0;
        int spotCount = 0;
        bool hasDir = false;

        for (size_t i = 0; i < sceneLights.size(); ++i) {
            Light* l = sceneLights[i];

            if (l->type == LightType::DIRECTIONAL) {
                glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.direction"), 1, myglm::value_ptr(l->direction));
                glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.color"), 1, myglm::value_ptr(l->color));
                glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.ambientStrength"), l->ambientStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.diffuseStrength"), l->diffuseStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.specularStrength"), l->specularStrength);
                hasDir = true;
            }
            else if (l->type == LightType::POINT && pointCount < 4) {
                std::string base = "pointLights[" + std::to_string(pointCount) + "].";
                glUniform3fv(glGetUniformLocation(shaderProgram, (base + "position").c_str()), 1, myglm::value_ptr(l->position));
                glUniform3fv(glGetUniformLocation(shaderProgram, (base + "color").c_str()), 1, myglm::value_ptr(l->color));
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "ambientStrength").c_str()), l->ambientStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "diffuseStrength").c_str()), l->diffuseStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "specularStrength").c_str()), l->specularStrength);
                pointCount++;
            }
            else if (l->type == LightType::SPOTLIGHT && spotCount < 4) {
                std::string base = "spotLights[" + std::to_string(spotCount) + "].";
                glUniform3fv(glGetUniformLocation(shaderProgram, (base + "position").c_str()), 1, myglm::value_ptr(l->position));
                glUniform3fv(glGetUniformLocation(shaderProgram, (base + "direction").c_str()), 1, myglm::value_ptr(l->direction));
                glUniform3fv(glGetUniformLocation(shaderProgram, (base + "color").c_str()), 1, myglm::value_ptr(l->color));
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "cutOff").c_str()), l->cutOff);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "outerCutOff").c_str()), l->outerCutOff);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "ambientStrength").c_str()), l->ambientStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "diffuseStrength").c_str()), l->diffuseStrength);
                glUniform1f(glGetUniformLocation(shaderProgram, (base + "specularStrength").c_str()), l->specularStrength);
                spotCount++;
            }
        }

        // Pass the control counts up to the GPU conditional loops
        glUniform1i(glGetUniformLocation(shaderProgram, "hasDirLight"), hasDir ? 1 : 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "activePointLightCount"), pointCount);
        glUniform1i(glGetUniformLocation(shaderProgram, "activeSpotLightCount"), spotCount);

        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, myglm::value_ptr(camera.position));
    }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (g_SceneInstance && height > 0) {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        g_SceneInstance->projection = myglm::perspective(myglm::radians(45.0f), aspect, 0.1f, 200.0f);
        g_SceneInstance->isSceneDirty = true;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool isPressed = (action == GLFW_PRESS);
        switch (key) {
            // View Rotation Controls
            case GLFW_KEY_LEFT:  g_LeftPressed  = isPressed; break;
            case GLFW_KEY_RIGHT: g_RightPressed = isPressed; break;
            case GLFW_KEY_UP:    g_UpPressed    = isPressed; break;
            case GLFW_KEY_DOWN:  g_DownPressed  = isPressed; break;

            // Free Flight Movement Controls
            case GLFW_KEY_W:     g_WPressed     = isPressed; break;
            case GLFW_KEY_S:     g_SPressed     = isPressed; break;
            case GLFW_KEY_A:     g_APressed     = isPressed; break;
            case GLFW_KEY_D:     g_DPressed     = isPressed; break;

            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
            default: break;
        }
    }
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Boats", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) return -1;

    glfwSwapInterval(1); 
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.25f, 0.3f, 1.0f);

    Scene scene(myglm::radians(45.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT));
    g_SceneInstance = &scene;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    unsigned int shaderProgram = loadShaders("shader.vert", "shader.frag");
    if (shaderProgram == 0) { glfwTerminate(); return -1; }

    std::cout << "\n[ENGINE] Starting model initialization block...\n";
    Model vikingModel("viking", "viking.obj");
    Model galleonModel("galleon", "galleon.obj");
    Model boatModel("boat", "boat.obj");
    Model lighthouseModel("lighthouse", "lighthouse.obj");
    std::cout << "[ENGINE] Initialization block complete!\n\n";

    // Space positions laid out cleanly across the sea
    myglm::mat4 m1 = myglm::scale(myglm::translate(myglm::mat4(1.0f), myglm::vec3(-5.0f, 0.0f, 0.0f)), myglm::vec3(1.0f, 1.0f, 1.0f));
    scene.addModel(&vikingModel, m1);

    myglm::mat4 m2 = myglm::scale(myglm::translate(myglm::mat4(1.0f), myglm::vec3(5.0f, 0.0f, 0.0f)), myglm::vec3(1.0f, 1.0f, 1.0f));
    scene.addModel(&galleonModel, m2);

    myglm::mat4 m3 = myglm::scale(myglm::translate(myglm::mat4(1.0f), myglm::vec3(0.0f, 0.0f, 0.0f)), myglm::vec3(0.001f, 0.001f, 0.001f));
    scene.addModel(&boatModel, m3);

    myglm::mat4 m4 = myglm::scale(myglm::translate(myglm::mat4(1.0f), myglm::vec3(0.0f, -10.0f, 0.0f)), myglm::vec3(1.0f, 1.0f, 1.0f));
    scene.addModel(&lighthouseModel, m4);

    // 1. DIRECTIONAL LIGHT: The Golden Sun (No position, only a parallel direction vector)
    Light sunLight(LightType::DIRECTIONAL);
    sunLight.setDirection(myglm::vec3(0.5f, -1.0f, -0.3f)); // Angled downward sunlight
    sunLight.color = myglm::vec3(1.0f, 0.90f, 0.75f);      // Warm evening hue
    sunLight.ambientStrength = 0.10f;
    scene.addLight(&sunLight);

    // 2. POINT LIGHT: The Green Deck Lantern
    Light lantern(LightType::POINT, myglm::vec3(-5.0f, 3.0f, 0.0f));
    lantern.color = myglm::vec3(0.0f, 0.6f, 0.2f); // Neon green glow
    lantern.diffuseStrength = 1.0f;
    scene.addLight(&lantern);

    // 3. SPOTLIGHT: The Overhead Searchlight (Has position AND a focused direction cone)
    Light searchLight(LightType::SPOTLIGHT, myglm::vec3(0.0f, 18.0f, 0.0f)); // Hovering high directly over center channel
    searchLight.setDirection(myglm::vec3(0.0f, -1.0f, 0.0f));                // Pointing directly down
    searchLight.color = myglm::vec3(1.0f, 0.2f, 0.2f);                       // Sharp Crimson warning ray
    
    // Customize cone beam focus limits using cosines
    searchLight.cutOff      = std::cos(myglm::radians(10.0f)); // Sharp inner beam radius
    searchLight.outerCutOff = std::cos(myglm::radians(15.0f)); // Smooth edge falloff boundary
    searchLight.diffuseStrength = 1.5f;
    scene.addLight(&searchLight);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene.run(shaderProgram);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
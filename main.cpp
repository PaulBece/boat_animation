#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "include/myglm.h"
#include "include/model.h"
#undef STB_IMAGE_IMPLEMENTATION
#include "include/light.h"
#include "include/light_element.h"
#include "include/ocean.h"
#include "include/skybox.h"
#include "include/camera.h"
#include "include/scene_element.h"
#include "include/animation_group.h"
#include "include/animation_system.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define WIDTH 1000
#define HEIGHT 800

class Scene;
Scene *g_SceneInstance = nullptr;

// Global input tracking arrays
bool g_LeftPressed = false; bool g_RightPressed = false; bool g_UpPressed = false; bool g_DownPressed = false;
bool g_WPressed = false;    bool g_SPressed = false;    bool g_APressed = false; bool g_DPressed = false;

// Robust shader loading assembly
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
        vShaderFile.open(vertexPath); fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf(); fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close(); fShaderFile.close();
        vertexCode   = vShaderStream.str(); fragmentCode = fShaderStream.str();
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n"; return 0;
    }

    const char* vShaderCode = vertexCode.c_str(); const char* fShaderCode = fragmentCode.c_str();
    int success; char infoLog[512];

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL); glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertex, 512, NULL, infoLog); std::cerr << "VERTEX_FAIL:\n" << infoLog << std::endl; }

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL); glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragment, 512, NULL, infoLog); std::cerr << "FRAGMENT_FAIL:\n" << infoLog << std::endl; }

    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vertex); glAttachShader(programID, fragment); glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(programID, 512, NULL, infoLog); std::cerr << "LINK_FAIL:\n" << infoLog << std::endl; }

    glDeleteShader(vertex); glDeleteShader(fragment);
    return programID;
}

// ====================================================================
// CENTRAL SCENE MANAGER CORE
// ====================================================================
class Scene {
public:
    // Pointers used to prevent memory reallocation and vector invalidation bugs
    std::vector<SceneElement*> objects;
    std::vector<LightElement*> lightObjects; 
    std::vector<AnimationGroup*> animationGroups; 
    
    // Viewport management variables
    std::vector<Camera*> sceneCameras;      
    size_t activeCameraIdx = 0;             
    Camera* activeCamera = nullptr;         

    myglm::mat4 projection;
    std::vector<Light*> sceneLights;       
    Ocean* sceneOcean = nullptr;
    Skybox* sceneSkybox = nullptr; 

    unsigned int litShader = 0, waterShader = 0, unlitShader = 0, skyboxShader = 0, lastShaderProgram = 0;    unsigned int litModelLoc, litViewLoc, litProjLoc, litViewPosLoc;
    unsigned int waterModelLoc, waterViewLoc, waterProjLoc, waterViewPosLoc, waterTimeLoc, waterSurfaceYLoc;
    unsigned int unlitModelLoc, unlitViewLoc, unlitProjLoc, unlitColorLoc;
    unsigned int unlitDirLoc;

    float lastTime = 0.0f; float deltaTime = 0.0f; bool isSceneDirty = true;

    Scene(float fovy = myglm::radians(45.0f), float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), float zNear = 0.1f, float zFar = 200.0f) {
        projection = myglm::perspective(fovy, aspect, zNear, zFar);
        lastTime = static_cast<float>(glfwGetTime());
    }

    void setShaders(unsigned int standardLit, unsigned int water, unsigned int unlit, unsigned int skybox) {
        litShader = standardLit; waterShader = water; unlitShader = unlit; skyboxShader = skybox;
        litModelLoc   = glGetUniformLocation(litShader, "model");
        litViewLoc    = glGetUniformLocation(litShader, "view");
        litProjLoc    = glGetUniformLocation(litShader, "projection");
        litViewPosLoc = glGetUniformLocation(litShader, "viewPos");

        waterModelLoc     = glGetUniformLocation(waterShader, "model");
        waterViewLoc      = glGetUniformLocation(waterShader, "view");
        waterProjLoc      = glGetUniformLocation(waterShader, "projection");
        waterViewPosLoc   = glGetUniformLocation(waterShader, "viewPos");
        waterTimeLoc      = glGetUniformLocation(waterShader, "time");
        waterSurfaceYLoc  = glGetUniformLocation(waterShader, "waterSurfaceY");

        unlitModelLoc = glGetUniformLocation(unlitShader, "model");
        unlitViewLoc  = glGetUniformLocation(unlitShader, "view");
        unlitProjLoc  = glGetUniformLocation(unlitShader, "projection");
        unlitColorLoc = glGetUniformLocation(unlitShader, "lightColor");
        unlitDirLoc   = glGetUniformLocation(unlitShader, "lightDir");
    }

    void setOcean(Ocean* ocean) { sceneOcean = ocean; }
    void setSkybox(Skybox* skybox) { sceneSkybox = skybox; }
    void addLight(Light* light) { sceneLights.push_back(light); }
    void addSceneElement(SceneElement* obj) { objects.push_back(obj); }
    void addLightElement(LightElement* obj) { lightObjects.push_back(obj); addLight(&obj->light); }
    void addAnimationGroup(AnimationGroup* group) { animationGroups.push_back(group); }

    void addCamera(Camera* cameraPointer) {
        sceneCameras.push_back(cameraPointer);
        if (activeCamera == nullptr) {
            activeCamera = cameraPointer;
            activeCameraIdx = 0;
        }
    }

    void cycleCamera() {
        if (sceneCameras.empty()) return;
        activeCameraIdx = (activeCameraIdx + 1) % sceneCameras.size();
        activeCamera = sceneCameras[activeCameraIdx];
        
        std::string modeLabel = "UNKNOWN";
        if (activeCamera->mode == CameraMode::DRONE)    modeLabel = "FREE FLIGHT DRONE";
        if (activeCamera->mode == CameraMode::STATIC)   modeLabel = "STATIONARY PERSPECTIVE";
        if (activeCamera->mode == CameraMode::ATTACHED) modeLabel = "RIGID GROUP LINKED";

        std::cout << "[CAMERA SYSTEM] Active viewport swapped to Index [" << activeCameraIdx 
                  << "] -> Running Mode: " << modeLabel << "\n";
        isSceneDirty = true;
    }

    float updateTime() {
        float currentTime = static_cast<float>(glfwGetTime());
        deltaTime = currentTime - lastTime; lastTime = currentTime;
        return currentTime;
    }

    void processInputs() {
        if (!activeCamera) return;
        float flightSpeed = 18.0f * deltaTime; float rotSpeed = 2.0f * deltaTime;
        if (g_LeftPressed)  activeCamera->rotateView(rotSpeed, 0.0f);   if (g_RightPressed) activeCamera->rotateView(-rotSpeed, 0.0f);
        if (g_UpPressed)    activeCamera->rotateView(0.0f, rotSpeed);   if (g_DownPressed)  activeCamera->rotateView(0.0f, -rotSpeed);
        if (g_WPressed)     activeCamera->moveForward(flightSpeed);     if (g_SPressed)     activeCamera->moveForward(-flightSpeed);
        if (g_APressed)     activeCamera->moveStrafe(-flightSpeed);     if (g_DPressed)     activeCamera->moveStrafe(flightSpeed);
    }

    void run() {
        float currentTime = updateTime();
        processInputs();

        if (!activeCamera) return;

        if (sceneCameras.size() > 1 && animationGroups.size() > 0 && !animationGroups[0]->lightElements.empty()) {
            animationGroups[0]->lightElements[0].localOffsetRot.y = sceneCameras[1]->yaw + myglm::PI;
        }

        // 1. Unified physical transformation calculations pass
        float seaLevel = (sceneOcean != nullptr) ? sceneOcean->height : 0.0f;
        AnimationSystem::update(animationGroups, currentTime, deltaTime, seaLevel);

        myglm::mat4 view = activeCamera->getViewMatrix();

        // Pass 1: Lit Scenery Geometry
        updateGlobalUniforms(litShader);
        for (const auto& obj : objects) {
            if (!obj->isVisible || obj->asset == nullptr) continue; 
            glUniformMatrix4fv(litModelLoc, 1, GL_FALSE, myglm::value_ptr(obj->getModelMatrix()));
            obj->asset->draw(litShader);
        }

        // Pass 2: Unlit Emissive Indicators
        glUseProgram(unlitShader); lastShaderProgram = unlitShader;
        glUniformMatrix4fv(unlitViewLoc, 1, GL_FALSE, myglm::value_ptr(view));
        glUniformMatrix4fv(unlitProjLoc, 1, GL_FALSE, myglm::value_ptr(projection));

        for (const auto& obj : lightObjects) {
            if (!obj->isVisible || obj->asset == nullptr) continue;
            glUniformMatrix4fv(unlitModelLoc, 1, GL_FALSE, myglm::value_ptr(obj->getModelMatrix()));
            glUniform3fv(unlitColorLoc, 1, myglm::value_ptr(obj->light.color));
            if (obj->light.type == LightType::SPOTLIGHT) {
                // Pass the active tracking direction of the beam
                glUniform3fv(unlitDirLoc, 1, myglm::value_ptr(obj->light.direction));
            } else {
                // Point lights (like the boat lantern) have no direction; pass zero to disable masking
                glUniform3fv(unlitDirLoc, 1, myglm::value_ptr(myglm::vec3(0.0f)));
            }
            obj->asset->draw(unlitShader);
        }
        // Pass 2.5: Skybox Background
        if (sceneSkybox != nullptr) {
            sceneSkybox->draw(skyboxShader, view, projection);
            lastShaderProgram = skyboxShader;
        }
        // Pass 3: Translucent Water Line
        if (sceneOcean != nullptr) {
            updateGlobalUniforms(waterShader);
            glUniform1f(waterTimeLoc, currentTime);
            glUniform1f(waterSurfaceYLoc, sceneOcean->height);
            sceneOcean->draw(waterShader);
        }
        isSceneDirty = false;
    }

    void updateGlobalUniforms(unsigned int shaderProgram) {
        bool shaderSwapped = false;
        if (lastShaderProgram != shaderProgram) { glUseProgram(shaderProgram); lastShaderProgram = shaderProgram; shaderSwapped = true; }
        
        myglm::mat4 view = activeCamera->getViewMatrix();
        if (shaderProgram == litShader) {
            glUniformMatrix4fv(litViewLoc, 1, GL_FALSE, myglm::value_ptr(view));
            glUniformMatrix4fv(litProjLoc, 1, GL_FALSE, myglm::value_ptr(projection));
            glUniform3fv(litViewPosLoc, 1, myglm::value_ptr(activeCamera->position));
        } else if (shaderProgram == waterShader) {
            glUniformMatrix4fv(waterViewLoc, 1, GL_FALSE, myglm::value_ptr(view));
            glUniformMatrix4fv(waterProjLoc, 1, GL_FALSE, myglm::value_ptr(projection));
            glUniform3fv(waterViewPosLoc, 1, myglm::value_ptr(activeCamera->position));
        }

        if (isSceneDirty || shaderSwapped) {
            int pointCount = 0; int spotCount = 0; bool hasDir = false;
            for (size_t i = 0; i < sceneLights.size(); ++i) {
                Light* l = sceneLights[i];
                if (l->type == LightType::DIRECTIONAL) {
                    glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.direction"), 1, myglm::value_ptr(l->direction));
                    glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.color"), 1, myglm::value_ptr(l->color));
                    glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.ambientStrength"), l->ambientStrength);
                    glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.diffuseStrength"), l->diffuseStrength);
                    glUniform1f(glGetUniformLocation(shaderProgram, "dirLight.specularStrength"), l->specularStrength);
                    hasDir = true;
                } else if (l->type == LightType::POINT && pointCount < 4) {
                    std::string base = "pointLights[" + std::to_string(pointCount) + "].";
                    glUniform3fv(glGetUniformLocation(shaderProgram, (base + "position").c_str()), 1, myglm::value_ptr(l->position));
                    glUniform3fv(glGetUniformLocation(shaderProgram, (base + "color").c_str()), 1, myglm::value_ptr(l->color));
                    glUniform1f(glGetUniformLocation(shaderProgram, (base + "ambientStrength").c_str()), l->ambientStrength);
                    glUniform1f(glGetUniformLocation(shaderProgram, (base + "diffuseStrength").c_str()), l->diffuseStrength);
                    glUniform1f(glGetUniformLocation(shaderProgram, (base + "specularStrength").c_str()), l->specularStrength);
                    pointCount++;
                } else if (l->type == LightType::SPOTLIGHT && spotCount < 4) {
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
            glUniform1i(glGetUniformLocation(shaderProgram, "hasDirLight"), hasDir ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "activePointLightCount"), pointCount);
            glUniform1i(glGetUniformLocation(shaderProgram, "activeSpotLightCount"), spotCount);
        }
    }
};

// ====================================================================
// WINDOW & INPUT HOOK CALLS
// ====================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (g_SceneInstance && height > 0) {
        g_SceneInstance->projection = myglm::perspective(myglm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 200.0f);
        g_SceneInstance->isSceneDirty = true;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_C) {
            if (g_SceneInstance) g_SceneInstance->cycleCamera();
            return;
        }
    }
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool isPressed = (action == GLFW_PRESS);
        switch (key) {
            case GLFW_KEY_LEFT:  g_LeftPressed  = isPressed; break;
            case GLFW_KEY_RIGHT: g_RightPressed = isPressed; break;
            case GLFW_KEY_UP:    g_UpPressed    = isPressed; break;
            case GLFW_KEY_DOWN:  g_DownPressed  = isPressed; break;
            case GLFW_KEY_W:     g_WPressed     = isPressed; break;
            case GLFW_KEY_S:     g_SPressed     = isPressed; break;
            case GLFW_KEY_A:     g_APressed     = isPressed; break;
            case GLFW_KEY_D:     g_DPressed     = isPressed; break;
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
            default: break;
        }
    }
}

// ====================================================================
// APPLICATION ENTRY POINT
// ====================================================================
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Boats Simulation", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window); if (!gladLoadGL(glfwGetProcAddress)) return -1;
    glfwSwapInterval(1); glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glClearColor(0.01f, 0.02f, 0.05f, 1.0f);

    Scene scene(myglm::radians(45.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT));
    g_SceneInstance = &scene; glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); glfwSetKeyCallback(window, key_callback);

    unsigned int shaderProgram = loadShaders("shader.vert", "shader.frag");
    unsigned int waterShader   = loadShaders("water.vert", "water.frag");
    unsigned int unlitShader   = loadShaders("unlit.vert", "unlit.frag");
    unsigned int skyboxShader  = loadShaders("skybox.vert", "skybox.frag");

    scene.setShaders(shaderProgram, waterShader, unlitShader,skyboxShader);

    std::cout << "\n[ENGINE] Starting model initialization block...\n";
    Model galleonModel("galleon", "galleon.obj");
    Model boatModel("boat", "boat.obj");      
    Model lighthouseModel("lighthouse2", "lighthouse2.obj");
    Model ballModel("ball", "ball.obj");
    std::cout << "[ENGINE] Initialization block complete!\n\n";

    // ====================================================================
    // CAMERA SYSTEM INVENTORIES
    // ====================================================================
    // Camera 1: Free flight drone camera
    Camera freeDroneCam(CameraMode::DRONE, myglm::vec3(0.0f, 15.0f, 50.0f));
    scene.addCamera(&freeDroneCam);

    // Camera 2: Static viewpoint positioned on top of the lighthouse deck
    Camera lighthouseStationaryCam(CameraMode::STATIC, myglm::vec3(-1.0f, 4.45f, -0.9f));
    scene.addCamera(&lighthouseStationaryCam);

    // Camera 3: Attached perspective that rides along with the orbiting boat group
    Camera boatRiderCam(CameraMode::ATTACHED, myglm::vec3(0.0f));
    scene.addCamera(&boatRiderCam);

    // ====================================================================
    // BEHAVIORAL GROUPS ALLOCATIONS (Pivot-Offset Managers)

    // ====================================================================
    AnimationGroup lighthouseGroup(AnimationType::STATIC, myglm::vec3(0.0f, -10.0f, 0.0f));
    AnimationGroup galleonGroup(AnimationType::ANCHORED_WATER, myglm::vec3(15.0f, 0.0f, 0.0f));
    
    AnimationGroup activeOrbitGroup(AnimationType::SAILING_ORBIT, myglm::vec3(0.0f));
    activeOrbitGroup.orbitRadius = 42.0f; 
    activeOrbitGroup.orbitSpeed  = 0.20f;

    // ====================================================================
    // LIGHTS PACKET INSTANTIATIONS
    // ====================================================================
    // 1. Core Ambient Directional Moonlight
    Light midnightMoon(LightType::DIRECTIONAL);
    midnightMoon.setDirection(myglm::vec3(0.2f, -1.0f, 0.4f));
    midnightMoon.color            = myglm::vec3(0.12f, 0.18f, 0.32f);
    midnightMoon.ambientStrength  = 0.05f;
    midnightMoon.diffuseStrength  = 0.25f;
    midnightMoon.specularStrength = 0.40f;
    scene.addLight(&midnightMoon);

    // 2. Focused Sweeping Searchlight (Lighthouse)
    LightElement searchlightBeam(&ballModel, LightType::SPOTLIGHT, myglm::vec3(0.0f), myglm::vec3(0.006f));
    searchlightBeam.light.color            = myglm::vec3(1.0f, 0.95f, 0.82f);
    searchlightBeam.light.ambientStrength  = 0.00f;
    searchlightBeam.light.diffuseStrength  = 6.0f;
    searchlightBeam.light.specularStrength = 4.0f;
    searchlightBeam.light.cutOff           = std::cos(myglm::radians(7.5f));
    searchlightBeam.light.outerCutOff      = std::cos(myglm::radians(13.0f));
    scene.addLightElement(&searchlightBeam);

    // 3. Floating Masthead Beacon (Sailing Boat)
    LightElement boatLantern(&ballModel, LightType::POINT, myglm::vec3(0.0f), myglm::vec3(0.001f));
    boatLantern.light.color            = myglm::vec3(0.1f, 0.9f, 0.3f); // Glowing emerald
    boatLantern.light.ambientStrength  = 0.00f;
    boatLantern.light.diffuseStrength  = 2.5f;
    boatLantern.light.specularStrength = 1.5f;
    scene.addLightElement(&boatLantern);

    
    SceneElement towerStructure(&lighthouseModel, myglm::vec3(0.0f),myglm::vec3(0.05f));
    scene.addSceneElement(&towerStructure);

    SceneElement merchantGalleon(&galleonModel, myglm::vec3(0.0f), myglm::vec3(3.0f));
    scene.addSceneElement(&merchantGalleon);

    SceneElement playerCruiser(&boatModel, myglm::vec3(0.0f), myglm::vec3(0.005f));
    scene.addSceneElement(&playerCruiser);

    // ====================================================================
    // RIGID HIERARCHICAL MEMBER REGISTRATION (Offset Linking Maps)
    // ====================================================================
    // A. Populate Stationary Lighthouse Tower Cluster
    lighthouseGroup.addMember(&towerStructure, myglm::vec3(0.0f, 0.0f, 0.0f));
    lighthouseGroup.addMember(&searchlightBeam, myglm::vec3(-1.0f, 16.4f, -1.1f), myglm::vec3(1.35f, -1.0f, 0.0f)); // Lantern platform placement

    // B. Populate Anchored Bobbing Galleon Cluster
    galleonGroup.addMember(&merchantGalleon, myglm::vec3(0.0f, 2.3f, 0.0f));

    // C. Populate Sailing Orbit Boat Cluster
    activeOrbitGroup.addMember(&playerCruiser, myglm::vec3(0.0f, -0.5f, 0.0f),myglm::vec3(-90.0f, 0.0f, 0.0f));
    activeOrbitGroup.addMember(&boatLantern, myglm::vec3(-0.60f, 4.4f, -2.35f)); // Mounted to top bow rigging
    
    // Mount camera 6 units behind and 3.5 units above the boat pivot center
    // localYaw offset of 3.14159f spins the camera lens 180 degrees to look forward at the boat's direction of travel
    activeOrbitGroup.addMember(&boatRiderCam, myglm::vec3(2.0f, 2.0f, -1.0f), myglm::PI, -0.15f);

    // Register active behavioral groups to management arrays
    scene.addAnimationGroup(&lighthouseGroup);
    scene.addAnimationGroup(&galleonGroup);
    scene.addAnimationGroup(&activeOrbitGroup);

    // Initialize ocean plane simulation lines
    Ocean ocean(500.0f, 0.50f, -08.00f); scene.setOcean(&ocean);
    // set skybox
    Skybox skybox({
    std::string(MODELS_PATH) + "skybox/px.png",
    std::string(MODELS_PATH) + "skybox/nx.png",
    std::string(MODELS_PATH) + "skybox/py.png",
    std::string(MODELS_PATH) + "skybox/ny.png",
    std::string(MODELS_PATH) + "skybox/pz.png",
    std::string(MODELS_PATH) + "skybox/nz.png"
});
    scene.setSkybox(&skybox);

    // Central application processing block loops
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene.run();
        glfwSwapBuffers(window); glfwPollEvents();
    }

    glDeleteProgram(shaderProgram); glDeleteProgram(waterShader); glDeleteProgram(unlitShader);glDeleteProgram(skyboxShader);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
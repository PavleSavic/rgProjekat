#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <math.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool gammaCorrection);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0.1f,0.1f,0.1f);
    Camera camera;
    bool ImGuiEnabled = false;
    bool CameraMouseMovementUpdateEnabled = true;
    bool CameraScrollingEnabled = true;
    bool KeyboardMovementEnabled = true;
    bool skyBoxEnabled = true;
    glm::vec3 dragonPosition = glm::vec3(4.0f,4.0f,-10.0f);
    float dragonScale = 0.2f;
    DirLight dirLight;
    PointLight pointLights[4];
    SpotLight spotLight;
    float materialShininess = 16.0f; // 32.0f
    bool gamma = false;
    ProgramState()
    : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}
};

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    if (programState->CameraMouseMovementUpdateEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    //Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);
    // enabling blending and setting factors for blend function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader lightingShader("resources/shaders/lights.vs", "resources/shaders/lights.fs");
    Shader lightCubeShader("resources/shaders/light_cube.vs", "resources/shaders/light_cube.fs");
    Shader targetShader("resources/shaders/target_shader.vs", "resources/shaders/target_shader.fs");
    Shader windowShader("resources/shaders/windows.vs", "resources/shaders/windows.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    // positions all containers
    glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f,  0.0f,  0.0f),
            glm::vec3( 2.0f,  5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f,  3.0f, -7.5f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f,  2.0f, -2.5f),
            glm::vec3( 1.5f,  0.2f, -1.5f),
            glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
            glm::vec3( 0.7f,  0.2f,  2.0f),
            glm::vec3( 1.5f, -1.3f, -2.0f),
            glm::vec3(-2.0f,  2.0f, -4.0f),
            glm::vec3( 1.0f,  1.0f, -3.0f)
    };
    // positions of the rock models
    glm:: vec3 rockPositions[] = {
            glm::vec3( 3.0f, 4.1f,  -5.0f),
            glm::vec3( 4.5f, -2.5f, -8.3f),
            glm::vec3(-4.4f,  5.6f, -9.0f),
            glm::vec3( -3.0f,  -3.2f, -6.4f)
    };
    // configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures (we now use a utility function to keep the code more organized)
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/container2.png").c_str(), false);
    unsigned int diffuseMapGammaCorrected = loadTexture(FileSystem::getPath("resources/textures/container2.png").c_str(), true);
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/container2_specular.png").c_str(),false);

    lightingShader.use();
    lightingShader.setInt("material.texture_diffuse1", 0);
    lightingShader.setInt("material.texture_specular1", 1);

    // load models
    Model rockModel("resources/objects/rock/rock.obj");
    rockModel.SetShaderTextureNamePrefix("material.");

    Model bowModel("resources/objects/bow/bow.obj");
    bowModel.SetShaderTextureNamePrefix("material.");

    Model dragonModel("resources/objects/dragon/smaug.obj");
    dragonModel.SetShaderTextureNamePrefix("material.");

    for (auto& texture : rockModel.textures_loaded)
        std::cerr << texture.path << ' ' << texture.type << '\n';
    for (auto& texture : bowModel.textures_loaded)
        std::cerr << texture.path << ' ' << texture.type << '\n';
    for (auto& texture : dragonModel.textures_loaded)
        std::cerr << texture.path << ' ' << texture.type << '\n';

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    dirLight.specular = glm::vec3 (0.5f,0.5f,0.5f);

    for (int i=0; i<4; i++) {
        PointLight& pointLight = programState->pointLights[i];
        pointLight.position = pointLightPositions[i];
        pointLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        pointLight.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        pointLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

        pointLight.constant = 1.0f;
        pointLight.linear = 0.09f;
        pointLight.quadratic = 0.032f;
    }

    SpotLight& spotLight = programState->spotLight;
    spotLight.ambient = glm::vec3 (0.0f,0.0f,0.0f);
    spotLight.diffuse = glm::vec3 (1.0f,1.0f,1.0f);
    spotLight.specular = glm::vec3 (1.0f,1.0f,1.0f);

    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;

    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));

    // define target verticles, send to GPU
    float targetVertices[] = {
            // positions      // colors         // texture coords
            0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f,1.0f,
            0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f,0.0f,
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f,0.0f,
            -0.5f, 0.5f, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f,1.0f,
    };

    unsigned int indices[] = {
            0,1,3,
            1,2,3
    };

    unsigned int VBO1, VAO1, EBO1;
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);
    glGenBuffers(1, &EBO1);

    glBindVertexArray(VAO1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(targetVertices), targetVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int targetTexture = loadTexture(FileSystem::getPath("resources/textures/grass.jpg").c_str(),false);
    unsigned int targetTexture1 = loadTexture(FileSystem::getPath("resources/textures/target.png").c_str(),false);
    targetShader.use();
    targetShader.setInt("texture1", 0);
    targetShader.setInt("texture2", 1);

    // define verticles for window
    float windowVertices[] = {
            // positions         // texture Coords
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    vector<glm::vec3> windowPositions = {
            glm::vec3( 3.0f,  4.1f,  -3.0f),
            glm::vec3( 3.0f, 4.1f, -7.0f),
            glm::vec3(-3.0f,  -3.2f, -9.1f),
            glm::vec3( 1.0f,  1.0f, -10.9f)
    };

    // send to GPU
    unsigned int windowVAO, windowVBO;
    glGenVertexArrays(1, &windowVAO);
    glGenBuffers(1, &windowVBO);
    glBindVertexArray(windowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(windowVertices), windowVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int windowTexture = loadTexture(FileSystem::getPath("resources/textures/window.png").c_str(),false);

    windowShader.use();
    windowShader.setInt("texture1", 0);

    //cubemap implementation
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
    {
        FileSystem::getPath("resources/textures/skybox1/posx.jpg"), // right
        FileSystem::getPath("resources/textures/skybox1/negx.jpg"), // left
        FileSystem::getPath("resources/textures/skybox1/posy.jpg"), // top
        FileSystem::getPath("resources/textures/skybox1/negy.jpg"), // bottom
        FileSystem::getPath("resources/textures/skybox1/posz.jpg"), // front
        FileSystem::getPath("resources/textures/skybox1/negz.jpg") // back
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox",0);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // sort the windows before rendering (argument is distance between camera and window descending)
        std::sort(windowPositions.begin(), windowPositions.end(),
                  [cameraPosition = programState->camera.Position](const glm::vec3& a, const glm::vec3& b) {
            float d1 = glm::distance(a, cameraPosition);
            float d2 = glm::distance(b, cameraPosition);
            return d1 > d2;
        });

        // render
        glClearColor(programState->clearColor.x,programState->clearColor.y, programState->clearColor.z,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.use();
        lightingShader.setVec3("viewPos", programState->camera.Position);
        lightingShader.setFloat("material.shininess", programState-> materialShininess);

        lightingShader.setVec3("dirLight.direction", programState->dirLight.direction);
        lightingShader.setVec3("dirLight.ambient", programState->dirLight.ambient);
        lightingShader.setVec3("dirLight.diffuse", programState->dirLight.diffuse);
        lightingShader.setVec3("dirLight.specular", programState->dirLight.specular);

        glm::vec3 dynamicPointLightsPositions[4];

        // point light 1
        dynamicPointLightsPositions[0] = glm::vec3(programState->pointLights[0].position.x * cos(currentFrame), programState->pointLights[0].position.y, programState->pointLights[0].position.z * sin(currentFrame));
        lightingShader.setVec3("pointLights[0].position", dynamicPointLightsPositions[0]);
        lightingShader.setVec3("pointLights[0].ambient", programState->pointLights[0].ambient);
        lightingShader.setVec3("pointLights[0].diffuse", programState->pointLights[0].diffuse);
        lightingShader.setVec3("pointLights[0].specular", programState->pointLights[0].specular);
        lightingShader.setFloat("pointLights[0].constant", programState->pointLights[0].constant);
        lightingShader.setFloat("pointLights[0].linear", programState->pointLights[0].linear);
        lightingShader.setFloat("pointLights[0].quadratic", programState->pointLights[0].quadratic);
        // point light 2
        dynamicPointLightsPositions[1] = glm::vec3(programState->pointLights[1].position.x * cos(currentFrame), programState->pointLights[1].position.y, programState->pointLights[1].position.z * sin(currentFrame));
        lightingShader.setVec3("pointLights[1].position", dynamicPointLightsPositions[1]);
        lightingShader.setVec3("pointLights[1].ambient", programState->pointLights[1].ambient);
        lightingShader.setVec3("pointLights[1].diffuse", programState->pointLights[1].diffuse);
        lightingShader.setVec3("pointLights[1].specular", programState->pointLights[1].specular);
        lightingShader.setFloat("pointLights[1].constant", programState->pointLights[1].constant);
        lightingShader.setFloat("pointLights[1].linear", programState->pointLights[1].linear);
        lightingShader.setFloat("pointLights[1].quadratic", programState->pointLights[1].quadratic);
        // point light 3
        dynamicPointLightsPositions[2] = glm::vec3(programState->pointLights[2].position.x, programState->pointLights[2].position.y * cos(currentFrame), programState->pointLights[2].position.z * sin(currentFrame));
        lightingShader.setVec3("pointLights[2].position", dynamicPointLightsPositions[2]);
        lightingShader.setVec3("pointLights[2].ambient", programState->pointLights[2].ambient);
        lightingShader.setVec3("pointLights[2].diffuse", programState->pointLights[2].diffuse);
        lightingShader.setVec3("pointLights[2].specular", programState->pointLights[2].specular);
        lightingShader.setFloat("pointLights[2].constant", programState->pointLights[2].constant);
        lightingShader.setFloat("pointLights[2].linear", programState->pointLights[2].linear);
        lightingShader.setFloat("pointLights[2].quadratic", programState->pointLights[2].quadratic);
        // point light 4
        dynamicPointLightsPositions[3] = glm::vec3(programState->pointLights[3].position.x, programState->pointLights[3].position.y * cos(currentFrame), programState->pointLights[3].position.z * sin(currentFrame));
        lightingShader.setVec3("pointLights[3].position", dynamicPointLightsPositions[3]);
        lightingShader.setVec3("pointLights[3].ambient", programState->pointLights[3].ambient);
        lightingShader.setVec3("pointLights[3].diffuse", programState->pointLights[3].diffuse);
        lightingShader.setVec3("pointLights[3].specular", programState->pointLights[3].specular);
        lightingShader.setFloat("pointLights[3].constant", programState->pointLights[3].constant);
        lightingShader.setFloat("pointLights[3].linear", programState->pointLights[3].linear);
        lightingShader.setFloat("pointLights[3].quadratic", programState->pointLights[3].quadratic);
        // spotLight
        lightingShader.setVec3("spotLight.position", programState->camera.Position);
        lightingShader.setVec3("spotLight.direction", programState->camera.Front);
        lightingShader.setVec3("spotLight.ambient", programState->spotLight.ambient);
        lightingShader.setVec3("spotLight.diffuse", programState->spotLight.diffuse);
        lightingShader.setVec3("spotLight.specular", programState->spotLight.specular);
        lightingShader.setFloat("spotLight.constant", programState->spotLight.constant);
        lightingShader.setFloat("spotLight.linear", programState->spotLight.linear);
        lightingShader.setFloat("spotLight.quadratic", programState->spotLight.quadratic);
        lightingShader.setFloat("spotLight.cutOff", programState->spotLight.cutOff);
        lightingShader.setFloat("spotLight.outerCutOff", programState->spotLight.outerCutOff);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        //bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, programState->gamma ? diffuseMapGammaCorrected : diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        lightingShader.setInt("gamma", programState->gamma);

        // render containers
        glBindVertexArray(cubeVAO);
        for (unsigned int i = 0; i < 10; i++) {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            if(i%3 == 1) {
                angle = (1+sin(glfwGetTime()))/2 * 30.0f;
            }
            if(i%3 == 2) {
                angle = (1+cos(glfwGetTime()))/2 * 30.0f;
            }

            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            lightingShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // we can use same shader program for rendering rock models
        for (unsigned int i = 0; i < 4; i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, rockPositions[i]);
            model = glm::scale(model, glm::vec3(1.1f));
            lightingShader.setMat4("model", model);
            rockModel.Draw(lightingShader);
        }
        // bow model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(programState->camera.Position.x-0.15, programState->camera.Position.y, programState->camera.Position.z-1));
        model = glm::rotate(model, (float)(M_PI/2.0) ,glm::vec3(1.0f,0.0f,0.0f));
        model = glm::scale(model,glm::vec3(0.2f));
        lightingShader.setMat4("model",model);
        bowModel.Draw(lightingShader);

        // dragon model
        model = glm::mat4(1.0f);
        model = glm::translate(model, programState->dragonPosition);
        model = glm::scale(model, glm::vec3(programState->dragonScale));
        lightingShader.setMat4("model",model);
        dragonModel.Draw(lightingShader);

        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        // we now draw as many light bulbs as we have point lights.
        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < 4; i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, dynamicPointLightsPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightCubeShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // enable shader before setting uniforms
        targetShader.use();
        targetShader.setMat4("projection", projection);
        targetShader.setMat4("view", view);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, targetTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, targetTexture1);

        glBindVertexArray(VAO1);
        for (unsigned int i = 0; i < 2; i++) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3((float)i*5,2.0f,-18.0f));
            model = glm::scale(model, glm::vec3(1.5f));
            targetShader.setMat4("model", model);
            glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
        }

        windowShader.use();
        windowShader.setMat4("projection", projection);
        windowShader.setMat4("view", view);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, windowTexture);
        glBindVertexArray(windowVAO);

        for (const glm::vec3& w : windowPositions) {
            model = glm::mat4(1.0f);
            model = glm::translate(model,w);
            model = glm::scale(model, glm::vec3(3.0f));
            windowShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0 ,6);
        }

        // at the end draw skybox
        if(programState->skyBoxEnabled) {
            glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
            skyboxShader.use();
            view = glm::mat4(glm::mat3(view)); // remove translation from the view matrix
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);
            // skybox cube
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS); // set depth function back to default
        }

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &VAO1);
    glDeleteVertexArrays(1,&windowVAO);
    glDeleteVertexArrays(1,&skyboxVAO);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &EBO1);
    glDeleteBuffers(1, &windowVBO);
    glDeleteBuffers(1,&skyboxVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (programState->KeyboardMovementEnabled) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (programState->CameraScrollingEnabled)
        programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera movement settings");
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::Checkbox("Camera mouse scrolling", &programState->CameraScrollingEnabled);
        ImGui::Checkbox("Camera keyboard update", &programState->KeyboardMovementEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        std::cerr << "Change clear color to RED\n";
        programState->clearColor = glm::vec3(1.0f,0.0f,0.f);
    }
    if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
        std::cerr << "Change clear color to GREEN\n";
        programState->clearColor = glm::vec3 (0.0f,1.0f,0.f);
    }
    if (key == GLFW_KEY_B && action == GLFW_RELEASE) {
        std::cerr << "Change clear color to BLUE\n";
        programState->clearColor = glm::vec3 (0.0f,0.0f,1.0f);
    }
    if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
        std::cerr << "Change clear color to DEFAULT\n";
        programState->clearColor = glm::vec3 (0.1f,0.1f,0.1f);
    }
    if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
        programState->materialShininess = programState->materialShininess * 2.0f;
        std::cerr << "Material shininess = " << programState->materialShininess << "\n";
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
        std::cerr << "Material shininess --\n";
        programState->materialShininess = programState->materialShininess / 2.0f;
        std::cerr << "Material shininess = " << programState->materialShininess << "\n";
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled)
            std::cerr << "GUI enabled\n";
        else
            std::cerr << "GUI disabled\n";
    }
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->skyBoxEnabled = !programState->skyBoxEnabled;
        if (programState->skyBoxEnabled)
            std::cerr << "Skybox enabled\n";
        else
            std::cerr << "Skybox disabled\n";
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        programState->gamma = !programState->gamma;
        if(programState->gamma)
            std::cerr << "Gamma correction enabled\n";
        else
            std::cerr << "Gamma correction disabled\n";
    }
}

unsigned int loadTexture(char const * path, bool gammaCorrection) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
            internalFormat = dataFormat = GL_RED;
        else if (nrComponents == 3) {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4) {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // important for blending
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, dataFormat == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, dataFormat == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
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


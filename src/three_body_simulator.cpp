#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader/shader.h>
#include <camera/camera.h>
#include <body/body.h>
#include <stb_image/stb_image.h>

#include <iostream>
#include <cmath>
#include <vector>
using namespace std;

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);

vector<float> createSphereVertices();
vector<int> createSphereIndices();
vector<Body> creatBodies(int n);
glm::mat4 drawSphere(glm::dvec3 center_hp, double radius_hp);
glm::mat4 cvtMat4Lp(glm::dmat4 mat4Hp);
glm::vec3 cvtVec3Lp(glm::dvec3 vec3Hp);
vector<unsigned int> loadTextures(const int n, const char* rootPath, const char* textureType);

// PI
const double PI = 3.141592653589793238462643383279502884;

// settings
const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;

// sphere segments
const unsigned int Y_SEGMENTS = 50;
const unsigned int X_SEGMENTS = 50;

// camera setting
Camera camera(glm::vec3(0.0f, 0.0f, 50.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// time between current frame and last frame
float deltaTime = 0.0f;
float lastFrame = 0.0f;

double tPerFrame = 0.01;

// steps of calculation each frame
const int steps = 100;

enum DisplayMode
{
    RANDOM,
    SUN_PLANET,
    SUN_PLANET_MOON,
    SUN_PLANET_COMET,
    BINARY_STAR_PLANET,
    TROJAN_ASTERIODS,
    FOUR_STAR_BALLET,
    SLINGSHOT,
    DOUBLE_SLINGSHOT,
    HYPERBOLICS,
    ELLIPSES,
    DOUBLE_DOUBLE
};


int main()
{
    int numberOfBodies;
    unsigned int seed, mode;
    cout << "\nWelcome to the Three-Body Simulator...\n";
    cout << "\nSelect the display mode (0 ~ 10)\n";
    cout << "0: generate randomly\t";
    cout << "1: Sun and planet\n";
    cout << "2: Sun, planet, moon\t";
    cout << "3: Sun, planet, comet\n";
    cout << "4: Binary star, planet\t";
    cout << "5: Trojan asteroids\n";
    cout << "6: Four star ballet\t";
    cout << "7: Slingshot\n";
    cout << "8: Double slingshot\t";
    cout << "9: Hyperbolics\n";
    cout << "10: Ellipses\t\t";
    cout << "11: Double double\n\n";
    cin >> mode;
    if (mode == 0)
    {
        cout << "Input the number of body: ";
        cin >> numberOfBodies;
        cout << "Input the random seed [uint]: ";
        cin >> seed;
        srand(seed);
    }

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    bool isFullScreen = true;
    int monitorCount;
    GLFWmonitor** pMonitor = isFullScreen ? glfwGetMonitors(&monitorCount) : NULL;
    int holographic_screen = -1;
    for(int i = 0; i < monitorCount; i++)
    {
        int screen_x, screen_y;
        const GLFWvidmode *mode = glfwGetVideoMode(pMonitor[i]);
        screen_x = mode->width;
        screen_y = mode->height;
        if(screen_x==1920 && screen_y==1080){
            holographic_screen = i;
        }
    }
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Holographic projection", pMonitor[holographic_screen], NULL);
    // GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Three-Body Simulator", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // callback setting
    // ----------------
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // cursor disabled
    // ---------------
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // load shaders
    // ------------
    Shader sphereShader("../shader/sphere_lt.vs", "../shader/sphere_lt.fs");
    Shader pathShader("../shader/path.vs", "../shader/path.fs");
    Shader skyboxShader("../shader/skybox.vs", "../shader/skybox.fs");
    Shader lightcubeShader("../shader/lightcube.vs", "../shader/lightcube.fs");

    // calculate sphere vertices and indices
    // -------------------------------------
    vector<float> sphereVertices = createSphereVertices();
    vector<int> sphereIndices = createSphereIndices();

    // sphere VAO, VBO, EBO
    // --------------------
    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(int), &sphereIndices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);  // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);  // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));  // texture
    glEnableVertexAttribArray(2);

    sphereShader.use();
    sphereShader.setInt("material.diffuse", 0);
    sphereShader.setInt("material.specular", 1);

    // path
    // TODO: maybe use loop to generate.
    float cubeVertices[] = {
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

    // path VAO, VBO
    // -------------
    unsigned int pathVAO, pathVBO;

    glGenVertexArrays(1, &pathVAO);
    glGenBuffers(1, &pathVBO);

    glBindVertexArray(pathVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    vector<glm::vec3> pointLightPositions;
    pointLightPositions.push_back(glm::vec3(20.0f, 0.0f, 0.0f));
    pointLightPositions.push_back(glm::vec3(-12.0f, 12.0f, 0.0f));

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    vector<std::string> faces
    {
        "../asset/texture/skybox/StarSkybox043.png",
        "../asset/texture/skybox/StarSkybox044.png",
        "../asset/texture/skybox/StarSkybox045.png",
        "../asset/texture/skybox/StarSkybox046.png",
        "../asset/texture/skybox/StarSkybox041.png",
        "../asset/texture/skybox/StarSkybox042.png"
    };

    unsigned int cubemapTexture = loadCubemap(faces);

    // shader configuration
    // --------------------
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // or GL_FILL as default

    // background color state setting
    // ------------------------------
    // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // bodies
    // ------
    // vector<Body> bodies = creatBodies(numberOfBodies);
    vector<Body> bodies;

    if (mode == RANDOM)
    {
        if (numberOfBodies > 6) numberOfBodies = 6;
        bodies = creatBodies(numberOfBodies);
    }
    else if (mode == SUN_PLANET)
    {
        // sun and planet
        Body sun(20.0, 3.0, glm::vec3(1.0f, 0.7f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -0.6, 0.0));
        bodies.push_back(sun);
        Body planet(1.0, 1.0, glm::vec3(0.0f, 0.4f, 1.0f), glm::dvec3(15.0, 0.0, 0.0), glm::dvec3(0.0, 12.0, 0.0));
        bodies.push_back(planet);
    }
    else if (mode == SUN_PLANET_MOON)
    {
        // sun, planet and moon
        Body sun(20.0, 3.0, glm::vec3(1.0f, 0.7f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -0.6, 0.0));
        bodies.push_back(sun);
        Body planet(1.0, 1.0, glm::vec3(0.0f, 0.4f, 1.0f), glm::dvec3(16.0, 0.0, 0.0), glm::dvec3(0.0, 12.0, 0.0));
        bodies.push_back(planet);
        Body moon(0.0001, 0.2, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(14.0, 0.0, 0.0), glm::dvec3(0.0, 5.3, 0.0));
        bodies.push_back(moon);
    }
    else if (mode == SUN_PLANET_COMET)
    {
        // sun, planet and comet
        Body sun(20.0, 3.0, glm::vec3(1.0f, 0.7f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -0.6, 0.0));
        bodies.push_back(sun);
        Body planet(1.0, 1.0, glm::vec3(0.0f, 0.4f, 1.0f), glm::dvec3(15.0, 0.0, 0.0), glm::dvec3(0.0, 12.0, 0.0));
        bodies.push_back(planet);
        Body comet(0.0001, 0.2, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(-20.0, 10.0, 0.0), glm::dvec3(-2.0, -5.5, 0.0));
        bodies.push_back(comet);
    }
    else if (mode == BINARY_STAR_PLANET)
    {
        // binary star and planet
        Body star1(15.0, 1.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(-10.0, 0.0, 0.0), glm::dvec3(0.0, -6.0, 0.0));
        bodies.push_back(star1);
        Body star2(15.0, 1.0, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(10.0, 0.0, 0.0), glm::dvec3(0.0, 6.0, 0.0));
        bodies.push_back(star2);
        Body star3(0.001, 0.2, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(-5.0, 0.0, 0.0), glm::dvec3(0.0, 12.0, 0.0));
        bodies.push_back(star3);
    }
    else if (mode == TROJAN_ASTERIODS)
    {
        // trojan asteriods
        Body star1(20.0, 3.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 0.0));
        bodies.push_back(star1);
        Body star2(0.5, 1.0, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(15.0, 0.0, 0.0), glm::dvec3(0.0, 11.9, 0.0));
        bodies.push_back(star2);
        Body star3(0.001, 0.5, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(7.5, -13.0, 0.0), glm::dvec3(10.3, 6.0, 0.0));
        bodies.push_back(star3);
        Body star4(0.001, 0.5, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(7.5, 13.0, 0.0), glm::dvec3(-10.3, 6.0, 0.0));
        bodies.push_back(star4);
    }
    else if (mode == FOUR_STAR_BALLET)
    {
        // four star ballet
        Body star1(12.0, 1.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(-10.0, 10.0, 0.0), glm::dvec3(-5.0, -5.0, 0.0));
        bodies.push_back(star1);
        Body star2(12.0, 1.0, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(10.0, 10.0, 0.0), glm::dvec3(-5.0, 5.0, 0.0));
        bodies.push_back(star2);
        Body star3(12.0, 1.0, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(10.0, -10.0, 0.0), glm::dvec3(5.0, 5.0, 0.0));
        bodies.push_back(star3);
        Body star4(12.0, 1.0, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(-10.0, -10.0, 0.0), glm::dvec3(5.0, -5.0, 0.0));
        bodies.push_back(star4);
    }
    else if (mode == SLINGSHOT)
    {
        // slingshot
        Body star1(20.0, 3.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -0.6, 0.0));
        bodies.push_back(star1);
        Body star2(1.0, 1.0, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(15.0, 0.0, 0.0), glm::dvec3(0.0, 12.0, 0.0));
        bodies.push_back(star2);
        Body star3(0.001, 0.5, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(-0.6, -12.8, 0.0), glm::dvec3(10.0, 0.0, 0.0));
        bodies.push_back(star3);
    }
    else if (mode == DOUBLE_SLINGSHOT)
    {
        // double slingshot
        Body star1(20.0, 1.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -0.1, 0.0));
        bodies.push_back(star1);
        Body star2(0.5, 0.5, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(0.0, -11.2, 0.0), glm::dvec3(13.4, 0.0, 0.0));
        bodies.push_back(star2);
        Body star3(0.4, 0.5, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(18.6, -0.5, 0.0), glm::dvec3(0.1, 11.1, 0.0));
        bodies.push_back(star3);
        Body star4(0.001, 0.2, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(7.0, 7.2, 0.0), glm::dvec3(-4.7, 6.3, 0.0));
        bodies.push_back(star4);
    }
    else if (mode == HYPERBOLICS)
    {
        // hyperbolics
        double x = 20.0;
        double vx = -15.0;
        Body star1(25.0, 3.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(-5.0, -4.5, 0.0), glm::dvec3(0.0, 0.0, 0.0));
        bodies.push_back(star1);
        Body star2(0.001, 0.5, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(x, 5.0, 0.0), glm::dvec3(vx, 0.0, 0.0));
        bodies.push_back(star2);
        Body star3(0.001, 0.5, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(x, 12.0, 0.0), glm::dvec3(vx, 0.0, 0.0));
        bodies.push_back(star3);
        Body star4(0.001, 0.5, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(x, 19.0, 0.0), glm::dvec3(vx, 0.0, 0.0));
        bodies.push_back(star4);
    }
    else if (mode == ELLIPSES)
    {
        // ellipses
        Body star1(25.0, 3.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(-20.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 0.0));
        bodies.push_back(star1);
        Body star2(0.001, 0.5, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(-11.5, 0.0, 0.0), glm::dvec3(0.0, 15.1, 0.0));
        bodies.push_back(star2);
        Body star3(0.001, 0.5, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(5.0, 0.0, 0.0), glm::dvec3(0.0, 6.0, 0.0));
        bodies.push_back(star3);
        Body star4(0.001, 0.5, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(22.0, 0.0, 0.0), glm::dvec3(0.0, 3.7, 0.0));
        bodies.push_back(star4);
    }
    else if (mode == DOUBLE_DOUBLE)
    {
        // double double
        Body star1(6.0, 1.0, glm::vec3(1.0f, 1.0f, 0.0f), glm::dvec3(-11.5, -0.3, 0.0), glm::dvec3(0.0, -15.4, 0.0));
        bodies.push_back(star1);
        Body star2(7.0, 1.0, glm::vec3(1.0f, 0.0f, 1.0f), glm::dvec3(10.2, 0.0, 0.0), glm::dvec3(0.1, 15.0, 0.0));
        bodies.push_back(star2);
        Body star3(5.5, 1.0, glm::vec3(0.0f, 1.0f, 1.0f), glm::dvec3(-7.7, 0.2, 0.0), glm::dvec3(-0.1, 4.2, 0.0));
        bodies.push_back(star3);
        Body star4(6.2, 1.0, glm::vec3(1.0f, 1.0f, 1.0f), glm::dvec3(13.5, 0.0, 0.0), glm::dvec3(-0.1, -5.2, 0.0));
        bodies.push_back(star4);
    }
    else
    {
        numberOfBodies = 3;
        bodies = creatBodies(numberOfBodies);
    }
    
    BodySystem bodySystem(bodies);
    // bodySystem.info();

    // load texture
    // ------------
    int bodyCount = bodySystem.getBodies().size();
    vector<unsigned int> diffuseTextureIds = loadTextures(bodyCount, "../asset/texture/planet/", "diffuse");
    vector<unsigned int> specularTextureIds = loadTextures(bodyCount, "../asset/texture/planet/", "specular");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // background color update
        // -----------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set and update
        // --------------
        bodySystem.config(tPerFrame, steps);
        bodySystem.update();
        vector<Body> bdies = bodySystem.getBodies();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model;

        // light
        // -----
        sphereShader.use();
        sphereShader.setVec3("viewPos", camera.Position);
        // direct light
        sphereShader.setVec3("dirLights[0].direction", 0.0f, -1.0f, 0.0f);
        sphereShader.setVec3("dirLights[0].ambient", 0.2f, 0.2f, 0.2f);
        sphereShader.setVec3("dirLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        sphereShader.setVec3("dirLights[0].specular", 1.0f, 1.0f, 1.0f);
        // point light
        sphereShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        sphereShader.setVec3("pointLights[0].ambient", 0.2f, 0.2f, 0.2f);
        sphereShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        sphereShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        sphereShader.setFloat("pointLights[0].constant", 1.0f);
        sphereShader.setFloat("pointLights[0].linear", 0.022f);
        sphereShader.setFloat("pointLights[0].quadratic", 0.0019f);

        sphereShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        sphereShader.setVec3("pointLights[1].ambient", 0.2f, 0.2f, 0.2f);
        sphereShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        sphereShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        sphereShader.setFloat("pointLights[1].constant", 1.0f);
        sphereShader.setFloat("pointLights[1].linear", 0.022f);
        sphereShader.setFloat("pointLights[1].quadratic", 0.0019f);
        // material
        // sphereShader.setVec3("material.diffuse", glm::vec3(1.0f));
        // sphereShader.setVec3("material.specular", glm::vec3(0.2f));
        sphereShader.setVec3("material.emission", glm::vec3(0.0f));
        sphereShader.setFloat("material.shininess", 64.0f);

        // draw sphere
        // -----------
        sphereShader.setMat4("projection", projection);
        sphereShader.setMat4("view", view);
        int i = 0;
        for (auto body : bdies)
        {
            model = drawSphere(body.getPosition(), body.getRadius());
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
            
            sphereShader.setMat4("model", model);
            // sphereShader.setVec3("material.diffuse", body.getColor());
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTextureIds[i]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularTextureIds[i]);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);
            i++;
        }

        // draw path
        // ---------
        vector<vector<glm::dvec3>> paths = bodySystem.getPaths();
        
        pathShader.use();
        pathShader.setMat4("projection", projection);
        pathShader.setMat4("view", view);

        int k = 1;
        for (auto pos : paths)
        {
            int i = 0;
            for (auto body : bdies)
            {
                // pathShader.setVec3("ourColor", k*1.0f/PATH_LENGTH*body.getColor());
                pathShader.setVec3("ourColor", k*1.0f/PATH_LENGTH*glm::vec3(0.5f));

                model = glm::mat4(1.0f);
                model = glm::translate(model, cvtVec3Lp(pos[i]));
                model = glm::scale(model, glm::vec3(0.05f));
                pathShader.setMat4("model", model);
                
                glBindVertexArray(pathVAO);
                glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

                i++;
            }
            k++;
        }

        for (auto point : pointLightPositions)
        {
            pathShader.setVec3("ourColor", glm::vec3(1.0f));

            model = glm::mat4(1.0f);
            model = glm::translate(model, point);
            model = glm::scale(model, glm::vec3(0.5f));
            pathShader.setMat4("model", model);
            
            glBindVertexArray(pathVAO);
            glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
        }
        
        // draw skybox as last
        // -------------------
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &pathVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &pathVBO);
    glDeleteBuffers(1, &skyboxVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

    return 0;
}

vector<float> createSphereVertices()
{
    vector<float> sphereVertices;
    for (unsigned int y = 0; y <= Y_SEGMENTS; y++)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; x++)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
            float yPos = cos(ySegment * PI);
            float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

            // x, y, z
            sphereVertices.push_back(xPos);
            sphereVertices.push_back(yPos);
            sphereVertices.push_back(zPos);

            // u, v
            sphereVertices.push_back(xSegment);
            sphereVertices.push_back(ySegment);
        }
    }
    return sphereVertices;
}

vector<int> createSphereIndices()
{
    vector<int> sphereIndices;
    for (int i = 0; i < Y_SEGMENTS; i++)
    {
        for (int j = 0; j < X_SEGMENTS; j++)
        {
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);

            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
        }
    }
    return sphereIndices;
}

vector<Body> creatBodies(int n)
{
    vector<Body> bodies;
    for (int i = 0; i < n; i++)
    {
        // [-10, 10]
        double px = rand() % 100 / 100.0 * 20 - 10;
        double py = rand() % 100 / 100.0 * 20 - 10;
        double pz = rand() % 100 / 100.0 * 20 - 10;
        
        // [-10, 10]
        double vx = rand() % 100 / 100.0 * 20 - 10;
        double vy = rand() % 100 / 100.0 * 20 - 10;
        double vz = rand() % 100 / 100.0 * 20 - 10;

        float r = vx / 20 + 0.5;
        float g = vy / 20 + 0.5;
        float b = vz / 20 + 0.5;

        Body body(1.0, 1.0, glm::vec3(r, g, b), glm::dvec3(px, py, pz), glm::dvec3(vx, vy, vz));
        bodies.push_back(body);
    }
    return bodies;
}

glm::mat4 drawSphere(glm::dvec3 center_hp, double radius_hp)
{
    glm::vec3 center = cvtVec3Lp(center_hp);
    float radius = (float)radius_hp;

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, center);
    model = glm::scale(model, glm::vec3(radius));

    return model;
}

glm::mat4 cvtMat4Lp(glm::dmat4 mat4Hp)
{
    glm::mat4 mat4Lp;
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            mat4Lp[i][j] = (float)mat4Hp[i][j];
        }
    }

    return mat4Lp;
}

glm::vec3 cvtVec3Lp(glm::dvec3 vec3Hp)
{
    glm::vec3 vec3Lp;

    for (int i = 0; i < 3; i++)
    {
        vec3Lp[i] = (float)vec3Hp[i];
    }

    return vec3Lp;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        tPerFrame *= 1.01;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        tPerFrame /= 1.01;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
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

vector<unsigned int> loadTextures(const int n, const char* rootPath, const char* textureType)
{
    vector<unsigned int> textureIds;
    for (int i = 1; i < n + 1; i++)
    {
        char filePath[64];
        sprintf(filePath, "%s%.2d_%s.png", rootPath, i, textureType);

        unsigned int textureId = loadTexture(filePath);
        textureIds.push_back(textureId);
    }
    return textureIds;
}
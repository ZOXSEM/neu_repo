#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader source with raymarching
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform float u_time;
uniform vec2 u_resolution;

// Simplex noise function (simplified)
float noise(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

// Distance to infinite road
float sdRoad(vec2 p) {
    // Road width
    float roadWidth = 4.0;
    // Lane width
    float laneWidth = 1.5;
    
    // Position along road
    float roadPos = mod(p.y + u_time * 0.5, 10.0);
    
    // Road boundaries
    float distToRoad = abs(p.x) - roadWidth;
    
    // Road markings
    float marking = abs(mod(p.y + u_time * 0.5, 2.0) - 1.0) - 0.1;
    
    // Lane dividers
    float laneDiv = abs(mod(p.x, laneWidth * 2.0) - laneWidth) - 0.1;
    
    // Combine all elements
    float road = max(distToRoad, -marking);
    road = min(road, laneDiv);
    
    return road;
}

// Distance to building
float sdBuilding(vec2 p, float height) {
    // Building base shape (rectangular)
    float w = 3.0;
    float h = height;
    
    float dx = abs(p.x) - w;
    float dy = abs(p.y) - h;
    
    return max(dx, dy);
}

// Distance to city scene
float sdScene(vec2 p) {
    float d = 1e10;
    
    // Infinite road
    d = min(d, sdRoad(p));
    
    // Buildings on both sides of road
    for(int i = -5; i <= 5; i++) {
        if(i != 0) {
            // Position buildings
            vec2 pos = vec2(float(i) * 8.0, 0.0);
            // Random building height
            float height = 3.0 + noise(pos) * 5.0;
            
            // Building distance
            float d_building = sdBuilding(p - pos, height);
            d = min(d, d_building);
        }
    }
    
    return d;
}

// Ray marching function
float rayMarch(vec2 uv) {
    float d = 0.0;
    float totalDist = 0.0;
    
    // Ray marching loop
    for(int i = 0; i < 64; i++) {
        d = sdScene(uv + vec2(0.0, totalDist));
        totalDist += d;
        
        // Early termination if we're close enough
        if(totalDist > 50.0 || d < 0.001) break;
    }
    
    return totalDist;
}

// Color palette function
vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    
    return a + b * cos(6.28318 * (c * t + d));
}

void render() {
    // Simple raymarching rendering
    vec2 uv = (TexCoord * 2.0 - 1.0) * vec2(u_resolution.x / u_resolution.y, 1.0);
    
    // Add some perspective effect
    uv.x *= 1.0 + abs(uv.y) * 0.1;
    
    // Ray march
    float d = rayMarch(uv);
    
    // Color based on distance
    float colorFactor = smoothstep(0.0, 10.0, d);
    vec3 color = palette(colorFactor * 0.5 + 0.2);
    
    // Add some lighting
    float light = 0.5 + 0.5 * sin(u_time * 0.5);
    color *= light;
    
    // Apply fog
    color = mix(color, vec3(0.2, 0.3, 0.4), 1.0 - exp(-d * 0.05));
    
    FragColor = vec4(color, 1.0);
}

void main()
{
    // Get the position of the current pixel
    vec2 uv = TexCoord;
    
    // Convert to normalized device coordinates
    vec2 ndc = uv * 2.0 - 1.0;
    
    // Apply perspective correction
    ndc.x *= u_resolution.x / u_resolution.y;
    
    // Ray march to get distance
    float d = rayMarch(ndc);
    
    // Color based on distance
    float colorFactor = smoothstep(0.0, 10.0, d);
    vec3 color = palette(colorFactor * 0.5 + 0.2);
    
    // Lighting effect
    float light = 0.5 + 0.5 * sin(u_time * 0.5);
    color *= light;
    
    // Fog effect
    color = mix(color, vec3(0.2, 0.3, 0.4), 1.0 - exp(-d * 0.05));
    
    FragColor = vec4(color, 1.0);
}
)";

// Vertex data for a full-screen quad
float vertices[] = {
    // positions          // texture coords
     1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
     1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
    -1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
};

unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Project Infinity", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    });

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Build and compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up vertex data and buffers
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Render loop
    float lastTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Handle input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Update uniforms
        glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), WIDTH, HEIGHT);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_time"), currentTime);

        // Draw quad
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
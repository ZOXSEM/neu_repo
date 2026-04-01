#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec2 iResolution;
uniform float iTime;
uniform vec3 iCameraPosition;

// Hash function for pseudo-random numbers
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Noise function for procedural textures
float noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

// Fractal Brownian Motion for cloud-like patterns
float fbm(vec3 p) {
    float f = 0.0;
    f += 0.5000 * noise(p); p *= 2.0; 
    f += 0.2500 * noise(p); p *= 2.0;
    f += 0.1250 * noise(p); p *= 2.0;
    f += 0.0625 * noise(p);
    return f;
}

// Sky color based on position and time
vec3 getSkyColor(vec3 rayDir) {
    // Base sky color (dark blue at horizon, deeper blue at zenith)
    vec3 skyColor = mix(
        vec3(0.05, 0.05, 0.15),  // Horizon
        vec3(0.0, 0.0, 0.1),     // Zenith
        rayDir.y * 0.5 + 0.5
    );
    
    // Add subtle color variation over time
    float timeFactor = sin(iTime * 0.05) * 0.1 + 0.9;
    skyColor *= timeFactor;
    
    return skyColor;
}

// Moon shader with phase and movement
vec3 getMoon(vec3 rayDir) {
    // Moon position (moving across the sky)
    vec3 moonPos = vec3(
        cos(iTime * 0.1) * 0.8,
        sin(iTime * 0.05) * 0.6,
        sin(iTime * 0.1) * 0.8
    );
    
    // Calculate distance to moon
    float dist = length(rayDir - moonPos);
    
    if (dist < 0.1) {
        // Moon surface with craters
        vec3 moonColor = vec3(0.85, 0.85, 0.9);
        
        // Add crater pattern using FBm
        vec3 moonCoord = normalize(moonPos);
        float craterNoise = fbm(moonCoord * 5.0) * 0.3;
        moonColor -= craterNoise;
        
        // Phase effect (slightly darker side)
        float phaseEffect = 0.8 + sin(iTime * 0.2) * 0.1;
        moonColor *= phaseEffect;
        
        // Add glow around the moon
        float glow = smoothstep(0.08, 0.1, dist);
        moonColor += vec3(0.1) * glow;
        
        return moonColor;
    }
    
    return vec3(0.0);
}

// Star field with twinkling effect
vec3 getStars(vec3 rayDir) {
    vec3 starColor = vec3(0.0);
    
    // Generate star positions in a spherical distribution
    for (int i = 0; i < 500; i++) {
        // Use a hash to generate star positions
        float seed = float(i) * 12.9898 + iTime;
        vec3 starPos = vec3(
            sin(seed * 0.1) * 100.0,
            cos(seed * 0.13) * 100.0,
            sin(seed * 0.17) * 100.0
        );
        
        // Distance to star
        float dist = length(rayDir - starPos);
        
        // Only draw stars that are close enough
        if (dist < 0.05) {
            // Twinkling effect
            float twinkle = sin(iTime * 2.0 + seed * 0.5) * 0.5 + 0.5;
            float intensity = 0.8 + twinkle * 0.2;
            
            // Star color (white with slight blue tint)
            vec3 color = vec3(1.0, 0.95, 0.9) * intensity;
            
            starColor += color;
        }
    }
    
    return starColor;
}

// Main raymarching function
vec3 render(vec3 rayDir) {
    vec3 color = getSkyColor(rayDir);
    
    // Add moon
    vec3 moonColor = getMoon(rayDir);
    color = mix(color, moonColor, moonColor.r > 0.0 ? 1.0 : 0.0);
    
    // Add stars
    vec3 starColor = getStars(rayDir);
    color += starColor * 0.5;
    
    return color;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalize coordinates
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / min(iResolution.y, iResolution.x);
    
    // Camera setup
    vec3 rayDir = normalize(vec3(uv, 1.0));
    
    // Render the scene
    vec3 color = render(rayDir);
    
    // Apply gamma correction
    color = pow(color, vec3(0.4545));
    
    fragColor = vec4(color, 1.0);
}

void main() {
    mainImage(FragColor, TexCoord);
}
)";

class ProceduralSkyRenderer {
private:
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint resolutionLoc, timeLoc, cameraPosLoc;

public:
    ProceduralSkyRenderer() {
        initGL();
        initBuffers();
        initShaders();
    }

    void initGL() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(1280, 720, "Procedural Night Sky", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(-1);
        }

        glfwMakeContextCurrent(window);
        glewInit();

        glEnable(GL_DEPTH_TEST);
    }

    void initBuffers() {
        float vertices[] = {
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO;

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void initShaders() {
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        resolutionLoc = glGetUniformLocation(shaderProgram, "iResolution");
        timeLoc = glGetUniformLocation(shaderProgram, "iTime");
        cameraPosLoc = glGetUniformLocation(shaderProgram, "iCameraPosition");
    }

    void render(float currentTime) {
        glUseProgram(shaderProgram);
        glUniform2f(resolutionLoc, 1280.0f, 720.0f);
        glUniform1f(timeLoc, currentTime);
        glUniform3f(cameraPosLoc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~ProceduralSkyRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);
    }
};

int main() {
    ProceduralSkyRenderer renderer;
    float startTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(glfwGetCurrentContext())) {
        float currentTime = static_cast<float>(glfwGetTime()) - startTime;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.render(currentTime);
        glfwSwapBuffers(glfwGetCurrentContext());
        glfwPollEvents();
    }

    return 0;
}
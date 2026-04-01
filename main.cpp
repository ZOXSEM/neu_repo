#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 transform;
uniform mat4 projection;

void main()
{
    gl_Position = projection * transform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;
uniform float time;

void main()
{
    // Simple procedural road pattern
    float dashPattern = mod(TexCoord.x * 10.0, 2.0) > 1.0 ? 1.0 : 0.0;
    vec3 roadColor = vec3(0.2, 0.2, 0.2);
    vec3 lineColor = vec3(1.0, 1.0, 1.0);
    
    if (dashPattern > 0.5) {
        FragColor = vec4(lineColor, 1.0);
    } else {
        FragColor = vec4(roadColor, 1.0);
    }
}
)";

const char* carVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 transform;
uniform mat4 projection;

void main()
{
    gl_Position = projection * transform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* carFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;
uniform float time;

void main()
{
    // Simple triangle car shape
    vec2 center = vec2(0.5, 0.5);
    vec2 pos = TexCoord;
    
    // Car body
    float carShape = 1.0 - smoothstep(0.4, 0.5, length(pos - center));
    vec3 carColor = vec3(0.8, 0.2, 0.2); // Red car
    
    FragColor = vec4(carColor * carShape, 1.0);
}
)";

const char* uiVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 transform;
uniform mat4 projection;

void main()
{
    gl_Position = projection * transform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* uiFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;

void main()
{
    // Simple text-like rendering
    float textShape = 1.0 - smoothstep(0.9, 1.0, length(TexCoord - vec2(0.5, 0.5)));
    FragColor = vec4(color, textShape);
}
)";

struct Vertex {
    glm::vec2 position;
    glm::vec2 texCoords;
};

struct Car {
    glm::vec2 position;
    glm::vec2 velocity;
    float rotation;
    float speed;
    float maxSpeed;
    float acceleration;
    float friction;
    bool alive;
    
    Car() : position(0.0f, 0.0f), velocity(0.0f, 0.0f), rotation(0.0f),
            speed(0.0f), maxSpeed(0.1f), acceleration(0.005f), friction(0.02f), alive(true) {}
};

class RacingGame {
private:
    GLFWwindow* window;
    GLuint shaderProgram, carShaderProgram, uiShaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint carVAO, carVBO;
    GLuint uiVAO, uiVBO;
    Car playerCar;
    std::vector<glm::vec2> roadPoints;
    glm::vec2 cameraOffset;
    float gameTime;
    
public:
    RacingGame() : window(nullptr), gameTime(0.0f) {
        initializeWindow();
        setupShaders();
        setupBuffers();
        setupRoad();
    }
    
    void initializeWindow() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(1024, 768, "Procedural Racing Game", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        
        glfwMakeContextCurrent(window);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return;
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    void setupShaders() {
        // Road shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        // Car shader
        GLuint carVertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(carVertexShader, 1, &carVertexShaderSource, NULL);
        glCompileShader(carVertexShader);
        
        GLuint carFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(carFragmentShader, 1, &carFragmentShaderSource, NULL);
        glCompileShader(carFragmentShader);
        
        carShaderProgram = glCreateProgram();
        glAttachShader(carShaderProgram, carVertexShader);
        glAttachShader(carShaderProgram, carFragmentShader);
        glLinkProgram(carShaderProgram);
        glDeleteShader(carVertexShader);
        glDeleteShader(carFragmentShader);
        
        // UI shader
        GLuint uiVertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(uiVertexShader, 1, &uiVertexShaderSource, NULL);
        glCompileShader(uiVertexShader);
        
        GLuint uiFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(uiFragmentShader, 1, &uiFragmentShaderSource, NULL);
        glCompileShader(uiFragmentShader);
        
        uiShaderProgram = glCreateProgram();
        glAttachShader(uiShaderProgram, uiVertexShader);
        glAttachShader(uiShaderProgram, uiFragmentShader);
        glLinkProgram(uiShaderProgram);
        glDeleteShader(uiVertexShader);
        glDeleteShader(uiFragmentShader);
    }
    
    void setupBuffers() {
        // Road buffers
        float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f
        };
        
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Car buffers
        float carVertices[] = {
            0.0f, 0.5f, 0.5f, 0.5f,
            -0.3f, -0.5f, 0.0f, 1.0f,
            0.3f, -0.5f, 1.0f, 1.0f
        };
        
        glGenVertexArrays(1, &carVAO);
        glGenBuffers(1, &carVBO);
        
        glBindVertexArray(carVAO);
        glBindBuffer(GL_ARRAY_BUFFER, carVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(carVertices), carVertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // UI buffers
        glGenVertexArrays(1, &uiVAO);
        glGenBuffers(1, &uiVBO);
        
        glBindVertexArray(uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    
    void setupRoad() {
        // Create a simple straight road
        for (int i = 0; i < 100; ++i) {
            roadPoints.push_back(glm::vec2(i * 2.0f, 0.0f));
        }
    }
    
    void handleInput() {
        float deltaTime = 0.016f; // Approximate 60 FPS
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            playerCar.velocity += glm::vec2(cos(playerCar.rotation), sin(playerCar.rotation)) * playerCar.acceleration;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            playerCar.velocity -= glm::vec2(cos(playerCar.rotation), sin(playerCar.rotation)) * playerCar.acceleration;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            playerCar.rotation -= 0.05f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            playerCar.rotation += 0.05f;
        }
        
        // Apply friction
        playerCar.velocity *= (1.0f - playerCar.friction);
        
        // Limit max speed
        float speed = glm::length(playerCar.velocity);
        if (speed > playerCar.maxSpeed) {
            playerCar.velocity = glm::normalize(playerCar.velocity) * playerCar.maxSpeed;
        }
        
        playerCar.position += playerCar.velocity;
    }
    
    void updateCamera() {
        cameraOffset = -playerCar.position;
    }
    
    void renderRoad() {
        glUseProgram(shaderProgram);
        
        glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), cameraOffset);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    void renderCar() {
        glUseProgram(carShaderProgram);
        
        glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(carShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cameraOffset + playerCar.position);
        model = glm::rotate(model, playerCar.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.2f, 0.3f, 1.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(carShaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(model));
        
        glBindVertexArray(carVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    
    void renderUI() {
        glUseProgram(uiShaderProgram);
        
        glm::mat4 projection = glm::ortho(0.0f, 1024.0f, 0.0f, 768.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(uiShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3f(glGetUniformLocation(uiShaderProgram, "color"), 1.0f, 1.0f, 1.0f);
        
        // Render speed text
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, 740.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(uiShaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
        
        glBindVertexArray(uiVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    void render() {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderRoad();
        renderCar();
        renderUI();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    void run() {
        while (!glfwWindowShouldClose(window)) {
            handleInput();
            updateCamera();
            render();
            gameTime += 0.016f;
        }
        
        cleanup();
    }
    
    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteVertexArrays(1, &carVAO);
        glDeleteVertexArrays(1, &uiVAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &carVBO);
        glDeleteBuffers(1, &uiVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(carShaderProgram);
        glDeleteProgram(uiShaderProgram);
        
        glfwTerminate();
    }
};

int main() {
    RacingGame game;
    game.run();
    return 0;
}
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include "prepShader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Vector2 {
    float x, y;
};

struct Vector3 {
    float x, y, z;
};

struct Vertex {
    Vector3 position;
    Vector2 texCoord;
    Vector3 normal;
};

std::vector<Vertex> modelVertices;

GLuint vao, vbo, ebo, texture;
GLuint ground_vao, ground_vbo, ground_ebo;
GLuint texCoords_vbo;
GLuint textureID;

GLint width, height, bitDepth;

glm::mat4 model, view, projection;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;
float lastX = 800.0f / 2.0f, lastY = 600.0f / 2.0f;

glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 spotlightPos(0.0f, 5.0f, 0.0f);
glm::vec3 spotlightDir(0.0f, -1.0f, 0.0f);
float spotlightCutOff = glm::cos(glm::radians(12.5f));
float spotlightOuterCutOff = glm::cos(glm::radians(17.5f));

void init();
void display();
void reshape(int width, int height);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void setupBuffers();
void loadTexture(const char* filename);

static unsigned int
program,
groundProgram,
vertexShaderId,
fragmentShaderId,
groundVertexShaderId,
groundFragmentShaderId;

std::vector<Vertex> LoadOBJ(const char* filename) {
    std::vector<Vertex> vertices;
    std::vector<Vector3> positions;
    std::vector<Vector2> texCoords;
    std::vector<Vector3> normals;
    std::vector<unsigned int> posIndices, texIndices, normalIndices;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open OBJ file: " << filename << std::endl;
        return vertices;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vector3 pos;
            if (!(iss >> pos.x >> pos.y >> pos.z)) {
                std::cerr << "Error: Malformed vertex position in OBJ file." << std::endl;
                continue;
            }
            positions.push_back(pos);
        }
        else if (type == "vt") {
            Vector2 tex;
            if (!(iss >> tex.x >> tex.y)) {
                std::cerr << "Error: Malformed texture coordinate in OBJ file." << std::endl;
                continue;
            }
            texCoords.push_back(tex);
        }
        else if (type == "vn") {
            Vector3 normal;
            if (!(iss >> normal.x >> normal.y >> normal.z)) {
                std::cerr << "Error: Malformed normal vector in OBJ file." << std::endl;
                continue;
            }
            normals.push_back(normal);
        }
        else if (type == "f") {
            unsigned int p, t, n;
            for (int i = 0; i < 3; i++) {
                char slash1, slash2;
                if (!(iss >> p >> slash1 >> t >> slash2 >> n) || slash1 != '/' || slash2 != '/') {
                    std::cerr << "Error: Malformed face definition in OBJ file." << std::endl;
                    continue;
                }

                // Ensure indices are within bounds
                if (p - 1 >= positions.size() || t - 1 >= texCoords.size() || n - 1 >= normals.size()) {
                    std::cerr << "Error: Face indices out of range in OBJ file." << std::endl;
                    continue;
                }

                posIndices.push_back(p - 1);
                texIndices.push_back(t - 1);
                normalIndices.push_back(n - 1);
            }
        }
    }

    for (unsigned int i = 0; i < posIndices.size(); i++) {
        Vertex vertex;

        // Perform bounds checking before accessing the vectors
        if (posIndices[i] >= positions.size() || texIndices[i] >= texCoords.size() || normalIndices[i] >= normals.size()) {
            std::cerr << "Error: Skipping invalid indices during vertex assembly." << std::endl;
            continue;
        }

        vertex.position = positions[posIndices[i]];
        vertex.texCoord = texCoords[texIndices[i]];
        vertex.normal = normals[normalIndices[i]];

        vertices.push_back(vertex);
    }

    std::cout << "Loaded OBJ file: " << filename << std::endl;
    std::cout << "Vertices loaded: " << vertices.size() << std::endl;

    return vertices;
}

int main(int argc, char** argv) {
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Assignment 5");

    glewInit();
    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();

    return 0;
}

void init() {
    vertexShaderId = setShader("vertex", "vertex_shader.glsl");
    fragmentShaderId = setShader("fragment", "fragment_shader.glsl");
    groundVertexShaderId = setShader("vertex", "ground_vertex_shader.glsl");
    groundFragmentShaderId = setShader("fragment", "ground_fragment_shader.glsl");

    program = glCreateProgram();
    glAttachShader(program, vertexShaderId);
    glAttachShader(program, fragmentShaderId);
    glLinkProgram(program);

    groundProgram = glCreateProgram();
    glAttachShader(groundProgram, groundVertexShaderId);
    glAttachShader(groundProgram, groundFragmentShaderId);
    glLinkProgram(groundProgram);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Error: Shader program linking failed.\n" << infoLog << std::endl;
        exit(EXIT_FAILURE);
    }

    glUseProgram(program);

    GLuint groundProgram = glCreateProgram();
    glAttachShader(groundProgram, groundVertexShaderId);
    glAttachShader(groundProgram, groundFragmentShaderId);
    glLinkProgram(groundProgram);

    glUseProgram(groundProgram);

    glEnable(GL_DEPTH_TEST);

    modelVertices = LoadOBJ("C:/Users/theda/Documentos/Repos/OpenGL/Assignment 5/model.obj");
    if (modelVertices.empty()) {
        std::cerr << "Error: Model could not be loaded. Check your OBJ file." << std::endl;
        exit(EXIT_FAILURE);
    }

    setupBuffers();
    loadTexture("C:/Users/theda/Documentos/Repos/OpenGL/Assignment 5/texture.jpg");
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float radius = 5.0f;
    glm::vec3 pointLightPos(
        radius * cos(time),
        1.0f,
        radius * sin(time)
    );

    glUseProgram(program);

    // Set directional light
    glm::vec3 lightDir = glm::normalize(glm::vec3(-10.0f, -10.0f, -10.0f));
    glUniform3fv(glGetUniformLocation(program, "lightDir"), 1, glm::value_ptr(lightDir));

    // Set spotlight
    glUniform3fv(glGetUniformLocation(program, "spotlightPos"), 1, glm::value_ptr(spotlightPos));
    glUniform3fv(glGetUniformLocation(program, "spotlightDir"), 1, glm::value_ptr(spotlightDir));
    glUniform1f(glGetUniformLocation(program, "spotlightCutOff"), spotlightCutOff);
    glUniform1f(glGetUniformLocation(program, "spotlightOuterCutOff"), spotlightOuterCutOff);

    // Set point light
    glUniform3fv(glGetUniformLocation(program, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
    glUniform1f(glGetUniformLocation(program, "constantAttenuation"), 1.0f);
    glUniform1f(glGetUniformLocation(program, "linearAttenuation"), 0.09f);
    glUniform1f(glGetUniformLocation(program, "quadraticAttenuation"), 0.032f);

    // Set camera position
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos));

    // Set light color
    glUniform3f(glGetUniformLocation(program, "lightColor"), 1.0f, 1.0f, 1.0f);

    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));

    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, modelVertices.size());
    glBindVertexArray(0);



    glUseProgram(groundProgram);

// Set directional light
    glUniform3fv(glGetUniformLocation(groundProgram, "lightDir"), 1, glm::value_ptr(lightDir));

    // Set spotlight
    glUniform3fv(glGetUniformLocation(groundProgram, "spotlightPos"), 1, glm::value_ptr(spotlightPos));
    glUniform3fv(glGetUniformLocation(groundProgram, "spotlightDir"), 1, glm::value_ptr(spotlightDir));
    glUniform1f(glGetUniformLocation(groundProgram, "spotlightCutOff"), spotlightCutOff);
    glUniform1f(glGetUniformLocation(groundProgram, "spotlightOuterCutOff"), spotlightOuterCutOff);

    // Set point light
    glUniform3fv(glGetUniformLocation(groundProgram, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
    glUniform1f(glGetUniformLocation(groundProgram, "constantAttenuation"), 1.0f);
    glUniform1f(glGetUniformLocation(groundProgram, "linearAttenuation"), 0.09f);
    glUniform1f(glGetUniformLocation(groundProgram, "quadraticAttenuation"), 0.032f);

    // Set camera position
    glUniform3fv(glGetUniformLocation(groundProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

    // Set light color
    glUniform3f(glGetUniformLocation(groundProgram, "lightColor"), 1.0f, 1.0f, 1.0f);

    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(groundProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(groundProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(groundProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(groundProgram, "groundColor"), 0.5f, 0.5f, 0.5f);

    glBindVertexArray(ground_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(60.0f), (float)width / height, 0.1f, 100.0f);
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    float cameraSpeed = 0.05f;
    switch (key) {
    case 'w':
        cameraPos += cameraSpeed * cameraFront;
        break;
    case 's':
        cameraPos -= cameraSpeed * cameraFront;
        break;
    case 'a':
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        break;
    case 'd':
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        lastX = x;
        lastY = y;
    }
}

void motion(int x, int y) {
    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float xoffset = x - lastX;
    float yoffset = lastY - y;
    lastX = x;
    lastY = y;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glutPostRedisplay();
}


void setupBuffers() {
    if (modelVertices.empty()) {
        std::cerr << "Error: No vertices available for buffer setup." << std::endl;
        return;
    }
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, modelVertices.size() * sizeof(Vertex), &modelVertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    float groundVertices[] = {
     
     -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,
      10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,
      10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,
     -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f 
    };

    unsigned int groundIndices[] = {
        0, 1, 2,
        2, 3, 0
    };


    glGenVertexArrays(1, &ground_vao);
    glGenBuffers(1, &ground_vbo);
    glGenBuffers(1, &ground_ebo);

    glBindVertexArray(ground_vao);

    glBindBuffer(GL_ARRAY_BUFFER, ground_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ground_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void loadTexture(const char* filename) {
    int texWidth, texHeight, texChannels;
    unsigned char* data = stbi_load(filename, &texWidth, &texHeight, &texChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << "\nReason: " << stbi_failure_reason() << std::endl;
        return;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

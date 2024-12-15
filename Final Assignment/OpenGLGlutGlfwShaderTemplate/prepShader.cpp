#include "prepShader.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <GL/glew.h>

char* readShader(std::string fileName)
{
    std::ifstream inFile(fileName.c_str(), std::ios::binary);
    inFile.seekg(0, std::ios::end);
    int fileLength = inFile.tellg();
    char* fileContent = (char*)malloc((fileLength + 1) * sizeof(char));
    inFile.seekg(0, std::ios::beg);
    inFile.read(fileContent, fileLength);
    fileContent[fileLength] = '\0';
    inFile.close();
    return fileContent;
}

GLuint setShader(const char* shaderType, const char* shaderFile) {
    GLuint shaderID;
    char* shaderSource = readShader(shaderFile);

    if (!shaderSource) {
        std::cout << "Error reading shader file: " << shaderFile << std::endl;
        return 0;
    }

    if (strcmp(shaderType, "vertex") == 0) {
        shaderID = glCreateShader(GL_VERTEX_SHADER);
    }
    else if (strcmp(shaderType, "fragment") == 0) {
        shaderID = glCreateShader(GL_FRAGMENT_SHADER);
    }
    else {
        std::cout << "Invalid shader type specified: " << shaderType << std::endl;
        free(shaderSource);
        return 0;
    }

    glShaderSource(shaderID, 1, &shaderSource, NULL);
    glCompileShader(shaderID);

    GLint success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
        std::cerr << "Shader compilation error in " << shaderFile << ":\n" << infoLog << std::endl;
        free(shaderSource);
        return 0;
    }

    free(shaderSource);
    return shaderID;
}
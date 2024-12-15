#ifndef PREP_SHADER_H
#define PREP_SHADER_H

#include <string>
#include <GL/glew.h>

char* readShader(std::string fileName);
GLuint setShader(const char* shaderType, const char* shaderFile);

#endif
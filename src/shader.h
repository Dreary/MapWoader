#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID;
    Shader(const char *vertexPath, const char *fragmentPath); // <-- just the declaration

    void use();
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setInt(const std::string &name, int value) const;

private:
    void checkCompileErrors(GLuint shader, const std::string &type); // make this private too
};

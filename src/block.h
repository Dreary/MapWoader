#pragma once
#include <string>
#include <glm/glm.hpp>

struct Block
{
    std::string id;
    std::string modelName;
    glm::vec3 position;
    glm::vec3 rotation;
};

Block LoadFirstBlockFromXBlock(const std::string &filepath); // ← ADD THIS

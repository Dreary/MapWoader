
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

class Mesh
{
public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texcoord;
        glm::vec3 normal;
        glm::u8vec4 color = glm::u8vec4(255); // Default white if not set
        glm::vec3 tangent = glm::vec3(0.0f);
        glm::vec3 binormal = glm::vec3(0.0f);
        glm::vec3 morphpos = glm::vec3(0.0f);
        glm::u8vec4 blendIndices = glm::u8vec4(0);
        glm::vec4 blendWeight = glm::vec4(0.0f);
    };

    unsigned int VAO, VBO, EBO;
    size_t indexCount;

    Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices)
    {
        indexCount = indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texcoord));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *)offsetof(Vertex, color));
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(4);

        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, binormal));
        glEnableVertexAttribArray(5);

        glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, morphpos));
        glEnableVertexAttribArray(6);

        glVertexAttribPointer(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, blendIndices));
        glEnableVertexAttribArray(7);

        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, blendWeight));
        glEnableVertexAttribArray(8);

        glBindVertexArray(0);
    }

    void Draw()
    {
        if (indexCount == 0 || indexCount > 10000)
        {
            std::cerr << "[ERROR] Invalid indexCount: " << indexCount << std::endl;
            return;
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~Mesh()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

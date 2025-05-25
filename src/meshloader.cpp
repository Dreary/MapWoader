#include "MeshLoader.h"
#include "mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <VulkanGraphics/Scene/MeshData.h>
#include <Engine/Math/Vector2S.h>
#include <Engine/Math/Vector3S.h>

#include <iostream>

Mesh *MeshLoader::LoadFromNode(const Engine::Graphics::ModelPackageNode &node)
{
    const auto mesh = node.Mesh;
    const auto format = mesh ? mesh->GetFormat() : nullptr;
    if (!mesh || !format)
    {
        std::cerr << "[MeshLoader] No mesh or format.\n";
        return nullptr;
    }

    const auto *posAttr = format->GetAttribute("position");
    const auto *texAttr = format->GetAttribute("texcoord");

    if (!posAttr || !texAttr)
    {
        std::cerr << "[MeshLoader] Required attribute(s) missing: "
                  << (!posAttr ? "'position' " : "")
                  << (!texAttr ? "'texcoord'" : "") << "\n";
        return nullptr;
    }

    size_t posIndex = format->GetAttributeIndex("position");
    size_t texIndex = format->GetAttributeIndex("texcoord");

    std::vector<Vector3SF> posData;
    mesh->ForEach<Vector3SF>(posIndex, [&](const Vector3SF &p)
                             { posData.emplace_back(p); });
    std::vector<Vector2SF> texData;
    mesh->ForEach<Vector2SF>(texIndex, [&](const Vector2SF &t)
                             { texData.emplace_back(t); });

    std::vector<Vector3SF> normData;
    if (format->GetAttribute("normal"))
    {
        size_t normIndex = format->GetAttributeIndex("normal");
        mesh->ForEach<Vector3SF>(normIndex, [&](const Vector3SF &n)
                                 { normData.emplace_back(n); });
    }

    if (posData.size() != texData.size())
    {
        std::cerr << "[MeshLoader] Mismatch between positions and texcoords.\n";
        return nullptr;
    }

    std::vector<Mesh::Vertex> vertices;

    glm::mat4 fixOrientation =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0));

    glm::mat3 normalFix = glm::mat3(fixOrientation); // Extract 3x3 normal matrix

    for (size_t i = 0; i < posData.size(); ++i)
    {
        glm::vec4 pos(posData[i].X, posData[i].Y, posData[i].Z, 1.0f);
        pos = fixOrientation * pos;

        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
        if (i < normData.size())
        {
            Vector3SF rawNorm = normData[i];
            normal = normalFix * glm::vec3(rawNorm.X, rawNorm.Y, rawNorm.Z);
        }

        Mesh::Vertex v;
        v.position = glm::vec3(pos);
        v.texcoord = glm::vec2(texData[i].X, 1.0f - texData[i].Y); // Flip Y ✅
        v.normal = normal;
        v.color = glm::u8vec4(255); // white fallback

        vertices.push_back(v);
    }

    std::vector<unsigned int> indices;
    for (int i : mesh->GetIndexBuffer())
        indices.push_back(static_cast<unsigned int>(i));

    std::cout << "[MeshLoader] Loaded mesh: " << vertices.size() << " vertices, "
              << indices.size() << " indices\n";

    return new Mesh(vertices, indices);
}

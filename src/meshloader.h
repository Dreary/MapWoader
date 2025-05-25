#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "mesh.h"
#include <VulkanGraphics/Scene/MeshData.h>
#include <VulkanGraphics/FileFormats/PackageNodes.h> // ✅ include this

struct VertexPosBinding
{
    glm::vec3 Position;
};

class MeshLoader
{
public:
    static Mesh *LoadFromNode(const Engine::Graphics::ModelPackageNode &node); // ✅ confirmed correct
};

#include "block.h"
#include "tinyxml2.h"
#include <iostream>

Block LoadFirstBlockFromXBlock(const std::string &filepath)
{
    Block block;

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filepath.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "Failed to load .xblock: " << filepath << std::endl;
        return block;
    }

    tinyxml2::XMLElement *root = doc.FirstChildElement("game");
    if (!root)
    {
        std::cerr << "[block.cpp] Could not find <game> root element!\n";
        return block;
    }

    tinyxml2::XMLElement *entitySet = root->FirstChildElement("entitySet");
    if (!entitySet)
    {
        std::cerr << "[block.cpp] Could not find <entitySet>\n";
        return block;
    }

    tinyxml2::XMLElement *entity = entitySet->FirstChildElement("entity");
    if (!entity)
    {
        std::cerr << "[block.cpp] Could not find <entity>\n";
        return block;
    }

    const char *modelAttr = entity->Attribute("modelName");
    if (modelAttr)
    {
        block.modelName = modelAttr;
        std::cout << "[block.cpp] Found modelName: " << block.modelName << std::endl;
    }

    // Parse Position
    for (tinyxml2::XMLElement *prop = entity->FirstChildElement("property"); prop; prop = prop->NextSiblingElement("property"))
    {
        const char *name = prop->Attribute("name");
        if (name && std::string(name) == "Position")
        {
            const char *value = prop->FirstChildElement("set")->Attribute("value");
            float x, y, z;
            if (sscanf(value, "%f, %f, %f", &x, &y, &z) == 3)
            {
                block.position = glm::vec3(x / 150.0f, y / 150.0f, z / 150.0f); // scale down
                std::cout << "[block.cpp] Position: " << x << ", " << y << ", " << z << std::endl;
            }
        }
        else if (name && std::string(name) == "Rotation")
        {
            const char *value = prop->FirstChildElement("set")->Attribute("value");
            float x, y, z;
            if (sscanf(value, "%f, %f, %f", &x, &y, &z) == 3)
            {
                block.rotation = glm::vec3(x, y, z);
                std::cout << "[block.cpp] Rotation: " << x << ", " << y << ", " << z << std::endl;
            }
        }
    }

    return block;
}

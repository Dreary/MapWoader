#include "texture_manager.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

TextureManager::TextureManager(const std::string &textureRoot)
{
    ScanDirectory(textureRoot);
}

void TextureManager::ScanDirectory(const std::string &directory)
{
    for (const auto &entry : fs::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            textureMap[filename] = entry.path().string();
        }
    }
}

std::string TextureManager::GetTexturePathByModelName(const std::string &modelName)
{
    std::string name = modelName + ".png"; // For now we're using .png versions of the .dds files
    auto it = textureMap.find(name);
    if (it != textureMap.end())
    {
        return it->second;
    }
    else
    {
        std::cerr << "[TextureManager] Texture not found for model: " << modelName << std::endl;
        return ""; // fallback
    }
}

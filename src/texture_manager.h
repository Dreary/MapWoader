#pragma once
#include <string>
#include <unordered_map>

class TextureManager
{
public:
    TextureManager(const std::string &textureRoot);
    std::string GetTexturePathByModelName(const std::string &modelName);

private:
    std::unordered_map<std::string, std::string> textureMap;
    void ScanDirectory(const std::string &directory);
};

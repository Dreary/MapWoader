#include "textureloader.h"
#include <fstream>
#include <iostream>
#include <glad/glad.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

GLuint LoadDDSTexture(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "[ERROR] Could not open DDS file: " << path << "\n";
        return 0;
    }

    char fileCode[4];
    file.read(fileCode, 4);
    if (strncmp(fileCode, "DDS ", 4) != 0)
    {
        std::cerr << "[ERROR] Not a valid DDS file: " << path << "\n";
        return 0;
    }

    unsigned char header[124];
    file.read(reinterpret_cast<char *>(header), 124);

    unsigned int height = *(unsigned int *)&(header[8]);
    unsigned int width = *(unsigned int *)&(header[12]);
    unsigned int linearSize = *(unsigned int *)&(header[16]);
    unsigned int mipMapCount = *(unsigned int *)&(header[24]);
    unsigned int fourCC = *(unsigned int *)&(header[80]);

    unsigned char *buffer = new unsigned char[linearSize * 2];
    file.read(reinterpret_cast<char *>(buffer), linearSize * 2);
    file.close();

    unsigned int format;
    switch (fourCC)
    {
    case '1TXD':
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;
    case '3TXD':
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
    case '5TXD':
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    default:
        std::cerr << "[ERROR] Unsupported DDS format: " << fourCC << "\n";
        delete[] buffer;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
    {
        unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, size, buffer + offset);

        offset += size;
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] buffer;

    return texID;
}

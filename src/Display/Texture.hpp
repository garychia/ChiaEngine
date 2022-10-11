#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Data/String.hpp"

struct Texture
{
    String imagePath;
    size_t identifier;
    bool loaded;

    Texture(const String &imagePath);
};

#endif

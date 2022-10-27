#include "Display/Shader.hpp"

Shader::Shader(const String &path) : path(path)
{
}

void Shader::SetPath(const String &path)
{
    this->path = path;
}

const String &Shader::GetPath() const
{
    return path;
}

bool Shader::IsLoaded() const
{
    return loaded;
}

size_t Shader::GetID() const
{
    return identifier;
}

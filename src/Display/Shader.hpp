#ifndef SHADER_HPP
#define SHADER_HPP

#include "Data/String.hpp"

class Shader
{
  private:
    bool loaded;
    size_t identifier;

  protected:
    String path;

  public:
    Shader(const String &path);

    void SetPath(const String &path);

    const String &GetPath() const;

    bool IsLoaded() const;

    size_t GetID() const;

#ifdef DIRECTX_ENABLED
    friend class DirectXRenderer;
#endif
};

#endif
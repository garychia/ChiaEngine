#ifndef IRENDERABLE_HPP
#define IRENDERABLE_HPP

#include "Color.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Texture.hpp"

class Shader;

using VertexIndexType = unsigned short;
using ColorIndexType = unsigned short;

struct RenderInfo
{
    // Vertex
    const Point3D *vertexBuffer;
    unsigned int numOfVertices;
    // Vertex Index
    const VertexIndexType *vertexIndexBuffer;
    unsigned int numOfVertexIndices;
    // Color
    const Color *colorBuffer;
    unsigned int numOfColors;
    const ColorIndexType *colorIndexBuffer;
    unsigned int numOfColorIndices;
    // Texture
    Texture *pTexture;
    const Point2D *textureCoordinates;
    unsigned int numOfTextureCoordinates;
    // Shader
    Shader *pVertexShader;
    Shader *pPixelShader;

    RenderInfo();
};

struct Transformation
{
    // right-handed cooridnate system
    Point3D position;
    Point3D rotation;
    Point3D scale;

    Transformation(const Point3D &position = Point3D(), const Point3D &rotaion = Point3D(),
                   const Point3D &scale = Point3D(1, 1, 1));
};

class IRenderable
{
  private:
    bool loaded;
    size_t identifier;

  protected:
    RenderInfo renderInfo;
    Transformation transformation;

  public:
    IRenderable(const Point3D &position = Point3D(), const Point3D &rotation = Point3D(),
                const Point3D &scale = Point3D(1, 1, 1));

    IRenderable(const Transformation &transformation);

    virtual RenderInfo GetRenderInfo() const;

    virtual Transformation &GetTransformation();

    virtual void Translate(float deltaX, float deltaY = 0.f, float deltaZ = 0.f);

    virtual void Translate(const Point3D &delta);

    virtual void Scale(float deltaX, float deltaY = 0.f, float deltaZ = 0.f);

    virtual void Scale(const Point3D &delta);

    virtual void Rotate(float deltaRow, float deltaPitch = 0.f, float deltaYaw = 0.f);

    virtual void Rotate(const Point3D &delta);

    virtual const Transformation &GetTransformation() const;

    virtual void SetTexture(Texture *pTexture);

#ifdef DIRECTX_ENABLED
    friend class DirectXRenderer;
#endif
};

#endif

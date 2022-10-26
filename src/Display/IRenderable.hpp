#ifndef IRENDERABLE_HPP
#define IRENDERABLE_HPP

#include "Color.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "System/Operation/Event.hpp"
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
    Color color;

  public:
    IRenderable(const Point3D &position = Point3D(), const Point3D &rotation = Point3D(),
                const Point3D &scale = Point3D(1, 1, 1), const Color &color = Color(1.f, 1.f, 1.f));

    IRenderable(const Transformation &transformation, const Color &color = Color(1.f, 1.f, 1.f));

    virtual void MarkLoaded(size_t id);

    virtual bool IsLoaded() const;

    virtual size_t GetIdentifier() const;

    virtual RenderInfo GetRenderInfo() const;

    virtual void SetPosition(float x, float y, float z);

    virtual void SetRotation(float row, float pitch, float yaw);

    virtual void SetScale(float x, float y, float z);

    virtual const Point3D &GetPosition() const;

    virtual const Point3D &GetRotation() const;

    virtual const Point3D &GetScale() const;

    virtual void Translate(float deltaX, float deltaY = 0.f, float deltaZ = 0.f);

    virtual void Translate(const Point3D &delta);

    virtual void Scale(float x, float y = 1.f, float z = 1.f);

    virtual void Scale(const Point3D &newScale);

    virtual void Zoom(float deltaX, float deltaY = 0.f, float deltaZ = 0.f);

    virtual void Zoom(const Point3D &delta);

    virtual void Rotate(float deltaRow, float deltaPitch = 0.f, float deltaYaw = 0.f);

    virtual void Rotate(const Point3D &delta);

    virtual void SetTexture(Texture *pTexture);

    virtual void SetColor(const Color &color);
};

#endif

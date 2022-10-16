#include "IRenderable.hpp"

RenderInfo::RenderInfo()
    : vertexBuffer(nullptr), numOfVertices(0), vertexIndexBuffer(nullptr), numOfVertexIndices(0), colorBuffer(nullptr),
      numOfColors(0), colorIndexBuffer(nullptr), numOfColorIndices(0), pTexture(nullptr), textureCoordinates(nullptr),
      numOfTextureCoordinates(0), pVertexShader(nullptr), pPixelShader(nullptr)
{
}

Transformation::Transformation(const Point3D &position, const Point3D &rotaion, const Point3D &scale)
    : position(position), rotation(rotaion), scale(scale)
{
}

IRenderable::IRenderable(const Point3D &position, const Point3D &rotation, const Point3D &scale)
    : loaded(false), identifier(0), renderInfo(), transformation(position, rotation, scale)
{
}

IRenderable::IRenderable(const Transformation &transformation)
    : IRenderable(transformation.position, transformation.rotation, transformation.scale)
{
}

RenderInfo IRenderable::GetRenderInfo() const
{
    return renderInfo;
}

Transformation &IRenderable::GetTransformation()
{
    return transformation;
}

void IRenderable::Translate(float deltaX, float deltaY, float deltaZ)
{
    transformation.position += Point3D(deltaX, deltaY, deltaZ);
}

void IRenderable::Translate(const Point3D &delta)
{
    transformation.position += delta;
}

void IRenderable::Scale(float x, float y, float z)
{
    transformation.scale = Point3D(x, y, z);
}

void IRenderable::Scale(const Point3D &newScale)
{
    transformation.scale = newScale;
}

void IRenderable::Zoom(float deltaX, float deltaY, float deltaZ)
{
    transformation.scale += Point3D(deltaX, deltaY, deltaZ);
}

void IRenderable::Zoom(const Point3D &delta)
{
    transformation.scale += delta;
}

void IRenderable::Rotate(float deltaRow, float deltaPitch, float deltaYaw)
{
    transformation.rotation += Point3D(deltaRow, deltaPitch, deltaYaw);
}

void IRenderable::Rotate(const Point3D &delta)
{
    transformation.rotation += delta;
}

const Transformation &IRenderable::GetTransformation() const
{
    return transformation;
}

void IRenderable::SetTexture(Texture *pTexture)
{
    renderInfo.pTexture = pTexture;
}

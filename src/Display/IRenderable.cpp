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

IRenderable::IRenderable(const Point3D &position, const Point3D &rotation, const Point3D &scale, const Color &color)
    : loaded(false), identifier(0), renderInfo(), transformation(position, rotation, scale), color(color)
{
}

IRenderable::IRenderable(const Transformation &transformation, const Color &color)
    : IRenderable(transformation.position, transformation.rotation, transformation.scale, color)
{
}

void IRenderable::MarkLoaded(size_t id)
{
    loaded = true;
    identifier = id;
}

bool IRenderable::IsLoaded() const
{
    return loaded;
}

size_t IRenderable::GetIdentifier() const
{
    return identifier;
}

RenderInfo IRenderable::GetRenderInfo() const
{
    return renderInfo;
}

void IRenderable::SetPosition(float x, float y, float z)
{
    transformation.position.x = x;
    transformation.position.y = y;
    transformation.position.z = z;
}

void IRenderable::SetRotation(float row, float pitch, float yaw)
{
    transformation.rotation.x = row;
    transformation.rotation.y = pitch;
    transformation.rotation.z = yaw;
}

void IRenderable::SetScale(float x, float y, float z)
{
    transformation.scale.x = x;
    transformation.scale.y = y;
    transformation.scale.z = z;
}

const Point3D &IRenderable::GetPosition() const
{
    return transformation.position;
}

const Point3D &IRenderable::GetRotation() const
{
    return transformation.rotation;
}

const Point3D &IRenderable::GetScale() const
{
    return transformation.scale;
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

void IRenderable::SetTexture(Texture *pTexture)
{
    renderInfo.pTexture = pTexture;
}

void IRenderable::SetColor(const Color &color)
{
    this->color = color;
}

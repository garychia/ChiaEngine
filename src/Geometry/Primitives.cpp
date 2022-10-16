#include "Primitives.hpp"

const unsigned int Triangle::NumberOfVertexIndices = 3;
const unsigned int Triangle::NumberOfVertices = 3;
const Point3D Triangle::vertexBuffer[] = {{0.f, 0.5f, 0.f}, {-0.5f, -0.5f, 0.f}, {0.5f, -0.5f, 0.f}};
const VertexIndexType Triangle::vertexIndexBuffer[] = {0, 1, 2};
const Point2D Triangle::textureCoordinates[] = {{0.5f, 0.5f}, {0.f, 0.f}, {1.f, 0.f}};

Triangle::Triangle(const Point3D &position, const Point3D &rotation, const Point3D &scale, const Color &color)
    : IRenderable(position, rotation, scale), color(color)
{
    this->renderInfo.vertexBuffer = vertexBuffer;
    this->renderInfo.numOfVertices = NumberOfVertices;
    this->renderInfo.vertexIndexBuffer = vertexIndexBuffer;
    this->renderInfo.numOfVertexIndices = NumberOfVertexIndices;
    this->renderInfo.colorBuffer = &this->color;
    this->renderInfo.numOfColors = 1;
    this->renderInfo.colorIndexBuffer = new ColorIndexType[NumberOfVertexIndices]{};
    this->renderInfo.numOfColorIndices = NumberOfVertexIndices;
    this->renderInfo.textureCoordinates = textureCoordinates;
    this->renderInfo.numOfTextureCoordinates = sizeof(textureCoordinates) / sizeof(Point2D);
}

Triangle::Triangle(const Transformation &transformation, const Color &color) : IRenderable(transformation), color(color)
{
}

Triangle::Triangle(const Triangle &other) : Triangle(other.transformation, other.color)
{
    this->renderInfo.pVertexShader = other.renderInfo.pVertexShader;
    this->renderInfo.pPixelShader = other.renderInfo.pPixelShader;
    this->renderInfo.pTexture = other.renderInfo.pTexture;
}

Triangle::~Triangle()
{
    if (this->renderInfo.colorIndexBuffer)
        delete[] renderInfo.colorIndexBuffer;
}

const unsigned int Rectangle::NumberOfVertexIndices = 6;
const unsigned int Rectangle::NumberOfVertices = 4;
const Point3D Rectangle::vertexBuffer[] = {{0.5, 0.5, 0.f}, {-0.5, 0.5, 0.f}, {-0.5, -0.5, 0.f}, {0.5, -0.5, 0.f}};
const VertexIndexType Rectangle::vertexIndexBuffer[] = {0, 1, 2, 2, 3, 0};
const Point2D Rectangle::textureCoordinates[] = {{1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}, {1.f, 0.f}};

Rectangle::Rectangle(const Point3D &position, const Point3D &rotation, const Point3D &scale, const Color &color)
    : IRenderable(position, rotation, scale), color(color)
{
    this->renderInfo.vertexBuffer = vertexBuffer;
    this->renderInfo.numOfVertices = NumberOfVertices;
    this->renderInfo.vertexIndexBuffer = vertexIndexBuffer;
    this->renderInfo.numOfVertexIndices = NumberOfVertexIndices;
    this->renderInfo.colorBuffer = &this->color;
    this->renderInfo.numOfColors = 1;
    this->renderInfo.colorIndexBuffer = new ColorIndexType[NumberOfVertexIndices]{};
    this->renderInfo.numOfColorIndices = NumberOfVertexIndices;
    this->renderInfo.textureCoordinates = textureCoordinates;
    this->renderInfo.numOfTextureCoordinates = sizeof(textureCoordinates) / sizeof(Point2D);
}

Rectangle::Rectangle(const Transformation &transformation, const Color &color)
    : IRenderable(transformation), color(color)
{
}

Rectangle::Rectangle(const Rectangle &other) : Rectangle(other.transformation, other.color)
{
    this->renderInfo.pVertexShader = other.renderInfo.pVertexShader;
    this->renderInfo.pPixelShader = other.renderInfo.pPixelShader;
    this->renderInfo.pTexture = other.renderInfo.pTexture;
}

Rectangle::~Rectangle()
{
    if (this->renderInfo.colorIndexBuffer)
        delete[] renderInfo.colorIndexBuffer;
}

const Point3D Cube::vertexBuffer[] = {
    // Front
    {0.5, 0.5, 0.5},
    {-0.5, 0.5, 0.5},
    {-0.5, -0.5, 0.5},
    {0.5, -0.5, 0.5},
    // Back
    {-0.5, 0.5, -0.5},
    {0.5, 0.5, -0.5},
    {0.5, -0.5, -0.5},
    {-0.5, -0.5, -0.5},
    // Top
    {0.5, 0.5, -0.5},
    {-0.5, 0.5, -0.5},
    {-0.5, 0.5, 0.5},
    {0.5, 0.5, 0.5},
    // Bottom
    {0.5, -0.5, 0.5},
    {-0.5, -0.5, 0.5},
    {-0.5, -0.5, -0.5},
    {0.5, -0.5, -0.5},
    // Left
    {-0.5, 0.5, 0.5},
    {-0.5, 0.5, -0.5},
    {-0.5, -0.5, -0.5},
    {-0.5, -0.5, 0.5},
    // Right
    {0.5, 0.5, -0.5},
    {0.5, 0.5, 0.5},
    {0.5, -0.5, 0.5},
    {0.5, -0.5, -0.5}};
const VertexIndexType Cube::vertexIndexBuffer[] = {
    // Front
    0, 1, 2, 2, 3, 0,
    // Back
    4, 5, 6, 6, 7, 4,
    // Top
    8, 9, 10, 10, 11, 8,
    // Bottom
    12, 13, 14, 14, 15, 12,
    // Left
    16, 17, 18, 18, 19, 16,
    // Right
    20, 21, 22, 22, 23, 20};
const Point2D Cube::textureCoordinates[] = {{1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f},
                                            {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}, {1.f, 0.f},
                                            {1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f},
                                            {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}, {1.f, 0.f}};

Cube::Cube(const Point3D &position, const Point3D &rotation, const Point3D &scale, const Color &color)
    : IRenderable(position, rotation, scale), color(color)
{
    this->renderInfo.vertexBuffer = vertexBuffer;
    this->renderInfo.numOfVertices = sizeof(vertexBuffer) / sizeof(Point3D);
    this->renderInfo.vertexIndexBuffer = vertexIndexBuffer;
    this->renderInfo.numOfVertexIndices = sizeof(vertexIndexBuffer) / sizeof(VertexIndexType);
    this->renderInfo.colorBuffer = &this->color;
    this->renderInfo.numOfColors = 1;
    this->renderInfo.colorIndexBuffer = new ColorIndexType[sizeof(vertexIndexBuffer) / sizeof(VertexIndexType)]{};
    this->renderInfo.numOfColorIndices = sizeof(vertexIndexBuffer) / sizeof(VertexIndexType);
    this->renderInfo.textureCoordinates = textureCoordinates;
    this->renderInfo.numOfTextureCoordinates = sizeof(textureCoordinates) / sizeof(Point2D);
}

Cube::Cube(const Transformation &transformation, const Color &color)
    : Cube(transformation.position, transformation.rotation, transformation.scale, color)
{
}

Cube::Cube(const Cube &cube) : Cube(cube.transformation, cube.color)
{
    this->renderInfo.pVertexShader = cube.renderInfo.pVertexShader;
    this->renderInfo.pPixelShader = cube.renderInfo.pPixelShader;
    this->renderInfo.pTexture = cube.renderInfo.pTexture;
}

Cube::~Cube()
{
    if (this->renderInfo.colorIndexBuffer)
        delete[] renderInfo.colorIndexBuffer;
}

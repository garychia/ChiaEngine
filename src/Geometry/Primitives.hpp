#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include "Display/IRenderable.hpp"

class Triangle : public IRenderable
{
  private:
    static const Point3D vertexBuffer[];
    static const VertexIndexType vertexIndexBuffer[];
    static const Point2D textureCoordinates[];
    Color color;

  public:
    static const unsigned int NumberOfVertexIndices;

    static const unsigned int NumberOfVertices;

    Triangle(const Point3D &position = Point3D(), const Point3D &rotation = Point3D(),
             const Point3D &scale = Point3D(1, 1, 1), const Color &color = Color(1, 1, 1));

    Triangle(const Transformation &transformation, const Color &color = Color(1, 1, 1));

    Triangle(const Triangle &other);

    virtual ~Triangle();
};

class Cube : public IRenderable
{
  private:
    static const Point3D vertexBuffer[];
    static const VertexIndexType vertexIndexBuffer[];
    static const Point2D textureCoordinates[];
    Color color;

  public:
    Cube(const Point3D &position = Point3D(), const Point3D &rotation = Point3D(),
         const Point3D &scale = Point3D(1, 1, 1), const Color &color = Color(1, 1, 1));

    Cube(const Transformation &transformation, const Color &color = Color(1, 1, 1));

    Cube(const Cube &cube);

    virtual ~Cube();
};

#endif

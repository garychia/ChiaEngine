#include "GeometryModule.hpp"

GeometryModule::GeometryModule() : Module()
{
    AddTest<Point2DTest>();
    AddTest<Point3DTest>();
    AddTest<MathTest>();
}
#ifndef POINT3D_TEST_HPP
#define POINT3D_TEST_HPP

#include "Test.hpp"
#include "Geometry/3D/Point3D.hpp"

class Point3DTest : public Test
{
  public:
    Point3DTest() : Test("Point3D")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Point3D Default Constructor");
        Point3D p0;
        EXPECT_TRUE(p0.x == 0.f && p0.y == 0.f && p0.z == 0.f, "Default should be (0,0,0).", true);

        TEST_MESSAGE("Point3D Parameter Constructor");
        Point3D p1(1.f, 2.f, 3.f);
        EXPECT_TRUE(p1.x == 1.f && p1.y == 2.f && p1.z == 3.f, "Constructor.", true);

        TEST_MESSAGE("Point3D Addition");
        Point3D p2 = p1 + Point3D(10.f, 20.f, 30.f);
        EXPECT_TRUE(p2.x == 11.f && p2.y == 22.f && p2.z == 33.f, "Addition.", true);

        TEST_MESSAGE("Point3D Subtraction");
        Point3D p3 = p2 - p1;
        EXPECT_TRUE(p3.x == 10.f && p3.y == 20.f && p3.z == 30.f, "Subtraction.", true);

        TEST_MESSAGE("Point3D += and -=");
        Point3D p4(1.f, 1.f, 1.f);
        p4 += Point3D(2.f, 3.f, 4.f);
        EXPECT_TRUE(p4.x == 3.f && p4.y == 4.f && p4.z == 5.f, "+= works.", true);
        p4 -= Point3D(1.f, 2.f, 3.f);
        EXPECT_TRUE(p4.x == 2.f && p4.y == 2.f && p4.z == 2.f, "-= works.", true);

        TEST_MESSAGE("Point3D Unary Negation");
        Point3D p5 = -p1;
        EXPECT_TRUE(p5.x == -1.f && p5.y == -2.f && p5.z == -3.f, "Negation.", true);

        TEST_MESSAGE("Point3D Scalar Multiply");
        Point3D p6 = p1 * 2.f;
        EXPECT_TRUE(p6.x == 2.f && p6.y == 4.f && p6.z == 6.f, "Scalar multiply.", true);

        TEST_MESSAGE("Point3D Cross Product");
        Point3D v1(1.f, 0.f, 0.f);
        Point3D v2(0.f, 1.f, 0.f);
        Point3D cross = v1.Cross(v2);
        EXPECT_TRUE(cross.x == 0.f && cross.y == 0.f && cross.z == 1.f, "Cross product (1,0,0)×(0,1,0) = (0,0,1).", true);

        TEST_MESSAGE("Point3D Normalize");
        Point3D v3(3.f, 4.f, 0.f);
        Point3D norm = v3.Normalize();
        EXPECT_TRUE(norm.x > 0.59f && norm.x < 0.61f, "Normalize x ~0.6.", true);
        EXPECT_TRUE(norm.y > 0.79f && norm.y < 0.81f, "Normalize y ~0.8.", true);

        SUCCESS_MESSAGE("Point3D");
        return true;
    }
};

#endif
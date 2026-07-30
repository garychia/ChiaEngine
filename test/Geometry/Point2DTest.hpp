#ifndef POINT2D_TEST_HPP
#define POINT2D_TEST_HPP

#include "Test.hpp"
#include "Geometry/2D/Point2D.hpp"

class Point2DTest : public Test
{
  public:
    Point2DTest() : Test("Point2D")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Point2D Default Constructor");
        Point2D p0;
        EXPECT_TRUE(p0.x == 0.f && p0.y == 0.f, "Default should be (0,0).", true);

        TEST_MESSAGE("Point2D Parameter Constructor");
        Point2D p1(3.f, 4.f);
        EXPECT_TRUE(p1.x == 3.f && p1.y == 4.f, "Constructor sets correctly.", true);

        TEST_MESSAGE("Point2D Copy Assign");
        Point2D p2;
        p2 = p1;
        EXPECT_TRUE(p2.x == 3.f && p2.y == 4.f, "Copy assign works.", true);

        TEST_MESSAGE("Point2D Addition");
        Point2D p3 = p1 + Point2D(1.f, 2.f);
        EXPECT_TRUE(p3.x == 4.f && p3.y == 6.f, "Addition works.", true);

        TEST_MESSAGE("Point2D Subtraction");
        Point2D p4 = p1 - Point2D(1.f, 1.f);
        EXPECT_TRUE(p4.x == 2.f && p4.y == 3.f, "Subtraction works.", true);

        SUCCESS_MESSAGE("Point2D");
        return true;
    }
};

#endif